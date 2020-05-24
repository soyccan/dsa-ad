#ifndef _HEX_H_
#define _HEX_H_ 1

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "common.h"

#ifndef NDEBUG
#define DBGHEX(s) hexprint(s, sizeof(s), stderr);
#else
#define DBGHEX(...)
#endif

static inline int hex2bits(char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    else if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    else {
        DBG("hex2bits invalid char");
        assert(false);
        return -1;
    }
}

/* big-endian for lexigraphical order */
static inline int hex2byte(const char s[2])
{
    int x = hex2bits(s[0]);
    int y = hex2bits(s[1]);
    if (x < 0 || y < 0)
        return -1;
    return ((x & 0xf) << 4) | (y & 0xf);
}

static inline void hexcpy(uint8_t* dest, const char* src, size_t srclen)
{
    for (size_t i = 0; i < srclen; i += 2) {
        int x = hex2byte(src + i);
        if (x >= 0)
            dest[i >> 1] = static_cast<uint8_t>(x);
    }
}

static inline void hexprint(const uint8_t* src, size_t n, FILE* stream)
{
    // special value for product_id
    static char zeros[16];
    if (memcmp(src, zeros, 16) == 0) {
        fprintf(stream, "-1");
        return;
    }
    for (size_t i = 0; i < n; i++) {
        uint8_t c = src[i];
        for (int j = 0; j < 2; j++) {
            uint8_t x = (c & 0xf0) >> 4;
            if (x <= 9)
                fputc('0' + x, stream);
            else
                fputc('A' + x - 10, stream);
            c <<= 4;
        }
    }
}

#endif