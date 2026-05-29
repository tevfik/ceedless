/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/peripherals/vp_uart.h"
#include <string.h>

void vp_uart_init(vp_uart_t *u, const char *name, uint32_t baud)
{
    memset(u, 0, sizeof *u);
    u->name = name; u->baud = baud; u->open = 1;
}
void vp_uart_reset(vp_uart_t *u)
{
    uint32_t b = u->baud; const char *n = u->name;
    memset(u, 0, sizeof *u); u->baud = b; u->name = n; u->open = 1;
}

int vp_uart_putc(vp_uart_t *u, uint8_t c)
{
    if (!u->open) return -1;
    if (u->tx_len >= VP_UART_BUFSZ) { u->overrun_errors++; return -1; }
    u->tx_buf[u->tx_len++] = c; return 0;
}
int vp_uart_write(vp_uart_t *u, const uint8_t *b, size_t n)
{
    for (size_t i = 0; i < n; i++) if (vp_uart_putc(u, b[i]) != 0) return -1;
    return 0;
}
int vp_uart_getc(vp_uart_t *u, uint8_t *out)
{
    if (!u->open || u->rx_head == u->rx_tail) return -1;
    *out = u->rx_buf[u->rx_head];
    u->rx_head = (u->rx_head + 1) % VP_UART_BUFSZ;
    return 0;
}
int vp_uart_available(vp_uart_t *u)
{
    return (int)((u->rx_tail + VP_UART_BUFSZ - u->rx_head) % VP_UART_BUFSZ);
}
void vp_uart_inject(vp_uart_t *u, const uint8_t *b, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        size_t next = (u->rx_tail + 1) % VP_UART_BUFSZ;
        if (next == u->rx_head) { u->overrun_errors++; return; }
        u->rx_buf[u->rx_tail] = b[i];
        u->rx_tail = next;
    }
}
size_t vp_uart_tx_take(vp_uart_t *u, uint8_t *out, size_t cap)
{
    size_t n = u->tx_len < cap ? u->tx_len : cap;
    if (out && n) memcpy(out, u->tx_buf, n);
    /* shift remaining */
    if (n < u->tx_len) memmove(u->tx_buf, u->tx_buf + n, u->tx_len - n);
    u->tx_len -= n;
    return n;
}
