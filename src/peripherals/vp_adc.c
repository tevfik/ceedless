/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/peripherals/vp_adc.h"
#include <string.h>

void vp_adc_init(vp_adc_t *a, const char *name, uint8_t bits)
{
    memset(a, 0, sizeof *a);
    a->name = name; a->resolution_bits = bits ? bits : 12;
}
void vp_adc_reset(vp_adc_t *a)
{
    const char *n = a->name; uint8_t b = a->resolution_bits;
    memset(a, 0, sizeof *a); a->name = n; a->resolution_bits = b;
}
void vp_adc_set_fixed(vp_adc_t *a, uint8_t ch, uint32_t code)
{
    if (ch >= VP_ADC_CHANNELS) return;
    a->fixed[ch] = code; a->gen[ch] = NULL;
}
void vp_adc_set_gen(vp_adc_t *a, uint8_t ch, vp_adc_gen_fn fn, void *user)
{
    if (ch >= VP_ADC_CHANNELS) return;
    a->gen[ch] = fn; a->user[ch] = user;
}
uint32_t vp_adc_read(vp_adc_t *a, uint8_t ch)
{
    if (ch >= VP_ADC_CHANNELS) return 0;
    uint32_t mask = a->resolution_bits >= 32 ? 0xFFFFFFFFu
                                             : (1u << a->resolution_bits) - 1u;
    uint32_t v = a->gen[ch] ? a->gen[ch](ch, a->sample_idx[ch], a->user[ch])
                            : a->fixed[ch];
    a->sample_idx[ch]++;
    a->conversions[ch]++;
    return v & mask;
}
