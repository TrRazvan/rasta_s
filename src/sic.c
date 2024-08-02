#include "sic.h"
#include "assert.h"
#include "oscom.h"

static SafeCom SicInstance;
static SafeComConfig SicConfig = { .role = ROLE_SERVER, .instname = "JohnDoe\0" };
static const SafeComVtable SicVTable = { .SendSpdu = OsCom_SendSpdu, .ReceiveMsg = OsCom_ReceiveMsg };

static bool initialized = false; /* we are in a single threaded context */

StdRet_t Sic_Init_VTable(SafeComType* const pConfig) {
    assert(pConfig != NULL);
    assert(pConfig->vtable.ReceiveMsg != NULL);
    assert(pConfig->vtable.SendSpdu != NULL);

    return (initialized==true) ? NOT_OK : (initialized=true, SafeCom_Init(&SicInstance, pConfig));
}

StdRet_t Sic_Init(SafeComConfig* const pConfig) {
    if (pConfig != NULL) {
        SicConfig = *pConfig;
    }
    SafeComType config = {
        .vtable = {
            .SendSpdu = SicVTable.SendSpdu,
            .ReceiveMsg = SicVTable.ReceiveMsg
        },
        .config = SicConfig
    };
    return (initialized==true) ? NOT_OK : (initialized=true, SafeCom_Init(&SicInstance, &config) );
}

StdRet_t Sic_Main(void) {
    return SafeCom_Main(&SicInstance);
}

StdRet_t Sic_ReceiveSpdu(const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData) {
    assert(pSpduData != NULL);
    return SafeCom_ReceiveSpdu(&SicInstance, nodeId, spduLen, pSpduData);
}

StdRet_t Sic_SendData(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData) {
    assert(pMsgData != NULL);
    return SafeCom_SendData(&SicInstance, msgId, msgLen, pMsgData);
}

StdRet_t Sic_OpenConnection(const MsgId_t msgId) {
    return SafeCom_OpenConnection(&SicInstance, msgId);
}

StdRet_t Sic_CloseConnection(const MsgId_t msgId) {
    return SafeCom_CloseConnection(&SicInstance, msgId);
}

StdRet_t Sic_ConnectionStateRequest(const MsgId_t msgId) {
    return SafeCom_ConnectionStateRequest(&SicInstance, msgId);
}