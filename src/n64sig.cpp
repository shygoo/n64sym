/*

    n64sig
    Signature file generator for n64sym
    shygoo 2020
    License: MIT

*/

#ifdef WIN32
#include <windirent.h>
#else
#include <dirent.h>
#endif

#include <algorithm>
#include <cstring>

#include "n64sig.h"
#include "arutil.h"
#include "pathutil.h"
#include "crc32.h"

#ifndef min
#define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

CN64Sig::CN64Sig() :
    m_bVerbose(false),
    m_NumProcessedSymbols(0)
{
}

CN64Sig::~CN64Sig()
{
}

void CN64Sig::AddLibPath(const char *path)
{
    m_LibPaths.push_back(path);
}

int stricmp(const char* a, const char *b)
{
    size_t alen = strlen(a);
    size_t blen = strlen(b);

    size_t len = min(alen, blen);

    for(size_t i = 0; i < len; i++)
    {
        int ac = tolower(a[i]);
        int bc = tolower(b[i]);

        if(ac < bc) return -1;
        if(ac > bc) return  1;
    }

    if(alen < blen) return -1;
    if(alen > blen) return  1;

    return 0;
}

const char *strPastUnderscores(const char *s)
{
    while(*s == '_')
    {
        s++;
    }
    return s;
}

bool CN64Sig::Run()
{
    m_NumProcessedSymbols = 0;

    printf("# sig_v1\n\n");

    for(auto libPath : m_LibPaths)
    {
        ScanRecursive(libPath);
    }

    if(m_bVerbose)
    {
        printf("# %zu symbols\n", m_SymbolMap.size());
        printf("# %zu processed\n", m_NumProcessedSymbols);
    }

    // copy symbol map to into a vector and sort it by symbol name

    std::vector<symbol_entry_t> symbols;

    for(auto& i : m_SymbolMap)
    {
        symbols.push_back(i.second);
    }

    std::sort(symbols.begin(), symbols.end(), [](symbol_entry_t& a, symbol_entry_t& b){
        return stricmp(strPastUnderscores(a.name), strPastUnderscores(b.name)) < 0;
    });

    for(auto& symbolEntry : symbols)
    {
        //symbol_entry_t& symbolEntry = i.second;

        printf("%s 0x%04X 0x%08X 0x%08X\n",
            symbolEntry.name,
            symbolEntry.size,
            symbolEntry.crc_a,
            symbolEntry.crc_b);

        if(symbolEntry.relocs == NULL)
        {
            continue;
        }

        for(auto& j : *symbolEntry.relocs)
        {
            const reloc_entry_t& relocEntry = j.first;
            const std::vector<uint16_t>& offsets = j.second;

            printf(" .%-6s %s",
                GetRelTypeName(relocEntry.relocType),
                relocEntry.relocSymbolName);

            for(auto& offset : offsets)
            {
                printf(" 0x%03X", offset);
            }

            printf("\n");
        }

        delete symbolEntry.relocs;

        printf("\n");
    }

    return true;
}

const char *CN64Sig::GetRelTypeName(uint8_t relType)
{
    switch(relType)
    {
    case R_MIPS_26: return "targ26";
    case R_MIPS_LO16: return "lo16";
    case R_MIPS_HI16: return "hi16";
    }

    return NULL;
}

void CN64Sig::FormatAnonymousSymbol(char *symbolName)
{
    char *c = symbolName;
    while(*c++)
    {
        if(*c == '.')
        {
            *c = '_';
        }
    }
}

void CN64Sig::StripAndGetRelocsInSymbol(const char *objectName, reloc_map_t& relocs, CElfSymbol *symbol, CElfContext& elf)
{
    int numTextRelocations = elf.NumTextRelocations();
    uint32_t symbolOffset = symbol->Value();
    uint32_t symbolSize = symbol->Size();

    uint32_t lastHi16Addend = 0;

    for(int nRel = 0; nRel < numTextRelocations; nRel++)
    {
        CElfRelocation *relocation = elf.TextRelocation(nRel);

        uint32_t relOffset = relocation->Offset();

        if(relOffset < symbolOffset || relOffset >= symbolOffset + symbolSize)
        {
            continue;
        }

        char relSymbolName[128];

        CElfSymbol *relSymbol = relocation->Symbol(&elf);
        strncpy(relSymbolName, relSymbol->Name(&elf), sizeof(relSymbolName) - 1);
        uint8_t relType = relocation->Type();
        //const char *relTypeName = GetRelTypeName(relocation->Type());

        const uint8_t *textData = (const uint8_t *)elf.Section(".text")->Data(&elf);
        uint8_t *opcode = (uint8_t *) &textData[relocation->Offset()];
        reloc_entry_t relocEntry;
        //relocEntry.param = 0;

        if(relSymbol->Binding() == STB_LOCAL) // anonymous symbol
        {
            uint32_t addend = 0;
            uint32_t opcodeBE = bswap32(*(uint32_t*)opcode);

            if(relType == R_MIPS_HI16)
            {
                addend = (opcodeBE & 0xFFFF) << 16;
                CElfRelocation *relocation2 = elf.TextRelocation(nRel + 1); // todo guard

                // next relocation must be LO16
                if(relocation2->Type() != R_MIPS_LO16)
                {
                    exit(EXIT_FAILURE);
                }

                uint8_t *opcode2 = (uint8_t*) &textData[relocation2->Offset()];
                uint32_t opcode2BE = bswap32(*(uint32_t*)opcode2);

                addend += (int16_t)(opcode2BE & 0xFFFF);

                lastHi16Addend = addend;

                //printf("%08X\n", addend);
            }
            else if(relType == R_MIPS_LO16)
            {
                addend = lastHi16Addend;
            }
            else if(relType == R_MIPS_26)
            {
                addend = (opcodeBE & 0x03FFFFFF) << 2;
            }

            const char *relSymbolSectionName = elf.Section(relSymbol->SectionIndex())->Name(&elf);
            snprintf(relSymbolName, sizeof(relSymbolName), "%s_%s_%04X", objectName, &relSymbolSectionName[1], addend);

            //printf("# %08X\n", relSymbol->Value());
        }

        // set addend to 0 before crc
        if(relType == R_MIPS_HI16 || relType == R_MIPS_LO16)
        {
            opcode[2] = 0x00;
            opcode[3] = 0x00;
        }
        else if(relType == R_MIPS_26)
        {
            opcode[0] &= 0xFC;
            opcode[1] = 0x00;
            opcode[2] = 0x00;
            opcode[3] = 0x00;
        }
        else
        {
            printf("# warning unhandled relocation type\n");
            continue;
            //printf("unk rel %d\n", relType);
            //exit(0);
        }

        relocEntry.relocType = relType;
        strncpy(relocEntry.relocSymbolName, relSymbolName, sizeof(relocEntry.relocSymbolName));

        relocs[relocEntry].push_back(relOffset - symbolOffset);
    }
}

void CN64Sig::ProcessLibrary(const char *path)
{
    CArReader arReader;
    CElfContext elf;

    if(!arReader.Load(path))
    {
        return;
    }

    while(arReader.SeekNextBlock())
    {
        const char *blockId = arReader.GetBlockIdentifier();
        uint8_t    *objectData = arReader.GetBlockData();
        size_t      objectSize = arReader.GetBlockSize();

        if(!PathIsObjectFile(blockId))
        {
            continue;
        }

        char objectName[256];
        PathGetFileName(blockId, objectName, sizeof(objectName));

        elf.LoadFromMemory(objectData, objectSize);

        ProcessObject(elf, objectName);
    }
}

void CN64Sig::ProcessObject(CElfContext& elf, const char *objectName)
{
    //printf("# object: %s\n", objectName);

    CElfSection *textSection;
    const uint8_t *textData;
    int indexOfText;

    // todo rename IndexOfSection
    if(!elf.SectionIndexOf(".text", &indexOfText))
    {
        return;
    }

    textSection = elf.Section(indexOfText);
    textData = (const uint8_t*)textSection->Data(&elf);

    int numSymbols = elf.NumSymbols();

    for(int nSymbol = 0; nSymbol < numSymbols; nSymbol++)
    {
        CElfSymbol *symbol = elf.Symbol(nSymbol);

        int         symbolSectionIndex = symbol->SectionIndex();
        const char* symbolName = symbol->Name(&elf);
        uint8_t     symbolType = symbol->Type();
        uint32_t    symbolSize = symbol->Size();
        uint32_t    symbolOffset = symbol->Value();

        if(symbolSectionIndex != indexOfText ||
           symbolType != STT_FUNC ||
           symbolSize == 0)
        {
            continue;
        }

        symbol_entry_t symbolEntry;
        strncpy(symbolEntry.name, symbolName, sizeof(symbolEntry.name) - 1);
        symbolEntry.relocs = new reloc_map_t;

        StripAndGetRelocsInSymbol(objectName, *symbolEntry.relocs, symbol, elf);

        symbolEntry.size = symbolSize;
        symbolEntry.crc_a = crc32(&textData[symbolOffset], min(symbolSize, 8));
        symbolEntry.crc_b = crc32(&textData[symbolOffset], symbolSize);

        m_NumProcessedSymbols++;

        if(m_SymbolMap.count(symbolEntry.crc_b) != 0)
        {
            if(m_bVerbose)
            {
                if(strcmp(symbolEntry.name, m_SymbolMap[symbolEntry.crc_b].name) != 0)
                {
                    printf("# warning: skipped %s (have %s, crc: %08X)\n",
                        symbolEntry.name,
                        m_SymbolMap[symbolEntry.crc_b].name,
                        symbolEntry.crc_b);
                }
            }

            delete symbolEntry.relocs;
            continue;
        }

        m_SymbolMap[symbolEntry.crc_b] = symbolEntry;
    }
}

void CN64Sig::ProcessObject(const char *path)
{
    char objectName[256];
    PathGetFileName(path, objectName, sizeof(objectName));

    CElfContext elf;
    if(elf.Load(path))
    {
        ProcessObject(elf, objectName);
    }
}

void CN64Sig::ProcessFile(const char *path)
{
    if(PathIsStaticLibrary(path))
    {
        ProcessLibrary(path);
    }
    else if(PathIsObjectFile(path))
    {
        ProcessObject(path);
    }
}

void CN64Sig::ScanRecursive(const char* path)
{
    if (PathIsStaticLibrary(path) || PathIsObjectFile(path))
    {
        ProcessFile(path);
        return;
    }

    DIR *dir;
    dir = opendir(path);
    if (dir == NULL)
    {
        printf("%s is neither a directory or file with symbols.\n", path);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        char next_path[PATH_MAX];

        if (!entry->d_name)
        {
            continue;
        }

        snprintf(next_path, sizeof(next_path), "%s/%s", path, entry->d_name);

        switch (entry->d_type)
        {
        case DT_DIR:
            // skip "." dirs
            if (entry->d_name[0] == '.')
            {
                continue;
            }
            // scan subdirectory
            ScanRecursive(next_path);
            break;
        case DT_REG:
        {
            if (PathIsStaticLibrary(next_path) || PathIsObjectFile(next_path))
            {
                //printf("# file: %s\n\n", next_path);
                ProcessFile(next_path);
            }
            break;
        }
        default:
            break;
        }
    }
    closedir(dir);
}

void CN64Sig::SetVerbose(bool bVerbose)
{
    m_bVerbose = bVerbose;
}
