#ifndef _COMMON_H_
#define _COMMON_H_ 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMCPY(dest, src) memcpy(dest, src, sizeof(dest))
#define MEMCMP(dest, src) memcmp(dest, src, sizeof(dest))

#ifndef NDEBUG
// guard syscall error
#define G(expr)                         \
    if ((expr) < 0) {                   \
        perror("\e[31m" #expr "\e[0m"); \
        exit(-1);                       \
    }

// guard an error by value
#define GG(expr, err_value)             \
    if ((expr) == err_value) {          \
        perror("\e[31m" #expr "\e[0m"); \
        exit(-1);                       \
    }

#define DBG(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#define DBGN(format, ...) fprintf(stderr, format, ##__VA_ARGS__)

#else
#define G(expr) (expr)
#define GG(expr, err_value) (expr)
#define DBG(...)
#define DBGN(...)
#endif

#endif