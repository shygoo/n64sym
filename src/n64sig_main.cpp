/*

    n64sig
    Signature file generator for n64sym
    shygoo 2020
    License: MIT

*/

#include <cstdio>
#include <cstdlib>

#include "n64sig.h"

int main(int argc, const char *argv[])
{
    if(argc < 2)
    {
        printf (
            "n64sig - signature file generator for n64sym (https://github.com/shygoo/n64sym)\n\n"
            "  Usage: n64sig [options]\n\n"
            "  Options:\n"
            "    -l <lib/obj path>     add a library/object path\n"
            "    -f <format>           set the output format (json, default)\n"
        );

        return EXIT_FAILURE;
    }

    CN64Sig n64sig;

    for(int argi = 1; argi < argc; argi++)
    {
        //printf("[%s]\n", argv[argi]);

        if(argv[argi][0] != '-')
        {
            printf("Error: Unexpected '%s' in command line\n", argv[argi]);
            return EXIT_FAILURE;
        }

        if(strlen(&argv[argi][1]) != 1)
        {
            printf("Error: Invalid switch '%s'\n", argv[argi]);
            return EXIT_FAILURE;
        }

        switch(argv[argi][1])
        {
        case 'l':
            if(argi+1 >= argc)
            {
                printf("Error: No path specified for '-l'\n");
            }
            n64sig.AddLibPath(argv[argi+1]);
            argi++;
            break;
        case 'f':
            if(argi+1 >= argc)
            {
                printf("Error: No output format specified for '-f'\n");
                return EXIT_FAILURE;
            }
            if(!n64sig.SetOutputFormat(argv[argi+1]))
            {
                printf("Error: Invalid output format '%s'\n", argv[argi+1]);
                return EXIT_FAILURE;
            }
            argi++;
            break;
        case 'v':
            n64sig.SetVerbose(true);
            break;
        }
    }

    n64sig.Run();

    return EXIT_SUCCESS;
}
