/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/mock.h"
#include "ceedless/hybrid_runner.h"
#include "ceedless/exception.h"

#include <string.h>
#include <stdio.h>

void mock_reset(mock_t *m)
{
    const char *n = m->name;
    memset(m, 0, sizeof *m);
    m->name = n;
}
void mock_set_name(mock_t *m, const char *name) { m->name = name; }

static mock_record_t *next_slot(mock_t *m)
{
    if (m->count >= CEEDLESS_MOCK_MAX_CALLS) {
        char buf[96];
        snprintf(buf, sizeof buf, "%s: too many queued expectations (max %d)",
                 m->name ? m->name : "(mock)", CEEDLESS_MOCK_MAX_CALLS);
        ceedless_fail_msg_(m->name ? m->name : "mock", __FILE__, __LINE__, buf);
        return &m->records[0];
    }
    return &m->records[m->count++];
}

static void copy_bounded(uint8_t *dst, size_t cap, const void *src, size_t n)
{
    if (n > cap) n = cap;
    memcpy(dst, src, n);
}

void mock_expect(mock_t *m, const void *args, size_t alen)
{
    mock_record_t *r = next_slot(m);
    memset(r, 0, sizeof *r);
    r->has_args = 1;
    r->args_len = alen;
    copy_bounded(r->args, sizeof r->args, args, alen);
}
void mock_expect_and_return(mock_t *m, const void *args, size_t alen,
                            const void *ret, size_t rlen)
{
    mock_record_t *r = next_slot(m);
    memset(r, 0, sizeof *r);
    r->has_args = 1; r->args_len = alen;
    copy_bounded(r->args, sizeof r->args, args, alen);
    r->has_ret = 1; r->ret_len = rlen;
    copy_bounded(r->ret, sizeof r->ret, ret, rlen);
}
void mock_expect_any_args(mock_t *m)
{
    mock_record_t *r = next_slot(m);
    memset(r, 0, sizeof *r);
    r->ignore_args = 1;
}
void mock_expect_any_args_and_return(mock_t *m, const void *ret, size_t rlen)
{
    mock_record_t *r = next_slot(m);
    memset(r, 0, sizeof *r);
    r->ignore_args = 1;
    r->has_ret = 1; r->ret_len = rlen;
    copy_bounded(r->ret, sizeof r->ret, ret, rlen);
}
void mock_expect_and_throw(mock_t *m, const void *args, size_t alen, int code)
{
    mock_record_t *r = next_slot(m);
    memset(r, 0, sizeof *r);
    if (args && alen) {
        r->has_args = 1; r->args_len = alen;
        copy_bounded(r->args, sizeof r->args, args, alen);
    } else {
        r->ignore_args = 1;
    }
    r->throw_set = 1; r->throw_value = code;
}
void mock_return_thru_ptr(mock_t *m, size_t arg_offset, const void *data, size_t dlen)
{
    if (m->count == 0) { mock_expect_any_args(m); }
    mock_record_t *r = &m->records[m->count - 1];
    r->rt_set = 1; r->rt_arg_offset = arg_offset; r->rt_len = dlen;
    copy_bounded(r->rt_data, sizeof r->rt_data, data, dlen);
}

void mock_ignore(mock_t *m) { m->ignore = 1; m->ignore_has_ret = 0; }
void mock_ignore_and_return(mock_t *m, const void *ret, size_t rlen)
{
    m->ignore = 1; m->ignore_has_ret = 1;
    m->ignore_ret_len = rlen;
    copy_bounded(m->ignore_ret, sizeof m->ignore_ret, ret, rlen);
}
void mock_stop_ignore(mock_t *m) { m->ignore = 0; m->ignore_has_ret = 0; }
void mock_stub_with_callback(mock_t *m, mock_callback_t cb) { m->stub_cb = cb; }

static void fmt_bytes(char *dst, size_t cap, const uint8_t *p, size_t n)
{
    size_t o = 0;
    for (size_t i = 0; i < n && o + 4 < cap; i++) {
        int w = snprintf(dst + o, cap - o, "%02X ", p[i]);
        if (w < 0) break;
        o += (size_t)w;
    }
    if (o < cap) dst[o] = '\0';
}

void mock_invoke(mock_t *m, const void *args, size_t alen,
                 void *ret, size_t rlen)
{
    m->calls++;

    if (m->stub_cb) {
        m->stub_cb(m, args, alen, ret, rlen, m->calls - 1);
        return;
    }

    if (m->ignore) {
        if (m->ignore_has_ret && ret && rlen) {
            size_t n = rlen < m->ignore_ret_len ? rlen : m->ignore_ret_len;
            memcpy(ret, m->ignore_ret, n);
        } else if (ret && rlen) {
            memset(ret, 0, rlen);
        }
        return;
    }

    if (m->head >= m->count) {
        char buf[128];
        snprintf(buf, sizeof buf, "%s: unexpected call #%d (no more expectations)",
                 m->name ? m->name : "(mock)", m->calls);
        ceedless_fail_msg_(m->name ? m->name : "mock", __FILE__, __LINE__, buf);
        return;
    }

    mock_record_t *r = &m->records[m->head++];

    if (r->has_args && !r->ignore_args) {
        if (alen != r->args_len ||
            memcmp(args, r->args, alen < r->args_len ? alen : r->args_len) != 0) {
            char ea[3 * CEEDLESS_MOCK_ARG_BYTES + 8];
            char aa[3 * CEEDLESS_MOCK_ARG_BYTES + 8];
            char buf[3 * CEEDLESS_MOCK_ARG_BYTES * 2 + 128];
            fmt_bytes(ea, sizeof ea, r->args,  r->args_len);
            fmt_bytes(aa, sizeof aa, args,     alen);
            snprintf(buf, sizeof buf, "%s call#%d arg mismatch: expected [%s] got [%s]",
                     m->name ? m->name : "(mock)", m->calls, ea, aa);
            ceedless_fail_msg_(m->name ? m->name : "mock", __FILE__, __LINE__, buf);
            return;
        }
    }

    /* ReturnThruPtr: pull pointer out of args and write into *(void*) */
    if (r->rt_set && args && r->rt_arg_offset + sizeof(void *) <= alen) {
        void *p = NULL;
        memcpy(&p, (const uint8_t *)args + r->rt_arg_offset, sizeof p);
        if (p) memcpy(p, r->rt_data, r->rt_len);
    }

    if (ret && rlen) {
        if (r->has_ret) {
            size_t n = rlen < r->ret_len ? rlen : r->ret_len;
            memcpy(ret, r->ret, n);
        } else {
            memset(ret, 0, rlen);
        }
    }

    if (r->throw_set) {
        Throw(r->throw_value);
    }
}

void mock_verify(mock_t *m)
{
    if (m->ignore) return;
    if (m->head != m->count) {
        char buf[128];
        snprintf(buf, sizeof buf, "%s: %d expectation(s) not consumed",
                 m->name ? m->name : "(mock)", m->count - m->head);
        ceedless_fail_msg_(m->name ? m->name : "mock", __FILE__, __LINE__, buf);
    }
}
int mock_pending(mock_t *m) { return m->count - m->head; }
int mock_calls  (mock_t *m) { return m->calls; }
