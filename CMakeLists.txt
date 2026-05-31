# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
#
# Top-level CMakeLists for building ceedless's own self-tests via CMake.
# Most users should clone the repo and either (a) `make all` or
# (b) include `cmake/ceedless.cmake` from their own CMake project.

cmake_minimum_required(VERSION 3.16)
project(ceedless C)

set(CEEDLESS_HOME "${CMAKE_CURRENT_SOURCE_DIR}")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ceedless)

enable_testing()

set(SELFTEST_SRC
    tests/test_main.c
    tests/test_trace_port.c
    tests/test_virtual_peripheral.c
    tests/test_runner.c
    tests/test_assertions.c
    tests/test_exception.c
    tests/test_mock.c
    tests/test_peripherals.c
    tests/test_extensions.c
)

add_ceedless_test(ceedless_selftest
    SOURCES   ${SELFTEST_SRC}
    INCLUDE   include tests
    DEFINES   CEEDLESS_TRACE_BUFFER
)

add_ceedless_test(ceedless_example_sensor
    SOURCES
        examples/sensor_driver/sensor_driver.c
        examples/sensor_driver/test_sensor_driver.c
    INCLUDE   examples/sensor_driver
)
