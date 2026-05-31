/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 *
 * Cortex-M PIL self-test: the SAME assertion macros run on the MCU and
 * report results via ARM semihosting. Build & run with `make qemu`.
 */
#include "ceedless/ceedless.h"

extern void ceedless_semihost_exit(int code);

static void test_arithmetic_on_target(void)
{
    TEST_ASSERT_EQUAL_INT(42, 6 * 7);
    TEST_ASSERT_GREATER_THAN(0, 1);
}

static void test_assert_hw_macro_is_active(void)
{
    /* CEEDLESS_TARGET=cortex_m3 means TEST_ASSERT_HW runs, _HOST is no-op. */
    TEST_ASSERT_HW(1 == 1);
    TEST_ASSERT_HOST(0); /* would normally fail on host */
}

static void test_bit_ops(void)
{
    uint32_t v = 0x1234;
    TEST_ASSERT_EQUAL_HEX32(0x1234, v);
    TEST_ASSERT_BIT_HIGH(2, v);
    TEST_ASSERT_BIT_LOW (0, v);
}

int main(void)
{
    ceedless_begin("cortex_m3_pil");
    RUN_TEST(test_arithmetic_on_target);
    RUN_TEST(test_assert_hw_macro_is_active);
    RUN_TEST(test_bit_ops);
    int rc = ceedless_end();
    ceedless_semihost_exit(rc);
    return rc;
}
