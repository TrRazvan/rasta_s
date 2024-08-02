#ifndef SAFE_COM_VTABLE_H
#define SAFE_COM_VTABLE_H

#include "types.h"

typedef StdRet_t (*SendSpdu_t)(const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData);
typedef StdRet_t (*ReceiveMsg_t)(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData);

typedef struct {
    SendSpdu_t SendSpdu;
    ReceiveMsg_t ReceiveMsg;
} SafeComVtable;

#endif /* SAFE_COM_VTABLE_H */
