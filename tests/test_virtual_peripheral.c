/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/ceedless.h"
#include "tests.h"
#include <string.h>

static uint8_t        s_regs[16];
static vp_device_t    s_dev;

static void setup_dev(void) { vp_device_init(&s_dev, "td", s_regs, sizeof s_regs); }

static void test_write_then_read(void)
{
    setup_dev();
    uint8_t in[3] = { 0xAA, 0xBB, 0xCC };
    uint8_t out[3] = { 0 };
    TEST_ASSERT_EQUAL_INT(0, vp_device_write(&s_dev, 2, in, 3));
    TEST_ASSERT_EQUAL_INT(0, vp_device_read (&s_dev, 2, out, 3));
    TEST_ASSERT_EQUAL_MEMORY(in, out, 3);
    TEST_ASSERT_EQUAL_INT(1, s_dev.write_calls);
    TEST_ASSERT_EQUAL_INT(1, s_dev.read_calls);
    TEST_ASSERT_EQUAL_INT(2, s_dev.last_reg);
}

static void test_out_of_range_fails(void)
{
    setup_dev();
    uint8_t x = 0;
    TEST_ASSERT_EQUAL_INT(-1, vp_device_read (&s_dev, 100, &x, 1));
    TEST_ASSERT_EQUAL_INT(-1, vp_device_write(&s_dev, 14, &x, 4));
}

static int on_read_double(vp_device_t *d, uint32_t reg, uint8_t *buf, size_t n)
{
    (void)d; (void)reg;
    for (size_t i = 0; i < n; i++) buf[i] = (uint8_t)(buf[i] * 2);
    return 0;
}

static void test_on_read_hook_runs(void)
{
    setup_dev();
    s_dev.on_read = on_read_double;
    s_regs[5]     = 7;
    uint8_t out   = 0;
    TEST_ASSERT_EQUAL_INT(0, vp_device_read(&s_dev, 5, &out, 1));
    TEST_ASSERT_EQUAL_INT(14, out);
}

static int on_write_reject(vp_device_t *d, uint32_t reg, uint8_t *buf, size_t n)
{
    (void)d; (void)reg; (void)buf; (void)n;
    return -42;
}

static void test_on_write_hook_can_reject(void)
{
    setup_dev();
    s_dev.on_write = on_write_reject;
    uint8_t x = 1;
    TEST_ASSERT_EQUAL_INT(-42, vp_device_write(&s_dev, 0, &x, 1));
    TEST_ASSERT_EQUAL_INT(-42, s_dev.last_status);
}

static void test_reset_clears_counters(void)
{
    setup_dev();
    uint8_t x = 9;
    vp_device_write(&s_dev, 0, &x, 1);
    vp_device_read (&s_dev, 0, &x, 1);
    TEST_ASSERT_EQUAL_INT(1, s_dev.read_calls);
    vp_device_reset(&s_dev);
    TEST_ASSERT_EQUAL_INT(0, s_dev.read_calls);
    TEST_ASSERT_EQUAL_INT(0, s_dev.write_calls);
    TEST_ASSERT_EQUAL_INT(0, s_regs[0]);
}

/* --- I2C scenario: write reg pointer + payload, then read it back -------- */

static void test_i2c_eeprom_round_trip(void)
{
    static uint8_t eep_regs[64];
    vp_i2c_t eep;
    vp_i2c_init(&eep, "eeprom", 0x50, eep_regs, sizeof eep_regs);

    TEST_ASSERT_EQUAL_INT( 0, vp_i2c_start(&eep, 0x50));
    TEST_ASSERT_EQUAL_INT(-1, vp_i2c_start(&eep, 0x51)); /* NACK */

    uint8_t w[] = { 0x10, 0xDE, 0xAD, 0xBE, 0xEF };
    TEST_ASSERT_EQUAL_INT(0, vp_i2c_master_write(&eep, w, sizeof w));

    /* re-arm cursor */
    uint8_t set_ptr[] = { 0x10 };
    TEST_ASSERT_EQUAL_INT(0, vp_i2c_master_write(&eep, set_ptr, 1));

    uint8_t r[4] = { 0 };
    TEST_ASSERT_EQUAL_INT(0, vp_i2c_master_read(&eep, r, 4));

    uint8_t exp[] = { 0xDE, 0xAD, 0xBE, 0xEF };
    TEST_ASSERT_EQUAL_MEMORY(exp, r, 4);
    /* cursor advanced */
    TEST_ASSERT_EQUAL_INT(0x14, eep.cursor);
}

void run_virtual_peripheral_tests(void)
{
    RUN_TEST(test_write_then_read);
    RUN_TEST(test_out_of_range_fails);
    RUN_TEST(test_on_read_hook_runs);
    RUN_TEST(test_on_write_hook_can_reject);
    RUN_TEST(test_reset_clears_counters);
    RUN_TEST(test_i2c_eeprom_round_trip);
}
