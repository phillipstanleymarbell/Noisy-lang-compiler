/* Single-precision sincos function.
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

#include "fdlibm.h"
//#if !__OBSOLETE_MATH

#include <stdint.h>
#include <math.h>
#include "math_config.h"
#include "sincosf.h"

/*
* Definitions generated from Newton
*/
typedef double bmx055xAcceleration;

/* Fast sincosf implementation.  Worst-case ULP is 0.5607, maximum relative
   error is 0.5303 * 2^-23.  A single-step range reduction is used for
   small values.  Large inputs have their range reduced using fast integer
   arithmetic.  */
void
libc_sincosf (bmx055xAcceleration y, float *sinp, float *cosp)
{
#ifdef ASSUME
    __builtin_assume(y > -16 && y < 16);
#endif
    double x = y;
    double s;
    int n;
    const sincos_t *p = &__sincosf_table[0];

    if (abstop12 (y) < abstop12 (pio4))
    {
        double x2 = x * x;

        if (unlikely (abstop12 (y) < abstop12 (0x1p-12f)))
        {
            if (unlikely (abstop12 (y) < abstop12 (0x1p-126f)))
                /* Force underflow for tiny y.  */
                force_eval_float (x2);
            *sinp = y;
            *cosp = 1.0f;
            return;
        }

        sincosf_poly (x, x2, p, 0, sinp, cosp);
    }
    // [0, 5]                  [6, 10]
    else if (abstop12 (y) < abstop12 (120.0f))
    {
        x = reduce_fast (x, p, &n);

        /* Setup the signs for sin and cos.  */
        s = p->sign[n & 3];

        if (n & 2)
            p = &__sincosf_table[1];

        sincosf_poly (x * s, x * x, p, n, sinp, cosp);
    }
    else if (likely (abstop12 (y) < abstop12 (INFINITY)))
    {
        uint32_t xi = asuint (y);
        int sign = xi >> 31;

        x = reduce_large (xi, &n);

        /* Setup signs for sin and cos - include original sign.  */
        s = p->sign[(n + sign) & 3];

        if ((n + sign) & 2)
            p = &__sincosf_table[1];

        sincosf_poly (x * s, x * x, p, n, sinp, cosp);
    }
    else
    {
        /* Return NaN if Inf or NaN for both sin and cos.  */
        *sinp = *cosp = y - y;
#if WANT_ERRNO
        /* Needed to set errno for +-Inf, the add is a hack to work
       around a gcc register allocation issue: just passing y
       affects code generation in the fast path.  */
//        __math_invalidf (y + y);
#endif
    }
}

// clang ../c-files/sincosf.c -D DEBUG -D ASSUME -O3 -o sincosf_assume -lm
#ifdef DEBUG

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define iteration_num 500000

typedef struct timespec timespec;
timespec diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec-start.tv_nsec)<0) {
        temp.tv_sec = end.tv_sec-start.tv_sec-1;
        temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
    } else {
        temp.tv_sec = end.tv_sec-start.tv_sec;
        temp.tv_nsec = end.tv_nsec-start.tv_nsec;
    }
    return temp;
}

timespec sum(timespec t1, timespec t2) {
    timespec temp;
    if (t1.tv_nsec + t2.tv_nsec >= 1000000000) {
        temp.tv_sec = t1.tv_sec + t2.tv_sec + 1;
        temp.tv_nsec = t1.tv_nsec + t2.tv_nsec - 1000000000;
    } else {
        temp.tv_sec = t1.tv_sec + t2.tv_sec;
        temp.tv_nsec = t1.tv_nsec + t2.tv_nsec;
    }
    return temp;
}

void printTimeSpec(timespec t, const char* prefix) {
    printf("%s: %d.%09d\n", prefix, (int)t.tv_sec, (int)t.tv_nsec);
}

timespec tic( )
{
    timespec start_time;
    clock_gettime(CLOCK_REALTIME, &start_time);
    return start_time;
}

void toc( timespec* start_time, const char* prefix )
{
    timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);
    printTimeSpec( diff( *start_time, current_time ), prefix );
    *start_time = current_time;
}

/*
 * random floating point, [min, max]
 * */
static bmx055xAcceleration
randomDouble(bmx055xAcceleration min, bmx055xAcceleration max)
{
    bmx055xAcceleration randDbValue = min + 1.0 * rand() / RAND_MAX * (max - min);
    return randDbValue;
}

int main(int argc, char** argv) {
    double parameters[2];
    char *pEnd;
    if (argc == 3) {
        for (size_t idx = 0; idx < argc - 1; idx++) {
            parameters[idx] = strtod(argv[idx + 1], &pEnd);
        }
    } else {
        parameters[0] = 3.0;
        parameters[1] = 10.0;
    }
    double result[iteration_num];
    bmx055xAcceleration xOps[iteration_num];
    for (size_t idx = 0; idx < iteration_num; idx++) {
        xOps[idx] = randomDouble(parameters[0], parameters[1]);
    }

    timespec timer = tic();
    float sinp, cosp;
    for (size_t idx = 0; idx < iteration_num; idx++) {
        sinp = cosp = 0;
        libc_sincosf(xOps[idx], &sinp, &cosp);
        result[idx] = sinp;
    }

    toc(&timer, "computation delay");

    printf("results: %f\t%f\t%f\t%f\t%f\n", result[0], result[1], result[2], result[3], result[4]);

    return 0;
}
#endif
