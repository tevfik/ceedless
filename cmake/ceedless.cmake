# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
#
# ceedless.cmake — CMake integration for the ceedless test framework.
#
# Usage in your CMakeLists.txt:
#
#   list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
#   set(CEEDLESS_HOME "/path/to/ceedless")
#   include(ceedless)
#
#   enable_testing()
#   add_ceedless_test(test_hello
#       SOURCES test/test_hello.c src/hello.c
#       INCLUDE include
#   )
#
# Provides:
#   add_ceedless_test(<name> SOURCES … [INCLUDE …] [DEFINES …] [LIBS …])
#       Builds <name> as an executable linking the ceedless core sources,
#       registers it with CTest, and points the JUnit env var at
#       build/Testing/<name>.xml so `ctest --output-junit` works.

if(NOT DEFINED CEEDLESS_HOME)
    message(FATAL_ERROR "CEEDLESS_HOME must be set before include(ceedless)")
endif()
if(NOT IS_DIRECTORY "${CEEDLESS_HOME}")
    message(FATAL_ERROR "CEEDLESS_HOME='${CEEDLESS_HOME}' is not a directory")
endif()

set(CEEDLESS_CORE_SRC
    "${CEEDLESS_HOME}/src/runner.c"
    "${CEEDLESS_HOME}/src/exception.c"
    "${CEEDLESS_HOME}/src/mock.c"
    "${CEEDLESS_HOME}/src/virtual_peripheral.c"
    "${CEEDLESS_HOME}/src/trace/trace_port.c"
    "${CEEDLESS_HOME}/src/trace/trace_host.c"
    "${CEEDLESS_HOME}/src/trace/trace_uart.c"
    "${CEEDLESS_HOME}/src/trace/trace_rtt.c"
    "${CEEDLESS_HOME}/src/trace/trace_itm.c"
    "${CEEDLESS_HOME}/src/trace/trace_buffer.c"
    "${CEEDLESS_HOME}/src/peripherals/vp_spi.c"
    "${CEEDLESS_HOME}/src/peripherals/vp_uart.c"
    "${CEEDLESS_HOME}/src/peripherals/vp_gpio.c"
    "${CEEDLESS_HOME}/src/peripherals/vp_adc.c"
)

function(add_ceedless_test name)
    cmake_parse_arguments(ARG "" "" "SOURCES;INCLUDE;DEFINES;LIBS" ${ARGN})
    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "add_ceedless_test(${name}): SOURCES is required")
    endif()
    add_executable(${name} ${ARG_SOURCES} ${CEEDLESS_CORE_SRC})
    target_include_directories(${name}
        PRIVATE "${CEEDLESS_HOME}/include" ${ARG_INCLUDE})
    # Pick any trace backend explicitly via DEFINES. If none is supplied,
    # trace_config.h falls back to CEEDLESS_TRACE_HOST automatically.
    target_compile_definitions(${name} PRIVATE ${ARG_DEFINES})
    target_compile_features(${name} PRIVATE c_std_11)
    if(ARG_LIBS)
        target_link_libraries(${name} PRIVATE ${ARG_LIBS})
    endif()
    target_link_libraries(${name} PRIVATE m)

    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES
        ENVIRONMENT "CEEDLESS_JUNIT=${CMAKE_BINARY_DIR}/Testing/${name}.xml")
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/Testing")
endfunction()
