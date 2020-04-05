#ifndef _DATABASE_H_
#define _DATABASE_H_ 1

#include <stdint.h>

#define NUM_ENTRY 15995634

struct Entry {
    // these field is used as key when sorted
    // for strings of length 16, they are converted from
    // hexidecimal string to unsigned char to attain most effective space
    uint8_t user_id[16];
    uint8_t product_id[16];
    int click_time;  // TODO: 64-bit?

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

#endif