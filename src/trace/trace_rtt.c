/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * SEGGER RTT backend. The actual SEGGER_RTT_Write call lives behind a weak
 * hook so this file links cleanly even without the SEGGER sources present.
 */
#include "ceedless/trace_config.h"
#ifdef CEEDLESS_TRACE_RTT

#include <stddef.h>
#include <stdint.h>

#ifndef CEEDLESS_WEAK
#  if defined(__GNUC__)
#    define CEEDLESS_WEAK __attribute__((weak))
#  else
#    define CEEDLESS_WEAK
#  endif
#endif

/* In a real integration, implement this as:
 *     return SEGGER_RTT_Write(0, d, n);
 */
CEEDLESS_WEAK void ceedless_rtt_init (void)                    { /* override */ }
CEEDLESS_WEAK void ceedless_rtt_write(const void *d, size_t n) { (void)d; (void)n; }
CEEDLESS_WEAK void ceedless_rtt_flush(void)                    { /* override */ }

void ceedless_trace_backend_init (void)                    { ceedless_rtt_init(); }
void ceedless_trace_backend_write(const void *d, size_t n) { ceedless_rtt_write(d, n); }
void ceedless_trace_backend_flush(void)                    { ceedless_rtt_flush(); }

#endif
typedef int ceedless_trace_rtt_tu_;
