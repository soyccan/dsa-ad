/* wc: 15995634 397938568 6426808162 CriteoSearchData
 * total entries: 15995634
 * longest line: 873 bytes
 * get(u,p,t): sorted by (u,p,t)
 * purchased(u): sorted by (u,p,t)
 * clicked(p1,p2): sorted by (p,u)
 * profit(t,θ = sales_amount/clicks): sorted by (t,θ,u)
 *
 * columns:
 * <Sale>,<SalesAmountInEuro>,<time_delay_for_conversion>,<click_timestamp>,<nb_clicks_1week>,<product_price>
 * ,<product_age_group> ,<device_type>,<audience_id> ,<product_gender>
 * ,<product_brand> ,<product_category(1-7)> ,<product_country>, <product_id>
 * ,<product_title> ,<partner_id> ,<user_id>
 *
 * some performance test:
 *    qsort vs. std::sort
 *    char[] vs. std::string
 *    FILE* vs. fd vs. mmap
 *    pre-sorted user_id/product_id...
 *    pre-calculated index of next user in sorted (u,p,t) and (u,t)
 */

#include "query.h"

#include <assert.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>

#include "common.h"


#define NUM_ENTRY 15995634

// CRITICAL: entry order is important, since scanf will append null-byte
//           if the order in structure is diffent from the order in data
//           file, may produce wrong result
struct entry {
    // int id;
    // size_t offset;  // offset in file
    bool sale;
    int click_time;  // TODO: 64-bit?
    float product_price;
    char product_age_group[32];
    char product_gender[32];
    char product_id[32];
    char user_id[32];
};
static struct entry* criteo_entries;
static int sorted_entry[3][NUM_ENTRY];


static int load_criteo_data()
{
    // int fd;
    // struct stat st;
    FILE* fs;

    GG(fs = fopen("Criteo_Conversion_Search/CriteoSearchData", "r"), NULL);
    // G(fd = open("Criteo_Conversion_Search/CriteoSearchData", O_RDONLY));
    // G(fstat(fd, &st));
    // GG(criteo_data = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE, fd, 0),
    //    MAP_FAILED);
    // G(close(fd));
    // GG(fdopen(fs, "r"), NULL);

    GG(criteo_entries = reinterpret_cast<struct entry*>(
           mmap(NULL, (NUM_ENTRY + 1) * sizeof(struct entry),
                PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0)),
       MAP_FAILED);

    static char buf[0x400];
    // criteo_entries[0].offset = 0;
    int i = 0;
    for (i = 0; i < NUM_ENTRY; i++) {
        if (fgets(buf, sizeof buf, fs) == NULL)
            break;
        int l = 0;
        // TODO: CRITICAL note the appended null-byte (by scanf) is overwritten
        sscanf(
            buf,
            "%hhd\t%*[^\t]\t%*[^\t]\t%d\t%*[^\t]\t%f\t%[^\t]\t%*[^\t]\t%*[^\t]"
            "\t%[^\t]\t%*[^\t]\t%*[^\t]\t%*[^\t]\t%*[^\t]\t%*[^\t]\t%*[^\t]\t%*"
            "[^\t]\t%*[^\t]\t%*[^\t]\t%[^\t]\t%*[^\t]\t%*[^\t]\t%[^\t]%n",
            &criteo_entries[i].sale, &criteo_entries[i].click_time,
            &criteo_entries[i].product_price,
            criteo_entries[i].product_age_group,
            criteo_entries[i].product_gender, criteo_entries[i].product_id,
            criteo_entries[i].user_id, &l);

        // criteo_entries[i].id = i;
        // DBG("i:%d user:%.32s product:%.32s time:%u", i,
        //     criteo_entries[i].user_id, criteo_entries[i].product_id,
        //     criteo_entries[i].click_time);

        // if (i < NUM_ENTRY - 1)
        //     criteo_entries[i + 1].offset = criteo_entries[i].offset + l;
    }
    DBG("entries: %d", i);  // 15995634
    fclose(fs);
    return 0;
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

static void sort_criteo_data()
{
    // TODO: since qsort is slow, consider jump to C++?

    FOR(i, 0, 3) FOR(j, 0, NUM_ENTRY) sorted_entry[i][j] = j;

    // 0: sorted by (u,p,t)
    // qsort(sorted_entry[0], NUM_ENTRY, sizeof(sorted_entry[0][0]), cmp_upt);
    std::sort(sorted_entry[0], sorted_entry[0] + NUM_ENTRY, cmp_upt);

    // FOR(i, 0, NUM_ENTRY)
    // {
    //     struct entry* x = &criteo_entries[sorted_entry[0][i]];
    //     DBG("i:%d user:%.32s product:%.32s time:%u", x->id, x->user_id,
    //         x->product_id, x->click_time);
    // }

    // 1: sorted by (p,u)
    // qsort(sorted_entry[1], NUM_ENTRY, sizeof(sorted_entry[1][0]), cmp_pu);
    std::sort(sorted_entry[1], sorted_entry[1] + NUM_ENTRY, cmp_pu);

    // 2: sorted by (u,t)
    std::sort(sorted_entry[2], sorted_entry[2] + NUM_ENTRY, cmp_ut);
}

static void init_criteo_data()
{
    if (!criteo_entries) {
        load_criteo_data();
        sort_criteo_data();
    }
}

static inline bool cmp_u_l(int x, const char* y)
{
    return strncmp(criteo_entries[x].user_id, y, 32) < 0;
}
static inline bool cmp_u_r(const char* x, int y)
{
    return strncmp(x, criteo_entries[y].user_id, 32) < 0;
}
static inline bool cmp_p_l(int x, const char* y)
{
    return strncmp(criteo_entries[x].product_id, y, 32) < 0;
}
static inline bool cmp_p_r(const char* x, int y)
{
    return strncmp(x, criteo_entries[y].product_id, 32) < 0;
}
static inline bool cmp_t_l(int x, int y)
{
    return criteo_entries[x].click_time < y;
}
static inline bool cmp_t_r(int x, int y)
{
    return x < criteo_entries[y].click_time;
}

int get(const char* user_id, const char* product_id, int click_time)
{
    init_criteo_data();

    int *l, *r;

    l = std::lower_bound(sorted_entry[0], sorted_entry[0] + NUM_ENTRY, user_id,
                         cmp_u_l);
    r = std::upper_bound(sorted_entry[0], sorted_entry[0] + NUM_ENTRY, user_id,
                         cmp_u_r);

    l = std::lower_bound(l, r, product_id, cmp_p_l);
    r = std::upper_bound(l, r, product_id, cmp_p_r);

    l = std::lower_bound(l, r, click_time, cmp_t_l);
    r = std::upper_bound(l, r, click_time, cmp_t_r);

    if (l == sorted_entry[0] + NUM_ENTRY ||
        strncmp(criteo_entries[*l].user_id, user_id, 32) != 0 ||
        strncmp(criteo_entries[*l].product_id, product_id, 32) != 0 ||
        criteo_entries[*l].click_time != click_time)
        return -1;

    DBG("get index=%d user_id=%.32s product=%.32s time=%d", *l,
        criteo_entries[*l].user_id, criteo_entries[*l].product_id,
        criteo_entries[*l].click_time);

    printf("%d\n", criteo_entries[*l].sale);

    return *l;
}

int purchased(const char* user_id)
{
    init_criteo_data();

    int* l = std::lower_bound(sorted_entry[0], sorted_entry[0] + NUM_ENTRY,
                              user_id, cmp_u_l);
    int* r = std::upper_bound(sorted_entry[0], sorted_entry[0] + NUM_ENTRY,
                              user_id, cmp_u_r);

    while (l != r) {
        DBG("purchased sale=%hhd index=%d user=%.32s product=%.32s time=%d "
            "price=%f age=%.32s gender=%.32s",
            criteo_entries[*l].sale, *l, criteo_entries[*l].user_id,
            criteo_entries[*l].product_id, criteo_entries[*l].click_time,
            criteo_entries[*l].product_price,
            criteo_entries[*l].product_age_group,
            criteo_entries[*l].product_gender);

        if (criteo_entries[*l].sale) {
            printf("%.32s %d %f %.32s %.32s\n", criteo_entries[*l].product_id,
                   criteo_entries[*l].click_time,
                   criteo_entries[*l].product_price,
                   criteo_entries[*l].product_age_group,
                   criteo_entries[*l].product_gender);
        }
        l++;
    }
    return 0;
}

int clicked(const char* product_id1, const char* product_id2)
{
    init_criteo_data();

    int* l1 = std::lower_bound(sorted_entry[1], sorted_entry[1] + NUM_ENTRY,
                               product_id1, cmp_p_l);
    int* r1 = std::upper_bound(sorted_entry[1], sorted_entry[1] + NUM_ENTRY,
                               product_id1, cmp_p_r);

    int* l2 = std::lower_bound(sorted_entry[1], sorted_entry[1] + NUM_ENTRY,
                               product_id2, cmp_p_l);
    int* r2 = std::upper_bound(sorted_entry[1], sorted_entry[1] + NUM_ENTRY,
                               product_id2, cmp_p_r);

    while (l1 != r1 && l2 != r2) {
        int res = strncmp(criteo_entries[*l1].user_id,
                          criteo_entries[*l2].user_id, 32);
        if (res < 0) {
            l1++;
        } else if (res > 0) {
            l2++;
        } else {
            DBG("clicked id=%d,%d product1=%.32s prod2=%.32s user=%.32s", *l1,
                *l2, criteo_entries[*l1].product_id,
                criteo_entries[*l2].product_id, criteo_entries[*l1].user_id);
            printf("%.32s\n", criteo_entries[*l1].user_id);

            int p = *l1;
            while (l1 != r1 && strncmp(criteo_entries[p].user_id,
                                       criteo_entries[*l1].user_id, 32) == 0)
                l1++;
            while (l2 != r2 && strncmp(criteo_entries[p].user_id,
                                       criteo_entries[*l2].user_id, 32) == 0)
                l2++;
        }
    }
    return 0;
}

int profit(int from_time, float min_sales_per_click)
{
    init_criteo_data();

    for (int i = 0; i < NUM_ENTRY; i++) {
        int* r = sorted_entry[2] + i;
        while (r < sorted_entry[2] + NUM_ENTRY &&
               strncmp(criteo_entries[sorted_entry[2][i]].user_id,
                       criteo_entries[*r].user_id, 32) == 0)
            r++;

        int* l =
            std::lower_bound(sorted_entry[2] + i, r, from_time,
                             cmp_t_l);  // TODO: "AFTER" time t meas ">= t" ?

        int clicks = r - l;  // TODO: pointer difference
        int sales = 0;
        while (l < r) {
            if (criteo_entries[*l].sale)
                sales++;
            l++;
        }
        if ((clicks > 0 && (float) sales / clicks >= min_sales_per_click) ||
            (clicks == 0 && min_sales_per_click == 0)) {
            DBG("profit so much sales=%d clicks=%d from_time=%d user=%.32s",
                sales, clicks, from_time, criteo_entries[*(l - 1)].user_id);

            printf("%.32s\n", criteo_entries[*(l - 1)].user_id);
        }
    }
    return 0;
}