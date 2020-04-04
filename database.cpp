
// TODO: do strncmp behave correctly on unsigned char?
#include "database.h"

#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include <algorithm>

#include "common.h"


Entry* criteo_entries;
int* sorted_criteo_entries_upt;
int* sorted_criteo_entries_pu;
int* sorted_criteo_entries_ut;


static inline bool cmp_upt(int x, int y)
{
    int res;
    res = cmp_u(x, y);
    if (res != 0)
        return res < 0;
    res = cmp_p(x, y);
    if (res != 0)
        return res < 0;
    res = criteo_entries[x].click_time - criteo_entries[y].click_time;
    return res < 0;
}

static inline bool cmp_pu(int x, int y)
{
    int res;
    res = cmp_p(x, y);
    if (res != 0)
        return res < 0;
    res = cmp_u(x, y);
    return res < 0;
}

static inline bool cmp_ut(int x, int y)
{
    int res;
    res = cmp_u(x, y);
    if (res != 0)
        return res < 0;
    res = criteo_entries[x].click_time - criteo_entries[y].click_time;
    return res < 0;
}

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
    std::sort(sorted_criteo_entries_upt, sorted_criteo_entries_upt + NUM_ENTRY,
              cmp_upt);

    // 1: sorted by (product_id, user_id)
    FOR(j, 0, NUM_ENTRY) { sorted_criteo_entries_pu[j] = j; }
    std::sort(sorted_criteo_entries_pu, sorted_criteo_entries_pu + NUM_ENTRY,
              cmp_pu);

    // 2: sorted by (user_id, click_time)
    FOR(j, 0, NUM_ENTRY)
    {
        sorted_criteo_entries_ut[j] = sorted_criteo_entries_upt[j];
    }
    std::sort(sorted_criteo_entries_ut, sorted_criteo_entries_ut + NUM_ENTRY,
              cmp_ut);

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
        criteo_entries[i].flag = 0;

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
                strncpy(criteo_entries[i].product_price, ps, 32);
            else if (j == 6)
                strncpy(criteo_entries[i].product_age_group, ps, 32);
            else if (j == 9)
                strncpy(criteo_entries[i].product_gender, ps, 32);
            else if (j == 19) {
                if (strncmp(ps, "-1", 32) == 0)
                    criteo_entries[i].flag |= 1;
                else if (*ps == '\0')
                    criteo_entries[i].flag |= 2;
                else
                    __hexcpy(criteo_entries[i].product_id, ps, 32);
            } else if (j == 22) {
                if (strncmp(ps, "-1", 32) == 0)
                    criteo_entries[i].flag |= 4;
                else if (*ps == '\0')
                    criteo_entries[i].flag |= 8;
                else
                    __hexcpy(criteo_entries[i].user_id, ps, 32);
            }
        }
        DBGN("i:%d user:%.16s product:%.16s time:%u", i,
             criteo_entries[i].user_id, criteo_entries[i].product_id,
             criteo_entries[i].click_time);
        DBGN(" user:");
        __bytesprint(stderr, criteo_entries[i].user_id, 16);
        DBGN(" product:");
        __bytesprint(stderr, criteo_entries[i].product_id, 16);
        DBG("");
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