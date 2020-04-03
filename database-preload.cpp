#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "common.h"
#include "database.h"

static Entry* preload_criteo_entries;

static void preload_criteo_data()
{
    FILE* fs;
    int shmfd;
    size_t shmlen;
    char* buf = new char[0x400];  // 0x400 is enough to handle longest line

    GG(fs = fopen("Criteo_Conversion_Search/CriteoSearchData", "r"), NULL);

    // shared memory
    // show limits: $ ipcs -lm or cat /proc/sys/kernel/shmmax
    size_t pagesize = sysconf(_SC_PAGESIZE);
    shmlen = ((NUM_ENTRY * sizeof(Entry)) & ~(pagesize - 1)) + pagesize;
    G(shmfd =
          shm_open(CRITEO_ENTRIES_SHM_NAME, O_RDWR | O_CREAT | O_TRUNC, 0600))
    G(ftruncate(shmfd, shmlen));
    // for (size_t i = 0; i < shmlen; i += 0x1000) {
    //     // ftruncate only creates filehole,
    //     // but this will return error when shared memory exceed its limit
    //     DBG("pwrite i=%d", i);
    //     G(pwrite(shmfd, buf, 0x1000, i));
    // }
    GG(preload_criteo_entries = reinterpret_cast<Entry*>(
           mmap(NULL, shmlen, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0)),
       MAP_FAILED);
    G(close(shmfd));

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
    munmap(preload_criteo_entries, shmlen);
}

int main()
{
    preload_criteo_data();
    pause();
    shm_unlink(CRITEO_ENTRIES_SHM_NAME);
    return 0;
}