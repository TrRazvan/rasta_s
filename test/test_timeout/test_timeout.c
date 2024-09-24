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

static int setup_rass_init(void **state)
{
    int return_value = 0;

    SafeComType *p = (SafeComType*)malloc(sizeof(SafeComType));
    if (p == NULL) {
        return_value = -1;
    }

    const SafeComConfig config = {
        .instname = "server",
        .max_connections = MAX_CONNECTIONS,
        .role = ROLE_SERVER,
        .sms = sms
    };

    const SafeComVtable vtable = {
        .ReceiveMsg = My_ReceiveSpdu,
        .SendSpdu = My_SendSpdu
    };
    
    p->vtable = vtable; 
    p->config = config;
    *state = p;

    return return_value;
}

static int setup_conn_req(void **state)
{
    int return_value = 0;
    
    PDU_S *p = (PDU_S*)malloc(sizeof(PDU_S));
    if (p == NULL) {
        return_value = -1;
    }

    ConnReq(&sms[0], p);
    *state = p;

    return return_value;
}

static void test_rass_init(void** state)
{
    StdRet_t ret = OK;
    SafeComType *pConfig = (SafeComType*)(*state);

    ret = Rass_Init_VTable(pConfig);
    assert_true(ret == OK);
    for(uint8_t i = 0; i < MAX_CONNECTIONS; i++)
    {
        assert_true(sms[i].state == STATE_CLOSED);
    }
}

static void test_rass_open_connection(void** state)
{
    StdRet_t ret = OK;

    for(uint8_t i = 0; i < MAX_CONNECTIONS; i++)
    {
        ret = Rass_OpenConnection(i);
        assert_true(ret == OK);
        assert_true(sms[i].state == STATE_DOWN);
    }
}

static void test_rass_receive_spdu(void **state)
{
    PDU_S *pPdu = (PDU_S*)(*state);
    StdRet_t ret = OK;
    uint8_t buffer[50];

    serialize_pdu(pPdu, buffer, pPdu->message_length);
    ret = Rass_ReceiveSpdu(0, pPdu->message_length, buffer);
    assert_true(ret == OK);

    if (pPdu->message_type == CONNECTION_REQUEST)
    {
        assert_true(sms[0].state == STATE_START);
    }
}

static void test_elapsed_time(void** state)
{
    (void)state;

    PDU_S pdu = { 0 };

    while(sms[0].state != STATE_CLOSED)
    {
        for(int i = 0; i < 500000000; i++);
        sms[0].time.Ti--;
        if ((sms[0].time.Tlocal() % sms[0].time.timeouts.Th) == 0)
        {
            Sm_HandleEvent(&sms[0], EVENT_TH_ELAPSED, &pdu);
        }
        if(sms[0].time.Ti <= 0)
        {
            Sm_HandleEvent(&sms[0], EVENT_TI_ELAPSED, &pdu);
        }
        if (sms[0].time.Tlocal() == 5)
        {
            HB(&sms[0], &pdu);
            Sm_HandleEvent(&sms[0], EVENT_RECV_HB, &pdu);
        }
        
    }

    assert_true(sms[0].state == STATE_CLOSED);
}

extern int test_timeout(void) {
    int return_value = -1;

    const struct CMUnitTest test_timeout[] = {
        cmocka_unit_test_setup_teardown(test_rass_init, setup_rass_init, teardown_init),
        cmocka_unit_test(test_rass_open_connection),
        cmocka_unit_test_setup_teardown(test_rass_receive_spdu, setup_conn_req, teardown_receive),
        cmocka_unit_test(test_elapsed_time),
    };

    return_value = cmocka_run_group_tests_name("test_timeout", test_timeout, NULL, NULL);

    return return_value;
}

static StdRet_t My_ReceiveSpdu(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData)
{   
    StdRet_t ret = OK;

    PDU_S pdu = { 0 };
    deserialize_pdu(pMsgData, msgLen, &pdu);
    if (pdu.message_type == CONNECTION_REQUEST)
    {
        Sm_HandleEvent(&sms[msgId], EVENT_RECV_CONN_REQ, &pdu);
    }
    else if (pdu.message_type == HEARTBEAT)
    {
        Sm_HandleEvent(&sms[msgId], EVENT_RECV_HB, &pdu);
    }
    
    return ret;
}

static StdRet_t My_SendSpdu(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData)
{   
    StdRet_t ret = OK;
    for (int i = 0; i < msgLen; i++)
    {
        printf("%c", pMsgData[i]);
    }
    printf("\n");
    return ret;
}