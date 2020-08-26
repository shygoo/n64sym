#include "n64sig.h"
#include "arutil.h"
#include "pathutil.h"
#include "dirent.h"
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
        printf("# %d symbols\n", m_SymbolMap.size());
        printf("# %d processed\n", m_NumProcessedSymbols);
    }

    for(auto& i : m_SymbolMap)
    {
        symbol_entry_t& symbolEntry = i.second;

        //printf("%s\n", symbolEntry);
        printf("%s 0x%04X 0x%08X 0x%08X\n",
            symbolEntry.name,
            symbolEntry.size,
            symbolEntry.crc_a,
            symbolEntry.crc_b);

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

        printf("\n");
    }

    for(auto& i : m_SymbolMap)
    {
        symbol_entry_t& symbolEntry = i.second;
        if(symbolEntry.relocs != NULL)
        {
            delete symbolEntry.relocs;
        }
    }
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
        strncpy(relSymbolName, relSymbol->Name(&elf), sizeof(relSymbolName));
        uint8_t relType = relocation->Type();
        const char *relTypeName = GetRelTypeName(relocation->Type());

        const uint8_t *textData = (const uint8_t *)elf.Section(".text")->Data(&elf);
        const uint8_t *test = &textData[relocation->Offset()];
        uint8_t *opcode = (uint8_t *) &textData[relocation->Offset()];

        if(relType == 5 || relType == 6)
        {
            opcode[2] = 0x00;
            opcode[3] = 0x00;
        }
        if(relType == 4)
        {
            opcode[0] &= 0xFC;
            opcode[1] = 0x00;
            opcode[2] = 0x00;
            opcode[3] = 0x00;
        }

        if(relSymbolName[0] == '.')
        {
            snprintf(relSymbolName, sizeof(relSymbolName), "%s_%s", objectName, relSymbol->Name(&elf));
            FormatAnonymousSymbol(relSymbolName);
        }

        if(relTypeName == NULL)
        {
            continue; 
        }

        reloc_entry_t relocEntry;
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
        const char *objectName = arReader.GetBlockIdentifier();
        uint8_t    *objectData = arReader.GetBlockData();
        size_t      objectSize = arReader.GetBlockSize();

        elf.LoadFromMemory(objectData, objectSize);
        
        ProcessObject(elf, objectName);
    }
}

void CN64Sig::ProcessObject(CElfContext& elf, const char *objectName)
{
    CElfSection *textSection;
    const uint8_t *textData;
    size_t textSize;
    int indexOfText;

    // todo rename IndexOfSection
    if(!elf.SectionIndexOf(".text", &indexOfText))
    {
        return;
    }

    //printf("# %s\n\n", objectName);

    textSection = elf.Section(indexOfText);
    textData = (const uint8_t*)textSection->Data(&elf);
    textSize = textSection->Size();

    int numSymbols = elf.NumSymbols();

    for(int nSymbol = 0; nSymbol < numSymbols; nSymbol++)
    {
        CElfSymbol *symbol = elf.Symbol(nSymbol);

        int         symbolSectionIndex = symbol->SectionIndex();
        const char* symbolName = symbol->Name(&elf);
        uint8_t     symbolType = symbol->Type();
        uint32_t    symbolSize = symbol->Size();
        uint32_t    symbolOffset = symbol->Value();

        //uint32_t crc_a, crc_b;

        if(symbolSectionIndex != indexOfText ||
           symbolType != STT_FUNC ||
           symbolSize == 0)
        {
            continue;
        }

        symbol_entry_t symbolEntry;
        strncpy(symbolEntry.name, symbolName, sizeof(symbolEntry.name));
        symbolEntry.relocs = new reloc_map_t;

        //reloc_map_t relocs;
        // note: writes to const buffer
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
    char objectName[128];
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
