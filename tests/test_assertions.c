/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/ceedless.h"
#include "tests.h"

static void test_int_sized(void)
{
    TEST_ASSERT_EQUAL_INT8 (-1, (int8_t)-1);
    TEST_ASSERT_EQUAL_INT16(-1, (int16_t)-1);
    TEST_ASSERT_EQUAL_INT32(-1, (int32_t)-1);
    TEST_ASSERT_EQUAL_INT64(-1LL,(int64_t)-1);
    TEST_ASSERT_EQUAL_UINT8 (0xAB, 0xAB);
    TEST_ASSERT_EQUAL_UINT16(0xABCD, 0xABCD);
    TEST_ASSERT_EQUAL_UINT32(0xDEADBEEF, 0xDEADBEEF);
    TEST_ASSERT_EQUAL_UINT64(0x1122334455667788ULL, 0x1122334455667788ULL);
}

static void test_hex(void)
{
    TEST_ASSERT_EQUAL_HEX8 (0xAB, 0xAB);
    TEST_ASSERT_EQUAL_HEX16(0x1234, 0x1234);
    TEST_ASSERT_EQUAL_HEX32(0xCAFEBABE, 0xCAFEBABE);
    TEST_ASSERT_EQUAL_HEX64(0xDEADBEEFCAFEBABEULL, 0xDEADBEEFCAFEBABEULL);
}

static void test_bits(void)
{
    uint32_t v = 0xA5;
    TEST_ASSERT_BITS     (0xFF, 0xA5, v);
    TEST_ASSERT_BITS_HIGH(0xA5, v);
    TEST_ASSERT_BITS_LOW (0x5A, v);
    TEST_ASSERT_BIT_HIGH (7, v);
    TEST_ASSERT_BIT_LOW  (1, v);
}

static void test_within_and_compare(void)
{
    TEST_ASSERT_INT_WITHIN  (3, 10, 12);
    TEST_ASSERT_UINT_WITHIN (3, 10u, 12u);
    TEST_ASSERT_GREATER_THAN(5, 10);
    TEST_ASSERT_LESS_THAN   (5, 1);
    TEST_ASSERT_GREATER_OR_EQUAL(5, 5);
    TEST_ASSERT_LESS_OR_EQUAL   (5, 5);
    TEST_ASSERT_NOT_EQUAL(1, 2);
}

static void test_arrays(void)
{
    int  a[] = { 1, 2, 3 };
    int  b[] = { 1, 2, 3 };
    TEST_ASSERT_EQUAL_INT_ARRAY(a, b, 3);
    uint8_t u1[] = { 0xAA, 0xBB };
    uint8_t u2[] = { 0xAA, 0xBB };
    TEST_ASSERT_EQUAL_HEX8_ARRAY(u1, u2, 2);
    int z[] = { 7, 7, 7 };
    TEST_ASSERT_EACH_EQUAL_INT(7, z, 3);
}

static void test_strings_and_memory(void)
{
    TEST_ASSERT_EQUAL_STRING("abc", "abc");
    TEST_ASSERT_EQUAL_STRING_LEN("abcdef", "abcxxx", 3);
    char a[4] = { 1, 2, 3, 4 };
    char b[4] = { 1, 2, 3, 4 };
    TEST_ASSERT_EQUAL_MEMORY(a, b, 4);

    const char *sa[] = { "one", "two" };
    const char *sb[] = { "one", "two" };
    TEST_ASSERT_EQUAL_STRING_ARRAY(sa, sb, 2);
}

static void test_float(void)
{
    TEST_ASSERT_EQUAL_FLOAT(1.0f, 1.0f);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 1.0f, 1.005f);
    TEST_ASSERT_GREATER_THAN_FLOAT(0.0f, 1.0f);
    TEST_ASSERT_FLOAT_IS_DETERMINATE(1.0f);
    TEST_ASSERT_FLOAT_IS_NOT_NAN(1.0f);

    double d = 1.0 / 0.0;
    TEST_ASSERT_DOUBLE_IS_INF(d);
    TEST_ASSERT_DOUBLE_IS_NOT_NAN(d);
}

static void test_ignore_macro(void)
{
    TEST_IGNORE_MESSAGE("intentionally ignored");
}

void run_assertions_tests(void)
{
    RUN_TEST(test_int_sized);
    RUN_TEST(test_hex);
    RUN_TEST(test_bits);
    RUN_TEST(test_within_and_compare);
    RUN_TEST(test_arrays);
    RUN_TEST(test_strings_and_memory);
    RUN_TEST(test_float);
    RUN_TEST(test_ignore_macro);
}
