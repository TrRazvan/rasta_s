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
#include "event_queue.h"

#define MAX_CONNECTIONS 1U

static StdRet_t My_ReceiveSpdu(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData);
static StdRet_t My_SendSpdu(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData);

static SmType sms[MAX_CONNECTIONS] = { 0 };
static TimerContext timer;
static EventQueue queue;

static int teardown_init(void **state)
{
    SafeComType *pConfig = (SafeComType*)(*state);
    free(pConfig);
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
        .instname = "server\0",
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

static void test_rass_init(void** state)
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

    timer_init(&timer, 0U, 0U);  /* Th = 0 sec, Ti = 0 sec */
    event_queue_init(&queue);
}

static void test_rass_open_connection(void** state)
{
    (void)state;

    StdRet_t ret = OK;

    for(uint8_t i = 0; i < MAX_CONNECTIONS; i++)
    {
        ret = Rass_OpenConnection(i);
        assert_true(ret == OK);
        assert_true(sms[i].state == STATE_DOWN);
    }
}

static void test_elapsed_time(void** state)
{
    (void)state;

    PDU_S pdu = { 0 };

    on_event_received(&queue, &timer, &sms[0], 0, EVENT_RECV_HB);

    process_events(&sms[0], &queue, &timer, &pdu);
    assert_true(sms[0].state == STATE_CLOSED);
}

extern int test_timeout(void) {
    int return_value = -1;

    const struct CMUnitTest test_timeout[] = {
        cmocka_unit_test_setup_teardown(test_rass_init, setup_rass_init, teardown_init),
        cmocka_unit_test(test_rass_open_connection),
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