#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "common.h"

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

static inline int hex2byte(const char s[2])
{
    int x = hex2bits(s[0]);
    int y = hex2bits(s[1]);
    if (x < 0 || y < 0)
        return -1;
    return ((y & 0xf) << 4) | (x & 0xf);
}

static inline void hexcpy(uint8_t* dest, const char* src, size_t srclen)
{
    for (size_t i = 0; i < srclen; i += 2) {
        dest[i >> 1] = hex2byte(src + i);
    }
}

static inline void hexprint(uint8_t* src, size_t n, FILE* stream)
{
    FOR(i, 0, n)
    {
        FOR(j, 0, 2)
        {
            int x = src[i] & 0xf;
            if (x <= 9)
                fputc('0' + x, stream);
            else
                fputc('A' + x - 10, stream);
            src[i] >>= 4;
        }
    }
}