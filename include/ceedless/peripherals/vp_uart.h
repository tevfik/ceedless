/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Virtual UART with simple TX/RX byte rings + parity/baud bookkeeping.
 */
#ifndef CEEDLESS_VP_UART_H
#define CEEDLESS_VP_UART_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VP_UART_BUFSZ
#define VP_UART_BUFSZ 256
#endif

typedef struct {
    const char *name;
    uint32_t baud;
    uint8_t  tx_buf[VP_UART_BUFSZ]; size_t tx_len;
    uint8_t  rx_buf[VP_UART_BUFSZ]; size_t rx_head, rx_tail;
    int      open;
    uint32_t framing_errors;
    uint32_t overrun_errors;
} vp_uart_t;

void   vp_uart_init   (vp_uart_t *u, const char *name, uint32_t baud);
void   vp_uart_reset  (vp_uart_t *u);

/* Driver-under-test side */
int    vp_uart_putc   (vp_uart_t *u, uint8_t c);
int    vp_uart_write  (vp_uart_t *u, const uint8_t *b, size_t n);
int    vp_uart_getc   (vp_uart_t *u, uint8_t *out);
int    vp_uart_available(vp_uart_t *u);

/* Test-side helpers (act as the other end of the wire) */
void   vp_uart_inject  (vp_uart_t *u, const uint8_t *b, size_t n);
size_t vp_uart_tx_take (vp_uart_t *u, uint8_t *out, size_t cap);

#ifdef __cplusplus
}
#endif
#endif
