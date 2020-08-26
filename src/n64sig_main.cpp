/*

    n64sig
    Signature file generator for n64sym
    shygoo 2020
    License: MIT

*/

//#include <string.h>
//#include <stdio.h>
//#include <vector>
//#include <map>
//#include <dirent.h>
//#include <fstream>
//
//#include "crc32.h"
//#include "arutil.h"
//#include "elfutil.h"
//#include "pathutil.h"


#include <cstdio>
#include <cstdlib>

#include "n64sig.h"

int main(int argc, const char *argv[])
{
    if(argc < 2)
    {
        printf(
            "n64sig - signature file generator for n64sym (https://github.com/n64sym)\n\n"
            "  Usage: n64sig <library/object path(s)>\n"
        );

        return EXIT_FAILURE;
    }

    CN64Sig n64sig;

    // n64sig.SetVerbose(true);

    n64sig.AddLibPath(argv[1]);
    n64sig.Run();

    return EXIT_SUCCESS;
}
