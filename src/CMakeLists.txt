add_library(modbus_pp
    core/error.cpp
    core/timestamp.cpp
    core/pdu.cpp
    transport/loopback_transport.cpp
    transport/frame_codec.cpp
    security/hmac_auth.cpp
    security/session.cpp
    pipeline/pipeline.cpp
    pubsub/publisher.cpp
    pubsub/subscriber.cpp
    discovery/scanner.cpp
    transport/tcp_transport.cpp
    transport/rtu_transport.cpp
    client/client.cpp
    client/server.cpp
)

target_include_directories(modbus_pp
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(modbus_pp
    PUBLIC
        Threads::Threads
    PRIVATE
        OpenSSL::Crypto
)

target_compile_features(modbus_pp PUBLIC cxx_std_17)

# Strict warnings
target_compile_options(modbus_pp PRIVATE
    -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor
    -Wold-style-cast -Wcast-align -Woverloaded-virtual
    -Wconversion -Wsign-conversion
)
