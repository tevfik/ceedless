/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Stateful virtual peripheral primitive plus an I2C specialization.
 *
 * Unlike call-counting mocks, a virtual peripheral owns a register bank and
 * exposes optional `on_read`/`on_write` callbacks so the test author can
 * model real device behaviour (auto-increment, side-effects, NACKs, etc.).
 *
 * Memory footprint per device: ~32 bytes + the register array you supply.
 * Nothing is allocated dynamically.
 */
#ifndef CEEDLESS_VIRTUAL_PERIPHERAL_H
#define CEEDLESS_VIRTUAL_PERIPHERAL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct vp_device;

/** I/O hook signature. Return 0 on success, negative on simulated error. */
typedef int (*vp_io_fn)(struct vp_device *dev, uint32_t reg,
                        uint8_t *data, size_t len);

typedef struct vp_device {
    const char *name;
    uint8_t    *regs;
    size_t      reg_count;
    void       *user;        /* opaque user context */
    vp_io_fn    on_read;     /* called AFTER bulk read  (may rewrite data)  */
    vp_io_fn    on_write;    /* called AFTER bulk write (may reject)        */
    /* observable history */
    uint32_t    read_calls;
    uint32_t    write_calls;
    uint32_t    last_reg;
    int         last_status;
} vp_device_t;

void vp_device_init (vp_device_t *d, const char *name,
                     uint8_t *regs, size_t n);
void vp_device_reset(vp_device_t *d);
int  vp_device_read (vp_device_t *d, uint32_t reg,
                     uint8_t *buf, size_t len);
int  vp_device_write(vp_device_t *d, uint32_t reg,
                     const uint8_t *buf, size_t len);

/* ----------------------------------------------------------------------- */
/* I2C specialization (master view).                                       */
/* The first byte of every master_write is interpreted as a register       */
/* pointer (EEPROM-style 8-bit addressing). Subsequent bytes are payload.  */
/* master_read pulls bytes starting at the current cursor and advances it. */
/* ----------------------------------------------------------------------- */
typedef struct {
    vp_device_t base;
    uint8_t  bus_address;    /* 7-bit slave address */
    uint32_t cursor;         /* internal register pointer */
    int      ack;            /* 1 = device acks address, 0 = NACK */
} vp_i2c_t;

void vp_i2c_init        (vp_i2c_t *d, const char *name, uint8_t addr,
                         uint8_t *regs, size_t n);
int  vp_i2c_start       (vp_i2c_t *d, uint8_t addr);
int  vp_i2c_master_write(vp_i2c_t *d, const uint8_t *bytes, size_t n);
int  vp_i2c_master_read (vp_i2c_t *d, uint8_t *bytes,       size_t n);

#ifdef __cplusplus
}
#endif
#endif /* CEEDLESS_VIRTUAL_PERIPHERAL_H */
