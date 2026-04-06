function(add_modbus_pp_test TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME}
        PRIVATE
            modbus_pp
            GTest::gtest_main
    )
    target_compile_options(${TEST_NAME} PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_modbus_pp_test(test_types       test_types.cpp)
add_modbus_pp_test(test_endian      test_endian.cpp)
add_modbus_pp_test(test_error       test_error.cpp)
add_modbus_pp_test(test_timestamp   test_timestamp.cpp)
add_modbus_pp_test(test_pdu         test_pdu.cpp)
add_modbus_pp_test(test_frame_codec   test_frame_codec.cpp)
add_modbus_pp_test(test_type_codec    test_type_codec.cpp)
add_modbus_pp_test(test_register_map  test_register_map.cpp)
add_modbus_pp_test(test_batch_request test_batch_request.cpp)
add_modbus_pp_test(test_hmac_auth     test_hmac_auth.cpp)
add_modbus_pp_test(test_pipeline      test_pipeline.cpp)
add_modbus_pp_test(test_pubsub        test_pubsub.cpp)
