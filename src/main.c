#include "rass.h"
#include "sic.h"
#include "assert.h"
#include <stdio.h>
#include <string.h>

static StdRet_t My_Fec_ReceiveBtp(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData) {
    assert(pMsgData != NULL);
    /* Implementation of Fec_ReceiveBtp */
    printf("[My_Fec_ReceiveBtp] msgId: %d, msgLen: %d, msg: %s\n", msgId, msgLen, pMsgData);
    return OK;
}

static StdRet_t My_IscDispApp_SendSpdu(const NodeId_t nodeId, const SpduLen_t spduLen, const uint8_t* const pSpduData) {
    assert(pSpduData != NULL);
    /* Implementation of IscDispApp_SendSpdu */
    return OK;
}


int main(int argc, char* argv[]) {
    const MsgId_t MAX_CONNECTIONS = 1;
    SmType sms[MAX_CONNECTIONS] = { 0 }; 

    const SafeComConfig config_server = { .instname="server\0", 
                                        .role=ROLE_SERVER, 
                                        .max_connections=MAX_CONNECTIONS,
                                        .sms=sms };
    const SafeComConfig config_client = { .instname="client\0", 
                                        .role=ROLE_CLIENT, 
                                        .max_connections=MAX_CONNECTIONS,
                                        .sms=sms}; 

    const SafeComVtable vtable = { .ReceiveMsg=My_Fec_ReceiveBtp, .SendSpdu=My_IscDispApp_SendSpdu};
    SafeComType config = { .vtable = vtable, 
                            .config = (argc==2 && (strcmp("server", argv[1])==0)) ? config_server : config_client };


    if (Rass_Init_VTable(&config) != OK) {
        fprintf(stderr, "Failed to initialize %s.\n", config.config.instname);
        return 1;
    }

    Rass_OpenConnection(0);






}
