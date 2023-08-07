
/* @(#)e_acosh.c 1.3 95/01/18 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunSoft, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 *
 */

/* __ieee754_acosh(x)
 * Method :
 *	Based on
 *		acosh(x) = log [ x + sqrt(x*x-1) ]
 *	we have
 *		acosh(x) := log(x)+ln2,	if x is large; else
 *		acosh(x) := log(2x-1/(sqrt(x*x-1)+x)) if x>2; else
 *		acosh(x) := log1p(t+sqrt(2.0*t+t*t)); where t=x-1.
 *
 * Special cases:
 *	acosh(x) is NaN with signal if x<1.
 *	acosh(NaN) is NaN without signal.
 */

#include "fdlibm.h"

#ifdef __STDC__
static const double
#else
static double
#endif
        one	= 1.0,
        ln2	= 6.93147180559945286227e-01;  /* 0x3FE62E42, 0xFEFA39EF */

/*
* Definitions generated from Newton
*/
typedef double bmx055xAcceleration;

#ifdef __STDC__
double __ieee754_acosh(bmx055xAcceleration x)
#else
double __ieee754_acosh(x)
        bmx055xAcceleration x;
#endif
{
#ifdef ASSUME
    __builtin_assume(x > -16 && x < 16);
#endif
    double t;
    int hx;
    hx = __HI(x);
    if(hx<0x3ff00000) {		/* x < 1 */
        return (x-x)/(x-x);
    } else if(hx >=0x41b00000) {	/* x > 2**28 */
        if(hx >=0x7ff00000) {	/* x is inf of NaN */
            return x+x;
        } else
            return log(x)+ln2;	/* acosh(huge)=log(2x) */
    } else if(((hx-0x3ff00000)|__LO(x))==0) {
        return 0.0;			/* acosh(1) = 0 */
    } else if (hx > 0x40000000) {	/* 2**28 > x > 2 */
        t=x*x;
        return log(2.0*x-one/(x+sqrt(t-one)));
    } else {			/* 1<x<2 */
        t = x-one;
        return log1p(t+sqrt(2.0*t+t*t));
    }
}

// clang ../c-files/e_acosh.c -D DEBUG -D ASSUME -O3 -o e_acosh_assume -lm
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
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = __ieee754_acosh(xOps[idx]);
    }

    toc(&timer, "computation delay");

    printf("results: %f\t%f\t%f\t%f\t%f\n", result[0], result[1], result[2], result[3], result[4]);

    return 0;
}
#endif
