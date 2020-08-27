/*

    Signature file reader for n64sym
    shygoo 2020
    License: MIT

*/

#include <algorithm>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <cctype>
#include <map>

#include "elfutil.h"
#include "signaturefile.h"
#include "crc32.h"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

CSignatureFile::CSignatureFile() :
    m_Buffer(NULL),
    m_Size(0),
    m_Pos(0)
{
}

CSignatureFile::~CSignatureFile()
{
    for(auto symbol : m_Symbols)
    {
        if(symbol.relocs != NULL)
        {
            delete symbol.relocs;
        }
    }

    delete[] m_Buffer;
}

size_t CSignatureFile::GetNumSymbols()
{
    return m_Symbols.size();
}

uint32_t CSignatureFile::GetSymbolSize(size_t nSymbol)
{
    if(nSymbol >= m_Symbols.size())
    {
        return 0;
    }

    return m_Symbols[nSymbol].size;
}

bool CSignatureFile::GetSymbolName(size_t nSymbol, char *str, size_t nMaxChars)
{
    if(nSymbol >= m_Symbols.size())
    {
        return false;
    }

    strncpy(str, m_Symbols[nSymbol].name, nMaxChars);

    return true;
}

void CSignatureFile::ReadStrippedWord(uint8_t *dst, const uint8_t *src, int relType)
{
    memcpy(dst, src, 4);

    switch(relType)
    {
    case 4:
        // targ26
        dst[0] &= 0xFC;
        dst[1] = 0x00;
        dst[2] = 0x00;
        dst[3] = 0x00;
        break;
    case 5:
    case 6:
        // hi/lo16
        dst[2] = 0x00;
        dst[3] = 0x00;
        break;
    }
}

void debug(const uint8_t *buf, size_t size)
{
    printf("\n");
    for(size_t i = 0; i < size; i++)
    {
        printf("%02X ", buf[i]);
    }
}

bool CSignatureFile::TestSymbol(size_t nSymbol, const uint8_t *buffer)
{
    if(nSymbol >= m_Symbols.size())
    {
        return 0;
    }

    symbol_info_t& symbol = m_Symbols[nSymbol];

    uint32_t crcA = crc32_begin();
    uint32_t crcB = crc32_begin();
    
    if(symbol.relocs == NULL)
    {
        crc32_read(buffer, min(symbol.size, 8), &crcA);
        crc32_end(&crcA);

        if(symbol.crcA != crcA)
        {
            return false;
        }

        crc32_read(buffer, symbol.size, &crcB);
        crc32_end(&crcB);

        return (symbol.crcB == crcB);
    }

    size_t offset = 0;

    auto reloc = symbol.relocs->begin();
    uint32_t crcA_limit = min(symbol.size, 8);

    while(offset < crcA_limit && reloc != symbol.relocs->end())
    {
        if(offset < reloc->offset)
        {
            // read up to relocated op or crcA_limit
            crc32_read(&buffer[offset], min(reloc->offset, crcA_limit) - offset, &crcA);
            crc32_read(&buffer[offset], min(reloc->offset, crcA_limit) - offset, &crcB);
            offset = min(reloc->offset, crcA_limit);
        }
        else if(offset == reloc->offset) 
        {
            // strip and read relocated op
            uint8_t op[4];
            ReadStrippedWord(op, &buffer[offset], reloc->type);
            crc32_read(op, 4, &crcA);
            crc32_read(op, 4, &crcB);
            offset += 4;
            reloc++;
        }
    }

    if(offset < crcA_limit)
    {
        crc32_read(&buffer[offset], crcA_limit - offset, &crcA);
        crc32_read(&buffer[offset], crcA_limit - offset, &crcB);
        offset = crcA_limit;
    }

    crc32_end(&crcA);

    if(symbol.crcA != crcA)
    {
        return false;
    }

    while(offset < symbol.size && reloc != symbol.relocs->end())
    {
        if(offset < reloc->offset)
        {
            // read up to relocated op
            crc32_read(&buffer[offset], reloc->offset - offset, &crcB);
            offset = reloc->offset;
        }
        else if(offset == reloc->offset) 
        {
            // strip and read relocated op
            uint8_t op[4];
            ReadStrippedWord(op, &buffer[offset], reloc->type);
            crc32_read(op, sizeof(op), &crcB);
            offset += 4;
            reloc++;
        }
    }

    if(offset < symbol.size)
    {
        crc32_read(&buffer[offset], symbol.size - offset, &crcB);
        offset = symbol.size;
    }

    crc32_end(&crcB);

    return (symbol.crcB == crcB);
}

bool CSignatureFile::RelocOffsetCompare(const reloc_t& a, const reloc_t& b)
{
    return a.offset < b.offset;
}

void CSignatureFile::SortRelocationsByOffset()
{
    for(auto symbol : m_Symbols)
    {
        if(symbol.relocs != NULL)
        {
            std::sort(symbol.relocs->begin(), symbol.relocs->end(), RelocOffsetCompare);
        }
    }
}

int CSignatureFile::GetRelocationDirectiveValue(const char *str)
{
    if(strcmp(".targ26", str) == 0) return R_MIPS_26;
    if(strcmp(".hi16", str) == 0) return R_MIPS_HI16;
    if(strcmp(".lo16", str) == 0) return R_MIPS_LO16;
    return -1;
}

bool CSignatureFile::LoadFromMemory(const char *contents)
{
    if(m_Buffer != NULL)
    {
        delete[] m_Buffer;
        m_Buffer = NULL;
        m_Size = 0;
    }

    m_Size = strlen(contents);
    m_Buffer = new char[m_Size + 1];
    memcpy(m_Buffer, contents, m_Size);
    m_Buffer[m_Size] = '\0';

    Parse();
    SortRelocationsByOffset();

    return true;
}

bool CSignatureFile::Load(const char *path)
{
    if(m_Buffer != NULL)
    {
        delete[] m_Buffer;
        m_Buffer = NULL;
        m_Size = 0;
    }

    std::ifstream file;
    file.open(path, std::ifstream::binary);

    if(!file.is_open())
    {
        return false;
    }

    file.seekg(0, file.end);
    m_Size = file.tellg();
    file.seekg(0, file.beg);
    m_Buffer = new char[m_Size];
    file.read(m_Buffer, m_Size);

    Parse();
    SortRelocationsByOffset();

    return true;
}

void CSignatureFile::Parse()
{
    const char *token;
    while((token = GetNextToken()))
    {
        top_level:
        
        if(token[0] == '.')
        {
            // relocation directive
            int relocType = GetRelocationDirectiveValue(token);
            if(relocType == -1)
            {
                printf("error: invalid relocation directive '%s'\n", token);
                goto errored;
            }

            if(m_Symbols.size() == 0)
            {
                printf("error: no symbol defined for this relocation directive\n");
            }

            const char *relName = GetNextToken();

            if(m_Symbols.back().relocs == NULL)
            {
                m_Symbols.back().relocs = new std::vector<reloc_t>;
            }
            
            while((token = GetNextToken()))
            {
                uint32_t offset;
                if(!ParseNumber(token, &offset))
                {
                    goto top_level;
                }

                m_Symbols.back().relocs->push_back({relName, (uint8_t)relocType, offset});
            }

            continue;
        }

        if(!isalpha(token[0]) && token[0] != '_')
        {
            printf("error: unexpected '%s'\n", token);
            goto errored;
        }

        symbol_info_t symbolInfo;
        symbolInfo.relocs = NULL;
        symbolInfo.name = token;

        const char *szSize = GetNextToken();
        const char *szCrcA = GetNextToken();
        const char *szCrcB = GetNextToken();

        if(!ParseNumber(szSize, &symbolInfo.size) ||
           !ParseNumber(szCrcA, &symbolInfo.crcA) ||
           !ParseNumber(szCrcB, &symbolInfo.crcB))
        {
            printf("error: invalid symbol parameters\n");
            goto errored;
        }

        m_Symbols.push_back(symbolInfo);
    }

    errored:;
}

bool CSignatureFile::IsEOF()
{
    return (m_Pos >= m_Size);
}

bool CSignatureFile::AtEndOfLine()
{
    while(!IsEOF() && (m_Buffer[m_Pos] == ' ' || m_Buffer[m_Pos] == '\t' || m_Buffer[m_Pos] == '\r'))
    {
        m_Pos++;
    }

    return (m_Buffer[m_Pos] == '\n' || m_Buffer[m_Pos] == '\0');
}

void CSignatureFile::SkipWhitespace()
{
    while(!IsEOF() && isspace(m_Buffer[m_Pos]))
    {
        m_Pos++;
    }

    while(m_Buffer[m_Pos] == '#')
    {
        while(!IsEOF() && m_Buffer[m_Pos] != '\n')
        {
            m_Pos++;
        }

        while(!IsEOF() && isspace(m_Buffer[m_Pos]))
        {
            m_Pos++;
        }
    }
}

char *CSignatureFile::GetNextToken()
{
    if(IsEOF())
    {
        return NULL;
    }

    SkipWhitespace();

    if(IsEOF())
    {
        return NULL;
    }

    size_t tokenPos = m_Pos;

    while(!IsEOF() && !isspace(m_Buffer[m_Pos]))
    {
        m_Pos++;
    }

    m_Buffer[m_Pos++] = '\0';

    return &m_Buffer[tokenPos];
}

bool CSignatureFile::ParseNumber(const char *str, uint32_t *result)
{
    char *endp;
    *result = strtoull(str, &endp, 0);
    return (size_t)(endp - str) == strlen(str);
}
