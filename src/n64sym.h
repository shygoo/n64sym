/*

    n64sym
    Symbol identifier tool for N64 games
    shygoo 2017, 2020
    License: MIT

*/

#ifndef N64SYM_H
#define N64SYM_H

#include <stdarg.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>
#include <set>
#include <fstream>

#include "arutil.h"
#include "elfutil.h"
#include "threadpool.h"
#include "signaturefile.h"
#include "pathutil.h"

typedef enum
{
    N64SYM_FMT_DEFAULT,
    N64SYM_FMT_PJ64,
    N64SYM_FMT_NEMU,
    N64SYM_FMT_ARMIPS,
    N64SYM_FMT_N64SPLIT,
    N64SYM_FMT_SPLAT
} n64sym_output_fmt_t;

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
    bool SetOutputFormat(const char *fmtName);
    void SetHeaderSize(uint32_t headerSize);
    bool SetOutputPath(const char *path);
    bool Run();
    void DumpResults();
    
private:
    typedef struct
    {
        const char *name;
        n64sym_output_fmt_t fmt;
    } n64sym_fmt_lut_t;

    static n64sym_fmt_lut_t FormatNames[];

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

    CThreadPool m_ThreadPool;

    uint8_t* m_Binary;
    size_t   m_BinarySize;
    uint32_t m_HeaderSize;

    bool     m_bVerbose;
    bool     m_bUseBuiltinSignatures;
    bool     m_bThoroughScan;
    bool     m_bOverrideHeaderSize;
    
    std::ostream *m_Output;
    std::ofstream m_OutputFile;

    n64sym_output_fmt_t m_OutputFormat;

    size_t m_NumSymbolsToCheck;
    size_t m_NumSymbolsChecked;

    pthread_mutex_t m_ProgressMutex;

    std::vector<search_result_t> m_Results;
    std::vector<const char*> m_LibPaths;
    std::set<uint32_t> m_LikelyFunctionOffsets;

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
    void CountSymbolsInFile(const char *path);
    size_t CountGlobalSymbolsInElf(CElfContext& elf);

    bool AddResult(search_result_t result);
    void AddSymbolResults(CElfContext* elf, uint32_t baseAddress, uint32_t maxTextOffset = 0);
    void AddRelocationResults(CElfContext* elf, const char* block, const char* altNamePrefix, int maxTextOffset = 0);
    static bool ResultCmp(search_result_t a, search_result_t b);
    void SortResults();

    void ProgressInc(size_t numSymbols);
    void Log(const char* format, ...);
    void Output(const char *format, ...);
    static void ClearLine(int nChars);
};

#endif // N64SYM_H
