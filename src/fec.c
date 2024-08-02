#include "fec.h"
#include "assert.h"

/* Implement the Fec functions here */

StdRet_t Fec_ReceiveBtp(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData) {
    assert(pMsgData != NULL);
    /* Implementation of Fec_ReceiveBtp */
    printf("[Fec_ReceiveBtp] msgId: %d, msgLen: %d, msg: %s\n", msgId, msgLen, pMsgData);
    return OK;
}
