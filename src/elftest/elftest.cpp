#include "../elfutil.h"
#include "../arutil.h"

#include <string.h>
#include <windows.h>

static HANDLE hConsoleHandle;
static WORD OriginalColors;

void con_init_color()
{
    hConsoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO *ConsoleInfo = new CONSOLE_SCREEN_BUFFER_INFO();
    GetConsoleScreenBufferInfo(hConsoleHandle, ConsoleInfo);
    OriginalColors = ConsoleInfo->wAttributes;
}

void con_color_restore()
{
    SetConsoleTextAttribute(hConsoleHandle, OriginalColors);
}

void con_color(WORD color)
{
    SetConsoleTextAttribute(hConsoleHandle, color);
}


void printd(const char* data, size_t length, int row)
{
    for(int i = 0; i < length; i++)
    {
        if(i%row == 0)
        {
            printf("%08X: ", i);
        }

        printf("%02X", (uint8_t)data[i]);

        if((i+1)%row == 0)
        {
            printf("\n");
        }
        else if((i+1)%4 == 0)
        {
            printf(" ");
        }
    }
}

const char* mips_rel_typename(int rel_type)
{
    switch(rel_type)
    {
        case R_MIPS_16   : return    "R_MIPS_16";
        case R_MIPS_32      : return "R_MIPS_32";
        case R_MIPS_REL32   : return "R_MIPS_REL32";
        case R_MIPS_26      : return "R_MIPS_26";
        case R_MIPS_HI16    : return "R_MIPS_HI16";
        case R_MIPS_LO16    : return "R_MIPS_LO16";
        case R_MIPS_GPREL16 : return "R_MIPS_GPREL16";
        case R_MIPS_LITERAL : return "R_MIPS_LITERAL";
        case R_MIPS_GOT16   : return "R_MIPS_GOT16";
        case R_MIPS_CALL16  : return "R_MIPS_CALL16";
        default:
        case R_MIPS_NONE: return "R_MIPS_NONE";
    }
}

void elf_dump_section(CElfContext* elf, const char* section_name, int row)
{
    CElfSection* sec = elf->Section(section_name);

    if(sec != NULL)
    {
        int sec_size = sec->Size();
        const char* sec_data = sec->Data(elf);
        printf("\n%s section (%d bytes):\n", section_name, sec_size);
        printd(sec_data, sec_size, row);
    }
}

// compare elf's text section to data, skipping opcodes with relocations
// return true if the data matches

void elf_dump(CElfContext* elf)
{

    //CElfSection* test_sec = elf->Section(0);
    //printf("[[[%s\n", test_sec->Name(elf));

    int nsymbols = elf->NumSymbols();

    int text_sec_index;
    elf->SectionIndexOf(".text", &text_sec_index);

    printf(".symtab:\n\n");
    con_color(8);
    printf("%3s  %-28s %-10s %-6s %-11s %-4s", "#", "name", "value", "size", "section", "info\n");
    con_color_restore();
    for(int i = 0; i < nsymbols; i++)
    {
        CElfSymbol* symbol = elf->Symbol(i);

        uint16_t secidx = symbol->SectionIndex();

        if(symbol->Binding() != STB_GLOBAL)
        {
            continue;
        }
        
        const char* secname = "";

        if(secidx > 0 && secidx < 0xF000)
        {
            CElfSection* sec = elf->Section(secidx);
            secname = sec->Name(elf);
            //printf("%s\n", );
        }
        else
        {
            switch(secidx)
            {
                case SHN_UNDEF: secname = "SHN_UNDEF"; break;
                case SHN_ABS: secname = "SHN_ABS"; break;
                case SHN_COMMON: secname = "SHN_COMMON"; break;
            }
        }

        con_color(8);
        printf("%3d:", i);
        (symbol->Info() > 0x10) ? con_color(10) : con_color(11);
        printf(" %-28s", symbol->Name(elf));
        con_color_restore();
        printf(" 0x%08X 0x%04X %-11s 0x%02X\n",
        symbol->Value(), symbol->Size(), secname, symbol->Info());
    }
    printf("\n");

    CElfSection* text_sec = elf->Section(".text");
    const char* text_sec_data = text_sec->Data(elf);
    int text_sec_size = text_sec->Size();

    CElfSection* text_rel_sec = elf->Section(".rel.text");
    int num_text_relocations = elf->NumTextRelocations();

    CElfRelocation* text_relocations = NULL;

    if(text_rel_sec)
    {
        text_relocations = (CElfRelocation*) text_rel_sec->Data(elf);
    }

    int rel_index = 0;

    CElfRelocation* rel = NULL;

    printf(".text (%d bytes):\n\n", text_sec->Size());

    for(int i = 0; i < text_sec_size; i += 4)
    {
        if(text_relocations != NULL)
        {
            rel = &text_relocations[rel_index];
        }
        
        uint32_t opcode = bswap32(*(uint32_t*)&text_sec_data[i]);

        bool printedop = true;

        con_color(8);
        printf(" %04X:", i);
        con_color_restore();
        printf(" %08X", opcode);

        // terribly slow
        for(int j = 0; j < nsymbols; j++)
        {
            CElfSymbol* symbol = elf->Symbol(j);

            if(symbol->Binding() != STB_GLOBAL)
            {
                continue;
            }

            if(symbol->Value() == i && symbol->SectionIndex() == text_sec_index)
            {
                printedop = true;
                con_color(8);

                con_color(10);
                printf(" %s", symbol->Name(elf));
                con_color_restore();
            }
        }
    
        if(rel != NULL && rel_index < num_text_relocations && i == rel->Offset())
        {
            if(!printedop)
            {
                //printedop = true;
                //con_color(8);
                //printf(" %04X:", i);
                //con_color_restore();
                //printf(" %08X", opcode);
            }
            const char* name = rel->Symbol(elf)->Name(elf);
            printf(" [%-11s ", mips_rel_typename(rel->Type()));
            con_color(11);
            printf("%s", name);
            con_color_restore();
            printf(" (%d)]", rel->SymbolIndex());
            rel_index++;
        }

        if(printedop)
        {
            printf("\n");
        }
    }

    printf("\n");
}

static bool EndsWith(const char *str, const char *suffix)
{
	if (!str || !suffix)
	{
		return false;
	}
	size_t len_str = strlen(str);
	size_t len_suffix = strlen(suffix);
	if (len_suffix > len_str)
	{
		return false;
	}
	return (0 == strncmp(str + len_str - len_suffix, suffix, len_suffix));
}

int main(int argc, const char* argv[])
{
    con_init_color();

    if(argc < 2)
    {
        printf("elftest <file>\n");
        return EXIT_FAILURE;
    }

    if(EndsWith(argv[1], ".a"))
    {
        CArReader ar(argv[1]);
        while(ar.SeekNextBlock())
        {
            CElfContext elf((const char*)ar.GetBlockData(), ar.GetBlockSize());
            elf_dump(&elf);
        }

        return EXIT_SUCCESS;
    }

    FILE* fp = fopen(argv[1], "rb");
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    const char* buffer = (const char*)malloc(size);
    rewind(fp);
    fread((void*)buffer, 1, size, fp);
    fclose(fp);

    CElfContext elf(buffer, size);

    elf_dump(&elf);

    return EXIT_SUCCESS;

    //CArReader ar("libgultra_rom.a");

    //while(ar.SeekNextBlock())
    //{
    //    printf("%s\n\n", ar.GetBlockIdentifier());
    //    
//
    //    elf_dump(&elf);
    //    printf("-------------------------\n");
    //}
}