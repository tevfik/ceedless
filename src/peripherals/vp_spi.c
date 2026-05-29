/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/peripherals/vp_spi.h"
#include <string.h>

void vp_spi_init(vp_spi_t *s, const char *name, uint8_t *regs, size_t n)
{
    memset(s, 0, sizeof *s);
    vp_device_init(&s->base, name, regs, n);
}

void vp_spi_reset(vp_spi_t *s)
{
    s->tx_len = 0; s->rx_head = s->rx_tail = 0; s->cs_asserted = 0;
    vp_device_reset(&s->base);
}

void vp_spi_load_rx(vp_spi_t *s, const uint8_t *data, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        size_t next = (s->rx_tail + 1) % VP_SPI_BUFSZ;
        if (next == s->rx_head) break;
        s->rx_queue[s->rx_tail] = data[i];
        s->rx_tail = next;
    }
}

void vp_spi_cs_assert  (vp_spi_t *s) { s->cs_asserted = 1; }
void vp_spi_cs_deassert(vp_spi_t *s) { s->cs_asserted = 0; }

uint8_t vp_spi_xfer_byte(vp_spi_t *s, uint8_t mosi)
{
    if (s->tx_len < VP_SPI_BUFSZ) s->tx_log[s->tx_len++] = mosi;
    uint8_t miso = 0xFF;
    if (s->rx_head != s->rx_tail) {
        miso = s->rx_queue[s->rx_head];
        s->rx_head = (s->rx_head + 1) % VP_SPI_BUFSZ;
    }
    return miso;
}

int vp_spi_xfer(vp_spi_t *s, const uint8_t *mosi, uint8_t *miso, size_t n)
{
    for (size_t i = 0; i < n; i++) {
        uint8_t b = vp_spi_xfer_byte(s, mosi ? mosi[i] : 0xFF);
        if (miso) miso[i] = b;
    }
    return 0;
}
