/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Umbrella header — pulls in everything you typically want in a test file.
 */
#ifndef CEEDLESS_H
#define CEEDLESS_H
#include "ceedless/trace_port.h"
#include "ceedless/hybrid_runner.h"
#include "ceedless/virtual_peripheral.h"
#include "ceedless/peripherals/vp_spi.h"
#include "ceedless/peripherals/vp_uart.h"
#include "ceedless/peripherals/vp_gpio.h"
#include "ceedless/peripherals/vp_adc.h"
#include "ceedless/exception.h"
#include "ceedless/mock.h"

/* Ceedling-style annotation: declare extra C sources the CLI should
 * compile into this test binary. The macro itself expands to nothing —
 * the bash CLI scans for the literal pattern in the source file. */
#define TEST_SOURCE_FILE(path)

#endif
