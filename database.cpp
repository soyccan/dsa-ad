
#include "database.h"

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>

#include "common.h"

Entry* criteo_entries;
int sorted_criteo_entries_upt[NUM_ENTRY];
int sorted_criteo_entries_pu[NUM_ENTRY];
int sorted_criteo_entries_ut[NUM_ENTRY];

void load_criteo_data()
{
    int shmfd;
    G(shmfd = shm_open(CRITEO_ENTRIES_SHM_NAME, O_RDONLY, 0));
    GG(criteo_entries = reinterpret_cast<Entry*>(mmap(
           NULL, NUM_ENTRY * sizeof(Entry), PROT_READ, MAP_SHARED, shmfd, 0)),
       MAP_FAILED);
    G(close(shmfd));
}

static inline bool cmp_upt(int x, int y)
{
    int res;
    res = strncmp(criteo_entries[x].user_id, criteo_entries[y].user_id, 32);
    if (res != 0)
        return res < 0;
    res =
        strncmp(criteo_entries[x].product_id, criteo_entries[y].product_id, 32);
    if (res != 0)
        return res < 0;
    res = criteo_entries[x].click_time - criteo_entries[y].click_time;
    return res < 0;
}

static inline bool cmp_pu(int x, int y)
{
    int res;
    res =
        strncmp(criteo_entries[x].product_id, criteo_entries[y].product_id, 32);
    if (res != 0)
        return res < 0;
    res = strncmp(criteo_entries[x].user_id, criteo_entries[y].user_id, 32);
    return res < 0;
}

static inline bool cmp_ut(int x, int y)
{
    int res;
    res = strncmp(criteo_entries[x].user_id, criteo_entries[y].user_id, 32);
    if (res != 0)
        return res < 0;
    res = criteo_entries[x].click_time - criteo_entries[y].click_time;
    return res < 0;
}

void sort_criteo_data()
{
    // TODO: since qsort is slow, consider jump to C++?

    FOR(j, 0, NUM_ENTRY) sorted_criteo_entries_upt[j] = j;
    FOR(j, 0, NUM_ENTRY) sorted_criteo_entries_pu[j] = j;
    FOR(j, 0, NUM_ENTRY) sorted_criteo_entries_ut[j] = j;

    // 0: sorted by (u,p,t)
    // qsort(sorted_entry[0], NUM_ENTRY, sizeof(sorted_entry[0][0]),
    // cmp_upt);
    std::sort(sorted_criteo_entries_upt, sorted_criteo_entries_upt + NUM_ENTRY,
              cmp_upt);

    // FOR(i, 0, NUM_ENTRY)
    // {
    //     struct entry* x = &criteo_entries[sorted_entry[0][i]];
    //     DBG("i:%d user:%.32s product:%.32s time:%u", x->id, x->user_id,
    //         x->product_id, x->click_time);
    // }

    // 1: sorted by (p,u)
    // qsort(sorted_entry[1], NUM_ENTRY, sizeof(sorted_entry[1][0]),
    // cmp_pu);
    std::sort(sorted_criteo_entries_pu, sorted_criteo_entries_pu + NUM_ENTRY,
              cmp_pu);

    // 2: sorted by (u,t)
    std::sort(sorted_criteo_entries_ut, sorted_criteo_entries_ut + NUM_ENTRY,
              cmp_ut);
}

void init_criteo_data()
{
    if (!criteo_entries) {
        load_criteo_data();
        sort_criteo_data();
    }
}