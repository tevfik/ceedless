/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/peripherals/vp_gpio.h"
#include <string.h>

void vp_gpio_init(vp_gpio_t *g, const char *name)
{
    memset(g, 0, sizeof *g); g->name = name;
}
void vp_gpio_reset(vp_gpio_t *g)
{
    const char *n = g->name; memset(g, 0, sizeof *g); g->name = n;
}

static uint32_t bit(uint8_t pin) { return 1u << (pin & 31); }

void vp_gpio_set_mode(vp_gpio_t *g, uint8_t pin, vp_gpio_mode_t mode)
{
    if (mode == VP_GPIO_OUT) g->mode_mask |=  bit(pin);
    else                     g->mode_mask &= ~bit(pin);
}
void vp_gpio_set_pull(vp_gpio_t *g, uint8_t pin, vp_gpio_pull_t pull)
{
    g->pulls_up   &= ~bit(pin);
    g->pulls_down &= ~bit(pin);
    if (pull == VP_PULL_UP)   g->pulls_up   |= bit(pin);
    if (pull == VP_PULL_DOWN) g->pulls_down |= bit(pin);
}
void vp_gpio_write(vp_gpio_t *g, uint8_t pin, int level)
{
    if (level) g->levels |=  bit(pin);
    else       g->levels &= ~bit(pin);
    if (g->event_count < VP_GPIO_EVENT_LOG) {
        g->events[g->event_count++] = (vp_gpio_event_t){ pin, (uint8_t)(!!level) };
    }
}
int vp_gpio_read(vp_gpio_t *g, uint8_t pin)
{
    /* output? return our last written value. input? apply pull if no driver. */
    if (g->mode_mask & bit(pin)) return (g->levels & bit(pin)) ? 1 : 0;
    if (g->pulls_up   & bit(pin)) return 1;
    if (g->pulls_down & bit(pin)) return 0;
    return (g->levels & bit(pin)) ? 1 : 0;
}
void vp_gpio_inject(vp_gpio_t *g, uint8_t pin, int level)
{
    if (level) g->levels |=  bit(pin);
    else       g->levels &= ~bit(pin);
}
int vp_gpio_mode(vp_gpio_t *g, uint8_t pin)
{
    return (g->mode_mask & bit(pin)) ? VP_GPIO_OUT : VP_GPIO_IN;
}
