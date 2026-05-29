/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "sensor_driver.h"
#include "i2c_hal.h"

int sensor_read_raw_temp(int16_t *out)
{
    if (!out) return -1;
    uint8_t reg = SENSOR_REG_TEMP;
    if (hal_i2c_write(SENSOR_I2C_ADDR, &reg, 1) != 0) return -1;
    uint8_t b[2] = { 0, 0 };
    if (hal_i2c_read(SENSOR_I2C_ADDR, b, 2) != 0) return -1;
    *out = (int16_t)(((uint16_t)b[0] << 8) | (uint16_t)b[1]);
    return 0;
}
