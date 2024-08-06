#ifndef SAFE_COM_H
#define SAFE_COM_H

#include "safecom_vtable.h"
#include "safecom_config.h"
#include "types.h"

typedef struct {
    SafeComVtable vtable;
    SafeComConfig config;
} SafeComType;

#if 0
typedef struct {
    SafeComVtable vtable;
    SafeComConfig config;
} SafeCom;
#endif
typedef SafeComType SafeCom;

StdRet_t SafeCom_Init(SafeCom* const self, const SafeComType* const pConfig);
StdRet_t SafeCom_Main(const SafeCom* const self);
StdRet_t SafeCom_ReceiveSpdu(const SafeCom* const self, const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData);
StdRet_t SafeCom_SendData(const SafeCom* const self, const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData);
StdRet_t SafeCom_OpenConnection(const SafeCom* const self, const MsgId_t msgId);
StdRet_t SafeCom_CloseConnection(const SafeCom* const self, const MsgId_t msgId);
StdRet_t SafeCom_ConnectionStateRequest(const SafeCom* const self, const MsgId_t msgId);

#endif /* SAFE_COM_H */
