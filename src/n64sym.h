/*

n64sym

Symbol identifier tool for N64 games

shygoo 2017
License: MIT

*/

#include <stdarg.h>
#include <dirent.h>
#include <stdlib.h>
#include <vector>
#include <algorithm>

#include "arutil.h"
#include "elfutil.h"
#include "threadpool.h"

class CN64Sym
{
	typedef struct
	{
		CN64Sym* mt_this;
		const char* libraryPath;
		const char* blockIdentifier;
		void* blockData;
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

	typedef std::vector<search_result_t> search_results_t;
	typedef std::vector<const char*> str_vector_t;

	uint8_t* m_Data;
	size_t m_DataSize;
	search_results_t* m_Results;
	str_vector_t* m_LibPaths;
	bool m_bVerbose;

private:

	static void* ProcessObjectProc(void* _objProcessingCtx);

	static bool EndsWith(const char *str, const char *suffix);
	static bool PathIsStaticLibrary(const char *path);
	static bool PathIsObjectFile(const char *path);
	static bool IsFileWithSymbols(const char *path);

	static bool ResultCmp(search_result_t a, search_result_t b);
	static bool ResultCmp2(search_result_t a, search_result_t b);

	CN64Sym(FILE* pfile);
	
public:

	~CN64Sym();

	static CN64Sym* Create(const char* binPath);
	size_t DataSize();

	void AddLibPath(const char* libPath);
	bool Run();

	void ScanRecursive(const char* path);

	void ProcessFile(const char* filePath);
	void ProcessObject(obj_processing_context_t* objProcessingCtx);
	void ProcessObject(const char* path);
	void ProcessLibrary(const char* path);

	bool ElfTextDataCompare(CElfContext* elf, const char* data, int* nBytesMatched);

	void SetVerbose(bool bVerbose);
	void Log(const char* format, ...);
	
	bool AddResult(search_result_t result);
	void SortResults();
	void DumpResults();

	void AddSymbolResults(CElfContext* elf, uint32_t baseAddress, int maxTextOffset = 0);
	void AddRelocationResults(CElfContext* elf, const char* block, const char* altNamePrefix, int maxTextOffset = 0);
};