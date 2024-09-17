#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include "cmocka.h"

#include "log.h"

extern int test_sm_connection(void);
extern int test_rass_server(void);
extern int test_rass_client(void);
extern int test_rass_send_data(void);
extern int test_timeout(void);

static void simple_test(void **state) 
{
    (void)state; /* unused */

    assert_null(NULL);
}

static int group_setup(void **state)
{
    (void)state;
    int return_value=0;

    return return_value; 
}

static int group_teardown(void **state)
{
    (void)state;
    int return_value = 0;

    return return_value;
}

int main(int argc, char* argv[]) {
    int return_value = -1;

    printf("LOG_LEVEL: %i\n", get_loglevel_filter());
    set_loglevel_filter(LOG_ERROR);
    printf("LOG_LEVEL: %i\n", get_loglevel_filter());


    const struct CMUnitTest tests[] = {
        cmocka_unit_test(simple_test),
    };

    // return_value = cmocka_run_group_tests_name("simple_test" ,tests, group_setup, group_teardown);
    // return_value |= test_sm_connection();
    // return_value |= test_rass_server();
    // return_value |= test_rass_client();
    // return_value |= test_rass_send_data();
    return_value |= test_timeout();

    return return_value;
}