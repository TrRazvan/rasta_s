#include "oscom.h"
#include "assert.h"

/* Implement the OsCom functions here */

StdRet_t OsCom_SendSpdu(const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData) {
    assert(pSpduData != NULL);
    /* Implementation of OsCom_SendSpdu */
    return OK;
}

StdRet_t OsCom_ReceiveMsg(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData) {
    assert(pMsgData != NULL);
    /* Implementation of OsCom_ReceiveMsg */
    printf("[OsCom_ReceiveMsg] msgId: %d, msgLen: %d, msg: %s\n", msgId, msgLen, pMsgData);
    return OK;
}
