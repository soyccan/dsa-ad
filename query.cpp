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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <algorithm>

#include "common.h"
#include "database.h"


/* for cmp_?_? functions, we assume the string is valid bytes of length 16
 */
static inline bool cmp_u_l(int x, const uint8_t* y)
{
    return memcmp(criteo_entries[x].user_id, y, 16);
}
static inline bool cmp_u_r(const uint8_t* x, int y)
{
    return memcmp(x, criteo_entries[y].user_id, 16);
}
static inline bool cmp_p_l(int x, const uint8_t* y)
{
    return memcmp(criteo_entries[x].product_id, y, 16);
}
static inline bool cmp_p_r(const uint8_t* x, int y)
{
    return memcmp(x, criteo_entries[y].product_id, 16);
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
    uint8_t bu[16], bp[16];
    G(__hexcpy(bu, user_id, 32));
    G(__hexcpy(bp, product_id, 32));

    int *l, *r;

    l = std::lower_bound(sorted_criteo_entries_upt,
                         sorted_criteo_entries_upt + NUM_ENTRY, bu, cmp_u_l);
    r = std::upper_bound(sorted_criteo_entries_upt,
                         sorted_criteo_entries_upt + NUM_ENTRY, bu, cmp_u_r);

    l = std::lower_bound(l, r, bp, cmp_p_l);
    r = std::upper_bound(l, r, bp, cmp_p_r);

    l = std::lower_bound(l, r, click_time, cmp_t_l);
    r = std::upper_bound(l, r, click_time, cmp_t_r);

    if (l == sorted_criteo_entries_upt + NUM_ENTRY ||
        memcmp(criteo_entries[*l].user_id, bu, 16) != 0 ||
        memcmp(criteo_entries[*l].product_id, bp, 16) != 0 ||
        criteo_entries[*l].click_time != click_time) {
        DBG("not found");
        return -1;
    }

    DBGN("get index=%d time=%d", *l, criteo_entries[*l].click_time);
    DBGN("    user=");
    __bytesprint(stderr, criteo_entries[*l].user_id, 16);
    DBGN("    product=");
    __bytesprint(stderr, criteo_entries[*l].product_id, 16);
    DBG("");

    printf("%d\n", criteo_entries[*l].sale);

    return *l;
}

int purchased(const char* user_id)
{
    uint8_t bu[16];
    G(__hexcpy(bu, user_id, 32));

    int* l =
        std::lower_bound(sorted_criteo_entries_upt,
                         sorted_criteo_entries_upt + NUM_ENTRY, bu, cmp_u_l);
    int* r =
        std::upper_bound(sorted_criteo_entries_upt,
                         sorted_criteo_entries_upt + NUM_ENTRY, bu, cmp_u_r);
    assert(l <= r);

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
    }
    return 0;
}

int clicked(const char* product_id1, const char* product_id2)
{
    uint8_t bp1[16], bp2[16];
    G(__hexcpy(bp1, product_id1, 32));
    G(__hexcpy(bp2, product_id2, 32));

    int* l1 =
        std::lower_bound(sorted_criteo_entries_pu,
                         sorted_criteo_entries_pu + NUM_ENTRY, bp1, cmp_p_l);
    int* r1 =
        std::upper_bound(sorted_criteo_entries_pu,
                         sorted_criteo_entries_pu + NUM_ENTRY, bp1, cmp_p_r);

    int* l2 =
        std::lower_bound(sorted_criteo_entries_pu,
                         sorted_criteo_entries_pu + NUM_ENTRY, bp2, cmp_p_l);
    int* r2 =
        std::upper_bound(sorted_criteo_entries_pu,
                         sorted_criteo_entries_pu + NUM_ENTRY, bp2, cmp_p_r);

    while (l1 != r1 && l2 != r2) {
        int res = cmp_u(*l1, *l2);
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
            while (l1 != r1 &&
                   criteo_entries[p].user_id == criteo_entries[*l1].user_id)
                l1++;
            while (l2 != r2 &&
                   criteo_entries[p].user_id == criteo_entries[*l2].user_id)
                l2++;
        }
    }
    return 0;
}

int profit(int from_time, float min_sales_per_click)
{
    for (int i = 0; i < NUM_ENTRY;) {
        int* r = sorted_criteo_entries_ut + i;
        while (r < sorted_criteo_entries_ut + NUM_ENTRY &&
               criteo_entries[sorted_criteo_entries_ut[i]].user_id ==
                   criteo_entries[*r].user_id)
            r++;

        int* l =
            std::lower_bound(sorted_criteo_entries_ut + i, r, from_time,
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
        i = r - sorted_criteo_entries_ut;  // TODO
    }
    return 0;
}