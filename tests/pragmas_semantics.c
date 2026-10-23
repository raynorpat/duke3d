#include "profile_test_support.h"
/*
 * Checks the engine's C fixed-point helpers (pragmas.h) against what the
 *  original x86 code computes. The reference functions below follow the
 *  instructions: imul gives the full 64-bit product, add/adc adds products
 *  modulo 2^64, shrd keeps bits [n, n+32) of that, and idiv truncates toward
 *  zero. The hand-worked rows pin the cases where C and x86 most easily part:
 *  negative values (arithmetic shift rounds down, division rounds toward
 *  zero) and values near INT32_MIN and INT32_MAX.
 */
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include "build/pragmas.h"

static int failures;

static void check(const char *what, int64_t got, int64_t want)
{
    if (got != want) {
        printf("FAIL %s: got %lld, want %lld\n", what, (long long)got, (long long)want);
        failures++;
    }
}

/* Bits [n, n+32) of a 64-bit two's-complement value, as shrd/sar leave them. */
static int32_t shrd64(uint64_t v, int n)
{
    return (int32_t)(uint32_t)(n == 64 ? (int64_t)v >> 63 : (uint64_t)((int64_t)v >> n));
}
static uint64_t imul64(int32_t a, int32_t b) { return (uint64_t)((int64_t)a * b); }
static int32_t ref_mulscale(int32_t a, int32_t b, int n) { return shrd64(imul64(a, b), n); }
static int32_t ref_dmulscale(int32_t a, int32_t b, int32_t c, int32_t d, int n)
{ return shrd64(imul64(a, b) + imul64(c, d), n); }
static int32_t ref_tmulscale(int32_t a, int32_t b, int32_t c, int32_t d, int32_t e, int32_t f, int n)
{ return shrd64(imul64(a, b) + imul64(c, d) + imul64(e, f), n); }
static int32_t ref_divscale(int32_t a, int32_t b, int n)
{ return (int32_t)((int64_t)((uint64_t)(int64_t)a << n) / b); }
static int32_t ref_scale(int32_t a, int32_t b, int32_t c) { return (int32_t)((int64_t)a * b / c); }

static const int32_t vals[] = {
    0, 1, -1, 2, -2, 3, -3, 7, -7, 255, -256, 65535, -65536, 65537,
    0x12345, -0x12345, 1000000, -1000000, 0x3fffffff, -0x40000000,
    INT32_MAX, INT32_MIN, INT32_MAX - 1, INT32_MIN + 1
};
#define NVALS ((int)(sizeof(vals) / sizeof(vals[0])))

int main(int argc, char **argv)
{
    if (!test_select_profile(&argc,argv,NULL)) return 1;
    char what[96];
    int i, j, k;

    /* Hand-worked rows. */
    check("mulscale16(0x10000,3)", mulscale16(0x10000, 3), 3);
    check("mulscale1(-3,1)", mulscale1(-3, 1), -2);          /* sar rounds down */
    check("mulscale16(-1,1)", mulscale16(-1, 1), -1);
    check("mulscale32(INT32_MIN,INT32_MIN)", mulscale32(INT32_MIN, INT32_MIN), 0x40000000);
    check("mulscale31(INT32_MAX,INT32_MAX)", mulscale31(INT32_MAX, INT32_MAX), 0x7ffffffe);
    check("divscale16(-1,2)", divscale16(-1, 2), -32768);
    check("divscale1(-3,4)", divscale1(-3, 4), -1);          /* -6/4 truncates to -1 */
    check("divscale2(-1,3)", divscale2(-1, 3), -1);          /* -4/3 truncates to -1 */
    check("divscale1(1,-3)", divscale1(1, -3), 0);
    check("scale(-7,1,2)", scale(-7, 1, 2), -3);             /* not -4 */
    check("scale(INT32_MAX,INT32_MAX,INT32_MAX)", scale(INT32_MAX, INT32_MAX, INT32_MAX), INT32_MAX);
    check("dmulscale32(MIN,MIN,MIN,MIN)", dmulscale32(INT32_MIN, INT32_MIN, INT32_MIN, INT32_MIN), INT32_MIN);
    check("klabs(-5)", klabs(-5), 5);
    check("klabs(INT32_MIN)", klabs(INT32_MIN), INT32_MIN);  /* neg leaves it */
    check("ksgn(-9)", ksgn(-9), -1);
    check("ksgn(0)", ksgn(0), 0);
    check("ksgn(INT32_MIN)", ksgn(INT32_MIN), -1);
    check("sqr(-46341)", sqr(-46341), (int32_t)(uint32_t)(46341u * 46341u));

    /* Every pair (and a spread of shifts) against the instruction model. */
    for (i = 0; i < NVALS; i++)
        for (j = 0; j < NVALS; j++) {
            int32_t a = vals[i], b = vals[j];
            for (k = 1; k <= 32; k++) {
                int64_t q;
                snprintf(what, sizeof(what), "mulscale(%d,%d,%d)", a, b, k);
                check(what, mulscale(a, b, k), ref_mulscale(a, b, k));
                snprintf(what, sizeof(what), "dmulscale(%d,%d,%d,%d,%d)", a, b, b, a, k);
                check(what, dmulscale(a, b, b, a, k), ref_dmulscale(a, b, b, a, k));
                snprintf(what, sizeof(what), "tmulscale(%d,%d,...,%d)", a, b, k);
                check(what, tmulscale(a, b, b, a, a, a, k), ref_tmulscale(a, b, b, a, a, a, k));
                if (b == 0) continue;
                /* idiv traps when the quotient doesn't fit; the game never relies on that. */
                q = (int64_t)((uint64_t)(int64_t)a << k);
                /* Skip the 64-bit overflow before computing the quotient. */
                if (q == INT64_MIN && b == -1) continue;
                q /= b;
                if (q < INT32_MIN || q > INT32_MAX) continue;
                snprintf(what, sizeof(what), "divscale(%d,%d,%d)", a, b, k);
                check(what, divscale(a, b, k), ref_divscale(a, b, k));
            }
            for (k = 0; k < NVALS; k++) {
                int64_t q;
                int32_t c = vals[k];
                if (c == 0) continue;
                q = (int64_t)a * b / c;
                if (q < INT32_MIN || q > INT32_MAX) continue;
                snprintf(what, sizeof(what), "scale(%d,%d,%d)", a, b, c);
                check(what, scale(a, b, c), ref_scale(a, b, c));
            }
        }

    if (failures) {
        printf("%d failures\n", failures);
        return 1;
    }
    printf("pragmas_semantics: all match\n");
    return 0;
}
