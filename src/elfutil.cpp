/*

    elfutil

    Basic 32-bit big endian ELF reader
    shygoo 2018, 2020
    License: MIT

    https://en.wikipedia.org/wiki/Executable_and_Linkable_Format
    http://www.skyfree.org/linux/references/ELF_Format.pdf
    http://www.sco.com/developers/devspecs/mipsabi.pdf

*/

#include <fstream>
#include <stdint.h>
#include "elfutil.h"

CElfContext::CElfContext():
    m_Buffer(NULL),
    m_Size(0)
{
}

bool CElfContext::Load(const char *path)
{
    std::ifstream file;
    file.open(path, std::ifstream::binary);
    if(!file.is_open())
    {
        return false;
    }
    file.seekg(0, file.end);
    m_Size = file.tellg();
    file.seekg(0, file.beg);
    m_Buffer = new uint8_t[m_Size];
    file.read((char *)m_Buffer, m_Size);
    return true;
}

bool CElfContext::LoadFromMemory(uint8_t *buffer, size_t size)
{
    if(m_Buffer != NULL)
    {
        delete[] m_Buffer;
    }

    m_Size = size;
    m_Buffer = new uint8_t[m_Size];
    memcpy(m_Buffer, buffer, m_Size);

    return true;
}

//////////////

CElfSection* CElfContext::Section(int index)
{
    if(index >= NumSections())
    {
        return NULL;
    }

    uint32_t offset = SectionHeaderOffset() + index * SectionHeaderEntrySize();

    if(offset >= Size())
    {
        return NULL;
    }

    return (CElfSection*)((char*)m_Buffer + offset);
}

CElfSection* CElfContext::Section(const char* name)
{
    int nsecs = NumSections();
    for(int i = 0; i < nsecs; i++)
    {
        CElfSection* sec = Section(i);
        const char *curName = sec->Name(this);

        if(curName != NULL &&strcmp(curName, name) == 0)
        {
            return sec;
        }
    }
    return NULL;
}

bool CElfContext::SectionIndexOf(const char* name, int* index)
{
    int nsecs = NumSections();
    for(int i = 0; i < nsecs; i++)
    {
        CElfSection* sec = Section(i);
        const char *curName = sec->Name(this);

        if(curName != NULL && strcmp(sec->Name(this), name) == 0)
        {
            *index = i;
            return true;
        }
    }
    return false;
}

int CElfContext::NumSymbols()
{
    CElfSection* sym_sec = Section(".symtab");
    if(sym_sec == NULL)
    {
        return 0;
    }
    return sym_sec->Size() / sizeof(CElfSymbol);
}

int CElfContext::NumTextRelocations()
{
    CElfSection* rel_text_sec = Section(".rel.text");
    if(rel_text_sec == NULL)
    {
        return 0;
    }
    return rel_text_sec->Size() / sizeof(CElfRelocation);
}

CElfRelocation* CElfContext::TextRelocation(int index)
{
    CElfSection* rel_text_sec = Section(".rel.text");
    if(rel_text_sec == NULL)
    {
        return NULL;
    }
    return (CElfRelocation*) (rel_text_sec->Data(this) + (index * sizeof(CElfRelocation)));
}

CElfSymbol* CElfContext::Symbol(int index)
{
    CElfSection* sym_sec = Section(".symtab");
    if(sym_sec == NULL)
    {
        return NULL;
    }
    return (CElfSymbol*) (sym_sec->Data(this) + (index * sizeof(CElfSymbol)));
}

//////////////

const char* CElfSection::Name(CElfContext* elf)
{
    CElfSection* shstr_sec = elf->Section(elf->SectionNamesIndex());

    if(shstr_sec == NULL)
    {
        return NULL;
    }

    uint32_t nameOffset = NameOffset();

    if(nameOffset >= shstr_sec->Size())
    {
        return NULL;
    }

    if(shstr_sec->Offset() + nameOffset >= elf->Size())
    {
        return NULL;
    }

    const char* shstr_data = shstr_sec->Data(elf);
    return &shstr_data[NameOffset()];
}

const char* CElfSection::Data(CElfContext* elf)
{
    uint32_t offset = Offset();

    if(offset >= elf->Size())
    {
        return NULL;
    }

    return ((char*)elf->Header()) + Offset();
}

//////////////

const char* CElfSymbol::Name(CElfContext* elf)
{
    CElfSection* str_sec = elf->Section(".strtab");

    if(str_sec == NULL)
    {
        return 0;
    }

    return (const char*)str_sec->Data(elf) + NameOffset();
}

CElfSection* CElfSymbol::Section(CElfContext* elf)
{
    return elf->Section(SectionIndex());
}

//////////////

CElfSymbol* CElfRelocation::Symbol(CElfContext* elf)
{
    return elf->Symbol(SymbolIndex());
}