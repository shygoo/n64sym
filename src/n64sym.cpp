/*

    n64sym
    Symbol identification tool for N64 games
    shygoo 2017, 2020
    License: MIT

*/

#include <fstream>
#include <iostream>
#include <cstdio>
#include <set>

#include <miniz/miniz.h>
#include <miniz/miniz.c>

#include "n64sym.h"
#include "builtin_signatures.h"
#include "signaturefile.h"
#include "pathutil.h"
#include "crc32.h"

#ifdef WIN32
#include <windirent.h>
#else
#include <dirent.h>
#endif

CN64Sym::n64sym_fmt_lut_t CN64Sym::FormatNames[] = {
    { "default",  N64SYM_FMT_DEFAULT },
    { "pj64",     N64SYM_FMT_PJ64 },
    { "nemu",     N64SYM_FMT_NEMU },
    { "armips",   N64SYM_FMT_ARMIPS },
    { "n64split", N64SYM_FMT_N64SPLIT }
};

CN64Sym::CN64Sym() :
    m_Binary(NULL),
    m_BinarySize(0),
    m_HeaderSize(0x80000000),
    m_bVerbose(false),
    m_bUseBuiltinSignatures(false),
    m_bThoroughScan(false),
    m_bOverrideHeaderSize(false),
    m_bHaveOutputFile(false),
    m_Output(&std::cout),
    m_OutputFormat(N64SYM_FMT_DEFAULT),
    m_NumSymbolsToCheck(0),
    m_NumSymbolsChecked(0)
{
    char *builtinSigFileContents = new char[gBuiltinSignatureFile.uncSize];

    uLong uncSize = gBuiltinSignatureFile.uncSize;
    uncompress((uint8_t *)builtinSigFileContents, &uncSize,
        gBuiltinSignatureFile.data, gBuiltinSignatureFile.cmpSize);

    m_BuiltinSigs.LoadFromMemory(builtinSigFileContents);

    delete[] builtinSigFileContents;
}

CN64Sym::~CN64Sym()
{
    if(m_Binary != NULL)
    {
        delete[] m_Binary;
    }
}

bool CN64Sym::LoadBinary(const char *binPath)
{
    if(m_Binary != NULL)
    {
        delete[] m_Binary;
        m_BinarySize = 0;
    }

    std::ifstream file;
    file.open(binPath, std::ifstream::binary);

    if(!file.is_open())
    {
        return false;
    }

    file.seekg(0, file.end);
    m_BinarySize = file.tellg();

    file.seekg(0, file.beg);
    m_Binary = new uint8_t[m_BinarySize];
    file.read((char *)m_Binary, m_BinarySize);

    if(PathIsN64Rom(binPath) && !m_bOverrideHeaderSize)
    {
        if(m_BinarySize < 0x101000)
        {
            delete[] m_Binary;
            m_BinarySize = 0;
            return false;
        }

        uint32_t endianCheck = bswap32(*(uint32_t *)&m_Binary[0x00]);

        switch(endianCheck)
        {
        case 0x80371240:
            break;
        case 0x40123780:
            for(size_t i = 0; i < m_BinarySize; i += sizeof(uint32_t))
            {
                *(uint32_t *)&m_Binary[i] = bswap32(*(uint32_t *)&m_Binary[i]);
            }
            break;
        case 0x37804012:
            for(size_t i = 0; i < m_BinarySize; i += sizeof(uint16_t))
            {
                *(uint16_t *)&m_Binary[i] = bswap16(*(uint16_t *)&m_Binary[i]);
            }
            break;
        }

        uint32_t entryPoint = bswap32(*(uint32_t *)&m_Binary[0x08]);

        uint32_t bootCheck = crc32_begin();
        crc32_read(&m_Binary[0x40], 0xFC0, &bootCheck);
        crc32_end(&bootCheck);

        switch(bootCheck)
        {
        case 0x0B050EE0: // 6103
            entryPoint -= 0x100000;
            break;
        case 0xACC8580A: // 6106
            entryPoint -= 0x200000;
            break;
        }
        
        m_HeaderSize = entryPoint - 0x1000;
    }

    return true;
}

void CN64Sym::AddLibPath(const char* libPath)
{
    m_LibPaths.push_back(libPath);
}

void CN64Sym::SetVerbose(bool bVerbose)
{
    m_bVerbose = bVerbose;
}

void CN64Sym::UseBuiltinSignatures(bool bUseBuiltinSignatures)
{
    m_bUseBuiltinSignatures = bUseBuiltinSignatures;
}

void CN64Sym::SetThoroughScan(bool bThoroughScan)
{
    m_bThoroughScan = bThoroughScan;
}

bool CN64Sym::SetOutputFormat(const char *fmtName)
{
    size_t numFormats = sizeof(FormatNames) / sizeof(FormatNames[0]);

    for(size_t i = 0; i < numFormats; i++)
    {
        if(strcmp(FormatNames[i].name, fmtName) == 0)
        {
            m_OutputFormat = FormatNames[i].fmt;
            return true;
        }
    }

    m_OutputFormat = N64SYM_FMT_DEFAULT;
    return false;
}

bool CN64Sym::SetOutputPath(const char *path)
{
    m_OutputFile.open(path, std::ofstream::binary);

    if(!m_OutputFile.is_open())
    {
        m_Output = &std::cout;
        return false;
    }

    m_Output = &m_OutputFile;
    return true;
}

void CN64Sym::SetHeaderSize(uint32_t headerSize)
{
    m_bOverrideHeaderSize = true;
    m_HeaderSize = headerSize;
}

bool CN64Sym::Run()
{
    if(m_Binary == NULL)
    {
        return false;
    }

    m_LikelyFunctionOffsets.clear();

    for(size_t i = 0; i < m_BinarySize; i += sizeof(uint32_t))
    {
        uint32_t word = bswap32(*(uint32_t*)&m_Binary[i]);

        // JR RA (+ 8)
        if(word == 0x03E00008)
        {
            if(*(uint32_t*)&m_Binary[i + 8] != 0x00000000)
            {
                m_LikelyFunctionOffsets.insert(i + 8);
            }
        }

        // ADDIU SP, SP, -n
        if((word & 0xFFFF0000) == 0x27BD0000 && (int16_t)(word & 0xFFFF) < 0)
        {
            m_LikelyFunctionOffsets.insert(i);
        }

        // todo JALs?
    }

    TallyNumSymbolsToCheck();

    if(m_bUseBuiltinSignatures)
    {
        ProcessSignatureFile(m_BuiltinSigs);
    }

    for(size_t i = 0; i < m_LibPaths.size(); i++)
    {
        ScanRecursive(m_LibPaths.at(i));
    }

    SortResults();
    DumpResults();

    return true;
}

void CN64Sym::DumpResults()
{
    switch(m_OutputFormat)
    {
    case N64SYM_FMT_PJ64:
        for(auto& result : m_Results)
        {
            Output("%08X,code,%s\n", result.address, result.name);
        }
        break;
    case N64SYM_FMT_NEMU:
        Output("Root\n");
        Output("\tCPU\n");
        for(auto& result : m_Results)
        {
            Output("\t\tCPU 0x%08X: %s\n", result.address, result.name);
        }
        Output("\tMemory\n");
        Output("\tRSP\n");
        break;
    case N64SYM_FMT_ARMIPS:
        for(auto& result : m_Results)
        {
            Output(".definelabel %s, 0x%08X\n", result.name, result.address);
        }
        break;
    case N64SYM_FMT_N64SPLIT:
        Output("labels:\n");
        for(auto &result : m_Results)
        {
            Output("   - [0x%08X, \"%s\"]\n", result.address, result.name);
        }
        break;
    case N64SYM_FMT_DEFAULT:
    default:
        for(auto& result : m_Results)
        {
            Output("%08X %s\n", result.address, result.name);
        }
        break;
    }
}

void CN64Sym::ScanRecursive(const char* path)
{
    if (IsFileWithSymbols(path))
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
        if (!entry->d_name) continue;
        snprintf(next_path, sizeof(next_path), "%s/%s", path, entry->d_name);
        switch (entry->d_type) {
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
                if (IsFileWithSymbols(next_path))
                {
                    //printf("next path %s\n", next_path);
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

void CN64Sym::ProcessFile(const char* path)
{
    if (PathIsStaticLibrary(path))
    {
        ProcessLibrary(path);
    }
    else if (PathIsObjectFile(path))
    {
        ProcessObject(path);
    }
    else if(PathIsSignatureFile(path))
    {
        ProcessSignatureFile(path);
    }
}

void CN64Sym::ProcessLibrary(const char* path)
{
    CArReader ar;

    if(!ar.Load(path))
    {
        return;
    }

    while(ar.SeekNextBlock())
    {
        if(!PathIsObjectFile(ar.GetBlockIdentifier()))
        {
            continue;
        }

        // worker thread will delete objProcessingCtx after it's done
        obj_processing_context_t* objProcessingCtx = new obj_processing_context_t;
        objProcessingCtx->mt_this = this;
        objProcessingCtx->libraryPath = path;
        objProcessingCtx->blockIdentifier = ar.GetBlockIdentifier();
        objProcessingCtx->blockData = ar.GetBlockData();
        objProcessingCtx->blockSize = ar.GetBlockSize();

        threadPool.AddWorker(ProcessObjectProc, (void*)objProcessingCtx);
    }

    threadPool.WaitForWorkers();
}

void CN64Sym::ProcessObject(const char* path)
{
    uint8_t* buffer;
    size_t size;

    FILE* fp = fopen(path, "rb");

    if(fp == NULL)
    {
        return;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    buffer = (uint8_t *)malloc(size);
    fread(buffer, 1, size, fp);
    fclose(fp);

    Log("%s\n", path);

    obj_processing_context_t objProcessingCtx;
    objProcessingCtx.mt_this = NULL;
    objProcessingCtx.libraryPath = NULL;
    objProcessingCtx.blockIdentifier = path;
    objProcessingCtx.blockData = buffer;
    objProcessingCtx.blockSize = size;

    ProcessObject(&objProcessingCtx);

    free(buffer);
}

void CN64Sym::ProcessObject(obj_processing_context_t* objProcessingCtx)
{
    CElfContext elf;
    elf.LoadFromMemory(objProcessingCtx->blockData, objProcessingCtx->blockSize);

    CElfSection* textSec = elf.Section(".text");

    if(textSec == NULL)
    {
        return;
    }

    const char* textBuf = textSec->Data(&elf);
    uint32_t textSize = textSec->Size();

    uint32_t endAddress = m_BinarySize - textSize;

    bool bHaveFullMatch;
    uint32_t matchedAddress;
    int nBytesMatched;
    int bestPartialMatchLength = 0;
    const char* matchedBlock = NULL;

    for(uint32_t blockAddress = 0; blockAddress < endAddress; blockAddress += sizeof(uint32_t))
    {
        const char* block = (const char*)&m_Binary[blockAddress];
        bHaveFullMatch = TestElfObjectText(&elf, block, &nBytesMatched);

        if(bHaveFullMatch)
        {
            matchedBlock = block;
            matchedAddress = blockAddress;
            break;
        }
        else if(nBytesMatched > bestPartialMatchLength)
        {
            matchedBlock = block;
            matchedAddress = blockAddress;
            bestPartialMatchLength = nBytesMatched;
        }
    }

    threadPool.LockDefaultMutex();

    Log("%s:%s\n", objProcessingCtx->libraryPath, objProcessingCtx->blockIdentifier);

    if(bHaveFullMatch)
    {
        Log("complete match\n");
        AddSymbolResults(&elf, matchedAddress);
        AddRelocationResults(&elf, matchedBlock, "__"); // fix me altNamePrefix
    }
    else if(bestPartialMatchLength >= 32)
    {
        Log("partial match (0x%02X bytes)\n", bestPartialMatchLength);
        AddSymbolResults(&elf, matchedAddress, bestPartialMatchLength);
        AddRelocationResults(&elf, matchedBlock, "__", bestPartialMatchLength); // fix me altNamePrefix
    }
    else
    {
        threadPool.UnlockDefaultMutex();
        return;
    }

    for(size_t i = 0; i < textSize; i += 4)
    {
        uint32_t buffOp = bswap32(*(uint32_t*)&matchedBlock[i]);
        uint32_t textOp = bswap32(*(uint32_t*)&textBuf[i]);

        CElfRelocation* relocation = NULL;
        CElfSymbol* symbol = NULL;
        bool bHaveRel = false;

        for(int j = 0; j < elf.NumTextRelocations(); j++)
        {
            relocation = elf.TextRelocation(j);
            if(relocation->Offset() == i)
            {
                Log("have reloc\n");
                symbol = relocation->Symbol(&elf);
                bHaveRel = true;
            }
        }

        Log("%08X/%04X: %08X %08X", m_HeaderSize + (matchedAddress + i), i, buffOp, textOp);
        textOp == buffOp ? Log("\n") : Log(" * %s\n", bHaveRel ? symbol->Name(&elf) : "");
    }

    Log("\n");
    threadPool.UnlockDefaultMutex();
}

void* CN64Sym::ProcessObjectProc(void* _objProcessingCtx)
{
    obj_processing_context_t* objProcessingCtx = (obj_processing_context_t*)_objProcessingCtx;
    CN64Sym* _this = objProcessingCtx->mt_this;

    _this->ProcessObject(objProcessingCtx);

    delete objProcessingCtx;

    return NULL;
}

void CN64Sym::ProcessSignatureFile(const char* path)
{
    CSignatureFile sigFile;

    if(sigFile.Load(path))
    {
        ProcessSignatureFile(sigFile);
    }
}

void CN64Sym::ProcessSignatureFile(CSignatureFile& sigFile)
{
    size_t numSymbols = sigFile.GetNumSymbols();

    const char *statusDescription = "(built-in signatures)";
    int percentDone = 0;
    int statusLineLen = printf("[  0%%] %s", statusDescription);

    for(size_t nSymbol = 0; nSymbol < numSymbols; nSymbol++)
    {
        uint32_t symbolSize = sigFile.GetSymbolSize(nSymbol);
        uint32_t endOffset = m_BinarySize - symbolSize;
        char symbolName[128];
        sigFile.GetSymbolName(nSymbol, symbolName, sizeof(symbolName));

        int percentNow = (int)(((float)nSymbol / numSymbols) * 100);
        if(percentNow > percentDone)
        {
            ClearLine(statusLineLen);
            statusLineLen = printf("[%3d%%] %s", percentDone, statusDescription);
            percentDone = percentNow;
        }

        for(auto offset : m_LikelyFunctionOffsets)
        {
            if(TestSignatureSymbol(sigFile, nSymbol, offset))
            {
                goto next_symbol;
            }
        }

        if(m_bThoroughScan)
        {
            for(uint32_t offset = 0; offset < endOffset; offset += 4)
            {
                if(TestSignatureSymbol(sigFile, nSymbol, offset))
                {
                    goto next_symbol;
                }
            }
        }

        next_symbol:;
    }

    ClearLine(statusLineLen);
}

bool CN64Sym::TestElfObjectText(CElfContext* elf, const char* data, int* nBytesMatched)
{
    CElfSection *text_sec, *rel_text_sec;
    CElfRelocation* text_relocations;
    const char* text_sec_data;
    uint32_t text_sec_size;
    int num_text_relocations;

    text_sec = elf->Section(".text");

    if(text_sec == NULL)
    {
        *nBytesMatched = 0;
        return false;
    }

    text_sec_data = text_sec->Data(elf);
    text_sec_size = text_sec->Size();
    num_text_relocations = elf->NumTextRelocations();

    if(num_text_relocations == 0)
    {
        // no relocations, do plain binary comparison
        if(memcmp(text_sec_data, data, text_sec_size) == 0)
        {
            *nBytesMatched = text_sec_size;
            return true;
        }
        
        *nBytesMatched = 0;
        return false;
    }

    rel_text_sec = elf->Section(".rel.text");
    text_relocations = (CElfRelocation*) rel_text_sec->Data(elf);

    int cur_reltab_index = 0;
    
    for(uint32_t i = 0; i < text_sec_size; i += sizeof(uint32_t))
    {
        // see if this opcode has a relocation

        CElfRelocation* curRelocation = &text_relocations[cur_reltab_index];

        if(cur_reltab_index < num_text_relocations && i == curRelocation->Offset())
        {
            // if for some reason the relocation is on a NOP, don't count it
            if(data[i] == 0x00000000)
            {
                *nBytesMatched = i;
                return false;
            }

            // only check the top 6 bits
            if((text_sec_data[i] & 0xFC) != (data[i] & 0xFC))
            {
                *nBytesMatched = i;
                return false;
            }

            cur_reltab_index++;
            continue;
        }

        // fetch the next relocation
        // this trusts that the object's relocation offsets are in order from least to greatest
        //for(int j = cur_reltab_index; j < num_text_relocations; j++)
        //{
        //    CElfRelocation* cur_relocation = &text_relocations[j];
        //    cur_relocation_offset = cur_relocation->Offset();
        //    
        //    if(cur_relocation_offset < i)
        //    {
        //        continue;
        //    }
//
        //    if(cur_relocation_offset == i)
        //    {
        //        have_relocation = true;
        //    }
        //}

        if(*(uint32_t*)&text_sec_data[i] != *(uint32_t*)&data[i])
        {
            *nBytesMatched = i;
            return false;
        }
    }
    return true;
}

bool CN64Sym::TestSignatureSymbol(CSignatureFile& sigFile, size_t nSymbol, uint32_t offset)
{
    if(sigFile.TestSymbol(nSymbol, &m_Binary[offset]))
    {
        search_result_t result;
        result.address = m_HeaderSize + offset;
        result.size = sigFile.GetSymbolSize(nSymbol);
        sigFile.GetSymbolName(nSymbol, result.name, sizeof(result.name));
        AddResult(result);
        return true;
    }
    return false;
}

void CN64Sym::TallyNumSymbolsToCheck()
{
    m_NumSymbolsToCheck = 0;

    if(m_bUseBuiltinSignatures)
    {
        m_NumSymbolsToCheck += m_BuiltinSigs.GetNumSymbols();
    }

    for(size_t i = 0; i < m_LibPaths.size(); i++)
    {
        CountSymbolsRecursive(m_LibPaths.at(i));
    }
}

void CN64Sym::CountSymbolsInFile(const char *path)
{
    if(PathIsSignatureFile(path))
    {
        CSignatureFile sigFile;
        if(sigFile.Load(path))
        {
            m_NumSymbolsToCheck += sigFile.GetNumSymbols();
        }
    }
    else if(PathIsStaticLibrary(path))
    {
        CArReader ar;
        if(ar.Load(path))
        {
            while(ar.SeekNextBlock())
            {
                if(!PathIsObjectFile(ar.GetBlockIdentifier()))
                {
                    continue;
                }

                CElfContext elf;
                elf.LoadFromMemory(ar.GetBlockData(), ar.GetBlockSize());
                m_NumSymbolsToCheck += CountGlobalSymbolsInElf(elf); // probably needs work
            }
        }
    }
    else if(PathIsObjectFile(path))
    {
        CElfContext elf;
        if(elf.Load(path))
        {
            m_NumSymbolsToCheck += CountGlobalSymbolsInElf(elf);
        }
    }
}

size_t CN64Sym::CountGlobalSymbolsInElf(CElfContext& elf)
{
    size_t count = 0;
    int numSymbols = elf.NumSymbols();

    for(int i = 0; i < numSymbols; i++)
    {
        CElfSymbol* symbol = elf.Symbol(i);
        if(symbol->Binding() == STB_GLOBAL &&
           symbol->Type() != STT_NOTYPE &&
           symbol->SectionIndex() != SHN_UNDEF &&
           symbol->Size() > 0)
        {
            count++; // probably needs work
        }
    }
    return count;
}

void CN64Sym::CountSymbolsRecursive(const char* path)
{
    if (IsFileWithSymbols(path))
    {
        CountSymbolsInFile(path);
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
        if (!entry->d_name) continue;
        snprintf(next_path, sizeof(next_path), "%s/%s", path, entry->d_name);
        switch (entry->d_type) {
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
                if (IsFileWithSymbols(next_path))
                {
                    CountSymbolsInFile(next_path);
                }
                break;
            }
            default:
                break;
        }
    }
    closedir(dir);
}

bool CN64Sym::AddResult(search_result_t result)
{
    // todo use map
    if(result.address == 0)
    {
        return false;
    }

    for(auto& otherResult : m_Results)
    {
        if(otherResult.address == result.address)
        {
            return false; // already have
        }
    }

    m_Results.push_back(result);
    return true;
}

void CN64Sym::AddSymbolResults(CElfContext* elf, uint32_t baseAddress, uint32_t maxTextOffset)
{
    int nSymbols = elf->NumSymbols();

    for(int i = nSymbols - 1; i >= 0; i--)
    {
        CElfSymbol* symbol = elf->Symbol(i);
        if(symbol->Binding() == STB_GLOBAL &&
           symbol->Type() != STT_NOTYPE &&
           symbol->SectionIndex() != SHN_UNDEF &&
           symbol->Size() > 0)
        {
            if(maxTextOffset > 0 && symbol->Value() >= maxTextOffset)
            {
                // exceeds maximum offset for a partial match
                continue;
            }

            search_result_t result;
            result.address = m_HeaderSize + (baseAddress + symbol->Value());
            result.size = symbol->Size();
            strcpy(result.name, symbol->Name(elf));

            Log("adding %s\n", result.name);
            AddResult(result);
        }
    }
}

void CN64Sym::AddRelocationResults(CElfContext* elf, const char* block, const char* altNamePrefix, int maxTextOffset)
{
    Log("Adding relocation results...\n");

    int nRelocations = elf->NumTextRelocations();

    for(int i = 0; i < nRelocations; i++)
    {
        CElfRelocation* relocation = elf->TextRelocation(i);
        CElfSymbol* symbol = relocation->Symbol(elf);
        int textOffset = relocation->Offset();
        uint32_t opcode = bswap32(*(uint32_t*)&block[textOffset]);
        uint8_t relType = relocation->Type();

        Log("%s %04X\n", symbol->Name(elf), textOffset);

        if(maxTextOffset > 0 && textOffset >= maxTextOffset)
        {
            // exceeds maximum offset for a partial match
            continue;
        }
        
        if(relType == R_MIPS_26 && (opcode >> 26) == 0x0C)
        {
            uint32_t jalTarget = m_HeaderSize | ((opcode & 0x3FFFFFF) * 4);
            
            search_result_t result;
            result.address = jalTarget;
            result.size = 0;
            strncpy(result.name, symbol->Name(elf), sizeof(result.name) - 1);

            if(relocation->SymbolIndex() == 1)
            {
                // Static function, compiler tossed out the symbol
                // Use object file name and text offset as a replacement
                int len = sprintf(result.name, "%s_%04X", altNamePrefix, textOffset);
                for(int i = 0; i < len; i++)
                {
                    if(result.name[i] == '.')
                    {
                        result.name[i] = '_';
                    }
                }
            }

            Log("adding %s (relocation)\n", result.name);

            AddResult(result);
        }
        else if(relType == R_MIPS_LO16 && i > 0)
        {
            CElfRelocation* prevRelocation = elf->TextRelocation(i - 1);

            if(prevRelocation->Type() == R_MIPS_HI16)
            {
                uint32_t upperOp = bswap32(*(uint32_t*)&block[prevRelocation->Offset()]);
                uint32_t lowerOp = opcode;

                // TODO: Implement

                CElfSymbol* symbol = relocation->Symbol(elf);
                Log("%04X%04X,data,%s\n", upperOp & 0xFFFF, lowerOp & 0xFFFF, symbol->Name(elf));
            }
        }
    }
}

bool CN64Sym::ResultCmp(search_result_t a, search_result_t b)
{
    return (a.address < b.address);
}

void CN64Sym::SortResults()
{
    std::sort(m_Results.begin(), m_Results.end(), ResultCmp);
}

void CN64Sym::ClearLine(int nChars)
{
    printf("\r");
    printf("%*s", nChars, "");
    printf("\r");
}

void CN64Sym::Log(const char* format, ...)
{
    if(!m_bVerbose)
    {
        return;
    }

    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

void CN64Sym::Output(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    size_t len = vsnprintf(NULL, 0, format, args);
    char *str = new char[len + 1];

    vsprintf(str, format, args);

    *m_Output << str;
    delete[] str;
    
    va_end(args);
}
