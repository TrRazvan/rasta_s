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

StdRet_t My_ReceiveSpdu(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData);
StdRet_t My_SendSpdu(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData);

static SmType sms[MAX_CONNECTIONS] = { 0 };

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
}

static void test_elapsed_time(void** state)
{
    (void)state;

    PDU_S pdu = { 0 };

    Sm_HandleEvent(&sms[0], EVENT_TI_ELAPSED, &pdu);
    assert_true(sms[0].state == STATE_CLOSED);
}

extern int mock_tests(void) {
    int return_value = -1;

    const struct CMUnitTest mock_tests[] = {
        cmocka_unit_test_setup_teardown(test_rass_init, setup_rass_init, teardown_init),
        cmocka_unit_test(test_elapsed_time),
    };

    return_value = cmocka_run_group_tests_name("mock_tests", mock_tests, NULL, NULL);

    return return_value;
}

StdRet_t My_ReceiveSpdu(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData)
{   
    StdRet_t ret = OK;

    PDU_S pdu = { 0 };
    deserialize_pdu(pMsgData, msgLen, &pdu);
    Sm_HandleEvent(&sms[msgId], pdu.message_type, &pdu);
    
    return ret;
}

StdRet_t My_SendSpdu(const MsgId_t msgId, const MsgLen_t msgLen, const uint8_t* const pMsgData)
{   
    StdRet_t ret = OK;
    printf("SendSpdu\n");
    return ret;
}