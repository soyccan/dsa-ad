
#include "database.h"

#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>

#include "common.h"

Entry* criteo_entries;
int* sorted_criteo_entries_upt;
int* sorted_criteo_entries_pu;
int* sorted_criteo_entries_ut;

static void load_criteo_data()
{
    int shmfd;
    int* ptr;

    G(shmfd = shm_open(CRITEO_ENTRIES_SHM_NAME, O_RDONLY, 0));
    GG(criteo_entries = reinterpret_cast<Entry*>(mmap(
           NULL, NUM_ENTRY * sizeof(Entry), PROT_READ, MAP_SHARED, shmfd, 0)),
       MAP_FAILED);
    G(close(shmfd));

    G(shmfd = shm_open(SORTED_CRITEO_ENTRIES_SHM_NAME, O_RDONLY, 0));
    GG(ptr = reinterpret_cast<int*>(mmap(NULL, NUM_ENTRY * sizeof(int) * 3,
                                         PROT_READ, MAP_SHARED, shmfd, 0)),
       MAP_FAILED);
    G(close(shmfd));
    sorted_criteo_entries_upt = ptr;
    sorted_criteo_entries_pu = ptr + NUM_ENTRY;
    sorted_criteo_entries_ut = ptr + NUM_ENTRY * 2;
}

void init_criteo_data()
{
    if (!criteo_entries) {
        load_criteo_data();
    }
}