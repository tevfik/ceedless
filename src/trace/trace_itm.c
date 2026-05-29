/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * ARM Cortex-M ITM (SWO) backend. Writes one byte at a time to stimulus
 * port 0. Zero-overhead in the sense that the CPU stalls only briefly
 * waiting for the port FIFO.
 */
#include "ceedless/trace_config.h"
#ifdef CEEDLESS_TRACE_ITM

#include <stddef.h>
#include <stdint.h>

#ifndef CEEDLESS_WEAK
#  if defined(__GNUC__)
#    define CEEDLESS_WEAK __attribute__((weak))
#  else
#    define CEEDLESS_WEAK
#  endif
#endif

/* Register addresses are the same on every Cortex-M with ITM.
 * Provided as weak so a host build still links.
 */
CEEDLESS_WEAK void ceedless_itm_putc(uint8_t c)
{
#if defined(__arm__) && defined(CEEDLESS_ITM_DIRECT)
    volatile uint32_t * const ITM_PORT0 = (volatile uint32_t *)0xE0000000UL;
    volatile uint32_t * const ITM_TER   = (volatile uint32_t *)0xE0000E00UL;
    volatile uint32_t * const ITM_TCR   = (volatile uint32_t *)0xE0000E80UL;
    if (((*ITM_TCR) & 1u) && ((*ITM_TER) & 1u)) {
        while (ITM_PORT0[0] == 0) { /* wait for FIFO */ }
        *(volatile uint8_t *)ITM_PORT0 = c;
    }
#else
    (void)c;
#endif
}

void ceedless_trace_backend_init (void)                    { /* nothing */ }
void ceedless_trace_backend_flush(void)                    { /* nothing */ }
void ceedless_trace_backend_write(const void *d, size_t n)
{
    const uint8_t *p = (const uint8_t *)d;
    for (size_t i = 0; i < n; i++) ceedless_itm_putc(p[i]);
}

#endif
typedef int ceedless_trace_itm_tu_;
