/* Header for single-precision sin/cos/sincos functions.
   Copyright (c) 2018 Arm Ltd.  All rights reserved.
   SPDX-License-Identifier: BSD-3-Clause
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
   3. The name of the company may not be used to endorse or promote
      products derived from this software without specific prior written
      permission.
   THIS SOFTWARE IS PROVIDED BY ARM LTD ``AS IS'' AND ANY EXPRESS OR IMPLIED
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL ARM LTD BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. */

#include <stdint.h>
#include <math.h>
#include "math_config.h"

/* 2PI * 2^-64.  */
static const double pi63 = 0x1.921FB54442D18p-62;
/* PI / 4.  */
static const double pio4 = 0x1.921FB54442D18p-1;

/* The constants and polynomials for sine and cosine.  */
typedef struct
{
    double sign[4];		/* Sign of sine in quadrants 0..3.  */
    double hpi_inv;		/* 2 / PI ( * 2^24 if !TOINT_INTRINSICS).  */
    double hpi;			/* PI / 2.  */
    double c0, c1, c2, c3, c4;	/* Cosine polynomial.  */
    double s1, s2, s3;		/* Sine polynomial.  */
} sincos_t;

/* Polynomial data (the cosine polynomial is negated in the 2nd entry).  */
extern const sincos_t __sincosf_table[2] HIDDEN;

/* Table with 4/PI to 192 bit precision.  */
extern const uint32_t __inv_pio4[] HIDDEN;

/* Top 12 bits of the float representation with the sign bit cleared.  */
static inline uint32_t
abstop12 (float x)
{
    return (asuint (x) >> 20) & 0x7ff;
}

/* Compute the sine and cosine of inputs X and X2 (X squared), using the
   polynomial P and store the results in SINP and COSP.  N is the quadrant,
   if odd the cosine and sine polynomials are swapped.  */
static inline void
sincosf_poly (double x, double x2, const sincos_t *p, int n, float *sinp,
              float *cosp)
{
    double x3, x4, x5, x6, s, c, c1, c2, s1;

    x4 = x2 * x2;
    x3 = x2 * x;
    c2 = p->c3 + x2 * p->c4;
    s1 = p->s2 + x2 * p->s3;

    /* Swap sin/cos result based on quadrant.  */
    float *tmp = (n & 1 ? cosp : sinp);
    cosp = (n & 1 ? sinp : cosp);
    sinp = tmp;

    c1 = p->c0 + x2 * p->c1;
    x5 = x3 * x2;
    x6 = x4 * x2;

    s = x + x3 * p->s1;
    c = c1 + x4 * p->c2;

    *sinp = s + x5 * s1;
    *cosp = c + x6 * c2;
}

/* Return the sine of inputs X and X2 (X squared) using the polynomial P.
   N is the quadrant, and if odd the cosine polynomial is used.  */
static inline float
sinf_poly (double x, double x2, const sincos_t *p, int n)
{
    double x3, x4, x6, x7, s, c, c1, c2, s1;

    if ((n & 1) == 0)
    {
        x3 = x * x2;
        s1 = p->s2 + x2 * p->s3;

        x7 = x3 * x2;
        s = x + x3 * p->s1;

        return s + x7 * s1;
    }
    else
    {
        x4 = x2 * x2;
        c2 = p->c3 + x2 * p->c4;
        c1 = p->c0 + x2 * p->c1;

        x6 = x4 * x2;
        c = c1 + x4 * p->c2;

        return c + x6 * c2;
    }
}

/* Fast range reduction using single multiply-subtract.  Return the modulo of
   X as a value between -PI/4 and PI/4 and store the quadrant in NP.
   The values for PI/2 and 2/PI are accessed via P.  Since PI/2 as a double
   is accurate to 55 bits and the worst-case cancellation happens at 6 * PI/4,
   the result is accurate for |X| <= 120.0.  */
static inline double
reduce_fast (double x, const sincos_t *p, int *np)
{
    double r;
#if TOINT_INTRINSICS
    /* Use fast round and lround instructions when available.  */
  r = x * p->hpi_inv;
  *np = converttoint (r);
  return x - roundtoint (r) * p->hpi;
#else
    /* Use scaled float to int conversion with explicit rounding.
       hpi_inv is prescaled by 2^24 so the quadrant ends up in bits 24..31.
       This avoids inaccuracies introduced by truncating negative values.  */
    r = x * p->hpi_inv;
    int n = ((int32_t)r + 0x800000) >> 24;
    *np = n;
    return x - n * p->hpi;
#endif
}

/* Reduce the range of XI to a multiple of PI/2 using fast integer arithmetic.
   XI is a reinterpreted float and must be >= 2.0f (the sign bit is ignored).
   Return the modulo between -PI/4 and PI/4 and store the quadrant in NP.
   Reduction uses a table of 4/PI with 192 bits of precision.  A 32x96->128 bit
   multiply computes the exact 2.62-bit fixed-point modulo.  Since the result
   can have at most 29 leading zeros after the binary point, the double
   precision result is accurate to 33 bits.  */
static inline double
reduce_large (uint32_t xi, int *np)
{
    const uint32_t *arr = &__inv_pio4[(xi >> 26) & 15];
    int shift = (xi >> 23) & 7;
    uint64_t n, res0, res1, res2;

    xi = (xi & 0xffffff) | 0x800000;
    xi <<= shift;

    res0 = xi * arr[0];
    res1 = (uint64_t)xi * arr[4];
    res2 = (uint64_t)xi * arr[8];
    res0 = (res2 >> 32) | (res0 << 32);
    res0 += res1;

    n = (res0 + (1ULL << 61)) >> 62;
    res0 -= n << 62;
    double x = (int64_t)res0;
    *np = n;
    return x * pi63;
}

/* The constants and polynomials for sine and cosine.  The 2nd entry
   computes -cos (x) rather than cos (x) to get negation for free.  */
const sincos_t __sincosf_table[2] =
        {
                {
                        { 1.0, -1.0, -1.0, 1.0 },
#if TOINT_INTRINSICS
                        0x1.45F306DC9C883p-1,
#else
                        0x1.45F306DC9C883p+23,
#endif
                        0x1.921FB54442D18p0,
                        0x1p0,
                        -0x1.ffffffd0c621cp-2,
                        0x1.55553e1068f19p-5,
                        -0x1.6c087e89a359dp-10,
                        0x1.99343027bf8c3p-16,
                        -0x1.555545995a603p-3,
                        0x1.1107605230bc4p-7,
                        -0x1.994eb3774cf24p-13
                },
                {
                        { 1.0, -1.0, -1.0, 1.0 },
#if TOINT_INTRINSICS
                        0x1.45F306DC9C883p-1,
#else
                        0x1.45F306DC9C883p+23,
#endif
                        0x1.921FB54442D18p0,
                        -0x1p0,
                        0x1.ffffffd0c621cp-2,
                        -0x1.55553e1068f19p-5,
                        0x1.6c087e89a359dp-10,
                        -0x1.99343027bf8c3p-16,
                        -0x1.555545995a603p-3,
                        0x1.1107605230bc4p-7,
                        -0x1.994eb3774cf24p-13
                }
        };

/* Table with 4/PI to 192 bit precision.  To avoid unaligned accesses
   only 8 new bits are added per entry, making the table 4 times larger.  */
const uint32_t __inv_pio4[24] =
        {
                0xa2,       0xa2f9,	  0xa2f983,   0xa2f9836e,
                0xf9836e4e, 0x836e4e44, 0x6e4e4415, 0x4e441529,
                0x441529fc, 0x1529fc27, 0x29fc2757, 0xfc2757d1,
                0x2757d1f5, 0x57d1f534, 0xd1f534dd, 0xf534ddc0,
                0x34ddc0db, 0xddc0db62, 0xc0db6295, 0xdb629599,
                0x6295993c, 0x95993c43, 0x993c4390, 0x3c439041
        };
