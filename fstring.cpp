/* faster string, using trie
 */
#include "fstring.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

static int fs_global_rank;


/* parent should be NULL for root node */
FSNode* fs_init(FSNode** head, FSNode* parent)
{
    if (*head == NULL) {
        *head = reinterpret_cast<FSNode*>(malloc(sizeof(FSNode)));
    }
    if (*head == NULL) {
        perror("fs_init failed");
        return NULL;
    }
    memset((*head)->chr, 0, sizeof((*head)->chr));
    (*head)->parent = parent;
    (*head)->end = false;
    (*head)->rank = 0;
    return *head;
}

static inline int __fs_index(char c)
{
    if ('0' <= c && c <= '9') {
        return c - '0';
    } else if ('A' <= c && c <= 'F') {
        return c - 'A' + 10;
    } else if (c == '-') {
        return 16;
    } else {
        DBG("Unsupported character by fstring");
        return -1;
    }
}

static inline int __fs_chr(int index)
{
    if (index < 0 || index >= FS_NUM_CHR)
        return -1;
    else if (index <= 9)
        return '0' + index;
    else if (index <= 15)
        return 'A' + index - 10;
    else
        return '-';
}

FSNode* fs_insert(FSNode* head, const char* str)
{
    if (*str == '\0') {
        head->end = true;
        return head;
    }

    int i = __fs_index(*str);
    if (i == -1)
        return NULL;
    if (!head->chr[i]) {
        if (!fs_init(&head->chr[i], head))
            return NULL;
    }
    return fs_insert(head->chr[i], str + 1);
}

static void __fs_sort(FSNode* head)
{
    if (head->end) {
        head->rank = fs_global_rank++;
    }
    FOR(i, 0, FS_NUM_CHR)
    {
        if (head->chr[i]) {
            fs_sort(head->chr[i]);
        }
    }
}

/* sort and give rank to each node */
void fs_sort(FSNode* head)
{
    fs_global_rank = 0;
    __fs_sort(head);
}

void fs_print(FSNode* head, FILE* stream)
{
    if (!head->parent)
        return;
    fs_print(head->parent, stream);
    FOR(i, 0, FS_NUM_CHR)
    {
        if (head->parent->chr[i] == head) {
            fputc(__fs_chr(i), stream);
        }
    }
}
