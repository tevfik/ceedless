/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/exception.h"
#include <stdlib.h>

ceedless_exc_frame_t ceedless_exc_stack_[CEEDLESS_EXC_STACK];
int                ceedless_exc_top_ = 0;

void ceedless_exc_throw(EXCEPTION_T v)
{
    if (ceedless_exc_top_ <= 0) {
        /* Uncaught — abort with a visible code. */
        abort();
    }
    ceedless_exc_frame_t *fr = &ceedless_exc_stack_[ceedless_exc_top_ - 1];
    fr->value = v;
    longjmp(fr->frame, 1);
}
