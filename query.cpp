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
#include "database.h"

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
    int *l, *r;

    l = std::lower_bound(sorted_criteo_entries_upt,
                         sorted_criteo_entries_upt + NUM_ENTRY, user_id,
                         cmp_u_l);
    r = std::upper_bound(sorted_criteo_entries_upt,
                         sorted_criteo_entries_upt + NUM_ENTRY, user_id,
                         cmp_u_r);
    if (l == sorted_criteo_entries_upt + NUM_ENTRY ||
        strncmp(criteo_entries[*l].user_id, user_id, 32) != 0)
        return -1;

    l = std::lower_bound(l, r, product_id, cmp_p_l);
    r = std::upper_bound(l, r, product_id, cmp_p_r);
    if (l == sorted_criteo_entries_upt + NUM_ENTRY ||
        strncmp(criteo_entries[*l].product_id, product_id, 32) != 0)
        return -1;

    l = std::lower_bound(l, r, click_time, cmp_t_l);
    r = std::upper_bound(l, r, click_time, cmp_t_r);
    if (l == sorted_criteo_entries_upt + NUM_ENTRY ||
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
    int* l = std::lower_bound(sorted_criteo_entries_upt,
                              sorted_criteo_entries_upt + NUM_ENTRY, user_id,
                              cmp_u_l);
    int* r = std::upper_bound(sorted_criteo_entries_upt,
                              sorted_criteo_entries_upt + NUM_ENTRY, user_id,
                              cmp_u_r);
    assert(l <= r);

    int cnt = 0;
    while (l != r) {
        DBG("purchased sale=%hhd index=%d user=%.32s product=%.32s time=%d "
            "price=%.8s age=%.32s gender=%.32s",
            criteo_entries[*l].sale, *l, criteo_entries[*l].user_id,
            criteo_entries[*l].product_id, criteo_entries[*l].click_time,
            criteo_entries[*l].product_price,
            criteo_entries[*l].product_age_group,
            criteo_entries[*l].product_gender);

        if (criteo_entries[*l].sale) {
            printf("%.32s %d %.8s %.32s %.32s\n", criteo_entries[*l].product_id,
                   criteo_entries[*l].click_time,
                   criteo_entries[*l].product_price,
                   criteo_entries[*l].product_age_group,
                   criteo_entries[*l].product_gender);
        }
        l++;
        cnt++;
    }
    return cnt;
}

int clicked(const char* product_id1, const char* product_id2)
{
    int* l1 = std::lower_bound(sorted_criteo_entries_pu,
                               sorted_criteo_entries_pu + NUM_ENTRY,
                               product_id1, cmp_p_l);
    int* r1 = std::upper_bound(sorted_criteo_entries_pu,
                               sorted_criteo_entries_pu + NUM_ENTRY,
                               product_id1, cmp_p_r);

    int* l2 = std::lower_bound(sorted_criteo_entries_pu,
                               sorted_criteo_entries_pu + NUM_ENTRY,
                               product_id2, cmp_p_l);
    int* r2 = std::upper_bound(sorted_criteo_entries_pu,
                               sorted_criteo_entries_pu + NUM_ENTRY,
                               product_id2, cmp_p_r);

    int cnt = 0;
    while (l1 != r1 && l2 != r2) {
        int res = strncmp(criteo_entries[*l1].user_id,
                          criteo_entries[*l2].user_id, 32);
        if (res < 0) {
            l1++;
        } else if (res > 0) {
            l2++;
        } else {
            cnt++;
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
    return cnt;
}

int profit(int from_time, float min_sales_per_click)
{
    int cnt = 0;
    for (int* i = sorted_criteo_entries_ut;
         i < sorted_criteo_entries_ut + NUM_ENTRY;) {
        int* r = i;
        while (r < sorted_criteo_entries_ut + NUM_ENTRY &&
               strncmp(criteo_entries[*i].user_id, criteo_entries[*r].user_id,
                       32) == 0)
            r++;  // TODO: optimize?

        int* l =
            std::lower_bound(i, r, from_time,
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
            cnt++;
            DBG("profit so much sales=%d clicks=%d from_time=%d user=%.32s",
                sales, clicks, from_time, criteo_entries[*(l - 1)].user_id);

            printf("%.32s\n", criteo_entries[*(l - 1)].user_id);
            if (cnt >= 10)
                break;
        }
        i = r;
    }
    return cnt;
}