#ifndef _DATABASE_H_
#define _DATABASE_H_ 1

#include <stdint.h>

#define NUM_ENTRY 15995634
// TODO: check if this is correct

/* keys and values are stored in different structure for cache friendly */
struct EntryValue {
    bool sale;
    char product_price[8];
    char product_age_group[32];
    char product_gender[32];
};

// for strings of length 16, they are converted from
// hexidecimal string to unsigned char to attain most effective space
struct EntryKeyUPT {
    uint8_t user_id[16];
    uint8_t product_id[16];
    int click_time;  // TODO: 64-bit?
    EntryValue* value;
};

struct EntryKeyPU {
    uint8_t product_id[16];
    uint8_t user_id[16];
    EntryValue* value;
};

struct EntryKeyUT {
    uint8_t user_id[16];
    int click_time;  // TODO: 64-bit?
    EntryValue* value;
};


extern EntryValue* criteo_entries;
extern EntryKeyUPT* sorted_criteo_entries_upt;
extern EntryKeyPU* sorted_criteo_entries_pu;
extern EntryKeyUT* sorted_criteo_entries_ut;

void init_criteo_data(const char* criteo_filename);

#endif