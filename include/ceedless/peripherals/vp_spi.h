/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Virtual SPI peripheral (full-duplex master view).
 *
 * MOSI bytes the driver under test "transmits" are appended to tx_log.
 * MISO bytes returned to the driver are drained from a pre-loaded rx_queue.
 * Optionally a vp_device_t base (register bank) backs the device so the
 * test can simulate register-mapped SPI slaves (e.g. accelerometers).
 */
#ifndef CEEDLESS_VP_SPI_H
#define CEEDLESS_VP_SPI_H

#include <stddef.h>
#include <stdint.h>
#include "ceedless/virtual_peripheral.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VP_SPI_BUFSZ
#define VP_SPI_BUFSZ 128
#endif

typedef struct {
    vp_device_t base;            /* optional register bank */
    uint8_t  tx_log[VP_SPI_BUFSZ];
    size_t   tx_len;
    uint8_t  rx_queue[VP_SPI_BUFSZ];
    size_t   rx_head, rx_tail;
    int      cs_asserted;
} vp_spi_t;

void vp_spi_init        (vp_spi_t *s, const char *name,
                         uint8_t *regs, size_t n);
void vp_spi_reset       (vp_spi_t *s);
void vp_spi_load_rx     (vp_spi_t *s, const uint8_t *data, size_t n);

void vp_spi_cs_assert   (vp_spi_t *s);
void vp_spi_cs_deassert (vp_spi_t *s);

/* Full-duplex byte exchange. Returns the MISO byte. */
uint8_t vp_spi_xfer_byte (vp_spi_t *s, uint8_t mosi);
int     vp_spi_xfer      (vp_spi_t *s,
                          const uint8_t *mosi, uint8_t *miso, size_t n);

#ifdef __cplusplus
}
#endif
#endif
