/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 * Hybrid test runner with Unity-parity assertion macros.
 *
 * The same test source compiles unchanged on host (GCC/x86) and target
 * (e.g. arm-none-eabi-gcc); platform-conditional assertions pick the right
 * behaviour based on CEEDLESS_TARGET. setUp/tearDown/suiteSetUp/suiteTearDown
 * hooks follow Unity conventions.
 */
#ifndef CEEDLESS_HYBRID_RUNNER_H
#define CEEDLESS_HYBRID_RUNNER_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <setjmp.h>

#include "ceedless/trace_port.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*ceedless_fn_t)(void);

typedef enum {
    CEEDLESS_PASS = 0,
    CEEDLESS_FAIL = 1,
    CEEDLESS_IGNORE = 2
} ceedless_result_t;

void ceedless_begin(const char *suite);
int  ceedless_end  (void);
int  ceedless_failed_count (void);
int  ceedless_passed_count (void);
int  ceedless_ignored_count(void);
int  ceedless_total_count  (void);

/* Emit a JUnit-style XML report to `path` on ceedless_end(). */
void ceedless_set_junit_path(const char *path);

/* Emit a TAP (Test Anything Protocol) report to `path` on ceedless_end(). */
void ceedless_set_tap_path(const char *path);

/* Test-order shuffle. seed==0 means "leave order alone".
 * When set, RUN_TEST() queues tests into an internal buffer and ceedless_end()
 * pulls them out in a deterministic shuffled order using xorshift32(seed). */
void     ceedless_set_shuffle_seed(uint32_t seed);
uint32_t ceedless_get_shuffle_seed(void);

/* User-overridable hooks (Unity-style). Defaults are weak no-ops. */
void setUp(void);
void tearDown(void);
void suiteSetUp(void);
int  suiteTearDown(int num_failures);

/* --- Internal ---------------------------------------------------------- */
extern jmp_buf ceedless_jmp_;

void ceedless_run_     (const char *name, ceedless_fn_t fn);
void ceedless_begin_case_(const char *name, const char *label);
void ceedless_end_case_  (void);

void ceedless_fail_msg_(const char *expr, const char *file, int line, const char *msg);
void ceedless_ignore_  (const char *file, int line, const char *msg);
void ceedless_pass_    (const char *file, int line, const char *msg);
void ceedless_message_ (const char *file, int line, const char *msg);

/* Pseudo-random number for property tests & shuffling. xorshift32. */
uint32_t ceedless_rand_u32(void);
void     ceedless_rand_seed(uint32_t s);

/* Golden-file comparison: hash `actual` (n bytes), compare to file
 *   tests/golden/<suite>/<label>.bin
 * If the file is missing, write it (golden capture). If env var
 *   CEEDLESS_GOLDEN_UPDATE=1
 * is set, the file is overwritten regardless. Returns 0 on match. */
int  ceedless_check_golden_(const char *label, const void *actual, size_t n,
                            const char *file, int line);

void ceedless_assert_equal_int_       (long long e, long long a, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_equal_uint_      (unsigned long long e, unsigned long long a, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_equal_hex_       (unsigned long long e, unsigned long long a, int width, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_not_equal_int_   (long long e, long long a, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_within_int_      (long long delta, long long e, long long a, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_within_uint_     (unsigned long long delta, unsigned long long e, unsigned long long a, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_greater_int_     (long long thr, long long a, int orequal, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_less_int_        (long long thr, long long a, int orequal, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_bits_            (unsigned long long mask, unsigned long long e, unsigned long long a, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_equal_string_    (const char *e, const char *a, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_equal_memory_    (const void *e, const void *a, size_t n, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_equal_int_array_ (const long long *e, const void *a, size_t es, size_t n, int sign, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_each_equal_int_  (long long e, const void *a, size_t es, size_t n, int sign, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_equal_string_array_(const char * const *e, const char * const *a, size_t n, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_within_float_    (double delta, double e, double a, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_equal_float_     (double e, double a, int is_double, const char *expr, const char *file, int line, const char *msg);
void ceedless_assert_float_class_     (double a, int want, const char *expr, const char *file, int line, const char *msg);

enum { CEEDLESS_FC_INF=1, CEEDLESS_FC_NEG_INF=2, CEEDLESS_FC_NAN=3, CEEDLESS_FC_DETERMINATE=4,
       CEEDLESS_FC_NOT_INF=5, CEEDLESS_FC_NOT_NEG_INF=6, CEEDLESS_FC_NOT_NAN=7, CEEDLESS_FC_NOT_DETERMINATE=8 };

/* ======================================================================
 * Public macros
 * ====================================================================== */

#define RUN_TEST(name)  ceedless_run_(#name, (name))

/* RUN_TEST_CASE(name, label, args...) runs `name(args)` with the printable
 * `label` shown in [RUN]/[PASS] output. Use with parametric tests:
 *
 *   static void test_add(int a, int b, int e) { TEST_ASSERT_EQUAL(e, a+b); }
 *   RUN_TEST_CASE(test_add, "1+2=3", 1, 2, 3);
 */
#define RUN_TEST_CASE(name, label, ...) do {                    \
    ceedless_begin_case_(#name, (label));                       \
    if (setjmp(ceedless_jmp_) == 0) {                           \
        setUp();                                                \
        (name)(__VA_ARGS__);                                    \
    }                                                           \
    tearDown();                                                 \
    ceedless_end_case_();                                       \
} while (0)

/* TEST_CASE(args...) is a runner-generator annotation. It expands to nothing
 * for the compiler; the CLI's auto-runner scans for these lines preceding a
 * `void test_<name>(...)` and emits one RUN_TEST_CASE call per occurrence. */
#define TEST_CASE(...) /* runner-discovery annotation */

#define TEST_FAIL()                  ceedless_fail_msg_("TEST_FAIL", __FILE__, __LINE__, "")
#define TEST_FAIL_MESSAGE(msg)       ceedless_fail_msg_("TEST_FAIL", __FILE__, __LINE__, (msg))
#define TEST_PASS()                  ceedless_pass_(__FILE__, __LINE__, "")
#define TEST_PASS_MESSAGE(msg)       ceedless_pass_(__FILE__, __LINE__, (msg))
#define TEST_IGNORE()                ceedless_ignore_(__FILE__, __LINE__, "")
#define TEST_IGNORE_MESSAGE(msg)     ceedless_ignore_(__FILE__, __LINE__, (msg))
#define TEST_MESSAGE(msg)            ceedless_message_(__FILE__, __LINE__, (msg))

#define TEST_ASSERT(c)                       TEST_ASSERT_TRUE(c)
#define TEST_ASSERT_TRUE(c)                  TEST_ASSERT_TRUE_MESSAGE((c), "expected true")
#define TEST_ASSERT_FALSE(c)                 TEST_ASSERT_FALSE_MESSAGE((c), "expected false")
#define TEST_ASSERT_UNLESS(c)                TEST_ASSERT_FALSE(c)
#define TEST_ASSERT_TRUE_MESSAGE(c,m)        do{ if(!(c)) ceedless_fail_msg_(#c,__FILE__,__LINE__,(m)); }while(0)
#define TEST_ASSERT_FALSE_MESSAGE(c,m)       do{ if( (c)) ceedless_fail_msg_(#c,__FILE__,__LINE__,(m)); }while(0)
#define TEST_ASSERT_NULL(p)                  TEST_ASSERT_TRUE_MESSAGE((p)==NULL,"expected NULL")
#define TEST_ASSERT_NOT_NULL(p)              TEST_ASSERT_TRUE_MESSAGE((p)!=NULL,"expected non-NULL")
#define TEST_ASSERT_NULL_MESSAGE(p,m)        TEST_ASSERT_TRUE_MESSAGE((p)==NULL,(m))
#define TEST_ASSERT_NOT_NULL_MESSAGE(p,m)    TEST_ASSERT_TRUE_MESSAGE((p)!=NULL,(m))
#define TEST_ASSERT_EMPTY(p)                 TEST_ASSERT_TRUE_MESSAGE(*(const char*)(p)==0,"expected empty")
#define TEST_ASSERT_NOT_EMPTY(p)             TEST_ASSERT_TRUE_MESSAGE(*(const char*)(p)!=0,"expected non-empty")

#define TEST_ASSERT_EQUAL(e,a)               TEST_ASSERT_EQUAL_INT((e),(a))
#define TEST_ASSERT_EQUAL_INT(e,a)           ceedless_assert_equal_int_((long long)(e),(long long)(a),#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_INT8(e,a)          ceedless_assert_equal_int_((long long)(int8_t)(e),(long long)(int8_t)(a),#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_INT16(e,a)         ceedless_assert_equal_int_((long long)(int16_t)(e),(long long)(int16_t)(a),#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_INT32(e,a)         ceedless_assert_equal_int_((long long)(int32_t)(e),(long long)(int32_t)(a),#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_INT64(e,a)         ceedless_assert_equal_int_((long long)(e),(long long)(a),#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_UINT(e,a)          ceedless_assert_equal_uint_((unsigned long long)(e),(unsigned long long)(a),#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_UINT8(e,a)         ceedless_assert_equal_uint_((unsigned long long)(uint8_t)(e),(unsigned long long)(uint8_t)(a),#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_UINT16(e,a)        ceedless_assert_equal_uint_((unsigned long long)(uint16_t)(e),(unsigned long long)(uint16_t)(a),#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_UINT32(e,a)        ceedless_assert_equal_uint_((unsigned long long)(uint32_t)(e),(unsigned long long)(uint32_t)(a),#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_UINT64(e,a)        ceedless_assert_equal_uint_((unsigned long long)(e),(unsigned long long)(a),#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_NOT_EQUAL(e,a)           ceedless_assert_not_equal_int_((long long)(e),(long long)(a),#e" != "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_NOT_EQUAL_INT(e,a)       TEST_ASSERT_NOT_EQUAL((e),(a))

#define TEST_ASSERT_EQUAL_HEX(e,a)           ceedless_assert_equal_hex_((unsigned long long)(e),(unsigned long long)(a),32,#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_HEX8(e,a)          ceedless_assert_equal_hex_((unsigned long long)(uint8_t)(e),(unsigned long long)(uint8_t)(a),8,#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_HEX16(e,a)         ceedless_assert_equal_hex_((unsigned long long)(uint16_t)(e),(unsigned long long)(uint16_t)(a),16,#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_HEX32(e,a)         ceedless_assert_equal_hex_((unsigned long long)(uint32_t)(e),(unsigned long long)(uint32_t)(a),32,#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_HEX64(e,a)         ceedless_assert_equal_hex_((unsigned long long)(e),(unsigned long long)(a),64,#e" == "#a,__FILE__,__LINE__,NULL)

#define TEST_ASSERT_EQUAL_CHAR(e,a)          TEST_ASSERT_EQUAL_INT8((e),(a))
#define TEST_ASSERT_EQUAL_PTR(e,a)           ceedless_assert_equal_hex_((unsigned long long)(uintptr_t)(e),(unsigned long long)(uintptr_t)(a),64,#e" == "#a,__FILE__,__LINE__,NULL)

#define TEST_ASSERT_BITS(m,e,a)              ceedless_assert_bits_((unsigned long long)(m),(unsigned long long)(e),(unsigned long long)(a),#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_BITS_HIGH(m,a)           ceedless_assert_bits_((unsigned long long)(m),(unsigned long long)(m),(unsigned long long)(a),#a,__FILE__,__LINE__,"bits high")
#define TEST_ASSERT_BITS_LOW(m,a)            ceedless_assert_bits_((unsigned long long)(m),0ULL,(unsigned long long)(a),#a,__FILE__,__LINE__,"bits low")
#define TEST_ASSERT_BIT_HIGH(b,a)            TEST_ASSERT_BITS_HIGH(1ULL<<(b),(a))
#define TEST_ASSERT_BIT_LOW(b,a)             TEST_ASSERT_BITS_LOW (1ULL<<(b),(a))

#define TEST_ASSERT_GREATER_THAN(t,a)            ceedless_assert_greater_int_((long long)(t),(long long)(a),0,#a" > "#t,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_GREATER_OR_EQUAL(t,a)        ceedless_assert_greater_int_((long long)(t),(long long)(a),1,#a" >= "#t,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_LESS_THAN(t,a)               ceedless_assert_less_int_   ((long long)(t),(long long)(a),0,#a" < "#t,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_LESS_OR_EQUAL(t,a)           ceedless_assert_less_int_   ((long long)(t),(long long)(a),1,#a" <= "#t,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_GREATER_THAN_INT(t,a)        TEST_ASSERT_GREATER_THAN((t),(a))
#define TEST_ASSERT_GREATER_OR_EQUAL_INT(t,a)    TEST_ASSERT_GREATER_OR_EQUAL((t),(a))
#define TEST_ASSERT_LESS_THAN_INT(t,a)           TEST_ASSERT_LESS_THAN((t),(a))
#define TEST_ASSERT_LESS_OR_EQUAL_INT(t,a)       TEST_ASSERT_LESS_OR_EQUAL((t),(a))
#define TEST_ASSERT_GREATER_THAN_UINT(t,a)       TEST_ASSERT_GREATER_THAN((long long)(unsigned long long)(t),(long long)(unsigned long long)(a))
#define TEST_ASSERT_LESS_THAN_UINT(t,a)          TEST_ASSERT_LESS_THAN   ((long long)(unsigned long long)(t),(long long)(unsigned long long)(a))

#define TEST_ASSERT_INT_WITHIN(d,e,a)        ceedless_assert_within_int_((long long)(d),(long long)(e),(long long)(a),#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_UINT_WITHIN(d,e,a)       ceedless_assert_within_uint_((unsigned long long)(d),(unsigned long long)(e),(unsigned long long)(a),#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_HEX_WITHIN(d,e,a)        TEST_ASSERT_UINT_WITHIN((d),(e),(a))
#define TEST_ASSERT_INT8_WITHIN(d,e,a)       TEST_ASSERT_INT_WITHIN((d),(int8_t)(e),(int8_t)(a))
#define TEST_ASSERT_INT16_WITHIN(d,e,a)      TEST_ASSERT_INT_WITHIN((d),(int16_t)(e),(int16_t)(a))
#define TEST_ASSERT_INT32_WITHIN(d,e,a)      TEST_ASSERT_INT_WITHIN((d),(int32_t)(e),(int32_t)(a))
#define TEST_ASSERT_INT64_WITHIN(d,e,a)      TEST_ASSERT_INT_WITHIN((d),(e),(a))
#define TEST_ASSERT_UINT8_WITHIN(d,e,a)      TEST_ASSERT_UINT_WITHIN((d),(uint8_t)(e),(uint8_t)(a))
#define TEST_ASSERT_UINT16_WITHIN(d,e,a)     TEST_ASSERT_UINT_WITHIN((d),(uint16_t)(e),(uint16_t)(a))
#define TEST_ASSERT_UINT32_WITHIN(d,e,a)     TEST_ASSERT_UINT_WITHIN((d),(uint32_t)(e),(uint32_t)(a))
#define TEST_ASSERT_UINT64_WITHIN(d,e,a)     TEST_ASSERT_UINT_WITHIN((d),(e),(a))

#define TEST_ASSERT_EQUAL_MEMORY(e,a,n)      ceedless_assert_equal_memory_((e),(a),(size_t)(n),#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_STRING(e,a)        ceedless_assert_equal_string_((e),(a),#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EQUAL_STRING_LEN(e,a,n)  ceedless_assert_equal_memory_((e),(a),(size_t)(n),#e" == "#a,__FILE__,__LINE__,"string-len")

#define TEST_ASSERT_EQUAL_INT_ARRAY(e,a,n)   ceedless_array_helper_int_((e),(a),sizeof((a)[0]),(n),1,#e,__FILE__,__LINE__)
#define TEST_ASSERT_EQUAL_INT8_ARRAY(e,a,n)  TEST_ASSERT_EQUAL_INT_ARRAY((e),(a),(n))
#define TEST_ASSERT_EQUAL_INT16_ARRAY(e,a,n) TEST_ASSERT_EQUAL_INT_ARRAY((e),(a),(n))
#define TEST_ASSERT_EQUAL_INT32_ARRAY(e,a,n) TEST_ASSERT_EQUAL_INT_ARRAY((e),(a),(n))
#define TEST_ASSERT_EQUAL_INT64_ARRAY(e,a,n) TEST_ASSERT_EQUAL_INT_ARRAY((e),(a),(n))
#define TEST_ASSERT_EQUAL_UINT_ARRAY(e,a,n)  ceedless_array_helper_int_((e),(a),sizeof((a)[0]),(n),0,#e,__FILE__,__LINE__)
#define TEST_ASSERT_EQUAL_UINT8_ARRAY(e,a,n) TEST_ASSERT_EQUAL_UINT_ARRAY((e),(a),(n))
#define TEST_ASSERT_EQUAL_UINT16_ARRAY(e,a,n)TEST_ASSERT_EQUAL_UINT_ARRAY((e),(a),(n))
#define TEST_ASSERT_EQUAL_UINT32_ARRAY(e,a,n)TEST_ASSERT_EQUAL_UINT_ARRAY((e),(a),(n))
#define TEST_ASSERT_EQUAL_UINT64_ARRAY(e,a,n)TEST_ASSERT_EQUAL_UINT_ARRAY((e),(a),(n))
#define TEST_ASSERT_EQUAL_HEX_ARRAY(e,a,n)   ceedless_array_helper_int_((e),(a),sizeof((a)[0]),(n),2,#e,__FILE__,__LINE__)
#define TEST_ASSERT_EQUAL_HEX8_ARRAY(e,a,n)  TEST_ASSERT_EQUAL_HEX_ARRAY((e),(a),(n))
#define TEST_ASSERT_EQUAL_HEX16_ARRAY(e,a,n) TEST_ASSERT_EQUAL_HEX_ARRAY((e),(a),(n))
#define TEST_ASSERT_EQUAL_HEX32_ARRAY(e,a,n) TEST_ASSERT_EQUAL_HEX_ARRAY((e),(a),(n))
#define TEST_ASSERT_EQUAL_HEX64_ARRAY(e,a,n) TEST_ASSERT_EQUAL_HEX_ARRAY((e),(a),(n))
#define TEST_ASSERT_EQUAL_CHAR_ARRAY(e,a,n)  TEST_ASSERT_EQUAL_INT_ARRAY((const int8_t*)(e),(a),(n))
#define TEST_ASSERT_EQUAL_PTR_ARRAY(e,a,n)   ceedless_assert_equal_memory_((e),(a),sizeof(void*)*(size_t)(n),#e" == "#a,__FILE__,__LINE__,"ptr-array")
#define TEST_ASSERT_EQUAL_MEMORY_ARRAY(e,a,len,n) ceedless_assert_equal_memory_((e),(a),(size_t)(len)*(size_t)(n),#e" == "#a,__FILE__,__LINE__,"memory-array")
#define TEST_ASSERT_EQUAL_STRING_ARRAY(e,a,n) ceedless_assert_equal_string_array_((const char* const*)(e),(const char* const*)(a),(size_t)(n),#e" == "#a,__FILE__,__LINE__,NULL)

#define TEST_ASSERT_EACH_EQUAL_INT(e,a,n)    ceedless_assert_each_equal_int_((long long)(e),(a),sizeof((a)[0]),(n),1,#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EACH_EQUAL_UINT(e,a,n)   ceedless_assert_each_equal_int_((long long)(unsigned long long)(e),(a),sizeof((a)[0]),(n),0,#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EACH_EQUAL_HEX(e,a,n)    ceedless_assert_each_equal_int_((long long)(unsigned long long)(e),(a),sizeof((a)[0]),(n),2,#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_EACH_EQUAL_INT8(e,a,n)   TEST_ASSERT_EACH_EQUAL_INT((e),(a),(n))
#define TEST_ASSERT_EACH_EQUAL_INT16(e,a,n)  TEST_ASSERT_EACH_EQUAL_INT((e),(a),(n))
#define TEST_ASSERT_EACH_EQUAL_INT32(e,a,n)  TEST_ASSERT_EACH_EQUAL_INT((e),(a),(n))
#define TEST_ASSERT_EACH_EQUAL_INT64(e,a,n)  TEST_ASSERT_EACH_EQUAL_INT((e),(a),(n))
#define TEST_ASSERT_EACH_EQUAL_UINT8(e,a,n)  TEST_ASSERT_EACH_EQUAL_UINT((e),(a),(n))
#define TEST_ASSERT_EACH_EQUAL_UINT16(e,a,n) TEST_ASSERT_EACH_EQUAL_UINT((e),(a),(n))
#define TEST_ASSERT_EACH_EQUAL_UINT32(e,a,n) TEST_ASSERT_EACH_EQUAL_UINT((e),(a),(n))
#define TEST_ASSERT_EACH_EQUAL_UINT64(e,a,n) TEST_ASSERT_EACH_EQUAL_UINT((e),(a),(n))
#define TEST_ASSERT_EACH_EQUAL_HEX8(e,a,n)   TEST_ASSERT_EACH_EQUAL_HEX((e),(a),(n))
#define TEST_ASSERT_EACH_EQUAL_HEX16(e,a,n)  TEST_ASSERT_EACH_EQUAL_HEX((e),(a),(n))
#define TEST_ASSERT_EACH_EQUAL_HEX32(e,a,n)  TEST_ASSERT_EACH_EQUAL_HEX((e),(a),(n))
#define TEST_ASSERT_EACH_EQUAL_HEX64(e,a,n)  TEST_ASSERT_EACH_EQUAL_HEX((e),(a),(n))
#define TEST_ASSERT_EACH_EQUAL_CHAR(e,a,n)   TEST_ASSERT_EACH_EQUAL_INT((e),(a),(n))

#define TEST_ASSERT_FLOAT_WITHIN(d,e,a)          ceedless_assert_within_float_((double)(d),(double)(e),(double)(a),#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_FLOAT_NOT_WITHIN(d,e,a)      ceedless_assert_within_float_((double)(d),(double)(e),(double)(a),#a,__FILE__,__LINE__,"NOT_WITHIN")
#define TEST_ASSERT_EQUAL_FLOAT(e,a)             ceedless_assert_equal_float_((double)(e),(double)(a),0,#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_NOT_EQUAL_FLOAT(e,a)         ceedless_assert_equal_float_((double)(e),(double)(a),0,#e" != "#a,__FILE__,__LINE__,"NOT_EQUAL")
#define TEST_ASSERT_LESS_THAN_FLOAT(t,a)         TEST_ASSERT_TRUE_MESSAGE((double)(a) <  (double)(t),"LESS_THAN_FLOAT")
#define TEST_ASSERT_LESS_OR_EQUAL_FLOAT(t,a)     TEST_ASSERT_TRUE_MESSAGE((double)(a) <= (double)(t),"LESS_OR_EQUAL_FLOAT")
#define TEST_ASSERT_GREATER_THAN_FLOAT(t,a)      TEST_ASSERT_TRUE_MESSAGE((double)(a) >  (double)(t),"GREATER_THAN_FLOAT")
#define TEST_ASSERT_GREATER_OR_EQUAL_FLOAT(t,a)  TEST_ASSERT_TRUE_MESSAGE((double)(a) >= (double)(t),"GREATER_OR_EQUAL_FLOAT")
#define TEST_ASSERT_FLOAT_IS_INF(a)              ceedless_assert_float_class_((double)(a),CEEDLESS_FC_INF,#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_FLOAT_IS_NEG_INF(a)          ceedless_assert_float_class_((double)(a),CEEDLESS_FC_NEG_INF,#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_FLOAT_IS_NAN(a)              ceedless_assert_float_class_((double)(a),CEEDLESS_FC_NAN,#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_FLOAT_IS_DETERMINATE(a)      ceedless_assert_float_class_((double)(a),CEEDLESS_FC_DETERMINATE,#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_FLOAT_IS_NOT_INF(a)          ceedless_assert_float_class_((double)(a),CEEDLESS_FC_NOT_INF,#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_FLOAT_IS_NOT_NEG_INF(a)      ceedless_assert_float_class_((double)(a),CEEDLESS_FC_NOT_NEG_INF,#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_FLOAT_IS_NOT_NAN(a)          ceedless_assert_float_class_((double)(a),CEEDLESS_FC_NOT_NAN,#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_FLOAT_IS_NOT_DETERMINATE(a)  ceedless_assert_float_class_((double)(a),CEEDLESS_FC_NOT_DETERMINATE,#a,__FILE__,__LINE__,NULL)

#define TEST_ASSERT_DOUBLE_WITHIN(d,e,a)         TEST_ASSERT_FLOAT_WITHIN((d),(e),(a))
#define TEST_ASSERT_DOUBLE_NOT_WITHIN(d,e,a)     TEST_ASSERT_FLOAT_NOT_WITHIN((d),(e),(a))
#define TEST_ASSERT_EQUAL_DOUBLE(e,a)            ceedless_assert_equal_float_((double)(e),(double)(a),1,#e" == "#a,__FILE__,__LINE__,NULL)
#define TEST_ASSERT_NOT_EQUAL_DOUBLE(e,a)        ceedless_assert_equal_float_((double)(e),(double)(a),1,#e" != "#a,__FILE__,__LINE__,"NOT_EQUAL")
#define TEST_ASSERT_LESS_THAN_DOUBLE(t,a)        TEST_ASSERT_LESS_THAN_FLOAT((t),(a))
#define TEST_ASSERT_LESS_OR_EQUAL_DOUBLE(t,a)    TEST_ASSERT_LESS_OR_EQUAL_FLOAT((t),(a))
#define TEST_ASSERT_GREATER_THAN_DOUBLE(t,a)     TEST_ASSERT_GREATER_THAN_FLOAT((t),(a))
#define TEST_ASSERT_GREATER_OR_EQUAL_DOUBLE(t,a) TEST_ASSERT_GREATER_OR_EQUAL_FLOAT((t),(a))
#define TEST_ASSERT_DOUBLE_IS_INF(a)             TEST_ASSERT_FLOAT_IS_INF(a)
#define TEST_ASSERT_DOUBLE_IS_NEG_INF(a)         TEST_ASSERT_FLOAT_IS_NEG_INF(a)
#define TEST_ASSERT_DOUBLE_IS_NAN(a)             TEST_ASSERT_FLOAT_IS_NAN(a)
#define TEST_ASSERT_DOUBLE_IS_DETERMINATE(a)     TEST_ASSERT_FLOAT_IS_DETERMINATE(a)
#define TEST_ASSERT_DOUBLE_IS_NOT_INF(a)         TEST_ASSERT_FLOAT_IS_NOT_INF(a)
#define TEST_ASSERT_DOUBLE_IS_NOT_NEG_INF(a)     TEST_ASSERT_FLOAT_IS_NOT_NEG_INF(a)
#define TEST_ASSERT_DOUBLE_IS_NOT_NAN(a)         TEST_ASSERT_FLOAT_IS_NOT_NAN(a)
#define TEST_ASSERT_DOUBLE_IS_NOT_DETERMINATE(a) TEST_ASSERT_FLOAT_IS_NOT_DETERMINATE(a)

/* --- _MESSAGE variants for the most common ones --- */
#define TEST_ASSERT_EQUAL_INT_MESSAGE(e,a,m)     ceedless_assert_equal_int_((long long)(e),(long long)(a),#e" == "#a,__FILE__,__LINE__,(m))
#define TEST_ASSERT_EQUAL_UINT_MESSAGE(e,a,m)    ceedless_assert_equal_uint_((unsigned long long)(e),(unsigned long long)(a),#e" == "#a,__FILE__,__LINE__,(m))
#define TEST_ASSERT_EQUAL_HEX_MESSAGE(e,a,m)     ceedless_assert_equal_hex_((unsigned long long)(e),(unsigned long long)(a),32,#e" == "#a,__FILE__,__LINE__,(m))
#define TEST_ASSERT_EQUAL_STRING_MESSAGE(e,a,m)  ceedless_assert_equal_string_((e),(a),#e" == "#a,__FILE__,__LINE__,(m))
#define TEST_ASSERT_EQUAL_MEMORY_MESSAGE(e,a,n,m) ceedless_assert_equal_memory_((e),(a),(size_t)(n),#e" == "#a,__FILE__,__LINE__,(m))

/* --- Snapshot / golden-file assertion ----------------------------------- *
 * Compares `n` bytes at `a` against tests/golden/<suite>/<label>.bin.
 * Captures the file on first run (when missing) or whenever the env var
 * CEEDLESS_GOLDEN_UPDATE=1 is set; otherwise fails the test on mismatch.
 */
#define TEST_ASSERT_GOLDEN_BYTES(label, a, n) \
    (void)ceedless_check_golden_((label),(a),(size_t)(n),__FILE__,__LINE__)

/* --- Property-based test loop ------------------------------------------ *
 * Runs `body` `iters` times with `var` set to a fresh u32 each iteration.
 * Use TEST_ASSERT_* inside body. The seed is the suite seed (set with
 * ceedless_set_shuffle_seed or CEEDLESS_SEED env var) so failures are
 * reproducible.
 */
#define TEST_PROPERTY(var, iters, body) do {                    \
    for (uint32_t _i = 0; _i < (uint32_t)(iters); _i++) {       \
        uint32_t var = ceedless_rand_u32();                     \
        body                                                    \
    }                                                           \
} while (0)

#ifdef CEEDLESS_TARGET
#  define TEST_ASSERT_HW(c)     TEST_ASSERT_TRUE(c)
#  define TEST_ASSERT_HOST(c)   ((void)0)
#else
#  define TEST_ASSERT_HW(c)     ((void)0)
#  define TEST_ASSERT_HOST(c)   TEST_ASSERT_TRUE(c)
#endif

/* Inline array shim so sizeof((a)[0]) is resolved at call site. */
static inline void ceedless_array_helper_int_(
    const void *e, const void *a, size_t es, size_t n, int sign,
    const char *expr, const char *file, int line)
{
    long long buf_e[64];
    if (n == 0) return;
    if (n <= sizeof buf_e / sizeof buf_e[0]) {
        const unsigned char *p = (const unsigned char *)e;
        for (size_t i = 0; i < n; i++) {
            long long v = 0;
            switch (es) {
            case 1: v = sign==1 ? *(const int8_t  *)(p+i*es) : *(const uint8_t *)(p+i*es); break;
            case 2: v = sign==1 ? *(const int16_t *)(p+i*es) : *(const uint16_t*)(p+i*es); break;
            case 4: v = sign==1 ? (long long)*(const int32_t *)(p+i*es) : (long long)*(const uint32_t*)(p+i*es); break;
            case 8: v = sign==1 ? *(const int64_t *)(p+i*es) : (long long)*(const uint64_t*)(p+i*es); break;
            default: ceedless_fail_msg_(expr,file,line,"unsupported element size"); return;
            }
            buf_e[i] = v;
        }
        ceedless_assert_equal_int_array_(buf_e, a, es, n, sign, expr, file, line, NULL);
    } else {
        ceedless_assert_equal_memory_(e, a, es*n, expr, file, line, "array-fallback");
    }
}

#ifdef __cplusplus
}
#endif
#endif /* CEEDLESS_HYBRID_RUNNER_H */
