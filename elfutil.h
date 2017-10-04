/*

elfutil.h

Wrapper for elfio

shygoo 2017
License: MIT

*/

#ifndef ELFUTIL_H
#define ELFUTIL_H

#include <elfio/elfio.hpp>
#include "mipsrel.h"

using namespace ELFIO;

typedef struct
{
	std::string name;
	Elf64_Addr value;
	Elf_Xword size;
	unsigned char bind;
	unsigned char type;
	Elf_Half section_index;
	unsigned char other;
} ELF_SYMBOL;

typedef struct
{
	uint32_t offset;
	uint16_t unk;
	uint8_t symbol_index;
	uint8_t type;
} ELF_RELOCATION;

class CElfObject
{
private:

	elfio m_Elf;

	section* m_SymTabSec;
	section* m_TextSec;
	section* m_RelTextSec;

	const char* m_TextData;
	int m_TextSize;
	bool m_bTextEmpty;

	const char* m_RelTextData;
	int m_RelTextSize;
	int m_nRelTextEntries;

	symbol_section_accessor* m_SymbolAccessor;
	int m_nSymbols;

public:

	static CElfObject* Create(std::istream& stream, std::streampos start_pos = 0)
	{
		return new CElfObject(stream, start_pos);
	}

	static CElfObject* Create(const char* path)
	{
		std::ifstream stream;
		stream.open(path, std::ios::in | std::ios::binary);

		if((stream.rdstate() & std::ifstream::failbit) != 0)
		{
			return NULL;
		}

		return new CElfObject(stream, 0);
	}

private:

	CElfObject(std::istream& stream, std::streampos start_pos = 0 ) :
		m_SymTabSec(NULL),
		m_TextSec(NULL),
		m_RelTextSec(NULL),
		m_SymbolAccessor(NULL),
		m_nSymbols(0),
		m_TextData(NULL),
		m_TextSize(0),
		m_bTextEmpty(true),
		m_RelTextData(NULL),
		m_RelTextSize(0),
		m_nRelTextEntries(0)
	{
		m_Elf.load(stream, start_pos);
		Init();
	}

public:

	~CElfObject()
	{
		 delete m_SymbolAccessor;
	}

	void Init()
	{
		// Fetch .text
		m_TextSec = m_Elf.sections[".text"];

		if(m_TextSec != NULL)
		{
			m_TextData = m_TextSec->get_data();
			m_TextSize = m_TextSec->get_size();

			for(int i = 0; i < m_TextSize; i++)
			{
				if(m_TextData[i] != 0)
				{
					m_bTextEmpty = false;
					break;
				}
			}
		}

		// Fetch .rel.text
		m_RelTextSec = m_Elf.sections[".rel.text"];

		if(m_RelTextSec != NULL)
		{
			m_RelTextData = m_RelTextSec->get_data();
			m_RelTextSize = m_RelTextSec->get_size();

			m_nRelTextEntries = m_RelTextSize / sizeof(ELF_RELOCATION);
		}

		// Fetch .symtab
		m_SymTabSec = m_Elf.sections[".symtab"];

		if(m_SymTabSec != NULL)
		{
			m_SymbolAccessor = new symbol_section_accessor(m_Elf, m_SymTabSec); 
			m_nSymbols = m_SymbolAccessor->get_symbols_num();
		}

		//DumpSectionHex(".text");
		//DumpRelTextEntries();
		//DumpSymbols();
	}

	bool HasTextSection()
	{
		return m_TextSec != NULL;
	}

	int TextSize()
	{
		return m_TextSize;
	}

	bool HasRelTextSection()
	{
		return m_RelTextSec != NULL;
	}

	bool GetSymbol(int index, ELF_SYMBOL& symbol)
	{
		if(GetNumSymbols() == 0)
		{
			return false;
		}

		m_SymbolAccessor->get_symbol(index, symbol.name, symbol.value, symbol.size, symbol.bind,
			symbol.type, symbol.section_index, symbol.other);

		return true;
	}

	int GetNumSymbols()
	{
		return m_nSymbols;
	}

	int GetNumRelTextEntries()
	{
		return m_nRelTextEntries;
	}

	bool GetRelTextEntry(int index, ELF_RELOCATION& relocation)
	{
		ELF_RELOCATION* rels = (ELF_RELOCATION*)m_RelTextSec->get_data();
		relocation = rels[index];
		
		relocation.offset = __builtin_bswap32(relocation.offset);

		return true;
	}

	bool TextMatchesData(const char* buf)
	{
		if(m_RelTextSec == NULL)
		{
			// no relocations, block needs to match exactly
			if(memcmp(m_TextData, buf, m_TextSize) == 0)
			{
				return true;
			}
			return false;
		}

		ELF_RELOCATION rel;
		int relIndex = 0;

		GetRelTextEntry(relIndex, rel);

		for(int offset = 0; offset < m_TextSize; offset += 4)
		{
			if(relIndex < m_nRelTextEntries && offset == rel.offset)
			{
				// TODO: should handle relocation based on its type
				// for now just ignore the entire instruction
				relIndex++;
				GetRelTextEntry(relIndex, rel);
				continue;
			}

			if(*(uint32_t*)&m_TextData[offset] != *(uint32_t*)&buf[offset])
			{
				return false;
			}
		}
		return true;
	}

	void DumpSectionHex(const char* sectionName, int byteGroupSize = 4, int byteRowSize = 16)
	{
		section* elfSection = m_Elf.sections[sectionName];
		
		uint8_t* data = (uint8_t*) elfSection->get_data();
		int size = elfSection->get_size();

		printf("%s dump:\n", sectionName);

		int i;

		for(i = 0; i < size; i++)
		{
			if(i % byteRowSize == 0)
			{
				printf("  %08X: ", i);
			}

			printf("%02X", data[i]);

			if((i+1) % byteRowSize == 0)
			{
				printf("\n");
			}
			else if((i+1) % byteGroupSize == 0)
			{
				printf(" ");
			}
		}

		if(i % byteRowSize != 0)
		{
			printf("\n");
		}

		printf("\n");
	}

	void DumpSymbols()
	{
		printf("Symbols:\n");

		for(int i = 0; i < m_nSymbols; i++)
		{
			ELF_SYMBOL symbol;
			GetSymbol(i, symbol);

			printf("  name: %-18s, size: %08X, type: %d, bind: %d, index: %04X, other: %d\n",
				symbol.name.c_str(), symbol.size, symbol.type, symbol.bind, symbol.section_index, symbol.other);
		}

		printf("\n");
	}

	void DumpRelTextEntries()
	{
		printf("Text relocations:\n");

		for(int i = 0; i < m_nRelTextEntries; i++)
		{
			ELF_RELOCATION rel;
			GetRelTextEntry(i, rel);
		
			ELF_SYMBOL symbol;
			GetSymbol(rel.symbol_index, symbol);

			const char* szType = get_mips_rel_string(rel.type);

			printf("  offset: %08X, symbol: %d %-16s, type: %02X (%s)\n",
				rel.offset, rel.symbol_index, symbol.name.c_str(), rel.type, szType);
		}

		printf("\n");
	}
};

/*

void elf_list_sections(ELFIO::elfio* lpElf)
{
	printf("elf sections:\n");
	int nSections = lpElf->sections.size();

	for(int i = 0; i < nSections; i++)
	{
		std::string name = lpElf->sections[i]->get_name();
		ELFIO::Elf_Xword size = lpElf->sections[i]->get_size();
		printf("%s %d\n", name.c_str(), size);
	}
}

void elf_print_report(elfio* lpElf)
{
	elf_list_sections(lpElf);
	elf_hexdump_section(lpElf, ".text");

	//elf_hexdump_section(&elf, ".note");
	elf_list_m_SymbolAccessor(lpElf);

	if(lpElf->sections[".rel.text"] != NULL)
	{
		//elf_hexdump_section(lpElf, ".rel.text", 4, 8);
		elf_list_relocations(lpElf, ".rel.text");
	}

	if(lpElf->sections[".data"] != NULL && lpElf->sections[".data"]->get_size() > 0)
	{
		printf("HAS DATA ------------------------------------------------------------------------------------");

		elf_hexdump_section(lpElf, ".data", 4, 8);
	}

	if(lpElf->sections[".rodata"] != NULL)
	{
		printf("HAS RODATA ------------------------------------------------------------------------------------");
		elf_hexdump_section(lpElf, ".rodata", 4, 8);
	}

	if(lpElf->sections[".rel.rodata"] != NULL)
	{
		elf_list_relocations(lpElf, ".rel.rodata");
	}
}
*/

#endif // ELFUTIL_H