#ifndef _DATABASE_H_
#define _DATABASE_H_

#define NUM_ENTRY 15995634
#define CRITEO_ENTRIES_SHM_NAME "/criteo_entries"
#define CRITEO_ENTRIES_SHM_KEY 9487

// CRITICAL: entry order is important, since scanf will append null-byte
//           if the order in structure is diffent from the order in data
//           file, may produce wrong result
struct Entry {
    // int id;
    // size_t offset;  // offset in file
    bool sale;
    int click_time;  // TODO: 64-bit?
    char product_price[8];
    char product_age_group[32];
    char product_gender[32];
    char product_id[32];
    char user_id[32];
};

extern Entry* criteo_entries;
extern int sorted_criteo_entries_upt[NUM_ENTRY];
extern int sorted_criteo_entries_pu[NUM_ENTRY];
extern int sorted_criteo_entries_ut[NUM_ENTRY];

void load_criteo_data();
void sort_criteo_data();
void init_criteo_data();

#endif