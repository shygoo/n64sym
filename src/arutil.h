/*

    arutil

    Basic GNU *.a reader utility
    shygoo 2018, 2020
    License: MIT

    https://en.wikipedia.org/wiki/Ar_(Unix)

*/

#ifndef ARUTIL_H
#define ARUTIL_H

#include <stdlib.h>
#include <stdint.h>

#define AR_FILE_SIG "!<arch>\n"
#define AR_FILE_SIG_LEN 8

class CArReader
{
    typedef struct
    {
        char szIdentifier[16];
        char szTimestamp[12];
        char szOwnerId[6];
        char szGroupId[6];
        char szFileMode[8];
        char szSize[10];
        char szEndChar[2];
    } ar_header_t;

    char* m_CurRealIdentifier;
    char* m_ExIdentifierBlock;
    uint8_t* m_CurBlock;
    size_t m_CurBlockSize;
    
    uint8_t* m_Buffer;
    size_t m_Size;
    size_t m_CurPos;

    static char* ArTrimIdentifier(char* str);

public:
    CArReader();
    ~CArReader();

    bool Load(const char *path);

    bool SeekNextBlock();
    const char* GetBlockIdentifier();
    uint8_t* GetBlockData();
    size_t GetBlockSize();
};

#endif // ARUTIL_H
