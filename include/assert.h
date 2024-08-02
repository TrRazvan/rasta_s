#ifndef ASSERT_H
#define ASSERT_H

#include <stdio.h>

#ifdef NDEBUG
#define assert(expression) ((void)0)
#else
#define assert(expression) \
    do { \
        if (!(expression)) { \
            fprintf(stderr, "Assertion failed: %s, file %s, line %d\n", #expression, __FILE__, __LINE__); \
            /* Implement logging or other actions here */ \
            while(1); \
        } \
    } while (0)
#endif

#endif /* ASSERT_H */

