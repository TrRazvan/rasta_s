#ifndef OSCOM_H
#define OSCOM_H

#include "types.h"

StdRet_t OsCom_SendSpdu(const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData);
StdRet_t OsCom_ReceiveMsg(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData);

#endif /* OSCOM_H */
