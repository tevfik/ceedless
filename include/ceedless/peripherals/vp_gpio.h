/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Virtual GPIO bank — 32 pins per instance, with mode/level tracking,
 * pull resistor simulation, and an edge-event log for interrupts.
 */
#ifndef CEEDLESS_VP_GPIO_H
#define CEEDLESS_VP_GPIO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum { VP_GPIO_IN = 0, VP_GPIO_OUT = 1, VP_GPIO_AF = 2 } vp_gpio_mode_t;
typedef enum { VP_PULL_NONE = 0, VP_PULL_UP = 1, VP_PULL_DOWN = 2 } vp_gpio_pull_t;

#ifndef VP_GPIO_EVENT_LOG
#define VP_GPIO_EVENT_LOG 32
#endif

typedef struct {
    uint8_t  pin;
    uint8_t  level;
} vp_gpio_event_t;

typedef struct {
    const char     *name;
    uint32_t        mode_mask;     /* bit=1 -> output */
    uint32_t        levels;
    uint32_t        pulls_up;
    uint32_t        pulls_down;
    vp_gpio_event_t events[VP_GPIO_EVENT_LOG];
    size_t          event_count;
} vp_gpio_t;

void vp_gpio_init      (vp_gpio_t *g, const char *name);
void vp_gpio_reset     (vp_gpio_t *g);

/* Driver-under-test API */
void vp_gpio_set_mode  (vp_gpio_t *g, uint8_t pin, vp_gpio_mode_t mode);
void vp_gpio_set_pull  (vp_gpio_t *g, uint8_t pin, vp_gpio_pull_t pull);
void vp_gpio_write     (vp_gpio_t *g, uint8_t pin, int level);
int  vp_gpio_read      (vp_gpio_t *g, uint8_t pin);

/* Test-side helpers */
void vp_gpio_inject    (vp_gpio_t *g, uint8_t pin, int level);  /* drive a line from outside */
int  vp_gpio_mode      (vp_gpio_t *g, uint8_t pin);

#ifdef __cplusplus
}
#endif
#endif
