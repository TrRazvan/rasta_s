#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include "cmocka.h"

#include "sm.h"
#include "safecom_config.h"
#include "rass.h"
#include "log.h"

#define MAX_CONNECTIONS 1U

static StdRet_t My_ReceiveSpdu(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData);
static StdRet_t My_SendSpdu(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData);

static SmType sms[MAX_CONNECTIONS] = { 0 };

static int teardown_init(void **state)
{
    SafeComType *pConfig = (SafeComType*)(*state);
    free(pConfig);
    return 0;
}

static int teardown_receive(void **state)
{
    PDU_S *pPdu = (PDU_S*)(*state);
    free(pPdu);
    return 0;
}

static int setup_rass_client_init(void **state)
{
    int return_value = 0;

    SafeComType *p = (SafeComType*)malloc(sizeof(SafeComType));
    if (p == NULL) {
        return_value = -1;
    }

    const SafeComConfig config_client = {
        .instname = "client\0",
        .max_connections = MAX_CONNECTIONS,
        .role = ROLE_CLIENT,
        .sms = sms
    };

    const SafeComVtable vtable = {
        .ReceiveMsg = My_ReceiveSpdu,
        .SendSpdu = My_SendSpdu
    };
    
    p->vtable = vtable; 
    p->config = config_client;
    *state = p;

    return return_value;
}

static int setup_conn_resp(void **state)
{
    int return_value = 0;
    
    PDU_S *p = (PDU_S*)malloc(sizeof(PDU_S));
    if (p == NULL) {
        return_value = -1;
    }

    *p = ConnResp(sms[0]);
    *state = p;

    return return_value;
}

static int setup_hb(void **state)
{
    PDU_S *p = (PDU_S*)malloc(sizeof(PDU_S));
    int return_value = 0;
    if (p == NULL) {
        return_value = -1;
    }
    *p = HB(sms[0]);
    *state = p;

   return return_value;
}

static void test_rass_client_init(void** state)
{
    (void)state;

    StdRet_t ret = OK;
    SafeComType *pConfig = (SafeComType*)(*state);

    ret = Rass_Init_VTable(pConfig);
    assert_true(ret == OK);
    for(uint8_t i = 0; i < MAX_CONNECTIONS; i++)
    {
        assert_true(sms[i].state == STATE_CLOSED);
    }
}

static void test_rass_client_open_connection(void** state)
{
    (void)state;

    StdRet_t ret = OK;

    for(uint8_t i = 0; i < MAX_CONNECTIONS; i++)
    {
        ret = Rass_OpenConnection(i);
        assert_true(ret == OK);
        assert_true(sms[i].state == STATE_START);
    }
}

static void test_rass_client_receive_spdu(void **state)
{
    PDU_S *pPdu = (PDU_S*)(*state);
    StdRet_t ret = OK;
    uint8_t buffer[50];

    serialize_pdu(*pPdu, buffer, pPdu->message_length);
    ret = Rass_ReceiveSpdu(0, pPdu->message_length, buffer);
    assert_true(ret == OK);

    if (pPdu->message_type == CONNECTION_RESPONSE)
    {
        assert_true(sms[0].state == STATE_UP);
    }
}

static void test_rass_client_close_connection(void** state)
{
    (void)state;

    StdRet_t ret = OK;

    for(uint8_t i; i < MAX_CONNECTIONS; i++)
    {
        ret = Rass_CloseConnection(i);
        assert_true(ret == OK);
        assert_true(sms[i].state == STATE_CLOSED);
    }
}

extern int test_rass_client(void)
{
    int return_value = -1;

    const struct CMUnitTest rass_client_connection_tests[] = {
        cmocka_unit_test_setup_teardown(test_rass_client_init, setup_rass_client_init, teardown_init), /* Initialize the client */
        cmocka_unit_test(test_rass_client_open_connection), /* The client is ready to open connection and send Connection Request */
        cmocka_unit_test_setup_teardown(test_rass_client_receive_spdu, setup_conn_resp, teardown_receive), /* The client received Connection Response and send Heartbeat*/
        cmocka_unit_test(test_rass_client_close_connection), /* The client initiates closing the connection  */
    };

    return_value = cmocka_run_group_tests_name("rass_client_connection_tests", rass_client_connection_tests, NULL, NULL);

    return return_value;
}

static StdRet_t My_ReceiveSpdu(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData)
{   
    StdRet_t ret = OK;

    PDU_S pdu = { 0 };
    deserialize_pdu(pMsgData, msgLen, &pdu);
    Sm_HandleEvent(&sms[msgId], pdu.message_type, pdu);
    
    return ret;
}

static StdRet_t My_SendSpdu(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData)
{   
    StdRet_t ret = OK;
    return ret;
}