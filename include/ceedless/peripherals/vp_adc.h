/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Virtual ADC. Each channel returns either a fixed value or, if a
 * generator callback is set, the value the callback produces (useful for
 * sweeps, noise, ramps). Resolution is configurable (default 12-bit).
 */
#ifndef CEEDLESS_VP_ADC_H
#define CEEDLESS_VP_ADC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef VP_ADC_CHANNELS
#define VP_ADC_CHANNELS 16
#endif

typedef uint32_t (*vp_adc_gen_fn)(uint8_t ch, uint32_t sample_index, void *user);

typedef struct {
    const char     *name;
    uint8_t         resolution_bits;
    uint32_t        fixed[VP_ADC_CHANNELS];
    vp_adc_gen_fn   gen [VP_ADC_CHANNELS];
    void           *user[VP_ADC_CHANNELS];
    uint32_t        sample_idx[VP_ADC_CHANNELS];
    uint32_t        conversions[VP_ADC_CHANNELS];
} vp_adc_t;

void     vp_adc_init        (vp_adc_t *a, const char *name, uint8_t bits);
void     vp_adc_reset       (vp_adc_t *a);
void     vp_adc_set_fixed   (vp_adc_t *a, uint8_t ch, uint32_t code);
void     vp_adc_set_gen     (vp_adc_t *a, uint8_t ch, vp_adc_gen_fn fn, void *user);
uint32_t vp_adc_read        (vp_adc_t *a, uint8_t ch);

#ifdef __cplusplus
}
#endif
#endif
