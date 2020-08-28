/*

    n64sym
    Symbol identification tool for N64 games
    shygoo 2017, 2020
    License: MIT

*/

#include <stdlib.h>
#include <stdio.h>

#include "n64sym.h"

int main(int argc, const char* argv[])
{
    CN64Sym n64sym;
    const char* binPath;
    
    if(argc < 2)
    {
        printf (
            "n64sym - N64 symbol identification tool\n\n"
            "  Usage: n64sym <binary path> [options]\n\n"
            "  Options:\n"
            "    -s                         scan for symbols from built-in signature file\n"
            "    -l <sig/lib/obj path>      scan for symbols from signature/library/object file(s)\n"
            "    -f <format>                set the output format (pj64, nemu, armips, n64split, default)\n"
            "    -o <output path>           set the output path\n"
            "    -h <headersize>            set the headersize (default: 0x80000000)"
            "    -t                         scan thoroughly\n"
            "    -v                         enable verbose logging\n"
        );
        
        return 0;
    }

    binPath = argv[1];
    
    if(!n64sym.LoadBinary(binPath))
    {
        printf("Error: Failed to load '%s'\n", binPath);
        return EXIT_FAILURE;
    }
    
    for(int argi = 2; argi < argc; argi++)
    {
        if(argv[argi][0] != '-')
        {
            printf("Error: Unexpected '%s' in command line\n", argv[argi]);
            return EXIT_FAILURE;
        }
        
        switch(argv[argi][1])
        {
        case 'l':
            if(argi+1 >= argc)
            {
                printf("Error: No path path specified for '-l'\n");
            }
            n64sym.AddLibPath(argv[argi+1]);
            break;
        case 's':
            n64sym.UseBuiltinSignatures(true);
            break;
        case 't':
            n64sym.SetThoroughScan(true);
            break;
        case 'v':
            n64sym.SetVerbose(true);
            break;
        case 'f':
            if(argi+1 >= argc)
            {
                printf("Error: no output format specified for '-f'\n");
                return EXIT_FAILURE;
            }
            if(!n64sym.SetOutputFormat(argv[argi+1]))
            {
                printf("Error: invalid output format '%s'\n", argv[argi+1]);
                return EXIT_FAILURE;
            }
            argi++;
            break;
        case 'h':
            if(argi+1 >= argc)
            {
                printf("Error: no header size specified for '-h'\n");
                return EXIT_FAILURE;
            }
            n64sym.SetHeaderSize(strtoul(argv[argi+1], NULL, 0));
            argi++;
            break;
        case 'o':
            if(argi+1 >= argc)
            {
                printf("Error: no path specified for '-o'\n");
                return EXIT_FAILURE;
            }
            if(!n64sym.SetOutputPath(argv[argi+1]))
            {
                printf("Error: could not open '%s'\n", argv[argi+1]);
                return EXIT_FAILURE;
            }
            argi++;
            break;
        default:
            printf("Error: Invalid switch '%s'\n", argv[argi]);
            return EXIT_FAILURE;
        }
    }

    if(!n64sym.Run())
    {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
