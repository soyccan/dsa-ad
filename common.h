#include <stdio.h>
#include <stdlib.h>

#define FOR(i, a, n) for (typeof(a) i = a; i < n; i++)

// guard syscall error
#define G(expr)        \
    if ((expr) < 0) {  \
        perror(#expr); \
        exit(-1);      \
    }

// guard an error by value
#define GG(expr, err_value)    \
    if ((expr) == err_value) { \
        perror(#expr);         \
        exit(-1);              \
    }

#define ERREXIT(msg) \
    {                \
        perror(msg); \
        exit(-1);    \
    }

#ifndef NDEBUG
#define DBG(format, ...) fprintf(stderr, format "\n", ##__VA_ARGS__)
#define DBGN(format, ...) fprintf(stderr, format, ##__VA_ARGS__)
#else
#define DBG(...)
#define DBGN(...)
#endif
