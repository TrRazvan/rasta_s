#ifndef SAFE_COM_IMPL_H
#define SAFE_COM_IMPL_H

#include "safecom.h"

StdRet_t SafeCom_Init_Impl(SafeCom* const self, const SafeComType* const pConfig);
StdRet_t SafeCom_Main_Impl(const SafeCom* const self);
StdRet_t SafeCom_ReceiveSpdu_Impl(const SafeCom* const self, const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData);
StdRet_t SafeCom_SendData_Impl(const SafeCom* const self, const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData);
StdRet_t SafeCom_OpenConnection_Impl(const SafeCom* const self, const MsgId_t msgId);
StdRet_t SafeCom_CloseConnection_Impl(const SafeCom* const self, const MsgId_t msgId);
StdRet_t SafeCom_ConnectionStateRequest_Impl(const SafeCom* const self, const MsgId_t msgId);

#endif /* SAFE_COM_IMPL_H */
