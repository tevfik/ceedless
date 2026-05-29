/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/ceedless.h"
#include "tests.h"
#include <stdlib.h>

int main(void)
{
    ceedless_begin("ceedless self-tests");
    const char *jx = getenv("CEEDLESS_JUNIT");
    if (jx) ceedless_set_junit_path(jx);

    run_trace_port_tests();
    run_virtual_peripheral_tests();
    run_runner_tests();
    run_assertions_tests();
    run_exception_tests();
    run_mock_tests();
    run_peripherals_tests();
    return ceedless_end();
}
