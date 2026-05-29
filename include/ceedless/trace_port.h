/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Front-end Trace API. The implementation dispatches to one selected backend
 * at compile time (see trace_config.h). Everything else in the framework is
 * platform-agnostic and only depends on these symbols.
 */
#ifndef CEEDLESS_TRACE_PORT_H
#define CEEDLESS_TRACE_PORT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Initialise the active trace backend. Safe to call multiple times. */
void trace_init(void);

/** Write `len` raw bytes to the trace channel. */
void trace_write(const void *data, size_t len);

/** Write a NUL-terminated string. */
void trace_puts(const char *s);

/** printf-style helper. Output is bounded by a small internal buffer. */
void trace_printf(const char *fmt, ...);

/** Block until any buffered output has been transmitted. */
void trace_flush(void);

/* --- Internal contract every backend must implement. -------------------- */
void ceedless_trace_backend_init (void);
void ceedless_trace_backend_write(const void *data, size_t len);
void ceedless_trace_backend_flush(void);

#ifdef __cplusplus
}
#endif
#endif /* CEEDLESS_TRACE_PORT_H */
