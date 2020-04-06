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
#include "hex.h"


/* for cmp_?_? functions, we assume the string is valid bytes of length 16
 */
template <typename T>
struct CompU {
    inline bool operator()(const T& x, const uint8_t* y) const
    {
        return memcmp(x.user_id, y, sizeof x.user_id) < 0;
    }
    inline bool operator()(const uint8_t* x, const T& y) const
    {
        return memcmp(x, y.user_id, sizeof y.user_id) < 0;
    }
};
template <typename T>
struct CompP {
    inline bool operator()(const T& x, const uint8_t* y) const
    {
        return memcmp(x.product_id, y, sizeof x.product_id) < 0;
    }
    inline bool operator()(const uint8_t* x, const T& y) const
    {
        return memcmp(x, y.product_id, sizeof y.product_id) < 0;
    }
};
template <typename T>
struct CompT {
    inline bool operator()(const T& x, int y) const { return x.click_time < y; }
    inline bool operator()(int x, const T& y) const { return x < y.click_time; }
};

/* acquire the first entry found */
int get(const char* user_id, const char* product_id, int click_time)
{
    uint8_t bu[16], bp[16];
    hexcpy(bu, user_id, 32);
    hexcpy(bp, product_id, 32);

    EntryKeyUPT *l, *r;

    l = std::lower_bound(sorted_criteo_entries_upt,
                         sorted_criteo_entries_upt + NUM_ENTRY, bu,
                         CompU<EntryKeyUPT>());
    r = std::upper_bound(sorted_criteo_entries_upt,
                         sorted_criteo_entries_upt + NUM_ENTRY, bu,
                         CompU<EntryKeyUPT>());
    if (l == sorted_criteo_entries_upt + NUM_ENTRY ||
        memcmp(l->user_id, bu, 16) != 0)
        return -1;

    l = std::lower_bound(l, r, bp, CompP<EntryKeyUPT>());
    r = std::upper_bound(l, r, bp, CompP<EntryKeyUPT>());
    if (l == sorted_criteo_entries_upt + NUM_ENTRY ||
        memcmp(l->product_id, bp, 16) != 0)
        return -1;

    l = std::lower_bound(l, r, click_time, CompT<EntryKeyUPT>());
    if (l == sorted_criteo_entries_upt + NUM_ENTRY ||
        l->click_time != click_time)
        return -1;

    DBGN("get index=%d user=", *l);
    DBGHEX(l->user_id, 16);
    DBGN(" product=");
    DBGHEX(l->product_id, 16);
    DBG(" time=%d", l->click_time);

    printf("%d\n", l->value->sale);

    return l->value - criteo_entries;
}

int purchased(const char* user_id)
{
    uint8_t bu[16];
    hexcpy(bu, user_id, 32);

    EntryKeyUPT* l = std::lower_bound(sorted_criteo_entries_upt,
                                      sorted_criteo_entries_upt + NUM_ENTRY, bu,
                                      CompU<EntryKeyUPT>());
    EntryKeyUPT* r = std::upper_bound(sorted_criteo_entries_upt,
                                      sorted_criteo_entries_upt + NUM_ENTRY, bu,
                                      CompU<EntryKeyUPT>());
    assert(l <= r);

    int cnt = 0;
    while (l != r) {
        DBGN("purchased sale=%hhd index=%d user=", l->value->sale, *l);
        DBGHEX(l->user_id, 16);
        DBGN(" product=");
        DBGHEX(l->product_id, 16);
        DBG(" time=%d price=%.8s age=%.32s gender=%.32s", l->click_time,
            l->value->product_price, l->value->product_age_group,
            l->value->product_gender);

        if (l->value->sale) {
            hexprint(l->product_id, 16, stdout);
            printf(" %d %.8s %.32s %.32s\n", l->click_time,
                   l->value->product_price, l->value->product_age_group,
                   l->value->product_gender);
        }
        l++;
        cnt++;
    }
    return cnt;
}

int clicked(const char* product_id1, const char* product_id2)
{
    uint8_t bp1[16], bp2[16];
    hexcpy(bp1, product_id1, 32);
    hexcpy(bp2, product_id2, 32);

    EntryKeyPU* l1 = std::lower_bound(sorted_criteo_entries_pu,
                                      sorted_criteo_entries_pu + NUM_ENTRY, bp1,
                                      CompP<EntryKeyPU>());
    EntryKeyPU* r1 = std::upper_bound(sorted_criteo_entries_pu,
                                      sorted_criteo_entries_pu + NUM_ENTRY, bp1,
                                      CompP<EntryKeyPU>());

    EntryKeyPU* l2 = std::lower_bound(sorted_criteo_entries_pu,
                                      sorted_criteo_entries_pu + NUM_ENTRY, bp2,
                                      CompP<EntryKeyPU>());
    EntryKeyPU* r2 = std::upper_bound(sorted_criteo_entries_pu,
                                      sorted_criteo_entries_pu + NUM_ENTRY, bp2,
                                      CompP<EntryKeyPU>());

    int cnt = 0;
    while (l1 != r1 && l2 != r2) {
        int res = memcmp(l1->user_id, l2->user_id, 16);
        if (res < 0) {
            l1++;
        } else if (res > 0) {
            l2++;
        } else {
            cnt++;
            hexprint(l1->user_id, 16, stdout);
            puts("");

            EntryKeyPU* p = l1;
            while (l1 != r1 && memcmp(p->user_id, l1->user_id, 16) == 0)
                l1++;
            while (l2 != r2 && memcmp(p->user_id, l2->user_id, 16) == 0)
                l2++;
        }
    }
    return cnt;
}

int profit(int from_time, float min_sales_per_click)
{
    int cnt = 0;
    for (EntryKeyUT* i = sorted_criteo_entries_ut;
         i < sorted_criteo_entries_ut + NUM_ENTRY;) {
        EntryKeyUT* r = i;
        while (r < sorted_criteo_entries_ut + NUM_ENTRY &&
               memcmp(i->user_id, r->user_id, 16) == 0)
            // TODO: optimize?
            r++;

        EntryKeyUT* l = std::lower_bound(i, r, from_time, CompT<EntryKeyUT>());
        // TODO: "AFTER" time t meas ">= t" ?

        int clicks = static_cast<int>(r - l);  // TODO: pointer difference
        int sales = 0;
        while (l < r) {
            if (l->value->sale)
                sales++;
            l++;
        }
        if ((clicks > 0 && (float) sales / clicks >= min_sales_per_click) ||
            (clicks == 0 && min_sales_per_click == 0)) {
            cnt++;
            DBGN("profit so much sales=%d clicks=%d from_time=%d user=", sales,
                 clicks, from_time);
            DBGHEX((l - 1)->user_id, 16);
            DBG("");

            hexprint((l - 1)->user_id, 16, stdout);
            puts("");
            if (cnt >= 10)
                break;
        }
        i = r;
    }
    return cnt;
}