#include "safecom.h"
#include "safecom_impl.h"
#include "assert.h"

StdRet_t SafeCom_Init(SafeCom* const self, const SafeComType* const pConfig) {
    assert(self != NULL);
    assert(pConfig != NULL);

    /* Initialize the vtable and config */
    self->vtable = pConfig->vtable;
    self->config = pConfig->config;

    /* Call implementation specific init function */
    return SafeCom_Init_Impl(self, pConfig);
}

StdRet_t SafeCom_Main(const SafeCom* const self) {
    assert(self != NULL);
    return SafeCom_Main_Impl(self);
}

StdRet_t SafeCom_ReceiveSpdu(const SafeCom* const self, const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData) {
    assert(self != NULL);
    assert(pSpduData != NULL);
    return SafeCom_ReceiveSpdu_Impl(self, nodeId, spduLen, pSpduData);
}

StdRet_t SafeCom_SendData(const SafeCom* const self, const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData) {
    assert(self != NULL);
    assert(pMsgData != NULL);
    return SafeCom_SendData_Impl(self, msgId, msgLen, pMsgData);
}

StdRet_t SafeCom_OpenConnection(const SafeCom* const self, const MsgId_t msgId) {
    assert(self != NULL);
    return SafeCom_OpenConnection_Impl(self, msgId);
}

StdRet_t SafeCom_CloseConnection(const SafeCom* const self, const MsgId_t msgId) {
    assert(self != NULL);
    return SafeCom_CloseConnection_Impl(self, msgId);
}

StdRet_t SafeCom_ConnectionStateRequest(const SafeCom* const self, const MsgId_t msgId) {
    assert(self != NULL);
    return SafeCom_ConnectionStateRequest_Impl(self, msgId);
}
