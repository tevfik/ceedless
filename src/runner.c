/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Tevfik — https://github.com/tevfik/ceedless
 */
#include "ceedless/hybrid_runner.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

jmp_buf ceedless_jmp_;

/* --- ANSI colour (disabled when CEEDLESS_NO_COLOR is set or stdout is not a TTY).
 * Resolved lazily on first use so cost is one getenv() per process. */
static const char *c_red(void);
static const char *c_grn(void);
static const char *c_ylw(void);
static const char *c_dim(void);
static const char *c_rst(void);
static int s_color_inited = 0;
static int s_color_on     = 0;
static void color_init(void) {
    if (s_color_inited) return;
    s_color_inited = 1;
    const char *no = getenv("CEEDLESS_NO_COLOR");
    if (no && no[0]) { s_color_on = 0; return; }
#if defined(_POSIX_VERSION) || defined(__unix__) || defined(__APPLE__) || defined(__linux__)
    extern int isatty(int);
    s_color_on = isatty(1);
#else
    s_color_on = 0;
#endif
}
static const char *c_red(void){ color_init(); return s_color_on ? "\033[1;31m" : ""; }
static const char *c_grn(void){ color_init(); return s_color_on ? "\033[1;32m" : ""; }
static const char *c_ylw(void){ color_init(); return s_color_on ? "\033[1;33m" : ""; }
static const char *c_dim(void){ color_init(); return s_color_on ? "\033[2m"    : ""; }
static const char *c_rst(void){ color_init(); return s_color_on ? "\033[0m"    : ""; }

static const char *s_suite          = "default";
static int  s_total                 = 0;
static int  s_passed                = 0;
static int  s_failed                = 0;
static int  s_ignored               = 0;
static int  s_current_failed        = 0;
static int  s_current_ignored       = 0;
static const char *s_current_name   = "";
static char s_current_msg[160]      = {0};

/* JUnit accumulation */
typedef struct {
    char name[64];
    int  result;   /* 0 pass 1 fail 2 ignore */
    char msg[160];
} junit_case_t;
#ifndef CEEDLESS_MAX_TESTS
#define CEEDLESS_MAX_TESTS 256
#endif
static junit_case_t s_cases[CEEDLESS_MAX_TESTS];
static int          s_case_count = 0;
static const char  *s_junit_path = NULL;

/* --- Weak default hooks (overridable). --------------------------------- */
#if defined(__GNUC__)
__attribute__((weak)) void setUp(void)         { }
__attribute__((weak)) void tearDown(void)      { }
__attribute__((weak)) void suiteSetUp(void)    { }
__attribute__((weak)) int  suiteTearDown(int f){ return f; }
#endif

int ceedless_failed_count (void) { return s_failed;  }
int ceedless_passed_count (void) { return s_passed;  }
int ceedless_ignored_count(void) { return s_ignored; }
int ceedless_total_count  (void) { return s_total;   }

void ceedless_set_junit_path(const char *p) { s_junit_path = p; }

static void xml_escape_into(char *dst, size_t cap, const char *src)
{
    size_t o = 0;
    for (size_t i = 0; src && src[i] && o + 8 < cap; i++) {
        char c = src[i];
        const char *r = NULL; size_t rl = 0;
        switch (c) {
        case '<':  r = "&lt;";   rl = 4; break;
        case '>':  r = "&gt;";   rl = 4; break;
        case '&':  r = "&amp;";  rl = 5; break;
        case '"':  r = "&quot;"; rl = 6; break;
        case '\'': r = "&apos;"; rl = 6; break;
        }
        if (r) { memcpy(dst+o, r, rl); o += rl; }
        else   { dst[o++] = c; }
    }
    dst[o] = '\0';
}

static void emit_junit(void)
{
    if (!s_junit_path) return;
    FILE *f = fopen(s_junit_path, "w");
    if (!f) return;
    char esuite[128]; xml_escape_into(esuite, sizeof esuite, s_suite);
    fprintf(f,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<testsuite name=\"%s\" tests=\"%d\" failures=\"%d\" skipped=\"%d\">\n",
        esuite, s_total, s_failed, s_ignored);
    for (int i = 0; i < s_case_count; i++) {
        char ename[128], emsg[256];
        xml_escape_into(ename, sizeof ename, s_cases[i].name);
        xml_escape_into(emsg,  sizeof emsg,  s_cases[i].msg);
        fprintf(f, "  <testcase name=\"%s\" classname=\"%s\">", ename, esuite);
        if (s_cases[i].result == 1)
            fprintf(f, "<failure message=\"%s\"/>", emsg);
        else if (s_cases[i].result == 2)
            fprintf(f, "<skipped message=\"%s\"/>", emsg);
        fprintf(f, "</testcase>\n");
    }
    fprintf(f, "</testsuite>\n");
    fclose(f);
}

void ceedless_begin(const char *suite)
{
    s_suite = suite ? suite : "default";
    s_total = s_passed = s_failed = s_ignored = 0;
    s_case_count = 0;
    trace_init();
    suiteSetUp();
    trace_printf("\n=== ceedless suite: %s ===\n", s_suite);
}

int ceedless_end(void)
{
    const char *col = (s_failed ? c_red() : c_grn());
    trace_printf("%s=== result: %d/%d passed, %d failed, %d ignored ===%s\n",
                 col, s_passed, s_total, s_failed, s_ignored, c_rst());
    trace_flush();
    emit_junit();
    int rc = suiteTearDown(s_failed);
    return rc ? rc : (s_failed ? 1 : 0);
}

void ceedless_run_(const char *name, ceedless_fn_t fn)
{
    if (s_total >= CEEDLESS_MAX_TESTS) {
        trace_printf("[SKIP] %s (too many tests)\n", name);
        return;
    }
    s_total++;
    s_current_failed = 0;
    s_current_ignored = 0;
    s_current_msg[0] = '\0';
    s_current_name = name;
    trace_printf("%s[RUN ]%s %s\n", c_dim(), c_rst(), name);
    if (setjmp(ceedless_jmp_) == 0) {
        setUp();
        fn();
    }
    tearDown();

    junit_case_t *jc = &s_cases[s_case_count++];
    size_t nl = strlen(name);
    if (nl >= sizeof jc->name) nl = sizeof jc->name - 1;
    memcpy(jc->name, name, nl); jc->name[nl] = '\0';
    strncpy(jc->msg, s_current_msg, sizeof jc->msg - 1);
    jc->msg[sizeof jc->msg - 1] = '\0';
    /* zero-init padding to avoid uninitialised reads later */
    (void)0;

    if (s_current_failed)        { s_failed++;  jc->result = 1; trace_printf("%s[FAIL]%s %s\n", c_red(), c_rst(), name); }
    else if (s_current_ignored)  { s_ignored++; jc->result = 2; trace_printf("%s[SKIP]%s %s\n", c_ylw(), c_rst(), name); }
    else                         { s_passed++;  jc->result = 0; trace_printf("%s[PASS]%s %s\n", c_grn(), c_rst(), name); }
    trace_flush();
}

void ceedless_fail_msg_(const char *expr, const char *file, int line, const char *msg)
{
    s_current_failed = 1;
    snprintf(s_current_msg, sizeof s_current_msg,
             "%s @ %s:%d %s", expr ? expr : "", file, line, msg ? msg : "");
    trace_printf("  ASSERT FAILED: %s\n    at %s:%d\n    %s\n",
                 expr ? expr : "", file, line, msg ? msg : "");
    longjmp(ceedless_jmp_, 1);
}

void ceedless_ignore_(const char *file, int line, const char *msg)
{
    s_current_ignored = 1;
    snprintf(s_current_msg, sizeof s_current_msg, "IGNORE @ %s:%d %s",
             file, line, msg ? msg : "");
    trace_printf("  IGNORED at %s:%d %s\n", file, line, msg ? msg : "");
    longjmp(ceedless_jmp_, 2);
}

void ceedless_pass_(const char *file, int line, const char *msg)
{
    trace_printf("  PASS at %s:%d %s\n", file, line, msg ? msg : "");
    longjmp(ceedless_jmp_, 3);
}

void ceedless_message_(const char *file, int line, const char *msg)
{
    trace_printf("  MSG  %s:%d %s\n", file, line, msg ? msg : "");
}

/* ====================================================================== */
/* Concrete assertion predicates                                          */
/* ====================================================================== */

void ceedless_assert_equal_int_(long long e, long long a,
                              const char *expr, const char *file, int line, const char *msg)
{
    if (e != a) {
        char m[96]; snprintf(m, sizeof m, "expected %lld actual %lld %s", e, a, msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
void ceedless_assert_equal_uint_(unsigned long long e, unsigned long long a,
                               const char *expr, const char *file, int line, const char *msg)
{
    if (e != a) {
        char m[96]; snprintf(m, sizeof m, "expected %llu actual %llu %s", e, a, msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
void ceedless_assert_equal_hex_(unsigned long long e, unsigned long long a, int width,
                              const char *expr, const char *file, int line, const char *msg)
{
    unsigned long long mask =
        width >= 64 ? ~0ULL : ((1ULL << width) - 1ULL);
    if ((e & mask) != (a & mask)) {
        char m[96]; snprintf(m, sizeof m, "expected 0x%llX actual 0x%llX %s",
                             (e & mask), (a & mask), msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
void ceedless_assert_not_equal_int_(long long e, long long a,
                                  const char *expr, const char *file, int line, const char *msg)
{
    if (e == a) {
        char m[96]; snprintf(m, sizeof m, "both %lld %s", e, msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
void ceedless_assert_within_int_(long long delta, long long e, long long a,
                               const char *expr, const char *file, int line, const char *msg)
{
    long long d = a - e; if (d < 0) d = -d;
    if (d > delta) {
        char m[128]; snprintf(m, sizeof m, "expected %lld+/-%lld actual %lld %s",
                              e, delta, a, msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
void ceedless_assert_within_uint_(unsigned long long delta, unsigned long long e,
                                unsigned long long a, const char *expr,
                                const char *file, int line, const char *msg)
{
    unsigned long long d = a > e ? a - e : e - a;
    if (d > delta) {
        char m[128]; snprintf(m, sizeof m, "expected %llu+/-%llu actual %llu %s",
                              e, delta, a, msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
void ceedless_assert_greater_int_(long long thr, long long a, int orequal,
                                const char *expr, const char *file, int line, const char *msg)
{
    int ok = orequal ? (a >= thr) : (a > thr);
    if (!ok) {
        char m[96]; snprintf(m, sizeof m, "thr %lld actual %lld %s", thr, a, msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
void ceedless_assert_less_int_(long long thr, long long a, int orequal,
                             const char *expr, const char *file, int line, const char *msg)
{
    int ok = orequal ? (a <= thr) : (a < thr);
    if (!ok) {
        char m[96]; snprintf(m, sizeof m, "thr %lld actual %lld %s", thr, a, msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
void ceedless_assert_bits_(unsigned long long mask, unsigned long long e,
                         unsigned long long a, const char *expr,
                         const char *file, int line, const char *msg)
{
    if ((e & mask) != (a & mask)) {
        char m[160]; snprintf(m, sizeof m,
            "mask 0x%llX expected 0x%llX actual 0x%llX %s",
            mask, e & mask, a & mask, msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
void ceedless_assert_equal_string_(const char *e, const char *a,
                                 const char *expr, const char *file, int line, const char *msg)
{
    if (e == a) return;
    if (!e || !a) { ceedless_fail_msg_(expr, file, line, msg?msg:"NULL string"); return; }
    if (strcmp(e, a) != 0) {
        char m[160]; snprintf(m, sizeof m, "expected \"%s\" actual \"%s\" %s",
                              e, a, msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
void ceedless_assert_equal_memory_(const void *e, const void *a, size_t n,
                                 const char *expr, const char *file, int line, const char *msg)
{
    if (e == a || n == 0) return;
    if (!e || !a) { ceedless_fail_msg_(expr, file, line, msg?msg:"NULL memory"); return; }
    if (memcmp(e, a, n) != 0) {
        const unsigned char *pe = e, *pa = a;
        size_t i = 0; while (i < n && pe[i] == pa[i]) i++;
        char m[160]; snprintf(m, sizeof m,
            "first diff at byte %zu (e=0x%02X a=0x%02X) %s",
            i, pe[i], pa[i], msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
void ceedless_assert_equal_int_array_(const long long *e, const void *a, size_t es,
                                    size_t n, int sign, const char *expr,
                                    const char *file, int line, const char *msg)
{
    const unsigned char *p = (const unsigned char *)a;
    for (size_t i = 0; i < n; i++) {
        long long v = 0;
        switch (es) {
        case 1: v = sign==1 ? *(const int8_t *)(p+i*es) : *(const uint8_t *)(p+i*es); break;
        case 2: v = sign==1 ? *(const int16_t*)(p+i*es) : *(const uint16_t*)(p+i*es); break;
        case 4: v = sign==1 ? (long long)*(const int32_t*)(p+i*es) : (long long)*(const uint32_t*)(p+i*es); break;
        case 8: v = sign==1 ? *(const int64_t*)(p+i*es) : (long long)*(const uint64_t*)(p+i*es); break;
        default: ceedless_fail_msg_(expr,file,line,"unsupported elem"); return;
        }
        if (v != e[i]) {
            char m[128]; snprintf(m, sizeof m,
                "at index %zu expected %lld actual %lld %s",
                i, e[i], v, msg?msg:"");
            ceedless_fail_msg_(expr, file, line, m);
        }
    }
}
void ceedless_assert_each_equal_int_(long long e, const void *a, size_t es,
                                   size_t n, int sign, const char *expr,
                                   const char *file, int line, const char *msg)
{
    const unsigned char *p = (const unsigned char *)a;
    for (size_t i = 0; i < n; i++) {
        long long v = 0;
        switch (es) {
        case 1: v = sign==1 ? *(const int8_t *)(p+i*es) : *(const uint8_t *)(p+i*es); break;
        case 2: v = sign==1 ? *(const int16_t*)(p+i*es) : *(const uint16_t*)(p+i*es); break;
        case 4: v = sign==1 ? (long long)*(const int32_t*)(p+i*es) : (long long)*(const uint32_t*)(p+i*es); break;
        case 8: v = sign==1 ? *(const int64_t*)(p+i*es) : (long long)*(const uint64_t*)(p+i*es); break;
        default: ceedless_fail_msg_(expr,file,line,"unsupported elem"); return;
        }
        if (v != e) {
            char m[128]; snprintf(m, sizeof m,
                "at index %zu expected %lld actual %lld %s",
                i, e, v, msg?msg:"");
            ceedless_fail_msg_(expr, file, line, m);
        }
    }
}
void ceedless_assert_equal_string_array_(const char *const *e, const char *const *a,
                                       size_t n, const char *expr,
                                       const char *file, int line, const char *msg)
{
    for (size_t i = 0; i < n; i++) {
        const char *se = e[i], *sa = a[i];
        if (se == sa) continue;
        if (!se || !sa || strcmp(se, sa) != 0) {
            char m[160]; snprintf(m, sizeof m,
                "at index %zu e=\"%s\" a=\"%s\" %s",
                i, se?se:"(null)", sa?sa:"(null)", msg?msg:"");
            ceedless_fail_msg_(expr, file, line, m);
        }
    }
}
void ceedless_assert_within_float_(double delta, double e, double a,
                                 const char *expr, const char *file, int line, const char *msg)
{
    int not_within = msg && strcmp(msg, "NOT_WITHIN") == 0;
    double d = a - e; if (d < 0) d = -d;
    int within = (d <= delta);
    if ((!not_within && !within) || (not_within && within)) {
        char m[160]; snprintf(m, sizeof m, "e=%g a=%g d=%g %s", e, a, delta, msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
void ceedless_assert_equal_float_(double e, double a, int is_double,
                                const char *expr, const char *file, int line, const char *msg)
{
    int negate = msg && strcmp(msg, "NOT_EQUAL") == 0;
    double eps = is_double ? 1e-12 : 1e-5;
    double ref = fabs(e); if (ref < 1.0) ref = 1.0;
    double tol = ref * eps;
    int equal = fabs(e - a) <= tol;
    if (e == 0.0 && a == 0.0) equal = 1;
    if ((!negate && !equal) || (negate && equal)) {
        char m[160]; snprintf(m, sizeof m, "e=%g a=%g tol=%g %s", e, a, tol, msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
void ceedless_assert_float_class_(double a, int want,
                                const char *expr, const char *file, int line, const char *msg)
{
    int is_inf  = isinf(a) != 0;
    int is_pinf = is_inf && a > 0;
    int is_ninf = is_inf && a < 0;
    int is_nan  = isnan(a) != 0;
    int is_det  = !is_inf && !is_nan;
    int ok = 0;
    switch (want) {
    case CEEDLESS_FC_INF:              ok =  is_pinf; break;
    case CEEDLESS_FC_NEG_INF:          ok =  is_ninf; break;
    case CEEDLESS_FC_NAN:              ok =  is_nan;  break;
    case CEEDLESS_FC_DETERMINATE:      ok =  is_det;  break;
    case CEEDLESS_FC_NOT_INF:          ok = !is_pinf; break;
    case CEEDLESS_FC_NOT_NEG_INF:      ok = !is_ninf; break;
    case CEEDLESS_FC_NOT_NAN:          ok = !is_nan;  break;
    case CEEDLESS_FC_NOT_DETERMINATE:  ok = !is_det;  break;
    default:                         ok = 0;        break;
    }
    if (!ok) {
        char m[96]; snprintf(m, sizeof m, "value=%g class want=%d %s", a, want, msg?msg:"");
        ceedless_fail_msg_(expr, file, line, m);
    }
}
