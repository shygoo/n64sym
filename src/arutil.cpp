/*

    arutil

    Basic GNU *.a reader utility
    shygoo 2018
    License: MIT

    https://en.wikipedia.org/wiki/Ar_(Unix)

*/

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "arutil.h"

char* CArReader::ArTrimIdentifier(char* str)
{
    char* org = str;

    while(*str != '\n' && *str != '\0' && *str != '/')
    {
        str++;
    }
    *str = '\0';

    return org;
}

CArReader::CArReader(const char* path):
        m_CurRealIdentifier(NULL),
        m_ExIdentifierBlock(NULL),
        m_CurBlock(NULL),
        m_CurBlockSize(0),
        m_Buffer(NULL),
        m_Size(0),
        m_CurPos(0)
{
    FILE* fp = fopen(path, "rb");

    if(fp == NULL)
    {
        return;
    }

    fseek(fp, 0, SEEK_END);
    m_Size = ftell(fp);
    rewind(fp);
    m_Buffer = (char*)malloc(m_Size);
    fread(m_Buffer, 1, m_Size, fp);
    fclose(fp);

    if(memcmp(AR_FILE_SIG, m_Buffer, AR_FILE_SIG_LEN) == 0)
    {
        m_CurPos += 8;
    }
    else
    {
        free(m_Buffer);
        m_Buffer = NULL;
        return;
    }
}

CArReader::~CArReader()
{
    if(m_Buffer != NULL)
    {
        free(m_Buffer);
    }
}

bool CArReader::SeekNextBlock()
{
    if(m_CurPos >= m_Size)
    {
        return false; // EOF
    }

    ar_header_t* header = (ar_header_t*)(m_Buffer + m_CurPos);

    m_CurPos += sizeof(ar_header_t);

    size_t blockSize = atoll(header->szSize);

    if(header->szIdentifier[0] == '/')
    {
        if(header->szIdentifier[1] == '/')
        {
			// extended identifier block
            m_ExIdentifierBlock = m_Buffer + m_CurPos;
            m_CurPos += blockSize;
            SeekNextBlock();
            return true;
        }
		
		if(header->szIdentifier[1] == ' ')
        {
            // symbol reference block, skip
            m_CurPos += blockSize;
            SeekNextBlock();
            return true;
        }
		
		// block uses extended identifier
		size_t exIdentifierOffset = atoll(&header->szIdentifier[1]);
		m_CurRealIdentifier = ArTrimIdentifier(&m_ExIdentifierBlock[exIdentifierOffset]);
    }
	else
	{
		m_CurRealIdentifier = ArTrimIdentifier(header->szIdentifier);
	}

    m_CurBlock = &m_Buffer[m_CurPos];
    m_CurBlockSize = blockSize;
    m_CurPos += blockSize;

    if(m_CurPos % 2 != 0)
    {
        m_CurPos++;
    }
    return true;
}

const char* CArReader::GetBlockIdentifier()
{
    return m_CurRealIdentifier;
}

void* CArReader::GetBlockData()
{
    return m_CurBlock;
}

size_t CArReader::GetBlockSize()
{
    return m_CurBlockSize;
}