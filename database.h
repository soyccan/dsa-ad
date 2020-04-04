#ifndef _DATABASE_H_
#define _DATABASE_H_ 1

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "common.h"

#define NUM_ENTRY 15995634

struct Entry {
    // these field is used as key when sorted
    // for strings of length 16, they are converted from
    // hexidecimal string to unsigned char to attain most effective space
    uint8_t product_id[16];
    uint8_t user_id[16];
    int click_time;  // TODO: 64-bit?

    uint8_t flag;  // flag bit:
                   // 0: product_id = "-1"
                   // 1: product_id = ""
                   // 2: user_id = "-1"
                   // 3: user_id = ""
    bool sale;
    char product_price[8];
    char product_age_group[32];
    char product_gender[32];
};

extern Entry* criteo_entries;
extern int* sorted_criteo_entries_upt;
extern int* sorted_criteo_entries_pu;
extern int* sorted_criteo_entries_ut;

void init_criteo_data(const char* criteo_filename);


static inline int __hex2byte(char c)
{
    if ('0' <= c && c <= '9')
        return c - '0';
    else if ('A' <= c && c <= 'F')
        return c - 'A' + 10;
    else {
        DBG("hex2byte invalid char");
        assert(false);
        return -1;
    }
}

static inline int __hexcpy(uint8_t* dest, const char* src, size_t srclen)
{
    for (size_t i = 0; i < srclen; i += 2) {
        int x = __hex2byte(src[i]), y = __hex2byte(src[i + 1]);
        if (x == -1 || y == -1)
            return -1;
        assert(!(x & ~0xf) && !(y & ~0xf));
        dest[i >> 1] = src[i] | (src[i + 1] << 4);
    }
    return 0;
}

static inline void __bytesprint(FILE* stream, uint8_t* src, size_t n)
{
    FOR(i, 0, n)
    {
        char c;
        int x = src[i] & 0xf, y = (src[i] >> 4) & 0xf;
        if (x <= 9)
            c = '0' + x;
        else
            c = 'A' + x - 10;
        fputc(c, stream);
        if (y <= 9)
            c = '0' + y;
        else
            c = 'A' + x - 10;
        fputc(c, stream);
    }
}

static inline int cmp_u(int x, int y)
{
    // "" < "-1" < "..." (16 bytes)
    if (criteo_entries[x].flag & 2) {    // x=""
        if (criteo_entries[y].flag & 2)  // y=""
            return 0;
        else
            return -1;
    } else if (criteo_entries[x].flag & 1) {  // x="-1"
        if (criteo_entries[y].flag & 2)       // y=""
            return 1;
        else if (criteo_entries[y].flag & 1)  // y="-1"
            return 0;
        else
            return -1;
    } else {
        return memcmp(criteo_entries[x].user_id, criteo_entries[y].user_id, 16);
    }
}

static inline int cmp_p(int x, int y)
{
    // "" < "-1" < "..." (16 bytes)
    if (criteo_entries[x].flag & 8) {    // x=""
        if (criteo_entries[y].flag & 8)  // y=""
            return 0;
        else
            return -1;
    } else if (criteo_entries[x].flag & 4) {  // x="-1"
        if (criteo_entries[y].flag & 8)       // y=""
            return 1;
        else if (criteo_entries[y].flag & 4)  // y="-1"
            return 0;
        else
            return -1;
    } else {
        return memcmp(criteo_entries[x].product_id,
                      criteo_entries[y].product_id, 16);
    }
}

#endif