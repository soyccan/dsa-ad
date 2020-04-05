
// TODO: do strncmp behave correctly on unsigned char?
#include "database.h"

#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <parallel/algorithm>

#include "common.h"
#include "hex.h"


Entry* criteo_entries;
int* sorted_criteo_entries_upt;
int* sorted_criteo_entries_pu;
int* sorted_criteo_entries_ut;


static void __sort_criteo_data()
{
    // TODO:  qsort vs. std::sort, which faster?

    GG(sorted_criteo_entries_upt =
           reinterpret_cast<int*>(malloc(NUM_ENTRY * sizeof(int))),
       NULL);
    GG(sorted_criteo_entries_pu =
           reinterpret_cast<int*>(malloc(NUM_ENTRY * sizeof(int))),
       NULL);
    GG(sorted_criteo_entries_ut =
           reinterpret_cast<int*>(malloc(NUM_ENTRY * sizeof(int))),
       NULL);

    // 0: sorted by (user_id, product_id, click_time)
    FOR(j, 0, NUM_ENTRY) { sorted_criteo_entries_upt[j] = j; }
    __gnu_parallel::sort(
        sorted_criteo_entries_upt, sorted_criteo_entries_upt + NUM_ENTRY,
        [](int x, int y) {
            int res =
                memcmp(criteo_entries[x].user_id, criteo_entries[y].user_id,
                       sizeof criteo_entries->user_id);
            if (res != 0)
                return res < 0;
            res = memcmp(criteo_entries[x].product_id,
                         criteo_entries[y].product_id,
                         sizeof criteo_entries->product_id);
            if (res != 0)
                return res < 0;
            return criteo_entries[x].click_time < criteo_entries[y].click_time;
        });
    // FOR(j, 0, NUM_ENTRY) DBG("upt[%d]=%d", j, sorted_criteo_entries_upt[j]);

    // 1: sorted by (product_id, user_id)
    FOR(j, 0, NUM_ENTRY) { sorted_criteo_entries_pu[j] = j; }
    __gnu_parallel::sort(
        sorted_criteo_entries_pu, sorted_criteo_entries_pu + NUM_ENTRY,
        [](int x, int y) {
            int res = memcmp(criteo_entries[x].product_id,
                             criteo_entries[y].product_id,
                             sizeof criteo_entries->product_id);
            if (res != 0)
                return res < 0;
            return memcmp(criteo_entries[x].user_id, criteo_entries[y].user_id,
                          sizeof criteo_entries->user_id) < 0;
        });
    // FOR(j, 0, NUM_ENTRY) DBG("pu[%d]=%d", j, sorted_criteo_entries_pu[j]);

    // 2: sorted by (user_id, click_time)
    FOR(j, 0, NUM_ENTRY)
    {
        sorted_criteo_entries_ut[j] = sorted_criteo_entries_upt[j];
    }
    __gnu_parallel::sort(
        sorted_criteo_entries_ut, sorted_criteo_entries_ut + NUM_ENTRY,
        [](int x, int y) {
            int res =
                memcmp(criteo_entries[x].user_id, criteo_entries[y].user_id,
                       sizeof criteo_entries->user_id);
            if (res != 0)
                return res < 0;
            return criteo_entries[x].click_time < criteo_entries[y].click_time;
        });
    // FOR(j, 0, NUM_ENTRY) DBG("ut[%d]=%d", j, sorted_criteo_entries_ut[j]);

    DBG("sort complete");
}

static void __load_criteo_data(const char* criteo_filename)
{
    FILE* fs;
    char* buf;

    GG(fs = fopen(criteo_filename, "r"), NULL);
    GG(buf = reinterpret_cast<char*>(malloc(0x400 * sizeof(char))),
       NULL);  // 0x400 is enough to handle longest line

    GG(criteo_entries =
           reinterpret_cast<Entry*>(malloc(NUM_ENTRY * sizeof(Entry))),
       NULL);

    int i = 0;
    for (i = 0; i < NUM_ENTRY; i++) {
        if (fgets(buf, 0x400, fs) == NULL)
            break;

        // TODO:
        // strncpy won't append null-byte if length is exactly 32, this is great
        // Deleted: ~~note the appended null-byte (by scanf) is overwritten~~
        char *s = buf, *ps;
        FOR(j, 0, 23)
        {
            if (j < 22)
                ps = strsep(&s, "\t");
            else
                ps = strsep(&s, "\n");

            if (j == 0)
                criteo_entries[i].sale = atoi(ps);
            else if (j == 3)
                criteo_entries[i].click_time = atoi(ps);
            else if (j == 5)
                strncpy(criteo_entries[i].product_price, ps,
                        sizeof criteo_entries[i].product_price);
            else if (j == 6)
                strncpy(criteo_entries[i].product_age_group, ps,
                        sizeof criteo_entries[i].product_age_group);
            else if (j == 9)
                strncpy(criteo_entries[i].product_gender, ps,
                        sizeof criteo_entries[i].product_gender);
            else if (j == 19) {
                assert(*ps);  // product_id should not be empty string
                if (*ps == '-')
                    // but in case of "-1", we see it as "0000..."
                    // TODO: is "000..." valid? may be duplicate key?
                    memset(criteo_entries[i].product_id, 0,
                           sizeof criteo_entries[i].product_id);
                else
                    hexcpy(criteo_entries[i].product_id, ps, 32);
            } else if (j == 22) {
                assert(*ps);         // user_id should not be empty string
                assert(*ps != '-');  // or "-1"
                hexcpy(criteo_entries[i].user_id, ps, 32);
            }
        }
        DBGN("i:%d user:", i);
        hexprint(criteo_entries[i].user_id, 16, stderr);
        DBGN(" product:");
        hexprint(criteo_entries[i].product_id, 16, stderr);
        DBG(" time:%d", criteo_entries[i].click_time);
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