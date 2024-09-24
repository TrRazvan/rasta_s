#include "rass.h"
#include "assert.h"
#include "iscdisp.h"
#include "fec.h"

static SafeCom RassInstance;
static SafeComConfig RassConfig = { .role = ROLE_CLIENT, .instname = "JohnDoe\0" };
static const SafeComVtable RassVTable = { .SendSpdu = IscDispApp_SendSpdu, .ReceiveMsg = Fec_ReceiveBtp };

static bool initialized = false; /* we are in a single threaded context */

StdRet_t Rass_Init_VTable(const SafeComType* const pConfig) {
    assert(pConfig != NULL);
    assert(pConfig->vtable.ReceiveMsg != NULL);
    assert(pConfig->vtable.SendSpdu != NULL);

    return (initialized==true) ? NOT_OK : (initialized=true, SafeCom_Init(&RassInstance, pConfig));
}

StdRet_t Rass_Init(SafeComConfig* const pConfig) {
    if (pConfig != NULL) {
        RassConfig = *pConfig;
    }
    SafeComType config = {
        .vtable = {
            .SendSpdu = RassVTable.SendSpdu,
            .ReceiveMsg = RassVTable.ReceiveMsg
        },
        .config = RassConfig
    };

    return (initialized==true) ? NOT_OK : (initialized=true, SafeCom_Init(&RassInstance, &config) );
}

StdRet_t Rass_Main(void) {
    return SafeCom_Main(&RassInstance);
}

StdRet_t Rass_ReceiveSpdu(const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData) {
    assert(pSpduData != NULL);
    return SafeCom_ReceiveSpdu(&RassInstance, nodeId, spduLen, pSpduData);
}

StdRet_t Rass_SendData(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData) {
    assert(pMsgData != NULL);
    return SafeCom_SendData(&RassInstance, msgId, msgLen, pMsgData);
}

StdRet_t Rass_OpenConnection(const MsgId_t msgId) {
    return SafeCom_OpenConnection(&RassInstance, msgId);
}

StdRet_t Rass_CloseConnection(const MsgId_t msgId) {
    return SafeCom_CloseConnection(&RassInstance, msgId);
}

StdRet_t Rass_ConnectionStateRequest(const MsgId_t msgId) {
    return SafeCom_ConnectionStateRequest(&RassInstance, msgId);
}