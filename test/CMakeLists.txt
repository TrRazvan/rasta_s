set(CMOCKA_TEST_NAME ${PROJECT_NAME}_test)
add_executable(${CMOCKA_TEST_NAME})

# add_library(cmocka SHARED IMPORTED)
# set_target_properties(cmocka PROPERTIES
#     IMPORTED_LOCATION "/usr/local/lib/libcmocka.dylib"
#     INTERFACE_INCLUDE_DIRECTORIES "/usr/local/include"    
# )

#add_subdirectory(test_sm_connection)
#file(GLOB test_sm_connection_src *.c)

target_sources(${CMOCKA_TEST_NAME} PRIVATE 
        test.c
        # test_sm_connection/test_sm_connection.c
        # test_rass_functionality/test_rass_server.c
        # test_rass_functionality/test_rass_client.c
        # mock_tests/mock_tests.c
        test_rass_functionality/test_rass_send_data.c
        )


target_link_libraries(${CMOCKA_TEST_NAME} PRIVATE common mock safecom cmocka)