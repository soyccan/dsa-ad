#ifndef _QUERY_H_
#define _QUERY_H_

#include <stdint.h>

int get(const char* user_id, const char* product_id, int click_time);

int purchased(const char* user_id);

int clicked(const char* product_id1, const char* product_id2);

int profit(int from_time, float min_sales_per_click);

#endif