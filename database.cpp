
#include "database.h"

#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <parallel/algorithm>

#include "common.h"


Entry* criteo_entries;
int* sorted_criteo_entries_upt;
int* sorted_criteo_entries_pu;
int* sorted_criteo_entries_ut;

char (*keys)[32];
int keys_len;


static inline bool cmp_upt(int x, int y)
{
    int res;
    res = criteo_entries[x].user_id - criteo_entries[y].user_id;
    if (res != 0)
        return res < 0;
    res = criteo_entries[x].product_id - criteo_entries[y].product_id;
    if (res != 0)
        return res < 0;
    res = criteo_entries[x].click_time - criteo_entries[y].click_time;
    return res < 0;
}

static inline bool cmp_pu(int x, int y)
{
    int res;
    res = criteo_entries[x].product_id - criteo_entries[y].product_id;
    if (res != 0)
        return res < 0;
    res = criteo_entries[x].user_id - criteo_entries[y].user_id;
    return res < 0;
}

static inline bool cmp_ut(int x, int y)
{
    int res;
    res = criteo_entries[x].user_id - criteo_entries[y].user_id;
    if (res != 0)
        return res < 0;
    res = criteo_entries[x].click_time - criteo_entries[y].click_time;
    return res < 0;
}

static void __sort_keys()
{
    char(*buf)[32];
    int *tmp, *rev;

    GG(buf = reinterpret_cast<char(*)[32]>(malloc(keys_len * sizeof(char[32]))),
       NULL);
    GG(tmp = reinterpret_cast<int*>(malloc(keys_len * sizeof(int))), NULL);
    GG(rev = reinterpret_cast<int*>(malloc(keys_len * sizeof(int))), NULL);

    FOR(i, 0, keys_len) tmp[i] = i;
    __gnu_parallel::sort(tmp, tmp + keys_len, [](int x, int y) {
        return strncmp(keys[x], keys[y], 32) < 0;
    });

    int uniq_len = 1;
    rev[tmp[0]] = 0;
    FOR(i, 1, keys_len)
    {
        if (strncmp(keys[tmp[i - 1]], keys[tmp[i]], 32) != 0) {
            rev[tmp[i]] = uniq_len;
            tmp[uniq_len++] = tmp[i];
        } else {
            rev[tmp[i]] = uniq_len - 1;
        }
    }
    DBG("uniq_len=%d", uniq_len);

    FOR(i, 0, uniq_len) { strncpy(buf[i], keys[tmp[i]], 32); }
    memcpy(keys, buf, uniq_len * sizeof(char[32]));

    FOR(i, 0, NUM_ENTRY)
    {
        criteo_entries[i].product_id = rev[criteo_entries[i].product_id];
        criteo_entries[i].user_id = rev[criteo_entries[i].user_id];
    }

    keys_len = uniq_len;
    GG(keys = reinterpret_cast<char(*)[32]>(
           realloc(keys, keys_len * sizeof(char[32]))),
       NULL);

    free(buf);
    free(tmp);
    free(rev);

    // FOR(i, 0, NUM_ENTRY)
    // {
    //     DBG("i:%d user:%.32s product:%.32s time:%u", i,
    //         keys[criteo_entries[i].user_id],
    //         keys[criteo_entries[i].product_id],
    //         criteo_entries[i].click_time);
    // }
}

static void __sort_criteo_data()
{
    // TODO:  qsort vs. std::sort, which faster?

    __sort_keys();

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
    __gnu_parallel::sort(sorted_criteo_entries_upt,
                         sorted_criteo_entries_upt + NUM_ENTRY, cmp_upt);
    // FOR(j, 0, NUM_ENTRY) DBG("upt[%d]=%d", j, sorted_criteo_entries_upt[j]);

    // 1: sorted by (product_id, user_id)
    FOR(j, 0, NUM_ENTRY) { sorted_criteo_entries_pu[j] = j; }
    __gnu_parallel::sort(sorted_criteo_entries_pu,
                         sorted_criteo_entries_pu + NUM_ENTRY, cmp_pu);
    // FOR(j, 0, NUM_ENTRY) DBG("pu[%d]=%d", j, sorted_criteo_entries_pu[j]);

    // 2: sorted by (user_id, click_time)
    FOR(j, 0, NUM_ENTRY)
    {
        sorted_criteo_entries_ut[j] = sorted_criteo_entries_upt[j];
    }
    __gnu_parallel::sort(sorted_criteo_entries_ut,
                         sorted_criteo_entries_ut + NUM_ENTRY, cmp_ut);
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
    GG(keys = reinterpret_cast<char(*)[32]>(
           malloc(NUM_ENTRY * 2 * sizeof(char[32]))),
       NULL);

    keys_len = 0;
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
                strncpy(criteo_entries[i].product_price, ps, 32);
            else if (j == 6)
                strncpy(criteo_entries[i].product_age_group, ps, 32);
            else if (j == 9)
                strncpy(criteo_entries[i].product_gender, ps, 32);
            else if (j == 19) {
                strncpy(keys[keys_len], ps, 32);
                criteo_entries[i].product_id = static_cast<int>(keys_len);
                keys_len++;
            } else if (j == 22) {
                strncpy(keys[keys_len], ps, 32);
                criteo_entries[i].user_id = static_cast<int>(keys_len);
                keys_len++;
            }
        }
        // DBG("i:%d user:%.32s product:%.32s time:%u", i,
        //     criteo_entries[i].user_id, criteo_entries[i].product_id,
        //     criteo_entries[i].click_time);
    }
    DBG("loaded entries:%d keys:%d", i, keys_len);  // 15995634
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