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
    assert_true(sm_client.state==STATE_START);
}

static void test_sm_server_init(void** state)
{
    (void)state;

    StdRet_t ret = Sm_Init(&sm_server);
    assert_true(ret==OK);
    assert_true(sm_server.state==STATE_CLOSED);
}

static void test_sm_server_init_to_open(void** state)
{
    PDU_S pdu = { 0 };

    test_sm_server_init(state);
    assert_true(sm_server.state==STATE_CLOSED);

    Sm_HandleEvent(&sm_server, EVENT_OPEN_CONN, &pdu);
    assert_true(sm_server.state==STATE_DOWN);
}

static void test_sm_server_conn_req(void** state)
{
    PDU_S pdu = { 0 };

    pdu = ConnReq(&sm_server);

    Sm_HandleEvent(&sm_server, EVENT_RECV_CONN_REQ, &pdu);
    assert_true(sm_server.state==STATE_START);
}

static void test_sm_client_conn_resp(void** state)
{
    PDU_S pdu = { 0 };

    pdu = ConnResp(&sm_client);

    Sm_HandleEvent(&sm_client, EVENT_RECV_CONN_RESP, &pdu);
    assert_true(sm_client.state==STATE_UP);
}

static void test_sm_server_heartbeat(void** state)
{
    PDU_S pdu = { 0 };

    pdu = HB(&sm_server);

    Sm_HandleEvent(&sm_server, EVENT_RECV_HB, &pdu);
    assert_true(sm_server.state==STATE_UP);
}

static void test_sm_client_close_conn(void** state)
{
    PDU_S pdu = { 0 };

    Sm_HandleEvent(&sm_client, EVENT_CLOSE_CONN, &pdu);
    assert_true(sm_client.state==STATE_CLOSED);
}

static void test_sm_server_disc_req(void** state)
{
    PDU_S pdu = { 0 };

    pdu = DiscReq(USER_REQUEST, NO_DETAILED_REASON, &sm_server);

    Sm_HandleEvent(&sm_server, EVENT_RECV_DISC_REQ, &pdu);
    assert_true(sm_server.state==STATE_CLOSED);
}

extern int test_sm_connection(void) {
    int return_value = -1;

    const struct CMUnitTest sm_connection_tests[] = {
        cmocka_unit_test(test_sm_client_init),          /* Initialise the client */
        cmocka_unit_test(test_sm_server_init),          /* Initialise the server */
        cmocka_unit_test(test_sm_server_init_to_open),  /* The server is ready to open connection */
        cmocka_unit_test(test_sm_client_init_to_open),  /* The client is ready to open connetion and sent ConnReq */
        cmocka_unit_test(test_sm_server_conn_req),      /* The server responds to ConnReq and send ConnResp */
        cmocka_unit_test(test_sm_client_conn_resp),     /* The client responds to ConnResp and send Heartbeat */
        cmocka_unit_test(test_sm_server_heartbeat),     /* The server responds to Heartbeat */
        cmocka_unit_test(test_sm_client_close_conn),    /* The client wants to close the connection and send DiscReq */
        cmocka_unit_test(test_sm_server_disc_req),      /* The server responds to DiscReq and close */
    };

    return_value = cmocka_run_group_tests_name("sm_connection_tests", sm_connection_tests, NULL, NULL);

    return return_value;
}