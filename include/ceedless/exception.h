/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * CException-style Try/Catch/Throw using setjmp/longjmp.
 *
 *   #include "ceedless/exception.h"
 *
 *   volatile EXCEPTION_T e;
 *   Try {
 *       Throw(MY_ERROR);
 *   } Catch (e) {
 *       printf("caught %d\n", e);
 *   }
 *
 * Nested Try blocks supported up to CEEDLESS_EXC_STACK (default 8).
 */
#ifndef CEEDLESS_EXCEPTION_H
#define CEEDLESS_EXCEPTION_H

#include <setjmp.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int EXCEPTION_T;
#define CEXCEPTION_T EXCEPTION_T
#define CEXCEPTION_NONE  (0x5a5a5a5a)

#ifndef CEEDLESS_EXC_STACK
#define CEEDLESS_EXC_STACK 8
#endif

typedef struct {
    jmp_buf      frame;
    EXCEPTION_T  value;
    int          active;
} ceedless_exc_frame_t;

extern ceedless_exc_frame_t ceedless_exc_stack_[CEEDLESS_EXC_STACK];
extern int                ceedless_exc_top_;

void ceedless_exc_throw(EXCEPTION_T v);

#define Try                                                                  \
    {                                                                        \
        ceedless_exc_stack_[ceedless_exc_top_].value  = CEXCEPTION_NONE;         \
        ceedless_exc_stack_[ceedless_exc_top_].active = 1;                       \
        ceedless_exc_top_++;                                                   \
        if (setjmp(ceedless_exc_stack_[ceedless_exc_top_ - 1].frame) == 0)

#define Catch(e)                                                             \
        ceedless_exc_top_--;                                                   \
        (e) = ceedless_exc_stack_[ceedless_exc_top_].value;                      \
        ceedless_exc_stack_[ceedless_exc_top_].active = 0;                       \
    }                                                                        \
    if ((e) != CEXCEPTION_NONE)

#define Throw(v)   ceedless_exc_throw((v))

#ifdef __cplusplus
}
#endif
#endif /* CEEDLESS_EXCEPTION_H */
