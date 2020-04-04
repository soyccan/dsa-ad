#include <stdbool.h>
#include <stdio.h>

#ifndef _FSTRING_H_
#define _FSTRING_H_ 1

#define FS_NUM_CHR 17
// 0-15: hexidecimal digit; 16: minus sign

struct FSNode {
    int rank;  // rank > 0 specifies the end of string
    FSNode* parent;
    FSNode* chr[0];  // GNU extension: array of length zero
};

FSNode* fs_init(FSNode** head, FSNode* parent);

FSNode* fs_insert(FSNode* head, const char* str);

void fs_sort(FSNode* head);

void fs_print(FSNode* head, FILE* stream);

#endif