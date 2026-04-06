function(add_modbus_pp_benchmark BM_NAME BM_SOURCE)
    add_executable(${BM_NAME} ${BM_SOURCE})
    target_link_libraries(${BM_NAME}
        PRIVATE
            modbus_pp
            benchmark::benchmark
            benchmark::benchmark_main
    )
    target_compile_options(${BM_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endfunction()

add_modbus_pp_benchmark(bm_pdu_serialize         bm_pdu_serialize.cpp)
add_modbus_pp_benchmark(bm_type_codec            bm_type_codec.cpp)
add_modbus_pp_benchmark(bm_pipeline_vs_sequential bm_pipeline_vs_sequential.cpp)
add_modbus_pp_benchmark(bm_batch_vs_individual   bm_batch_vs_individual.cpp)
add_modbus_pp_benchmark(bm_hmac_overhead         bm_hmac_overhead.cpp)
