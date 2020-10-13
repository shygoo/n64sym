#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <set>

#include "elfutil.h"

const char *get_uint_type_name(uint32_t symbol_size)
{
    switch(symbol_size)
    {
    case 8: return "s64";
    case 4: return "s32";
    case 2: return "s16";
    case 1: return "s8";
    }

    return "data";
}

void elf_report_symbols(CElfContext *elf)
{
    std::set<uint32_t> reported_addresses;

    int num_symbols = elf->NumSymbols();

    for(int i = 0; i < num_symbols; i++)
    {
        CElfSymbol *symbol = elf->Symbol(i);
        CElfSection *section = symbol->Section(elf);

        const char *symbol_name = symbol->Name(elf);
        uint32_t symbol_value = symbol->Value();
        uint32_t symbol_size = symbol->Size();

        const char* type_name;

        if(symbol->SectionIndex() >= SHN_LORESERVE ||
           symbol->Type() == STT_SECTION)
        {
            continue;
        }

        if(symbol->Size() == 0)
        {
            // assume it's code
            type_name = "code";
        }
        else
        {
            switch(symbol->Type())
            {
            case STT_NOTYPE:
                type_name = "data";
                break;
            case STT_OBJECT:
                type_name = get_uint_type_name(symbol_size);
                break;
            case STT_FUNC:
                type_name = "code";
                break;
            
            }
        }

        //printf("%s\n", section_name);
        if(reported_addresses.count(symbol_value) == 0)
        {
            printf("%s,0x%08X,%s\n", type_name, symbol_value, symbol_name);
            reported_addresses.insert(symbol_value);
        }
    }

    for(std::set<uint32_t>::iterator iter = reported_addresses.begin(); iter != reported_addresses.end(); iter++)
    {
        printf("%08X\n", *iter);
    }

    printf("finished\n");
}

int main(int argc, const char* argv[])
{
    const char *elf_path;
    FILE *elf_fp;
    char *elf_buffer;
    size_t elf_size;

    CElfContext *elf;

    if(argc < 2)
    {
        printf("elf2pj64 <elf_file>\n");
        return EXIT_FAILURE;
    }

    elf_path = argv[1];
    elf_fp = fopen(elf_path, "rb");

    if(elf_fp == NULL)
    {
        printf("failed to open %s'", elf_path);
        return EXIT_FAILURE;
    }

    fseek(elf_fp, 0, SEEK_END);
    elf_size = ftell(elf_fp);
    rewind(elf_fp);

    elf_buffer = (char*) malloc(elf_size);
    int nbytesread = fread(elf_buffer, 1, elf_size, elf_fp);
    fclose(elf_fp);

    elf = new CElfContext(elf_buffer, elf_size);

    elf_report_symbols(elf);

    free(elf_buffer);
    delete elf;
}