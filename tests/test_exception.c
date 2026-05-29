/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/ceedless.h"
#include "tests.h"

#define ERR_A 1001
#define ERR_B 1002

static int risky(int kind)
{
    if (kind == 1) Throw(ERR_A);
    if (kind == 2) Throw(ERR_B);
    return 42;
}

static void test_throw_is_caught(void)
{
    volatile EXCEPTION_T e = CEXCEPTION_NONE;
    Try {
        risky(1);
        TEST_FAIL_MESSAGE("Throw() did not abort");
    } Catch (e) {
        TEST_ASSERT_EQUAL_INT(ERR_A, e);
    }
    TEST_ASSERT_EQUAL_INT(ERR_A, e);
}

static void test_no_throw_no_catch(void)
{
    volatile EXCEPTION_T e = CEXCEPTION_NONE;
    int r = 0;
    Try {
        r = risky(0);
    } Catch (e) {
        TEST_FAIL_MESSAGE("unexpected catch");
    }
    TEST_ASSERT_EQUAL_INT(42, r);
    TEST_ASSERT_EQUAL_INT(CEXCEPTION_NONE, e);
}

static void test_nested_try(void)
{
    volatile EXCEPTION_T outer = CEXCEPTION_NONE;
    Try {
        volatile EXCEPTION_T inner = CEXCEPTION_NONE;
        Try {
            risky(2);
        } Catch (inner) {
            TEST_ASSERT_EQUAL_INT(ERR_B, inner);
            Throw(ERR_A);
        }
    } Catch (outer) {
        TEST_ASSERT_EQUAL_INT(ERR_A, outer);
    }
}

void run_exception_tests(void)
{
    RUN_TEST(test_throw_is_caught);
    RUN_TEST(test_no_throw_no_catch);
    RUN_TEST(test_nested_try);
}
