#ifndef _DATABASE_H_
#define _DATABASE_H_ 1

#include <stddef.h>

#define NUM_ENTRY 15995634

struct Entry {
    // these field is used as key when sorted
    int user_id;
    int product_id;
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
extern char (*keys)[32];
extern int keys_len;

void init_criteo_data(const char* criteo_filename);

#endif