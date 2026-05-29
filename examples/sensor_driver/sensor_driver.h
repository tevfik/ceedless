/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#ifndef SENSOR_DRIVER_H
#define SENSOR_DRIVER_H
#include <stdint.h>

#define SENSOR_I2C_ADDR  0x48u
#define SENSOR_REG_TEMP  0x00u

/** Reads the raw 16-bit temperature register. Returns 0 on success. */
int sensor_read_raw_temp(int16_t *out);

#endif
