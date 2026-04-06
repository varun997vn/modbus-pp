cmake_minimum_required(VERSION 3.14)

project(modbus_pp
    VERSION 1.0.0
    DESCRIPTION "Modern C++17 Modbus library addressing protocol limitations"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(MODBUS_PP_BUILD_TESTS      "Build unit and integration tests" ON)
option(MODBUS_PP_BUILD_BENCHMARKS "Build performance benchmarks"     ON)
option(MODBUS_PP_BUILD_EXAMPLES   "Build example programs"           ON)
option(MODBUS_PP_ENABLE_ASAN      "Enable AddressSanitizer"          OFF)
option(MODBUS_PP_ENABLE_TSAN      "Enable ThreadSanitizer"           OFF)

# Sanitizer flags
if(MODBUS_PP_ENABLE_ASAN)
    add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address)
endif()

if(MODBUS_PP_ENABLE_TSAN)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

# Dependencies
include(cmake/FetchDependencies.cmake)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Library
add_subdirectory(src)

# Optional targets
if(MODBUS_PP_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(MODBUS_PP_BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()

if(MODBUS_PP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Install rules
include(GNUInstallDirs)

install(TARGETS modbus_pp
    EXPORT modbus_pp-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/modbus_pp
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT modbus_pp-targets
    FILE modbus_pp-targets.cmake
    NAMESPACE modbus_pp::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/modbus_pp
)

configure_file(
    cmake/ModbusPPConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/modbus_pp-config.cmake
    @ONLY
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/modbus_pp-config.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/modbus_pp
)
