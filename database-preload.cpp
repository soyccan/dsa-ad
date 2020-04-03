#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>

#include "common.h"
#include "database.h"

static Entry* preload_criteo_entries;
static int* preload_sorted_criteo_entries;


static inline bool cmp_upt(int x, int y)
{
    int res;
    res = strncmp(preload_criteo_entries[x].user_id,
                  preload_criteo_entries[y].user_id, 32);
    if (res != 0)
        return res < 0;
    res = strncmp(preload_criteo_entries[x].product_id,
                  preload_criteo_entries[y].product_id, 32);
    if (res != 0)
        return res < 0;
    res = preload_criteo_entries[x].click_time -
          preload_criteo_entries[y].click_time;
    return res < 0;
}

static inline bool cmp_pu(int x, int y)
{
    int res;
    res = strncmp(preload_criteo_entries[x].product_id,
                  preload_criteo_entries[y].product_id, 32);
    if (res != 0)
        return res < 0;
    res = strncmp(preload_criteo_entries[x].user_id,
                  preload_criteo_entries[y].user_id, 32);
    return res < 0;
}

static inline bool cmp_ut(int x, int y)
{
    int res;
    res = strncmp(preload_criteo_entries[x].user_id,
                  preload_criteo_entries[y].user_id, 32);
    if (res != 0)
        return res < 0;
    res = preload_criteo_entries[x].click_time -
          preload_criteo_entries[y].click_time;
    return res < 0;
}

static void sort_criteo_data()
{
    // TODO:  qsort vs. std::sort, which faster?

    int shmfd;
    size_t pagesize = sysconf(_SC_PAGESIZE);
    size_t sce_len =
        ((NUM_ENTRY * sizeof(int) * 3) & ~(pagesize - 1)) + pagesize;
    G(shmfd = shm_open(SORTED_CRITEO_ENTRIES_SHM_NAME,
                       O_RDWR | O_CREAT | O_TRUNC, 0600))
    G(ftruncate(shmfd, sce_len));
    GG(preload_sorted_criteo_entries = reinterpret_cast<int*>(
           mmap(NULL, sce_len, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0)),
       MAP_FAILED);
    G(close(shmfd));

    int* preload_sorted_criteo_entries_upt = preload_sorted_criteo_entries;
    int* preload_sorted_criteo_entries_pu =
        preload_sorted_criteo_entries + NUM_ENTRY;
    int* preload_sorted_criteo_entries_ut =
        preload_sorted_criteo_entries + NUM_ENTRY * 2;

    FOR(j, 0, NUM_ENTRY) preload_sorted_criteo_entries_upt[j] = j;
    FOR(j, 0, NUM_ENTRY) preload_sorted_criteo_entries_pu[j] = j;
    FOR(j, 0, NUM_ENTRY) preload_sorted_criteo_entries_ut[j] = j;

    // 0: sorted by (u,p,t)
    // qsort(sorted_entry[0], NUM_ENTRY, sizeof(sorted_entry[0][0]),
    // cmp_upt);
    std::sort(preload_sorted_criteo_entries_upt,
              preload_sorted_criteo_entries_upt + NUM_ENTRY, cmp_upt);

    // FOR(i, 0, NUM_ENTRY)
    // {
    //     struct entry* x = &criteo_entries[sorted_entry[0][i]];
    //     DBG("i:%d user:%.32s product:%.32s time:%u", x->id, x->user_id,
    //         x->product_id, x->click_time);
    // }

    // 1: sorted by (p,u)
    // qsort(sorted_entry[1], NUM_ENTRY, sizeof(sorted_entry[1][0]),
    // cmp_pu);
    std::sort(preload_sorted_criteo_entries_pu,
              preload_sorted_criteo_entries_pu + NUM_ENTRY, cmp_pu);

    // 2: sorted by (u,t)
    std::sort(preload_sorted_criteo_entries_ut,
              preload_sorted_criteo_entries_ut + NUM_ENTRY, cmp_ut);

    munmap(preload_sorted_criteo_entries, sce_len);
}

static void preload_criteo_data()
{
    FILE* fs;
    size_t ce_len;
    int shmfd;
    char* buf = new char[0x400];  // 0x400 is enough to handle longest line

    // shared memory
    // Note: encountered problem: shared memory limit exceeded
    // show limits: $ ipcs -lm
    //           or $ cat /proc/sys/kernel/shmmax
    // ftruncate only creates filehole, and only raises bus error on accessing
    // out-of-bound memory
    // use this for friendlier error handling:
    // for (size_t i = 0; i < shmlen; i += 0x1000) {
    //     G(pwrite(shmfd, buf, 0x1000, i));
    // }
    size_t pagesize = sysconf(_SC_PAGESIZE);
    ce_len = ((NUM_ENTRY * sizeof(Entry)) & ~(pagesize - 1)) + pagesize;
    G(shmfd =
          shm_open(CRITEO_ENTRIES_SHM_NAME, O_RDWR | O_CREAT | O_TRUNC, 0600))
    G(ftruncate(shmfd, ce_len));
    GG(preload_criteo_entries = reinterpret_cast<Entry*>(
           mmap(NULL, ce_len, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0)),
       MAP_FAILED);
    G(close(shmfd));


    GG(fs = fopen("Criteo_Conversion_Search/CriteoSearchData", "r"), NULL);

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
                preload_criteo_entries[i].sale = atoi(ps);
            else if (j == 3)
                preload_criteo_entries[i].click_time = atoi(ps);
            else if (j == 5)
                strncpy(preload_criteo_entries[i].product_price, ps, 32);
            else if (j == 6)
                strncpy(preload_criteo_entries[i].product_age_group, ps, 32);
            else if (j == 9)
                strncpy(preload_criteo_entries[i].product_gender, ps, 32);
            else if (j == 19)
                strncpy(preload_criteo_entries[i].product_id, ps, 32);
            else if (j == 22)
                strncpy(preload_criteo_entries[i].user_id, ps, 32);
        }

        // DBG("i:%d user:%.32s product:%.32s time:%u", i,
        //     preload_criteo_entries[i].user_id,
        //     preload_criteo_entries[i].product_id,
        //     preload_criteo_entries[i].click_time);
    }
    DBG("loaded entries: %d", i);  // 15995634
    fclose(fs);
    delete[] buf;

    sort_criteo_data();

    munmap(preload_criteo_entries, ce_len);
}
int main()
{
    preload_criteo_data();
    pause();
    shm_unlink(CRITEO_ENTRIES_SHM_NAME);
    shm_unlink(SORTED_CRITEO_ENTRIES_SHM_NAME);
    return 0;
}