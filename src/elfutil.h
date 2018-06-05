/*

    elfutil

    Basic 32-bit big endian ELF reader
    shygoo 2018
    License: MIT

    https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
    http://www.skyfree.org/linux/references/ELF_Format.pdf
    http://www.sco.com/developers/devspecs/mipsabi.pdf

*/

#ifndef ELFUTIL_H
#define ELFUTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifndef bswap32
	#ifdef __GNUC__
		#define bswap32 __builtin_bswap32
	#elif _MSC_VER
		#define bswap32 _byteswap_ulong
	#else
		#define bswap32(n) (((unsigned)n & 0xFF000000) >> 24 | (n & 0xFF00) << 8 | (n & 0xFF0000) >> 8 | n << 24)
	#endif
#endif

#ifndef bswap16
	#ifdef __GNUC__
		#define bswap16 __builtin_bswap16
	#elif _MSC_VER
		#define bswap16 _byteswap_ushort
	#else
		#define bswap16(n) (((unsigned)n & 0xFF00) >> 8 | n << 8)
	#endif
#endif

#define EI_MAG0        0
#define EI_MAG1        1
#define EI_MAG2        2
#define EI_MAG3        3
#define EI_CLASS       4
#define EI_DATA        5
#define EI_VERSION     6
#define EI_OSABI       7
#define EI_ABIVERSION  8
#define EI_PAD         7
#define EI_NIDENT     16

#define ELFCLASSNONE 0 // Invalid class
#define ELFCLASS32   1 // 32-bit objects
#define ELFCLASS64   2 // 64-bit objects

// mips relocation types
#define R_MIPS_NONE     0
#define R_MIPS_16       1
#define R_MIPS_32       2
#define R_MIPS_REL32    3
#define R_MIPS_26       4
#define R_MIPS_HI16     5
#define R_MIPS_LO16     6
#define R_MIPS_GPREL16  7
#define R_MIPS_LITERAL  8
#define R_MIPS_GOT16    9
#define R_MIPS_CALL16  21

// special section numbers
#define SHN_UNDEF 0
#define SHN_LORESERVE 0xff00
#define SHN_LOPROC 0xff00
#define SHN_HIPROC 0xff1f
#define SHN_ABS 0xfff1
#define SHN_COMMON 0xfff2
#define SHN_HIRESERVE 0xffff

// symbol bindings
#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOPROC 13
#define STB_HIPROC 15

// symbol types
#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_LOPROC 13
#define STT_HIPROC 15

class CElfContext;
class CElfHeader;
class CElfSection;
class CElfSymbol;
class CElfRelocation;

typedef struct CElfHeader
{
    uint8_t  e_ident[EI_NIDENT];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} CElfHeader;

class CElfContext
{
    CElfHeader* m_ElfHeader;
    size_t m_Size;

    CElfContext();

public:
    CElfContext(const char* buffer, size_t bufferSize);

    uint8_t  ABI() { return m_ElfHeader->e_ident[EI_OSABI]; }
    uint16_t Machine() { return bswap16(m_ElfHeader->e_machine); }
    uint32_t SectionHeaderOffset() { return bswap32(m_ElfHeader->e_shoff); }
    uint16_t SectionHeaderEntrySize() { return bswap16(m_ElfHeader->e_shentsize); }
    uint16_t NumSections() { return bswap16(m_ElfHeader->e_shnum); }
    uint16_t SectionNamesIndex() { return bswap16(m_ElfHeader->e_shstrndx); }

    CElfHeader* Header() { return m_ElfHeader; }
    size_t Size() { return m_Size; }

    CElfSection* Section(int index);
    CElfSection* Section(const char* name);
    bool SectionIndexOf(const char* name, int* index);

    int NumSymbols();
    CElfSymbol* Symbol(int index);

    int NumTextRelocations();
    CElfRelocation* TextRelocation(int index);
};

class CElfSection
{
    uint32_t sh_name;
    uint32_t sh_type;
    uint32_t sh_flags;
    uint32_t sh_addr;
    uint32_t sh_offset;
    uint32_t sh_size;
    uint32_t sh_link;
    uint32_t sh_info;
    uint32_t sh_addralign;
    uint32_t sh_entsize;

public:
    uint32_t NameOffset() { return bswap32(sh_name); }
    uint32_t Offset() { return bswap32(sh_offset); }
    uint32_t Size() { return bswap32(sh_size); }

    const char* Name(CElfContext* elf);
    const char* Data(CElfContext* elf);
};

class CElfSymbol
{
    uint32_t st_name;
    uint32_t st_value;
    uint32_t st_size;
    uint8_t st_info;
    uint8_t st_other;
    uint16_t st_shndx;

public:
    uint32_t NameOffset(){ return bswap32(st_name); }
    uint32_t Value() { return bswap32(st_value); }
    uint32_t Size() { return bswap32(st_size); }
    uint8_t Info() { return st_info; }
    uint8_t Other() { return st_other; }
    uint16_t SectionIndex() { return bswap16(st_shndx); }

    uint8_t Type(){ return (uint8_t)(Info() & 0x0F); }
    uint8_t Binding(){ return (uint8_t)(Info() >> 4); }
    const char* Name(CElfContext* elf);
    CElfSection* Section(CElfContext* elf);
};

class CElfRelocation
{
    uint32_t r_offset;
    uint32_t r_info;

public:
    uint32_t Offset(){ return bswap32(r_offset); }
    uint32_t Info(){ return bswap32(r_info); }
    uint32_t SymbolIndex(){ return Info() >> 8; }
    uint8_t Type(){ return (uint8_t)(Info() & 0x0F); }

    CElfSymbol* Symbol(CElfContext* elf);
};

#endif // ELFUTIL_H
