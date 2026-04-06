function(add_modbus_pp_integration_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME}
        PRIVATE
            modbus_pp
            GTest::gtest_main
    )
    target_compile_options(${TEST_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_modbus_pp_integration_test(test_loopback_roundtrip test_loopback_roundtrip.cpp)
add_modbus_pp_integration_test(test_client_server test_client_server.cpp)
