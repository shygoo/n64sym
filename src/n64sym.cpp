#include "n64sym.h"

CN64Sym* CN64Sym::Create(const char* binPath)
{
	FILE* pfile = fopen(binPath, "rb");

	if(pfile == NULL)
	{
		return NULL;
	}

	return new CN64Sym(pfile);
}

CN64Sym::CN64Sym(FILE* pfile) :
	m_Data(NULL),
	m_DataSize(0),
	m_Results(NULL),
	m_LibPaths(NULL),
	m_bVerbose(false)
{
	fseek(pfile, 0, SEEK_END);
	
	m_DataSize = ftell(pfile);
	m_Data = (uint8_t*) malloc(m_DataSize);

	rewind(pfile);
	fread(m_Data, 1, m_DataSize, pfile);
	
	m_Results = new search_results_t;
	m_LibPaths = new str_vector_t;
}

CN64Sym::~CN64Sym()
{
	m_Results->clear();
	delete m_Results;

	m_LibPaths->clear();
	delete m_LibPaths;

	free(m_Data);
}

void CN64Sym::Log(const char* format, ...)
{
	if(!m_bVerbose	)
	{
		return;
	}

	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
}

void CN64Sym::AddLibPath(const char* libPath)
{
	m_LibPaths->push_back(libPath);
}

bool CN64Sym::AddResult(search_result_t result)
{
	// have link address from jump target
	if(result.address != 0)
	{
		for(int i = 0; i < m_Results->size(); i++)
		{
			search_result_t test_result = m_Results->at(i);
			if(test_result.address == result.address)
			{
				return false; // already have
			}
		}
		m_Results->push_back(result);
		//Log("    %08X %s\n", result.address, result.name);
		return true;
	}
	m_Results->push_back(result);
}

bool CN64Sym::ResultCmp(search_result_t a, search_result_t b)
{
	return (a.address < b.address);
}

bool CN64Sym::ResultCmp2(search_result_t a, search_result_t b)
{
	return (a.address < b.address);
}

void CN64Sym::SortResults()
{
	std::sort(m_Results->begin(), m_Results->end(), ResultCmp);
}

void CN64Sym::DumpResults()
{
	for(int i = 0; i < m_Results->size(); i++)
	{
		search_result_t result = m_Results->at(i);
		printf("%08X,code,%s\n", result.address, result.name);
	}
}

void CN64Sym::SetVerbose(bool bVerbose)
{
	m_bVerbose = bVerbose;
}

size_t CN64Sym::DataSize()
{
	return m_DataSize;
}

void CN64Sym::AddSymbolResults(CElfContext* elf, uint32_t baseAddress, int maxTextOffset)
{
	int nSymbols = elf->NumSymbols();

	for(int i = nSymbols - 1; i >= 0; i--)
	{
		CElfSymbol* symbol = elf->Symbol(i);
		if(symbol->Binding() == STB_GLOBAL &&
		   symbol->Type() != STT_NOTYPE &&
		   symbol->SectionIndex() != SHN_UNDEF &&
		   symbol->Size() > 0)
		{
			if(maxTextOffset > 0 && symbol->Value() >= maxTextOffset)
			{
				// exceeds maximum offset for a partial match
				continue;
			}

			search_result_t result;
			//result.file_address = 0;
			result.address = 0x80000000 | (baseAddress + symbol->Value());
			result.size = symbol->Size();
			strcpy(result.name, symbol->Name(elf));

			Log("adding %s\n", result.name);
			AddResult(result);
		}
	}
}

void CN64Sym::AddRelocationResults(CElfContext* elf, const char* block, const char* altNamePrefix, int maxTextOffset)
{
	Log("Adding relocation results...\n");

	int nRelocations = elf->NumTextRelocations();

	for(int i = 0; i < nRelocations; i++)
	{
		CElfRelocation* relocation = elf->TextRelocation(i);
		CElfSymbol* symbol = relocation->Symbol(elf);
		int textOffset = relocation->Offset();
		uint32_t opcode = bswap32(*(uint32_t*)&block[textOffset]);
		uint8_t relType = relocation->Type();

		Log("%s %04X\n", symbol->Name(elf), textOffset);

		if(maxTextOffset > 0 && textOffset >= maxTextOffset)
		{
			// exceeds maximum offset for a partial match
			continue;
		}
		
		if(relType == R_MIPS_26 && (opcode >> 26) == 0x0C)
		{
			uint32_t jalTarget = 0x80000000 | ((opcode & 0x3FFFFFF) * 4);
			
			search_result_t result;
			result.address = jalTarget;
			result.size = 0;
			strncpy(result.name, symbol->Name(elf), 64);

			if(relocation->SymbolIndex() == 1)
			{
				// Static function, compiler tossed out the symbol
				// Use object file name and text offset as a replacement
				int len = sprintf(result.name, "%s_%04X", altNamePrefix, textOffset);
				for(int i = 0; i < len; i++)
				{
					if(result.name[i] == '.')
					{
						result.name[i] = '_';
					}
				}
			}

			Log("adding %s (relocation)\n", result.name);

			AddResult(result);
		}
		else if(relType == R_MIPS_LO16 && i > 0)
		{
			CElfRelocation* prevRelocation = elf->TextRelocation(i - 1);

			if(prevRelocation->Type() == R_MIPS_HI16)
			{
				uint32_t upperOp = bswap32(*(uint32_t*)&block[prevRelocation->Offset()]);
				uint32_t lowerOp = opcode;

				// TODO: Implement

				CElfSymbol* symbol = relocation->Symbol(elf);
				Log("%04X%04X,data,%s\n", upperOp & 0xFFFF, lowerOp & 0xFFFF, symbol->Name(elf));
			}
		}
	}
}

bool CN64Sym::ElfTextDataCompare(CElfContext* elf, const char* data, int* nBytesMatched)
{
    CElfSection *text_sec, *rel_text_sec;
    CElfRelocation* text_relocations;
    const char* text_sec_data;
    uint32_t text_sec_size;
    int num_text_relocations;

    text_sec = elf->Section(".text");

    if(text_sec == NULL)
    {
		*nBytesMatched = 0;
        return false;
    }

    text_sec_data = text_sec->Data(elf);
    text_sec_size = text_sec->Size();
    num_text_relocations = elf->NumTextRelocations();

    if(num_text_relocations == 0)
    {
        // no relocations, do plain binary comparison
		if(memcmp(text_sec_data, data, text_sec_size) == 0)
		{
			*nBytesMatched = text_sec_size;
			return true;
		}
        
		*nBytesMatched = 0;
		return false;
    }

    rel_text_sec = elf->Section(".rel.text");
    text_relocations = (CElfRelocation*) rel_text_sec->Data(elf);

    int cur_reltab_index = 0;
    uint32_t cur_relocation_offset = 0;
	
    for(int i = 0; i < text_sec_size; i += sizeof(uint32_t))
    {
		//bool have_relocation = false;
		// see if this opcode has a relocation

		CElfRelocation* curRelocation = &text_relocations[cur_reltab_index];

		if(cur_reltab_index < num_text_relocations && i == curRelocation->Offset())
		{
			// if for some reason the relocation is on a NOP, don't count it
			if(data[i] == 0x00000000)
			{
				*nBytesMatched == i;
				return false;
			}

			// only check the top 6 bits
			if((text_sec_data[i] & 0xFC) != (data[i] & 0xFC))
			{
				*nBytesMatched = i;
				return false;
			}

			cur_reltab_index++;
			continue;
		}

		// fetch the next relocation
		// this trusts that the object's relocation offsets are in order from least to greatest
		//for(int j = cur_reltab_index; j < num_text_relocations; j++)
		//{
		//	CElfRelocation* cur_relocation = &text_relocations[j];
		//	cur_relocation_offset = cur_relocation->Offset();
		//	
		//	if(cur_relocation_offset < i)
		//	{
		//		continue;
		//	}
//
		//	if(cur_relocation_offset == i)
		//	{
		//		have_relocation = true;
		//	}
		//}

        if(*(uint32_t*)&text_sec_data[i] != *(uint32_t*)&data[i])
        {
			*nBytesMatched = i;
            return false;
        }
    }
    return true;
}

void CN64Sym::ProcessObject(obj_processing_context_t* objProcessingCtx)
{
	CElfContext elf((const char*)objProcessingCtx->blockData, objProcessingCtx->blockSize);
	CElfSection* textSec = elf.Section(".text");

	if(textSec == NULL)
	{
		return;
	}

	const char* textBuf = textSec->Data(&elf);
	uint32_t textSize = textSec->Size();

	int endAddress = DataSize() - textSize;

	bool bHaveFullMatch;
	uint32_t matchedAddress;
	int nBytesMatched;
	int bestPartialMatchLength = 0;
	const char* matchedBlock = NULL;

	for(uint32_t blockAddress = 0; blockAddress < endAddress; blockAddress += sizeof(uint32_t))
	{
		const char* block = (const char*)&m_Data[blockAddress];
		bHaveFullMatch = ElfTextDataCompare(&elf, block, &nBytesMatched);

		if(bHaveFullMatch)
		{
			matchedBlock = block;
			matchedAddress = blockAddress;
			break;
		}
		else if(nBytesMatched > bestPartialMatchLength)
		{
			matchedBlock = block;
			matchedAddress = blockAddress;
			bestPartialMatchLength = nBytesMatched;
		}
	}

	threadPool.LockDefaultMutex();

	Log("%s:%s\n", objProcessingCtx->libraryPath, objProcessingCtx->blockIdentifier);

	if(bHaveFullMatch)
	{
		
		Log("complete match\n");
		AddSymbolResults(&elf, matchedAddress);
		AddRelocationResults(&elf, matchedBlock, "__"); // fix me altNamePrefix
	}
	else if(bestPartialMatchLength >= 32)
	{
		Log("partial match (0x%02X bytes)\n", bestPartialMatchLength);
		AddSymbolResults(&elf, matchedAddress, bestPartialMatchLength);
		AddRelocationResults(&elf, matchedBlock, "__", bestPartialMatchLength); // fix me altNamePrefix
	}
	else
	{
		threadPool.UnlockDefaultMutex();
		return;
	}

	for(size_t i = 0; i < textSize; i += 4)
	{
		uint32_t buffOp = bswap32(*(uint32_t*)&matchedBlock[i]);
		uint32_t textOp = bswap32(*(uint32_t*)&textBuf[i]);

		CElfRelocation* relocation = NULL;
		CElfSymbol* symbol = NULL;
		bool bHaveRel = false;

		for(int j = 0; j < elf.NumTextRelocations(); j++)
		{
			relocation = elf.TextRelocation(j);
			if(relocation->Offset() == i)
			{
				Log("have reloc\n");
				symbol = relocation->Symbol(&elf);
				bHaveRel = true;
			}
		}

		Log("%08X/%04X: %08X %08X", 0x80000000 | (matchedAddress + i), i, buffOp, textOp);
		textOp == buffOp ? Log("\n") : Log(" * %s\n", bHaveRel ? symbol->Name(&elf) : "");
	}

	Log("\n");
	threadPool.UnlockDefaultMutex();
}

void CN64Sym::ProcessObject(const char* path)
{
    void* buffer;
    size_t size;

	FILE* fp = fopen(path, "rb");

    if(fp == NULL)
    {
        return;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    rewind(fp);

    buffer = malloc(size);
    fread(buffer, 1, size, fp);
    fclose(fp);

	Log("%s\n", path);

	obj_processing_context_t objProcessingCtx;
	objProcessingCtx.mt_this = NULL;
	objProcessingCtx.libraryPath = NULL;
	objProcessingCtx.blockIdentifier = path;
	objProcessingCtx.blockData = buffer;
	objProcessingCtx.blockSize = size;

	ProcessObject(&objProcessingCtx);

	free(buffer);
}

void* CN64Sym::ProcessObjectProc(void* _objProcessingCtx)
{
	obj_processing_context_t* objProcessingCtx = (obj_processing_context_t*)_objProcessingCtx;
	CN64Sym* _this = objProcessingCtx->mt_this;

	_this->ProcessObject(objProcessingCtx);

	delete objProcessingCtx;
}

void CN64Sym::ProcessLibrary(const char* path)
{
	CArReader ar(path);

	while(ar.SeekNextBlock())
	{
		// worker thread will delete objProcessingCtx after it's done
		obj_processing_context_t* objProcessingCtx = new obj_processing_context_t;
		objProcessingCtx->mt_this = this;
		objProcessingCtx->libraryPath = path;
		objProcessingCtx->blockIdentifier = ar.GetBlockIdentifier();
		objProcessingCtx->blockData = ar.GetBlockData();
		objProcessingCtx->blockSize = ar.GetBlockSize();

		threadPool.AddWorker(ProcessObjectProc, (void*)objProcessingCtx);
	}

	threadPool.WaitForWorkers();
}

// returns true if 'str' ends with 'suffix'
bool CN64Sym::EndsWith(const char *str, const char *suffix)
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

bool CN64Sym::PathIsStaticLibrary(const char *path)
{
	if (strlen(path) < 3)
	{
		return false;
	}
	return EndsWith(path, ".a") || EndsWith(path, ".A");
}

bool CN64Sym::PathIsObjectFile(const char *path)
{
	if (strlen(path) < 3)
	{
		return false;
	}
	return EndsWith(path, ".o") || EndsWith(path, ".O");
}

bool CN64Sym::IsFileWithSymbols(const char *path)
{
	return PathIsStaticLibrary(path) || PathIsObjectFile(path);
}

void CN64Sym::ProcessFile(const char* filePath)
{
	//Log("%s\n", filePath);
	if (PathIsStaticLibrary(filePath))
	{
		ProcessLibrary(filePath);
	}
	else if (PathIsObjectFile(filePath))
	{
		ProcessObject(filePath);
	}
}

void CN64Sym::ScanRecursive(const char* path)
{
	if (IsFileWithSymbols(path))
	{
		ProcessFile(path);
		return;
	}
	DIR *dir;
	dir = opendir(path);
	if (dir == NULL)
	{
		printf("%s is neither a directory or file with symbols.\n", path);
		return;
	}
	struct dirent *entry;
	while ((entry = readdir(dir)) != NULL)
	{
		char next_path[PATH_MAX];
		if (!entry->d_name) continue;
		snprintf(next_path, sizeof(next_path), "%s/%s", path, entry->d_name);
		switch (entry->d_type) {
			case DT_DIR:
				// skip "." dirs
				if (entry->d_name[0] == '.')
				{
					continue;
				}
				// scan subdirectory
				ScanRecursive(next_path);
				break;
			case DT_REG:
			{
				if (IsFileWithSymbols(next_path))
				{
					//printf("next path %s\n", next_path);
					ProcessFile(next_path);
				}
				break;
			}
			default:
				break;
		}
	}
	closedir(dir);
}

bool CN64Sym::Run()
{
	for(int i = 0; i < m_LibPaths->size(); i++)
	{
		ScanRecursive(m_LibPaths->at(i));
	}

	Log("\n");
}