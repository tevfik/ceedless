/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 *
 * Semihosting trace backend for ARM Cortex-M when running under QEMU
 * (`-semihosting`) or a debug probe with semihosting enabled. Compile
 * with -DCEEDLESS_TRACE_SEMIHOST.
 *
 * Uses ARMv7-M `bkpt #0xAB` with R0=SYS_WRITE0 (0x04) to print a NUL-
 * terminated string, and SYS_EXIT (0x18) for shutdown.
 */
#include "ceedless/trace_config.h"
#ifdef CEEDLESS_TRACE_SEMIHOST

#include <stddef.h>

#if !defined(__arm__) && !defined(__aarch64__)
#  error "CEEDLESS_TRACE_SEMIHOST requires an ARM target"
#endif

static inline int semihost_call(int op, void *arg)
{
    register int r0 __asm__("r0") = op;
    register void *r1 __asm__("r1") = arg;
    __asm__ volatile ("bkpt 0xAB" : "+r"(r0) : "r"(r1) : "memory");
    return r0;
}

/* SYS_WRITE0 takes a NUL-terminated string; chunk our writes into a small
 * stack buffer so non-NUL-terminated payloads are still printable. */
void ceedless_trace_backend_init (void) { /* nothing */ }

void ceedless_trace_backend_write(const void *d, size_t n)
{
    const char *p = (const char *)d;
    while (n) {
        char buf[65];
        size_t k = n > sizeof buf - 1 ? sizeof buf - 1 : n;
        for (size_t i = 0; i < k; i++) buf[i] = p[i];
        buf[k] = '\0';
        semihost_call(0x04, buf);
        p += k; n -= k;
    }
}

void ceedless_trace_backend_flush(void) { /* immediate */ }

/* Optional: clean QEMU exit. Call from main() return path if desired. */
void ceedless_semihost_exit(int code)
{
    /* SYS_EXIT_EXTENDED (0x20) with {reason, code}; falls back to SYS_EXIT */
    unsigned int args[2] = { 0x20026 /*ADP_Stopped_ApplicationExit*/, (unsigned)code };
    semihost_call(0x20, args);
    for (;;) { __asm__ volatile ("bkpt 0xAB"); }
}

#endif /* CEEDLESS_TRACE_SEMIHOST */
typedef int ceedless_trace_semihost_tu_;
