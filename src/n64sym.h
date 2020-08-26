/*

    n64sym
    Symbol identifier tool for N64 games
    shygoo 2017, 2020
    License: MIT

*/

#ifndef N64SYM_H
#define N64SYM_H

#include <stdarg.h>
#include <dirent.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <set>

#include "arutil.h"
#include "elfutil.h"
#include "threadpool.h"
#include "signaturefile.h"
#include "pathutil.h"

class CN64Sym
{
public:
    CN64Sym();
    ~CN64Sym();
    bool LoadBinary(const char *binPath);
    void AddLibPath(const char* libPath);
    void UseBuiltinSignatures(bool bUseBuiltinSignatures);
    void SetVerbose(bool bVerbose);
    void SetThoroughScan(bool bThorough);
    bool Run();
    void DumpResults();

private:
    typedef struct
    {
        CN64Sym* mt_this;
        const char* libraryPath;
        const char* blockIdentifier;
        uint8_t* blockData;
        size_t blockSize;
    } obj_processing_context_t;

    typedef struct
    {
        uint32_t address; // from jump target
        uint32_t size; // data match size
        char name[64];
    } search_result_t;

    typedef struct
    {
        uint32_t address;
        int nBytesMatched;
    } partial_match_t;

    CThreadPool threadPool;

    uint8_t* m_Binary;
    size_t   m_BinarySize;
    uint32_t m_HeaderSize;

    std::vector<search_result_t> m_Results;
    std::vector<const char*> m_LibPaths;
    std::set<uint32_t> m_LikelyFunctionOffsets;

    bool m_bVerbose;
    bool m_bUseBuiltinSignatures;
    bool m_bThoroughScan;

    pthread_mutex_t m_ProgressMutex;
    size_t m_NumSymbolsToCheck;
    size_t m_NumSymbolsChecked;

    CSignatureFile m_BuiltinSigs;

    void ScanRecursive(const char* path);

    void ProcessFile(const char* path);
    void ProcessLibrary(const char* path);
    void ProcessObject(const char* path);
    void ProcessObject(obj_processing_context_t* objProcessingCtx);
    static void* ProcessObjectProc(void* _objProcessingCtx);
    void ProcessSignatureFile(const char* path);
    void ProcessSignatureFile(CSignatureFile& sigFile);

    bool TestElfObjectText(CElfContext* elf, const char* data, int* nBytesMatched);
    bool TestSignatureSymbol(CSignatureFile& sigFile, size_t nSymbol, uint32_t offset);

    void TallyNumSymbolsToCheck();
    void CountSymbolsRecursive(const char *path);
    size_t CountSymbolsInFile(const char *path);
    size_t CountGlobalSymbolsInElf(CElfContext& elf);

    bool AddResult(search_result_t result);
    void AddSymbolResults(CElfContext* elf, uint32_t baseAddress, int maxTextOffset = 0);
    void AddRelocationResults(CElfContext* elf, const char* block, const char* altNamePrefix, int maxTextOffset = 0);
    static bool ResultCmp(search_result_t a, search_result_t b);
    void SortResults();

    void ProgressInc(size_t numSymbols);
    void Log(const char* format, ...);
    static void ClearLine(int nChars);
};

#endif // N64SYM_H
