/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Optional inspection API for the in-memory buffer backend.
 * Only meaningful when the binary is built with -DCEEDLESS_TRACE_BUFFER.
 */
#ifndef CEEDLESS_TRACE_BUFFER_H
#define CEEDLESS_TRACE_BUFFER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *ceedless_trace_buffer(void);
size_t      ceedless_trace_buffer_len(void);
void        ceedless_trace_buffer_clear(void);

#ifdef __cplusplus
}
#endif
#endif
