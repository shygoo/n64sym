#ifndef CRC32_H
#define CRC32_H

#include <stddef.h>
#include <stdint.h>

uint32_t crc32(const uint8_t *bytes, size_t length);
uint32_t crc32_begin(void);
void crc32_read(const uint8_t *bytes, size_t length, uint32_t *result);
void crc32_end(uint32_t *result);

#endif // CRC32_H

