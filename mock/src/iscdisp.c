#include "iscdisp.h"
#include "assert.h"

/* Implement the IscDispApp_SendSpdu function here */

StdRet_t IscDispApp_SendSpdu(const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData) {
    assert(pSpduData != NULL);
    /* Implementation of IscDispApp_SendSpdu */
    return OK;
}
