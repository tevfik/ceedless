/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Tests for the CMock-parity mock framework.
 *
 * We define a fake "hal_send" function whose stub forwards to mock_invoke.
 */
#include "ceedless/ceedless.h"
#include "tests.h"
#include <string.h>

/* ---- mocked function ---------------------------------------------------*/
struct hal_send_args { uint8_t addr; uint16_t value; };

MOCK_DEFINE(hal_send);

static int hal_send(uint8_t addr, uint16_t value)
{
    struct hal_send_args a;
    memset(&a, 0, sizeof a); a.addr = addr; a.value = value;
    int rv = -99;
    mock_invoke(&hal_send_mock, &a, sizeof a, &rv, sizeof rv);
    return rv;
}

/* ---- a function with a return-thru-ptr parameter ---------------------- */
struct hal_read_args { uint8_t reg; uint8_t *out; };

MOCK_DEFINE(hal_read);

static int hal_read(uint8_t reg, uint8_t *out)
{
    struct hal_read_args a;
    memset(&a, 0, sizeof a); a.reg = reg; a.out = out;
    int rv = 0;
    mock_invoke(&hal_read_mock, &a, sizeof a, &rv, sizeof rv);
    return rv;
}

/* ----------------------------------------------------------------------- */
static void test_expect_and_return_in_order(void)
{
    MOCK_RESET(hal_send);
    struct hal_send_args a1, a2;
    memset(&a1, 0, sizeof a1); a1.addr = 0x10; a1.value = 0xBEEF;
    memset(&a2, 0, sizeof a2); a2.addr = 0x20; a2.value = 0xCAFE;
    int r1 = 1, r2 = 2;
    MOCK_EXPECT_AND_RETURN(hal_send, &a1, sizeof a1, &r1, sizeof r1);
    MOCK_EXPECT_AND_RETURN(hal_send, &a2, sizeof a2, &r2, sizeof r2);

    TEST_ASSERT_EQUAL_INT(1, hal_send(0x10, 0xBEEF));
    TEST_ASSERT_EQUAL_INT(2, hal_send(0x20, 0xCAFE));
    MOCK_VERIFY(hal_send);
}

static void test_expect_any_args(void)
{
    MOCK_RESET(hal_send);
    int r = 7;
    MOCK_EXPECT_ANY_ARGS_AND_RETURN(hal_send, &r, sizeof r);
    TEST_ASSERT_EQUAL_INT(7, hal_send(0xFF, 0xFFFF));
    MOCK_VERIFY(hal_send);
}

static void test_ignore_and_return(void)
{
    MOCK_RESET(hal_send);
    int r = 33;
    MOCK_IGNORE_AND_RETURN(hal_send, &r, sizeof r);
    TEST_ASSERT_EQUAL_INT(33, hal_send(1,2));
    TEST_ASSERT_EQUAL_INT(33, hal_send(3,4));
    TEST_ASSERT_EQUAL_INT(33, hal_send(5,6));
    /* MOCK_VERIFY should not fail because ignore is active */
    MOCK_VERIFY(hal_send);
}

static void test_return_thru_ptr(void)
{
    MOCK_RESET(hal_read);
    struct hal_read_args a = { 0x05, NULL };
    int rv = 0;
    /* offset of `out` inside hal_read_args. */
    size_t off = offsetof(struct hal_read_args, out);
    uint8_t payload = 0xA5;

    MOCK_EXPECT_ANY_ARGS_AND_RETURN(hal_read, &rv, sizeof rv);
    MOCK_RETURN_THRU_PTR(hal_read, off, &payload, sizeof payload);

    uint8_t got = 0;
    TEST_ASSERT_EQUAL_INT(0, hal_read(0x05, &got));
    TEST_ASSERT_EQUAL_HEX8(0xA5, got);
    MOCK_VERIFY(hal_read);
    (void)a;
}

static void test_expect_and_throw(void)
{
    MOCK_RESET(hal_send);
    struct hal_send_args a;
    memset(&a, 0, sizeof a); a.addr = 1; a.value = 2;
    MOCK_EXPECT_AND_THROW(hal_send, &a, sizeof a, 555);

    volatile EXCEPTION_T e = CEXCEPTION_NONE;
    Try {
        hal_send(1, 2);
        TEST_FAIL_MESSAGE("mock should have thrown");
    } Catch (e) {
        TEST_ASSERT_EQUAL_INT(555, e);
    }
    MOCK_VERIFY(hal_send);
}

static void on_call(mock_t *m, const void *args, size_t alen,
                    void *ret, size_t rlen, int idx)
{
    (void)m; (void)alen; (void)idx;
    const struct hal_send_args *a = args;
    int v = (int)(a->addr + a->value);
    if (ret && rlen >= sizeof v) memcpy(ret, &v, sizeof v);
}

static void test_stub_with_callback(void)
{
    MOCK_RESET(hal_send);
    MOCK_STUB(hal_send, on_call);
    TEST_ASSERT_EQUAL_INT(0x10 + 5, hal_send(0x10, 5));
    TEST_ASSERT_EQUAL_INT(0x20 + 9, hal_send(0x20, 9));
    TEST_ASSERT_EQUAL_INT(2, MOCK_CALL_COUNT(hal_send));
}

/* Matcher: pass when value is in [lo, hi]. */
struct range { uint16_t lo, hi; };
static int match_value_range(const void *args, size_t alen, void *user)
{
    if (alen != sizeof(struct hal_send_args)) return 0;
    const struct hal_send_args *a = args;
    const struct range *r = user;
    return a->value >= r->lo && a->value <= r->hi;
}

static void test_expect_with_matcher(void)
{
    MOCK_RESET(hal_send);
    static struct range r = { .lo = 100, .hi = 200 };
    int rv = 42;
    MOCK_EXPECT_WITH_MATCHER(hal_send, match_value_range, &r, &rv, sizeof rv);
    TEST_ASSERT_EQUAL_INT(42, hal_send(0xAA, 150));
    MOCK_VERIFY(hal_send);
}

void run_mock_tests(void)
{
    RUN_TEST(test_expect_and_return_in_order);
    RUN_TEST(test_expect_any_args);
    RUN_TEST(test_ignore_and_return);
    RUN_TEST(test_return_thru_ptr);
    RUN_TEST(test_expect_and_throw);
    RUN_TEST(test_stub_with_callback);
    RUN_TEST(test_expect_with_matcher);
}
