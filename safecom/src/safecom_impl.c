#include "safecom_impl.h"
#include "assert.h"
#include "sm.h"
#include "log.h"

static const StdRet_t INIT_RET = OK;

static SmType* sms = NULL;

StdRet_t SafeCom_Init_Impl(SafeCom* const self, const SafeComType* const pConfig) {
    assert(self != NULL);
    assert(pConfig != NULL);
 
    /* Implementation specific to SafeCom_Init */
    LOG_INFO("init module %s in role %i", pConfig->config.instname, pConfig->config.role);
    LOG_INFO("callouts of module %s: %p, %p", pConfig->config.instname, self->vtable.SendSpdu, self->vtable.ReceiveMsg);

    sms = pConfig->config.sms;

    for (int i=0; i<pConfig->config.max_connections; i++) {
        sms[i].channel = i;
        sms[i].state = STATE_CLOSED;
        sms[i].role = pConfig->config.role;
        Sm_Init(&sms[i]);
    }
    
    return INIT_RET;
}

StdRet_t SafeCom_Main_Impl(const SafeCom* const self) {
    assert(self != NULL);
    /* Implementation specific to SafeCom_Main */
    return INIT_RET;
}

StdRet_t SafeCom_ReceiveSpdu_Impl(const SafeCom* const self, const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData) {
    assert(self != NULL);
    assert(pSpduData != NULL);
    /* Implementation specific to SafeCom_ReceiveSpdu */
    self->vtable.ReceiveMsg(nodeId, spduLen, pSpduData);
    return INIT_RET;
}

StdRet_t SafeCom_SendData_Impl(const SafeCom* const self, const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData) {
    assert(self != NULL);
    assert(pMsgData != NULL);
    /* Implementation specific to SafeCom_SendData */
    return INIT_RET;
}

StdRet_t SafeCom_OpenConnection_Impl(const SafeCom* const self, const MsgId_t msgId) {
    assert(self != NULL);
    /* Implementation specific to SafeCom_OpenConnection */
    
    PDU_S recv_pdu = { 0 };
    Sm_HandleEvent(&sms[msgId], EVENT_OPEN_CONN, recv_pdu);
    return INIT_RET;
}

StdRet_t SafeCom_CloseConnection_Impl(const SafeCom* const self, const MsgId_t msgId) {
    assert(self != NULL);
    /* Implementation specific to SafeCom_CloseConnection */

    PDU_S pdu = { 0 };
    Sm_HandleEvent(&sms[msgId], EVENT_CLOSE_CONN, pdu);
    return INIT_RET;
}

StdRet_t SafeCom_ConnectionStateRequest_Impl(const SafeCom* const self, const MsgId_t msgId) {
    assert(self != NULL);
    /* Implementation specific to SafeCom_ConnectionStateRequest */
    return INIT_RET;
}
