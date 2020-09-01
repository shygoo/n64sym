/*

    Signature file reader for n64sym
    shygoo 2020
    License: MIT

*/

#ifndef SIGNATUREFILE_H
#define SIGNATUREFILE_H

#include <cstdint>
#include <vector>

class CSignatureFile
{
private:
    typedef struct
    {
        const char *name;
        uint8_t     type;
        uint32_t    offset;
    } reloc_t;

    typedef struct
    {
        const char *name;
        uint32_t    size;
        uint32_t    crcA;
        uint32_t    crcB;
        std::vector<reloc_t> *relocs;
    } symbol_info_t;

    char  *m_Buffer;
    size_t m_Size;
    size_t m_Pos;

    std::vector<symbol_info_t> m_Symbols;

    static bool ParseNumber(const char *str, uint32_t *result);
    static int GetRelocationDirectiveValue(const char *str);
    static bool RelocOffsetCompare(const reloc_t& a, const reloc_t& b);
    static void ReadStrippedWord(uint8_t *dst, const uint8_t *src, int relType);

    void SkipWhitespace();
    char *GetNextToken();
    bool AtEndOfLine();
    void Parse();
    bool IsEOF();

    void SortRelocationsByOffset();

public:
    CSignatureFile();
    ~CSignatureFile();
    bool Load(const char *path);
    bool LoadFromMemory(const char *contents);
    size_t GetNumSymbols();
    uint32_t GetSymbolSize(size_t nSymbol);
    bool GetSymbolName(size_t nSymbol, char *str, size_t nMaxChars);
    bool TestSymbol(size_t nSymbol, const uint8_t *buffer);

    // relocs
    size_t GetNumRelocs(size_t nSymbol);
    bool GetRelocName(size_t nSymbol, size_t nReloc, char *str, size_t nMaxChars);
    uint8_t GetRelocType(size_t nSymbol, size_t nReloc);
    uint32_t GetRelocOffset(size_t nSymbol, size_t nReloc);
};

#endif // SIGNATUREFILE_H
