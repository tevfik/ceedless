/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * CMock-style stateful function mocks — no Ruby, no code generation.
 *
 * Each mocked function gets a mock_t (queue of expected call records). A
 * record stores: optional expected argument fingerprint, return value, and a
 * "throw" code that integrates with ceedless/exception.h. Records also support
 * ReturnThruPtr-style writeback to one user-chosen pointer arg, and a
 * StubWithCallback escape hatch.
 *
 * The user writes one tiny stub per mocked function, which calls
 * mock_invoke(). Helper macros generate Expect / Ignore / Verify wrappers.
 *
 * Example (replacing int hal_uart_tx(const uint8_t *b, size_t n)):
 *
 *   MOCK_DEFINE(hal_uart_tx);
 *   int hal_uart_tx(const uint8_t *b, size_t n) {
 *       struct { const uint8_t *b; size_t n; } args = { b, n };
 *       int rv = 0;
 *       mock_invoke(&hal_uart_tx_mock, &args, sizeof args, &rv, sizeof rv);
 *       return rv;
 *   }
 *
 *   // in a test:
 *   struct { const uint8_t *b; size_t n; } a = { buf, 4 };
 *   MOCK_EXPECT_AND_RETURN(hal_uart_tx, &a, sizeof a, 0);
 *   ...
 *   MOCK_VERIFY(hal_uart_tx);
 *
 * NOTE on struct padding: argument matching is a byte-wise memcmp. ALWAYS
 *   memset() your args struct to 0 before populating its fields, otherwise
 *   uninitialised padding bytes will cause false mismatches.
 *
 * Everything is statically allocated. CEEDLESS_MOCK_MAX_CALLS controls capacity.
 */
#ifndef CEEDLESS_MOCK_H
#define CEEDLESS_MOCK_H

#include <stddef.h>
#include <stdint.h>
#include "ceedless/exception.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CEEDLESS_MOCK_MAX_CALLS
#define CEEDLESS_MOCK_MAX_CALLS 16
#endif
#ifndef CEEDLESS_MOCK_ARG_BYTES
#define CEEDLESS_MOCK_ARG_BYTES 64
#endif
#ifndef CEEDLESS_MOCK_RET_BYTES
#define CEEDLESS_MOCK_RET_BYTES 16
#endif

struct mock_s;
typedef void (*mock_callback_t)(struct mock_s *m, const void *args, size_t alen,
                                void *ret, size_t rlen, int call_index);

typedef struct {
    int    has_args;
    size_t args_len;
    uint8_t args[CEEDLESS_MOCK_ARG_BYTES];

    int    has_ret;
    size_t ret_len;
    uint8_t ret[CEEDLESS_MOCK_RET_BYTES];

    int    ignore_args;       /* ExpectAnyArgs */
    int    throw_value;       /* if non-zero, Throw() this on call */
    int    throw_set;

    /* ReturnThruPtr: write `rt_data` (rt_len bytes) into the pointer at
     * args+rt_arg_offset (treated as void*). */
    int    rt_set;
    size_t rt_arg_offset;
    size_t rt_len;
    uint8_t rt_data[CEEDLESS_MOCK_ARG_BYTES];
} mock_record_t;

typedef struct mock_s {
    const char     *name;
    mock_record_t   records[CEEDLESS_MOCK_MAX_CALLS];
    int             head;          /* next record to use when calling */
    int             count;         /* queued records (head..count-1)  */
    int             ignore;        /* ignore all calls                */
    int             ignore_has_ret;
    size_t          ignore_ret_len;
    uint8_t         ignore_ret[CEEDLESS_MOCK_RET_BYTES];
    mock_callback_t stub_cb;       /* StubWithCallback                */
    int             calls;         /* total observed calls            */
} mock_t;

/* --- Setup --------------------------------------------------------------*/
void mock_reset(mock_t *m);
void mock_set_name(mock_t *m, const char *name);

/* --- Expect family ------------------------------------------------------*/
void mock_expect            (mock_t *m, const void *args, size_t alen);
void mock_expect_and_return (mock_t *m, const void *args, size_t alen,
                             const void *ret, size_t rlen);
void mock_expect_any_args            (mock_t *m);
void mock_expect_any_args_and_return (mock_t *m, const void *ret, size_t rlen);
void mock_expect_and_throw           (mock_t *m, const void *args, size_t alen, int code);
void mock_return_thru_ptr            (mock_t *m, size_t arg_offset,
                                      const void *data, size_t dlen);

/* --- Ignore / Stub ------------------------------------------------------*/
void mock_ignore                (mock_t *m);
void mock_ignore_and_return     (mock_t *m, const void *ret, size_t rlen);
void mock_stop_ignore           (mock_t *m);
void mock_stub_with_callback    (mock_t *m, mock_callback_t cb);

/* --- Call site ---------------------------------------------------------*/
void mock_invoke(mock_t *m, const void *args, size_t alen,
                 void *ret, size_t rlen);

/* --- Verification ------------------------------------------------------*/
void mock_verify  (mock_t *m);   /* fails the current test if mismatch  */
int  mock_pending (mock_t *m);   /* unconsumed records */
int  mock_calls   (mock_t *m);

/* --- Declarative helpers ----------------------------------------------- */
#define MOCK_DEFINE(_n)           mock_t _n##_mock = { .name = #_n }
#define MOCK_DECLARE(_n)          extern mock_t _n##_mock
#define MOCK_RESET(_n)            mock_reset(&_n##_mock)
#define MOCK_EXPECT(_n,a,al)            mock_expect(&_n##_mock,(a),(al))
#define MOCK_EXPECT_AND_RETURN(_n,a,al,r,rl) \
                                  mock_expect_and_return(&_n##_mock,(a),(al),(r),(rl))
#define MOCK_EXPECT_ANY_ARGS(_n)        mock_expect_any_args(&_n##_mock)
#define MOCK_EXPECT_ANY_ARGS_AND_RETURN(_n,r,rl) \
                                  mock_expect_any_args_and_return(&_n##_mock,(r),(rl))
#define MOCK_EXPECT_AND_THROW(_n,a,al,code) \
                                  mock_expect_and_throw(&_n##_mock,(a),(al),(code))
#define MOCK_RETURN_THRU_PTR(_n,off,d,dl) \
                                  mock_return_thru_ptr(&_n##_mock,(off),(d),(dl))
#define MOCK_IGNORE(_n)                 mock_ignore(&_n##_mock)
#define MOCK_IGNORE_AND_RETURN(_n,r,rl) mock_ignore_and_return(&_n##_mock,(r),(rl))
#define MOCK_STOP_IGNORE(_n)            mock_stop_ignore(&_n##_mock)
#define MOCK_STUB(_n,cb)                mock_stub_with_callback(&_n##_mock,(cb))
#define MOCK_VERIFY(_n)                 mock_verify(&_n##_mock)
#define MOCK_CALL_COUNT(_n)             mock_calls(&_n##_mock)

#ifdef __cplusplus
}
#endif
#endif /* CEEDLESS_MOCK_H */
