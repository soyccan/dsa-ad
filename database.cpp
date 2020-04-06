// TODO: do strncmp behave correctly on unsigned char?
#include "database.h"

#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <parallel/algorithm>

#include "common.h"
#include "hex.h"


EntryValue* criteo_entries;
EntryKeyUPT* sorted_criteo_entries_upt;
EntryKeyPU* sorted_criteo_entries_pu;
EntryKeyUT* sorted_criteo_entries_ut;


static void __sort_criteo_data()
{
    // TODO:  qsort vs. std::sort, which faster?

    GG(sorted_criteo_entries_pu = new EntryKeyPU[NUM_ENTRY], NULL);
    GG(sorted_criteo_entries_ut = new EntryKeyUT[NUM_ENTRY], NULL);

    // only upt is assigned keys already
    // 0: sorted by (user_id, product_id, click_time)
    for (size_t j = 0; j < NUM_ENTRY; j++) {
        sorted_criteo_entries_upt[j].value = criteo_entries + j;
    }
    __gnu_parallel::sort(sorted_criteo_entries_upt,
                         sorted_criteo_entries_upt + NUM_ENTRY,
                         [](const EntryKeyUPT& x, const EntryKeyUPT& y) {
                             int res = MEMCMP(x.user_id, y.user_id);
                             if (res != 0)
                                 return res < 0;
                             res = MEMCMP(x.product_id, y.product_id);
                             if (res != 0)
                                 return res < 0;
                             return x.click_time < y.click_time;
                         });
    // FOR(j, 0, NUM_ENTRY) DBG("upt[%d]=%d", j, sorted_criteo_entries_upt[j]);

    // 1: sorted by (product_id, user_id)
    for (size_t j = 0; j < NUM_ENTRY; j++) {
        MEMCPY(sorted_criteo_entries_pu[j].product_id,
               sorted_criteo_entries_upt[j].product_id);
        MEMCPY(sorted_criteo_entries_pu[j].user_id,
               sorted_criteo_entries_upt[j].user_id);
        sorted_criteo_entries_pu[j].value = sorted_criteo_entries_upt[j].value;
    }
    __gnu_parallel::sort(sorted_criteo_entries_pu,
                         sorted_criteo_entries_pu + NUM_ENTRY,
                         [](const EntryKeyPU& x, const EntryKeyPU& y) {
                             int res = MEMCMP(x.product_id, y.product_id);
                             if (res != 0)
                                 return res < 0;
                             return MEMCMP(x.user_id, y.user_id) < 0;
                         });
    // FOR(j, 0, NUM_ENTRY) DBG("pu[%d]=%d", j, sorted_criteo_entries_pu[j]);

    // 2: sorted by (user_id, click_time)
    for (size_t j = 0; j < NUM_ENTRY; j++) {
        MEMCPY(sorted_criteo_entries_ut[j].user_id,
               sorted_criteo_entries_upt[j].user_id);
        sorted_criteo_entries_ut[j].click_time =
            sorted_criteo_entries_upt[j].click_time;
        sorted_criteo_entries_ut[j].value = sorted_criteo_entries_upt[j].value;
    }
    __gnu_parallel::sort(sorted_criteo_entries_ut,
                         sorted_criteo_entries_ut + NUM_ENTRY,
                         [](const EntryKeyUT& x, const EntryKeyUT& y) {
                             int res = MEMCMP(x.user_id, y.user_id);
                             if (res != 0)
                                 return res < 0;
                             return x.click_time < y.click_time;
                         });
    // FOR(j, 0, NUM_ENTRY) DBG("ut[%d]=%d", j, sorted_criteo_entries_ut[j]);

    DBG("sort complete");
}

static void __load_criteo_data(const char* criteo_filename)
{
    int fd;
    FILE* fs;
    char* buf;

    // O_DIRECT to bypass kernel buffer cache
    // though not much effiency improvement
    G(fd = open(criteo_filename, O_RDONLY | O_DIRECT));
    GG(fs = fdopen(fd, "r"), NULL);

    // 0x400 is enough to handle longest line
    GG(buf = new char[0x400], NULL);

    GG(criteo_entries = new EntryValue[NUM_ENTRY], NULL);

    // only upt is assigned first
    GG(sorted_criteo_entries_upt = new EntryKeyUPT[NUM_ENTRY], NULL);

    int i = 0;
    for (i = 0; i < NUM_ENTRY; i++) {
        if (fgets(buf, 0x400, fs) == NULL)
            break;

        // TODO:
        // strncpy won't append null-byte if length is exactly 32, this is great
        // Deleted: ~~note the appended null-byte (by scanf) is overwritten~~
        char *s = buf, *ps;
        for (int j = 0; j < 23; j++) {
            if (j < 22)
                ps = strsep(&s, "\t");
            else
                ps = strsep(&s, "\n");

            if (j == 0)
                criteo_entries[i].sale = atoi(ps);
            else if (j == 3)
                sorted_criteo_entries_upt[i].click_time = atoi(ps);
            else if (j == 5)
                MEMCPY(criteo_entries[i].product_price, ps);
            else if (j == 6)
                MEMCPY(criteo_entries[i].product_age_group, ps);
            else if (j == 9)
                MEMCPY(criteo_entries[i].product_gender, ps);
            else if (j == 19) {
                assert(strlen(ps) == 2 || strlen(ps) == 32);
                assert(*ps);  // product_id should not be empty string
                if (*ps == '-') {
                    // but in case of "-1", we see it as "0000..."
                    // TODO: is "000..." valid? may be duplicate key?
                    memset(sorted_criteo_entries_upt[i].product_id, 0,
                           sizeof sorted_criteo_entries_upt[i].product_id);
                } else {
                    hexcpy(sorted_criteo_entries_upt[i].product_id, ps, 32);
                }
            } else if (j == 22) {
                assert(strlen(ps) == 32);
                assert(*ps);         // user_id should not be empty string
                assert(*ps != '-');  // or "-1"
                hexcpy(sorted_criteo_entries_upt[i].user_id, ps, 32);
            }
        }
        // DBGN("i:%d user:", i);
        // hexprint(criteo_entries[i].user_id, 16, stderr);
        // DBGN(" product:");
        // hexprint(criteo_entries[i].product_id, 16, stderr);
        // DBG(" time:%d", criteo_entries[i].click_time);
    }
    DBG("loaded entries: %d", i);  // 15995634
    fclose(fs);
    free(buf);
}

void init_criteo_data(const char* criteo_filename)
{
    if (!criteo_entries) {
        __load_criteo_data(criteo_filename);
        __sort_criteo_data();
    }
}