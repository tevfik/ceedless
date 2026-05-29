/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/virtual_peripheral.h"
#include <string.h>

void vp_device_init(vp_device_t *d, const char *name, uint8_t *regs, size_t n)
{
    d->name      = name;
    d->regs      = regs;
    d->reg_count = n;
    d->user      = 0;
    d->on_read   = 0;
    d->on_write  = 0;
    vp_device_reset(d);
}

void vp_device_reset(vp_device_t *d)
{
    d->read_calls  = 0;
    d->write_calls = 0;
    d->last_reg    = 0;
    d->last_status = 0;
    if (d->regs && d->reg_count) memset(d->regs, 0, d->reg_count);
}

int vp_device_read(vp_device_t *d, uint32_t reg, uint8_t *buf, size_t len)
{
    d->read_calls++;
    d->last_reg = reg;
    if (!buf || reg + len > d->reg_count) { d->last_status = -1; return -1; }
    memcpy(buf, d->regs + reg, len);
    if (d->on_read) { d->last_status = d->on_read(d, reg, buf, len); return d->last_status; }
    d->last_status = 0;
    return 0;
}

int vp_device_write(vp_device_t *d, uint32_t reg, const uint8_t *buf, size_t len)
{
    d->write_calls++;
    d->last_reg = reg;
    if (!buf || reg + len > d->reg_count) { d->last_status = -1; return -1; }
    memcpy(d->regs + reg, buf, len);
    if (d->on_write) {
        d->last_status = d->on_write(d, reg, (uint8_t *)(void *)buf, len);
        return d->last_status;
    }
    d->last_status = 0;
    return 0;
}

/* ---------------- I2C specialization ---------------- */

void vp_i2c_init(vp_i2c_t *d, const char *name, uint8_t addr,
                 uint8_t *regs, size_t n)
{
    vp_device_init(&d->base, name, regs, n);
    d->bus_address = addr;
    d->cursor      = 0;
    d->ack         = 1;
}

int vp_i2c_start(vp_i2c_t *d, uint8_t addr)
{
    return (d->ack && addr == d->bus_address) ? 0 : -1;
}

int vp_i2c_master_write(vp_i2c_t *d, const uint8_t *bytes, size_t n)
{
    if (n == 0 || !bytes) return 0;
    d->cursor = bytes[0];                       /* first byte: register ptr */
    if (n > 1) {
        return vp_device_write(&d->base, d->cursor, bytes + 1, n - 1);
    }
    /* pointer-only write still counts as a bus transaction */
    d->base.write_calls++;
    d->base.last_reg = d->cursor;
    d->base.last_status = 0;
    return 0;
}

int vp_i2c_master_read(vp_i2c_t *d, uint8_t *bytes, size_t n)
{
    int rc = vp_device_read(&d->base, d->cursor, bytes, n);
    if (rc == 0) d->cursor += (uint32_t)n;
    return rc;
}
