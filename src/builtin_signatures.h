#ifndef BUILTIN_SIGNATURES_H
#define BUILTIN_SIGNATURES_H

#include <stdint.h>

typedef struct
{
    const uint32_t uncSize;
    const uint32_t cmpSize;
    const uint8_t  data[];
} asset_t;

extern asset_t gBuiltinSignatureFile;

#endif // BUILTIN_SIGNATURES_H
