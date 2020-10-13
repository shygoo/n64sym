#include <cstring>
#include "pathutil.h"

// returns true if 'str' ends with 'suffix'
static bool EndsWith(const char *str, const char *suffix)
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

// extracts file name without extension
size_t PathGetFileName(const char *path, char *dstName, size_t maxLength)
{
    if(!path)
    {
        return 0;
    }

    size_t i = strlen(path);

    if(i == 0)
    {
        return 0;
    }

    const char *start = path;

    while(i--)
    {
        if(path[i] == '/' || path[i] == '\\')
        {
            start = &path[i + 1];
            break;
        }
    }

    const char *end = strchr(&path[i + 1], '.');

    if(!end || (size_t)((end - start) + 1) >= maxLength)
    {
        strncpy(dstName, start, maxLength);
        return maxLength;
    }
    
    strncpy(dstName, start, end - start);
    dstName[end - start] = '\0';
    return end - start;
}

bool PathIsStaticLibrary(const char *path)
{
    if (strlen(path) < 3)
    {
        return false;
    }
    return EndsWith(path, ".a") || EndsWith(path, ".A");
}

bool PathIsObjectFile(const char *path)
{
    if (strlen(path) < 3)
    {
        return false;
    }
    return EndsWith(path, ".o") || EndsWith(path, ".O");
}

bool PathIsSignatureFile(const char *path)
{
    if (strlen(path) < 5)
    {
        return false;
    }
    return EndsWith(path, ".sig") || EndsWith(path, ".SIG");
}

bool PathIsN64Rom(const char *path)
{
    if(strlen(path) < 5)
    {
        return false;
    }

    return (
        EndsWith(path, ".z64") ||
        EndsWith(path, ".n64") ||
        EndsWith(path, ".v64") ||
        EndsWith(path, ".Z64") ||
        EndsWith(path, ".N64") ||
        EndsWith(path, ".V64"));
}

bool IsFileWithSymbols(const char *path)
{
    return PathIsStaticLibrary(path) || PathIsObjectFile(path) || PathIsSignatureFile(path);
}
