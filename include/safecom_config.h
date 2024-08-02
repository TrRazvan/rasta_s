#ifndef SAFE_COM_CONFIG_H
#define SAFE_COM_CONFIG_H

#include "types.h"
#include "sm.h"

typedef struct {
    uint8_t instname[10];
    SmRole role;
    MsgId_t max_connections;
    SmType* sms;
} SafeComConfig;

#endif /* SAFE_COM_CONFIG_H */