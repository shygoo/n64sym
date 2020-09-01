/*

    n64sig
    Signature file generator for n64sym
    shygoo 2020
    License: MIT

*/

#include <cstdint>
#include <string>
#include <map>
#include <vector>

#include "elfutil.h"

#ifndef N64SIG_H
#define N64SIG_H

class CN64Sig
{
    typedef struct {
        uint8_t relocType;
        char relocSymbolName[128];
        //uint32_t param;
    } reloc_entry_t;

    struct reloc_entry_cmp_t
    {
        bool operator()(const reloc_entry_t& a, const reloc_entry_t& b) const
        {
            int t = strcmp(a.relocSymbolName, b.relocSymbolName);
            if(t == 0)
            {
                return a.relocType < b.relocType;
            }
            return t < 0;
        }
    };

    typedef std::map<reloc_entry_t, std::vector<uint16_t>, reloc_entry_cmp_t> reloc_map_t;

    typedef struct
    {
        char         name[64];
        uint32_t     size;
        uint32_t     crc_a;
        uint32_t     crc_b;
        reloc_map_t *relocs;
    } symbol_entry_t;

    std::map<uint32_t, symbol_entry_t> m_SymbolMap;
    std::vector<const char *> m_LibPaths;

    bool   m_bVerbose;
    size_t m_NumProcessedSymbols;
    
    static const char *GetRelTypeName(uint8_t relType);
    static void FormatAnonymousSymbol(char *symbolName);
    void StripAndGetRelocsInSymbol(const char *objectName, reloc_map_t& relocs, CElfSymbol *symbol, CElfContext& elf);
    void ProcessLibrary(const char *path);
    void ProcessObject(CElfContext& elf, const char *objectName);
    void ProcessObject(const char *path);
    void ProcessFile(const char *path);
    void ScanRecursive(const char* path);

public:
    CN64Sig();
    ~CN64Sig();

    void AddLibPath(const char *path);
    void SetVerbose(bool bVerbose);
    bool Run();
};

#endif
