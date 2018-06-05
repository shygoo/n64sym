#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../elfutil.h"

const char **section_bases = NULL;
int num_section_bases = 0;

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

bool secbase_eq_name(const char* name, const char* secbase)
{
    for(int i = 0;; i++)
    {
        if(name[i] == '\0' && secbase[i] == '=')
        {
            return true;
        }

        if(name[i] != secbase[i])
        {
            break;
        }
    }
    return false;
}

uint32_t secbase_value(const char *secbase)
{
    const char *sznum = strchr(secbase, '=') + 1;

    return strtoll(sznum, NULL, 0);
}


// maybe slow, oh well
uint32_t get_section_base(const char* name)
{
    for(int i = 0; i < num_section_bases; i++)
    {
        if(secbase_eq_name(name, section_bases[i]))
        {
            return secbase_value(section_bases[i]);
        }
    }

    return 0x00000000;
}

void elf_report_symbols(CElfContext *elf)
{
    int num_symbols = elf->NumSymbols();

    for(int i = 0; i < num_symbols; i++)
    {
        CElfSymbol *symbol = elf->Symbol(i);
        CElfSection *section = symbol->Section(elf);

        const char *symbol_name = symbol->Name(elf);
        uint32_t symbol_value = symbol->Value();
        uint32_t symbol_size = symbol->Size();

        const char *section_name = section->Name(elf);

        uint32_t section_base = get_section_base(section_name);

        const char* type_name;

        if(symbol->SectionIndex() == SHN_UNDEF ||
           symbol->Type() == STT_SECTION)
        {
            // skip
            continue;
        }

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

        printf("%s,0x%08X,%s\n", type_name, section_base + symbol_value, symbol_name);
    }
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

    if(argc >= 3)
    {
        section_bases = &argv[2];
        num_section_bases = argc - 2;

        uint32_t text_base = get_section_base(".text");
        uint32_t rodata_base = get_section_base(".rodata");

        //printf("%d\n", num_section_bases);
        //printf("%08X\n", text_base);
        //printf("%08X\n", rodata_base);
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
    fread(elf_buffer, 1, elf_size, elf_fp);

    elf = new CElfContext(elf_buffer, elf_size);

    elf_report_symbols(elf);

    free(elf_buffer);
    fclose(elf_fp);
    delete elf;
}