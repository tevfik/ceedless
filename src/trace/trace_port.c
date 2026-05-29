/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/trace_port.h"
#include "ceedless/trace_config.h"

#include <stdarg.h>
#include <stdio.h>

void trace_init (void)                        { ceedless_trace_backend_init(); }
void trace_write(const void *d, size_t n)     { ceedless_trace_backend_write(d, n); }
void trace_flush(void)                        { ceedless_trace_backend_flush(); }

void trace_puts(const char *s)
{
    size_t n = 0;
    while (s && s[n]) n++;
    ceedless_trace_backend_write(s, n);
}

void trace_printf(const char *fmt, ...)
{
    char buf[160];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n <= 0) return;
    if ((size_t)n >= sizeof buf) n = (int)sizeof buf - 1;
    ceedless_trace_backend_write(buf, (size_t)n);
}
