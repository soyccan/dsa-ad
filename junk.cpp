#include <stdio.h>
#include <string.h>

// junk code is put here
// uniq_len = 15336555

static inline void* __create_shm(size_t* size)
{
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

    int shmfd;
    size_t pagesize = sysconf(_SC_PAGESIZE);
    size_t shmlen = (*size & ~(pagesize - 1)) + pagesize;
    *size = shmlen;
    G(shmfd = shm_open(SORTED_CRITEO_ENTRIES_SHM_NAME,
                       O_RDWR | O_CREAT | O_TRUNC, 0600))
    G(ftruncate(shmfd, shmlen));
    return mmap(NULL, shmlen, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    // G(close(shmfd)); // TODO: this should be closed ?
}

static inline int cmp_pu(const void* a, const void* b)
{
    int x = *(int*) a;
    int y = *(int*) b;
    int res;
    res =
        strncmp(criteo_entries[x].product_id, criteo_entries[y].product_id, 32);
    if (res != 0)
        return res;
    res = strncmp(criteo_entries[x].user_id, criteo_entries[y].user_id, 32);
    if (res != 0)
        return res;
    return 0;
}

// static char* criteo_data;
// #define MAX_FD 2
// #define BUFFER_SIZE 0x1000
// struct {
//     char buffer[BUFFER_SIZE];
//     size_t file_offset;
//     size_t page_offset;
//     int last_fd;
// } buffer_cache_struct;

/* do not support seek */
// static inline int read_with_buffer(int fd, void* buf, size_t n)
// {
//     assert(fd < MAX_FD);
//     if (fd >= MAX_FD)
//         return -1;
//     if (fd != buffer_cache_struct.last_fd) {
//         buffer_cache_struct.last_fd = fd;
//         buffer_cache_struct.file_offset = buffer_cache_struct.page_offset =
//         0; if (lseek(fd, 0, SEEK_CUR) < 0)
//             return -1;
//         if (read(fd, buffer_cache_struct.buffer, BUFFER_SIZE) < 0)
//             return -1;
//     }
//     char* b = buf;
//     size_t i;
//     for (i = 0; i < n; i++) {
//         *b++ = buffer_cache_struct.buffer[buffer_cache_struct.page_offset++];
//         if (buffer_cache_struct.page_offset >= BUFFER_SIZE) {
//             buffer_cache_struct.page_offset = 0;
//             buffer_cache_struct.file_offset += BUFFER_SIZE;
//             if (lseek(fd, buffer_cache_struct.file_offset, SEEK_SET) < 0)
//                 return -1;
//             int res = read(fd, buffer_cache_struct.buffer, BUFFER_SIZE);
//             if (res < 0)
//                 return -1;
//             else if (res == 0)
//                 break;
//         }
//     }
//     return i;
// }
// static int load_entry_offsets()
// {
//     // TODO: apply Least Privilege Principle, don't open RW if not necessary
//     int fd = open("index/entry_offsets", O_RDWR);
//     bool scanoff = false;
//     if (fd < 0) {
//         G(fd = open("index/entry_offsets", O_CREAT | O_RDWR, 0644));
//         G(ftruncate(fd, entry_offsets_filesize));
//         scanoff = true;
//     }
//     GG(entry_offsets = mmap(NULL, entry_offsets_filesize,
//                             PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0),
//        MAP_FAILED);
//     close(fd);

//     if (scanoff) {
//         size_t i = 0;
//         char *cl = criteo_data, *nl;
//         entry_offsets[0] = 0;
//         while ((nl = strchr(cl, '\n'))) {
//             entry_offsets[i + 1] = entry_offsets[i] + (nl + 1 - cl);

//             cl = nl + 1;
//             i++;
//         }
//         DBG("entries: %d", i);  // 15995634
//         G(msync(entry_offsets, entry_offsets_filesize, MS_SYNC));
//         return i;
//     }
//     return 0;
// }

get()
{
    // unsigned int up, up_step, lb, lb_step;

    // user_id
    // up = 0, up_step = NUM_ENTRY;  // upper bound
    // lb = 0, lb_step = NUM_ENTRY;  // lower bound
    // while () {
    //     unsigned int m = (l + r) >> 1;
    //     int res =
    //         strncmp(criteo_entries[sorted_entry[0][m]].user_id, user_id, 32);
    //     if (res < 0)
    //         l = m + 1;
    //     else
    //         r = m;
    // }
    // while (L < R) {
    //     unsigned int m = (L + R) >> 1;
    //     int res =
    //         strncmp(criteo_entries[sorted_entry[0][m]].user_id, user_id, 32);
    //     if (res <= 0)
    //         L = m + 1;
    //     else
    //         R = m;
    // }
    // assert(l == r && L == R);

    // product_id
    // L = l, r = R;
    // while (l < r) {
    //     unsigned int m = (l + r) >> 1;
    //     int res = strncmp(criteo_entries[sorted_entry[0][m]].product_id,
    //                       product_id, 32);
    //     if (res < 0)
    //         l = m + 1;
    //     else
    //         r = m;
    // }
    // while (L < R) {
    //     unsigned int m = (L + R) >> 1;
    //     int res = strncmp(criteo_entries[sorted_entry[0][m]].product_id,
    //                       product_id, 32);
    //     if (res <= 0)
    //         L = m + 1;
    //     else
    //         R = m;
    // }
    // assert(l == r && L == R);

    // time
    // L = l, r = R;
    // while (l < r) {
    //     unsigned int m = (l + r) >> 1;
    //     int res = criteo_entries[sorted_entry[0][m]].click_time - click_time;
    //     if (res < 0)
    //         l = m + 1;
    //     else
    //         r = m;
    // }
    // while (L < R) {
    //     unsigned int m = (L + R) >> 1;
    //     int res = criteo_entries[sorted_entry[0][m]].click_time - click_time;
    //     if (res <= 0)
    //         L = m + 1;
    //     else
    //         R = m;
    // }
    // assert(l == r && L == R && l + 1 == L);

    // if (l == NUM_ENTRY ||
    //     strncmp(criteo_entries[sorted_entry[0][l]].user_id, user_id, 32) != 0
    //     || strncmp(criteo_entries[sorted_entry[0][l]].product_id, product_id,
    //             32) != 0 ||
    //     criteo_entries[sorted_entry[0][l]].click_time != click_time)
    //     l = -1;
}

// end junk code

char buf[200000000];

int main()
{
    FILE* f = fopen("Criteo_Conversion_Search/CriteoSearchData", "r");
    size_t maxn = 0;
    while (!feof(f)) {
        fgets(buf, sizeof buf, f);
        size_t n = strlen(buf);
        if (n > maxn)
            maxn = n;
    }
    printf("%lu\n", maxn);
}