#ifndef SAFE_COM_CONFIG_H
#define SAFE_COM_CONFIG_H

#include "types.h"
#include "sm.h"

#define INSTNAME_LENGTH 10U

typedef struct {
    uint8_t instname[INSTNAME_LENGTH];
    SmRole role;
    MsgId_t max_connections;
    SmType* sms;
} SafeComConfig;

#endif /* SAFE_COM_CONFIG_H */
