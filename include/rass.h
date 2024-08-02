#ifndef RASS_H
#define RASS_H

#include "safecom.h"

StdRet_t Rass_Init_VTable(const SafeComType* const pConfig);
StdRet_t Rass_Init(SafeComConfig* const pConfig);
StdRet_t Rass_Main(void);
StdRet_t Rass_ReceiveSpdu(const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData);
StdRet_t Rass_SendData(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData);
StdRet_t Rass_OpenConnection(const MsgId_t msgId);
StdRet_t Rass_CloseConnection(const MsgId_t msgId);
StdRet_t Rass_ConnectionStateRequest(const MsgId_t msgId);

#endif /* RASS_H */
