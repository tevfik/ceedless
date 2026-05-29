/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Example test that exercises the *real* sensor_driver code through the
 * stateful virtual I2C peripheral, while routing trace output through the
 * pluggable trace API. Build the same source for host or target.
 */
#include "ceedless/ceedless.h"
#include "sensor_driver.h"
#include "i2c_hal.h"

/* ---- virtual I2C device used by the mock HAL --------------------------- */
static uint8_t  s_regs[8];
static vp_i2c_t s_sensor;

/* ---- mock HAL: forwards to the virtual peripheral ---------------------- */
int hal_i2c_write(uint8_t addr, const uint8_t *data, size_t n)
{
    if (vp_i2c_start(&s_sensor, addr) != 0) return -1;
    return vp_i2c_master_write(&s_sensor, data, n);
}
int hal_i2c_read(uint8_t addr, uint8_t *data, size_t n)
{
    if (vp_i2c_start(&s_sensor, addr) != 0) return -1;
    return vp_i2c_master_read(&s_sensor, data, n);
}

/* ---- test fixtures ----------------------------------------------------- */
static void setup_sensor(void)
{
    vp_i2c_init(&s_sensor, "tmp_sensor", SENSOR_I2C_ADDR,
                s_regs, sizeof s_regs);
    /* stage temperature register: 0x1AC0 */
    s_regs[0] = 0x1A;
    s_regs[1] = 0xC0;
}

static void test_reads_temperature(void)
{
    setup_sensor();
    int16_t t = 0;
    TEST_ASSERT_EQUAL_INT(0,      sensor_read_raw_temp(&t));
    TEST_ASSERT_EQUAL_INT(0x1AC0, t);
    /* the virtual peripheral observed exactly one write (reg pointer)
     * followed by one read (the payload) -- this is the *stateful* part:
     * other mocks would only know "two calls happened". */
    TEST_ASSERT_EQUAL_INT(1, s_sensor.base.write_calls);
    TEST_ASSERT_EQUAL_INT(1, s_sensor.base.read_calls);
    TEST_ASSERT_EQUAL_INT(SENSOR_REG_TEMP, s_sensor.base.last_reg);
}

static void test_wrong_bus_address_fails(void)
{
    setup_sensor();
    s_sensor.bus_address = 0x49;     /* sensor moved on the bus */
    int16_t t = 0;
    TEST_ASSERT_EQUAL_INT(-1, sensor_read_raw_temp(&t));
}

static void test_null_output_is_rejected(void)
{
    setup_sensor();
    TEST_ASSERT_EQUAL_INT(-1, sensor_read_raw_temp((int16_t *)0));
}

int main(void)
{
    ceedless_begin("sensor_driver");
    RUN_TEST(test_reads_temperature);
    RUN_TEST(test_wrong_bus_address_fails);
    RUN_TEST(test_null_output_is_rejected);
    return ceedless_end();
}
