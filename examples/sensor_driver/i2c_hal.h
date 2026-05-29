/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Thin I2C HAL. The production build links it against the MCU's HAL; the
 * test build provides a software implementation that forwards to a
 * vp_i2c_t virtual peripheral.
 */
#ifndef I2C_HAL_H
#define I2C_HAL_H
#include <stddef.h>
#include <stdint.h>
int hal_i2c_write(uint8_t addr, const uint8_t *data, size_t n);
int hal_i2c_read (uint8_t addr,       uint8_t *data, size_t n);
#endif
