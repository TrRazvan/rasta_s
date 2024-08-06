#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Typedefs */
typedef uint32_t MsgId_t;
typedef uint32_t MsgLen_t;
typedef uint32_t NodeId_t;
typedef uint32_t SpduLen_t;

typedef enum {
    OK = 0U,
    NOT_OK
} StdRet_t;

#define UNUSED(x) (void)(x)

#endif /* TYPES_H */
