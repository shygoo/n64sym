/*

arutil.h

Basic GNU *.a reader utility

shygoo 2017
License: MIT

https://en.wikipedia.org/wiki/Ar_(Unix)

*/

#ifndef ARUTIL_H
#define ARUTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <fstream>

#define AR_FILE_SIG     "!<arch>\n"
#define AR_FILE_SIG_LEN 8

typedef struct ar_block_info
{
    const char* sz_identifier;
    size_t size;
    const unsigned char* data;
    std::ifstream* lpStream;
} ar_block_info;

typedef void (*ar_block_processor) (const ar_block_info*, void* p);

typedef struct
{
    char sz_identifier[16];
    char sz_timestamp[12];
    char sz_owner_id[6];
    char sz_group_id[6];
    char sz_file_mode[8];
    char sz_size[10];
    char sz_end_char[2];
} ar_block_ascii_header;

static char* ar_trim_identifier(char* str)
{
    char* org = str;
    
    while(*str != '\n' && *str != '\0' && *str != '/')
    {
        str++;
    }
    *str = '\0';

    return org;
}

static bool ar_check_sig(std::ifstream& stream)
{
    char sz_sig[AR_FILE_SIG_LEN + 1];
    sz_sig[AR_FILE_SIG_LEN] = '\0';
	
    stream.read(sz_sig, AR_FILE_SIG_LEN);
	
	if(stream.gcount() != AR_FILE_SIG_LEN)
    {
        return false;
    }
	
	if(strcmp(sz_sig, AR_FILE_SIG) != 0)
	{
		return false;
	}
	
	return true;
}

bool ar_process_blocks(const char* path, ar_block_processor cb_process_block, void* p)
{
	std::ifstream stream;
	char* ex_identifier_block = NULL;

	if(path == NULL)
	{
		return false;
	}

	if(cb_process_block == NULL)
	{
		return false;
	}
	
    stream.open(path, std::ios::in | std::ios::binary);
	stream.seekg(0);

	if(!ar_check_sig(stream))
	{
		return false;
	}
	
    while(true)
    {
        int header_pos = stream.tellg();
		int block_pos = header_pos + sizeof(ar_block_ascii_header);
		size_t block_size;
		char* sz_real_identifier;
		
        ar_block_ascii_header header;
		
        stream.read((char*)&header, sizeof(ar_block_ascii_header));

        if(stream.gcount() == 0)
        {
            break;
        }
		
        block_size = atoll(header.sz_size);

        if(header.sz_identifier[0] == '/')
        {
            if(header.sz_identifier[1] == '/')
            {
				// extended identifier block
                if(ex_identifier_block != NULL)
                {
                   free(ex_identifier_block);
                }
                ex_identifier_block = (char*)malloc(block_size);
                stream.read(ex_identifier_block, block_size);
				continue;
            }
			
			if(header.sz_identifier[1] == ' ')
            {
                // symbol reference block, skip
                stream.seekg(block_pos + block_size);
				continue;
            }
			
			// block uses extended identifier
			uint32_t ex_identifier_offset = atoll(&header.sz_identifier[1]);
			sz_real_identifier = ar_trim_identifier(&ex_identifier_block[ex_identifier_offset]);
        }
		else
		{
			sz_real_identifier = ar_trim_identifier(header.sz_identifier);
		}
		
        if(cb_process_block != NULL)
        {
            ar_block_info block_info;
            block_info.sz_identifier = sz_real_identifier;
            block_info.size = block_size;
            block_info.lpStream = &stream;
			
            cb_process_block(&block_info, p);
        }

        uint32_t next_block_offset = block_pos + block_size;
		
        if(next_block_offset % 2 != 0)
        {
            next_block_offset++;
        }
		
        stream.seekg(next_block_offset);
    }
	
	if(ex_identifier_block != NULL)
	{
		free(ex_identifier_block);
	}
	
	stream.close();
}

#endif // ARUTIL_H