#include <stdio.h>
#include <stdint.h>
#include "miniz/miniz.h"

typedef struct
{
    uint32_t uncSize;
    uint32_t cmpSize;
} header_t;

int main(int argc, const char *argv[])
{
    uLong src_len, cmp_len;
    uint8_t *src_buf, *cmp_buf;
    int cmp_status;
    const char *input_path, *output_path;
    FILE *fp_in, *fp_out;
    header_t header;

    if(argc < 3)
    {
        printf("compress <src> <dst>\n");
        return EXIT_FAILURE;
    }

    fp_in = fopen(argv[1], "rb");

    if(!fp_in)
    {
        printf("error: could not open %s\n", argv[1]);
        return EXIT_FAILURE;
    }

    fp_out = fopen(argv[2], "wb");

    if(!fp_out)
    {
        printf("error: could not open %s\n", argv[2]);
        return EXIT_FAILURE;
    }

    fseek(fp_in, 0, SEEK_END);
    
    src_len = ftell(fp_in);
    cmp_len = compressBound(src_len);

    src_buf = (uint8_t *)malloc(src_len);
    cmp_buf = (uint8_t *)malloc(cmp_len);

    fseek(fp_in, 0, SEEK_SET);
    fread(src_buf, 1, src_len, fp_in);

    cmp_status = compress(cmp_buf, &cmp_len, src_buf, src_len);

    if(cmp_status != Z_OK)
    {
        fclose(fp_in);
        fclose(fp_out);
        free(src_buf);
        free(cmp_buf);
        
        printf("error: compression failed\n", argv[2]);

        return EXIT_FAILURE;
    }

    header.uncSize = src_len;
    header.cmpSize = cmp_len;

    fwrite(&header, 1, sizeof(header), fp_out);
    fwrite(cmp_buf, 1, cmp_len, fp_out);

    fclose(fp_in);
    fclose(fp_out);
    free(src_buf);
    free(cmp_buf);

    return EXIT_SUCCESS;
}
