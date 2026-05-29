/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * UART backend. The user (BSP) provides the three weak hooks below.
 */
#include "ceedless/trace_config.h"
#ifdef CEEDLESS_TRACE_UART

#include <stddef.h>
#include <stdint.h>

#ifndef CEEDLESS_WEAK
#  if defined(__GNUC__)
#    define CEEDLESS_WEAK __attribute__((weak))
#  else
#    define CEEDLESS_WEAK
#  endif
#endif

CEEDLESS_WEAK void ceedless_uart_init (void)        { /* override in BSP */ }
CEEDLESS_WEAK void ceedless_uart_putc (uint8_t c)   { (void)c; }
CEEDLESS_WEAK void ceedless_uart_flush(void)        { /* override in BSP */ }

void ceedless_trace_backend_init (void) { ceedless_uart_init(); }
void ceedless_trace_backend_flush(void) { ceedless_uart_flush(); }

void ceedless_trace_backend_write(const void *d, size_t n)
{
    const uint8_t *p = (const uint8_t *)d;
    for (size_t i = 0; i < n; i++) ceedless_uart_putc(p[i]);
}

#endif
typedef int ceedless_trace_uart_tu_;
