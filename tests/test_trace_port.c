/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/ceedless.h"
#include "ceedless/trace_buffer.h"
#include "tests.h"

#include <string.h>

static int contains(const char *needle)
{
    return strstr(ceedless_trace_buffer(), needle) != 0;
}

static void test_trace_writes_data(void)
{
    ceedless_trace_buffer_clear();
    trace_puts("hello-trace");
    TEST_ASSERT_TRUE(contains("hello-trace"));
}

static void test_trace_printf_formats(void)
{
    ceedless_trace_buffer_clear();
    trace_printf("v=%d/%s", 42, "ok");
    TEST_ASSERT_TRUE(contains("v=42/ok"));
}

static void test_trace_write_raw_bytes(void)
{
    ceedless_trace_buffer_clear();
    const char bytes[] = "rawpayload";
    trace_write(bytes, sizeof bytes - 1);
    TEST_ASSERT_EQUAL_INT((long)(sizeof bytes - 1),
                          (long)ceedless_trace_buffer_len());
    TEST_ASSERT_TRUE(contains("rawpayload"));
}

void run_trace_port_tests(void)
{
    RUN_TEST(test_trace_writes_data);
    RUN_TEST(test_trace_printf_formats);
    RUN_TEST(test_trace_write_raw_bytes);
}
