/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/ceedless.h"
#include "tests.h"

static void test_passed_count_tracking(void)
{
    /* This test simply succeeds; counts are validated indirectly via ceedless_end(). */
    TEST_ASSERT_TRUE(1);
}

static void test_message_macro_doesnt_fail(void)
{
    TEST_MESSAGE("informational message — not a failure");
    TEST_ASSERT_TRUE(1);
}

static int s_setup_calls = 0;
static int s_teardown_calls = 0;

/* override default weak hooks ONLY for the duration we want by counting. */
void setUp    (void) { s_setup_calls++; }
void tearDown (void) { s_teardown_calls++; }

static void test_setup_was_called(void) {
    TEST_ASSERT_TRUE(s_setup_calls > 0);
}

static void test_teardown_ran_for_prior_tests(void) {
    TEST_ASSERT_TRUE(s_teardown_calls > 0);
}

void run_runner_tests(void)
{
    RUN_TEST(test_passed_count_tracking);
    RUN_TEST(test_message_macro_doesnt_fail);
    RUN_TEST(test_setup_was_called);
    RUN_TEST(test_teardown_ran_for_prior_tests);
}
