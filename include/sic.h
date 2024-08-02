#ifndef SIC_H
#define SIC_H

#include "safecom.h"

StdRet_t Sic_Init_VTable(SafeComType* const pConfig);
StdRet_t Sic_Init(SafeComConfig* const pConfig);
StdRet_t Sic_Main(void);
StdRet_t Sic_ReceiveSpdu(const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData);
StdRet_t Sic_SendData(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData);
StdRet_t Sic_OpenConnection(const MsgId_t msgId);
StdRet_t Sic_CloseConnection(const MsgId_t msgId);
StdRet_t Sic_ConnectionStateRequest(const MsgId_t msgId);

#endif /* SIC_H */
