/* wc: 15995634 397938568 6426808162 CriteoSearchData
 * total entries: 397938568 = 0x17b80f88 < 4e8
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
 */

#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

#define FOR(i, a, n) for (typeof(a) i = a; i < n; i++)

#ifndef NDEBUG
#define DBG(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#define DBGN(format, ...) fprintf(stderr, format, ##__VA_ARGS__)
#else
#define DBG(...)
#define DBGN(...)
#endif


struct entry_off {
    unsigned int id;
    off_t offset;
} * entry_offsets;

static unsigned char buf[0x400];

size_t load_entry_offsets()
{
    int fd = open("entry_offsets", O_RDONLY);
    bool scanoff = false;
    if (fd < 0) {
        fd = open("entry_offsets", O_CREAT | O_WRONLY);
        scanoff = true;
    }
    if (fd < 0) {
        perror("");
        exit(-1);
    }
    mmap(entry_offsets, 397938568 * sizeof(struct entry_off),
         PROT_READ | PROT_WRITE, MAP_FILE, fd, 0);

    if (scanoff) {
        FILE* f = fopen("Criteo_Conversion_Search/CriteoSearchData", "r");
        size_t i = 0;
        while (!feof(f)) {
            // TODO: need fseek(f, 0, SEEK_CUR) here?
            entry_offsets[i++].offset = ftello(f);
            fgets(buf, sizeof buf, f);
        }
        entry_offsets[i++].offset = ftello(f);
        DBG("entries: %d", i);
        return i;
    }
}


int main()
{
    return 0;
}