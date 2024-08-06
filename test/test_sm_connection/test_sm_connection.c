#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include "cmocka.h"

#include "sm.h"
#include "safecom_config.h"
#include "log.h"

static SmType sm_server = { 
    .channel=0U,
    .role=ROLE_SERVER,
    .state=STATE_CLOSED,
}; 
static SmType sm_client = {
    .channel=0U,
    .role=ROLE_CLIENT,
    .state=STATE_CLOSED,
};

static void test_sm_client_init(void** state)
{
    (void)state;

    StdRet_t ret = Sm_Init(&sm_client);
    assert_true(ret==OK);
    assert_true(sm_client.state==STATE_CLOSED);
}

static void test_sm_client_init_to_open(void** state)
{
    PDU_S pdu = { 0 };
    
    test_sm_client_init(state);
    assert_true(sm_client.state==STATE_CLOSED);

    Sm_HandleEvent(&sm_client, EVENT_OPEN_CONN, &pdu);
    assert_true(sm_client.state==STATE_DOWN);
}


extern int test_sm_connection(void) {
    int return_value = -1;

    const struct CMUnitTest sm_connection_tests[] = {
        cmocka_unit_test(test_sm_client_init),
        cmocka_unit_test(test_sm_client_init_to_open),
    };

    return_value = cmocka_run_group_tests_name("sm_connection_tests", sm_connection_tests, NULL, NULL);

    return return_value;
}