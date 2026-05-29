/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * In-memory backend used for self-testing the framework. Also tees output
 * to stdout so a developer can watch the run live.
 */
#include "ceedless/trace_config.h"
#ifdef CEEDLESS_TRACE_BUFFER

#include "ceedless/trace_buffer.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#ifndef CEEDLESS_TRACE_BUFFER_SIZE
#define CEEDLESS_TRACE_BUFFER_SIZE 4096
#endif

static char   s_buf[CEEDLESS_TRACE_BUFFER_SIZE];
static size_t s_len;

void ceedless_trace_backend_init (void) { s_len = 0; s_buf[0] = '\0'; }
void ceedless_trace_backend_flush(void) { fflush(stdout); }

void ceedless_trace_backend_write(const void *d, size_t n)
{
    /* live tee */
    fwrite(d, 1, n, stdout);

    if (s_len + n >= sizeof s_buf) {
        if (s_len >= sizeof s_buf - 1) return;
        n = sizeof s_buf - 1 - s_len;
    }
    memcpy(s_buf + s_len, d, n);
    s_len += n;
    s_buf[s_len] = '\0';
}

const char *ceedless_trace_buffer    (void) { return s_buf; }
size_t      ceedless_trace_buffer_len(void) { return s_len; }
void        ceedless_trace_buffer_clear(void) { s_len = 0; s_buf[0] = '\0'; }

#endif
typedef int ceedless_trace_buffer_tu_;
