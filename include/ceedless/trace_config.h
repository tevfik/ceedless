/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Compile-time selection of the trace backend.
 *
 * Define exactly ONE of these on the compiler command line:
 *   -DCEEDLESS_TRACE_HOST     (default; printf/stdout)
 *   -DCEEDLESS_TRACE_UART     (calls weak ceedless_uart_* hooks)
 *   -DCEEDLESS_TRACE_RTT      (calls weak ceedless_rtt_*  hooks)
 *   -DCEEDLESS_TRACE_ITM      (writes ARM ITM stimulus port 0, weakly)
 *   -DCEEDLESS_TRACE_BUFFER   (captures into an in-memory ring buffer)
 */
#ifndef CEEDLESS_TRACE_CONFIG_H
#define CEEDLESS_TRACE_CONFIG_H

#if !defined(CEEDLESS_TRACE_HOST)   && !defined(CEEDLESS_TRACE_UART) && \
    !defined(CEEDLESS_TRACE_RTT)    && !defined(CEEDLESS_TRACE_ITM)  && \
    !defined(CEEDLESS_TRACE_BUFFER)
#define CEEDLESS_TRACE_HOST 1
#endif

#endif /* CEEDLESS_TRACE_CONFIG_H */
