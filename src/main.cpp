/*

n64sym

Symbol identifier tool for N64 games

shygoo 2017
License: MIT

*/

#include <stdlib.h>
#include <stdio.h>

#include "n64sym.h"

bool SwitchLibPaths(CN64Sym* n64sym, int argc, const char* argv[], int& argi)
{
	int npaths = 0;
	
	for(; argi + 1 < argc; argi++)
	{
		const char* lib_path = argv[argi + 1];
	
		if(lib_path[0] == '-')
		{
			break;
		}
		
		n64sym->AddLibPath(lib_path);
		npaths++;
	}
	
	if(npaths == 0)
	{
		return false;
	}
	
	return true;
}

/***************************************/

int main(int argc, const char* argv[])
{
	const char* binPath;
	CN64Sym* n64sym;
	bool berrors = false;
	
	if(argc < 2)
	{
		printf (
			"n64sym - N64 symbol identification tool\n\n"
			"  Usage: n64sym <RAM dump path> [options]\n\n"
			"  Options:\n"
			"    -l <library/object path(s)>  scan for symbols from library or object file(s)\n"
			"    -v                           enable verbose logging\n"
			// "    -r                           enable ROM-to-RAM address resolution (experimental)\n"
		);
		return 0;
	}
	
	binPath = argv[1];
	
	n64sym = CN64Sym::Create(binPath);

	if(n64sym == NULL)
	{
		printf("Error: Failed load '%s'\n", binPath);
		return 0;
	}
	
	for(int argi = 2; argi < argc; argi++)
	{
		if(argv[argi][0] != '-')
		{
			printf("Error: Unexpected '%s' in command line\n", argv[argi]);
			goto exit_cleanup;
		}
		
		switch(argv[argi][1])
		{
		case 'l':
			if(!SwitchLibPaths(n64sym, argc, argv, argi))
			{
				printf("Error: No library/object path(s)");
				goto exit_cleanup;
			}
			break;
		case 'v':
			n64sym->SetVerbose(true);
			break;
		default:
			printf("Error: Invalid switch '%s'\n", argv[argi]);
			goto exit_cleanup;
		}
	}

	n64sym->Run();
	n64sym->SortResults();
	n64sym->DumpResults();
	
  exit_cleanup:
	delete n64sym;
	return 0;
}