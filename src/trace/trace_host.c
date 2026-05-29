/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/trace_config.h"
#ifdef CEEDLESS_TRACE_HOST

#include <stddef.h>
#include <stdio.h>

void ceedless_trace_backend_init (void)                    { /* nothing */ }
void ceedless_trace_backend_write(const void *d, size_t n) { fwrite(d, 1, n, stdout); }
void ceedless_trace_backend_flush(void)                    { fflush(stdout); }

#endif
typedef int ceedless_trace_host_tu_;
