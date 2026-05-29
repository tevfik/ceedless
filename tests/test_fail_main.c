/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Negative-path harness: contains one test that MUST fail. The Makefile
 * inverts the exit code so this counts as a passing build-system check.
 */
#include "ceedless/ceedless.h"

static void this_test_must_fail(void)
{
    TEST_ASSERT_EQUAL_INT(1, 2);
    /* unreachable */
    TEST_ASSERT_TRUE(0);
}

int main(void)
{
    ceedless_begin("negative");
    RUN_TEST(this_test_must_fail);
    return ceedless_end();  /* expected != 0 */
}
