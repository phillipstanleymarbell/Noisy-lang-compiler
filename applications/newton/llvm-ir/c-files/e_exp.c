
/* @(#)e_exp.c 1.6 04/04/22 */
/*
 * ====================================================
 * Copyright (C) 2004 by Sun Microsystems, Inc. All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 */

/* __ieee754_exp(x)
 * Returns the exponential of x.
 *
 * Method
 *   1. Argument reduction:
 *      Reduce x to an r so that |r| <= 0.5*ln2 ~ 0.34658.
 *	Given x, find r and integer k such that
 *
 *               x = k*ln2 + r,  |r| <= 0.5*ln2.
 *
 *      Here r will be represented as r = hi-lo for better
 *	accuracy.
 *
 *   2. Approximation of exp(r) by a special rational function on
 *	the interval [0,0.34658]:
 *	Write
 *	    R(r**2) = r*(exp(r)+1)/(exp(r)-1) = 2 + r*r/6 - r**4/360 + ...
 *      We use a special Remes algorithm on [0,0.34658] to generate
 * 	a polynomial of degree 5 to approximate R. The maximum error
 *	of this polynomial approximation is bounded by 2**-59. In
 *	other words,
 *	    R(z) ~ 2.0 + P1*z + P2*z**2 + P3*z**3 + P4*z**4 + P5*z**5
 *  	(where z=r*r, and the values of P1 to P5 are listed below)
 *	and
 *	    |                  5          |     -59
 *	    | 2.0+P1*z+...+P5*z   -  R(z) | <= 2
 *	    |                             |
 *	The computation of exp(r) thus becomes
 *                             2*r
 *		exp(r) = 1 + -------
 *		              R - r
 *                                 r*R1(r)
 *		       = 1 + r + ----------- (for better accuracy)
 *		                  2 - R1(r)
 *	where
 *			         2       4             10
 *		R1(r) = r - (P1*r  + P2*r  + ... + P5*r   ).
 *
 *   3. Scale back to obtain exp(x):
 *	From step 1, we have
 *	   exp(x) = 2^k * exp(r)
 *
 * Special cases:
 *	exp(INF) is INF, exp(NaN) is NaN;
 *	exp(-INF) is 0, and
 *	for finite argument, only exp(0)=1 is exact.
 *
 * Accuracy:
 *	according to an error analysis, the error is always less than
 *	1 ulp (unit in the last place).
 *
 * Misc. info.
 *	For IEEE double
 *	    if x >  7.09782712893383973096e+02 then exp(x) overflow
 *	    if x < -7.45133219101941108420e+02 then exp(x) underflow
 *
 * Constants:
 * The hexadecimal values are the intended ones for the following
 * constants. The decimal values may be used, provided that the
 * compiler will convert from decimal to binary accurately enough
 * to produce the hexadecimal values shown.
 */

#include "fdlibm.h"

#ifdef __STDC__
static const double
#else
static double
#endif
        one	= 1.0,
        halF[2]	= {0.5,-0.5,},
        huge	= 1.0e+300,
        twom1000= 9.33263618503218878990e-302,     /* 2**-1000=0x01700000,0*/
o_threshold=  7.09782712893383973096e+02,  /* 0x40862E42, 0xFEFA39EF */
u_threshold= -7.45133219101941108420e+02,  /* 0xc0874910, 0xD52D3051 */
ln2HI[2]   ={ 6.93147180369123816490e-01,  /* 0x3fe62e42, 0xfee00000 */
              -6.93147180369123816490e-01,},/* 0xbfe62e42, 0xfee00000 */
ln2LO[2]   ={ 1.90821492927058770002e-10,  /* 0x3dea39ef, 0x35793c76 */
              -1.90821492927058770002e-10,},/* 0xbdea39ef, 0x35793c76 */
invln2 =  1.44269504088896338700e+00, /* 0x3ff71547, 0x652b82fe */
P1   =  1.66666666666666019037e-01, /* 0x3FC55555, 0x5555553E */
P2   = -2.77777777770155933842e-03, /* 0xBF66C16C, 0x16BEBD93 */
P3   =  6.61375632143793436117e-05, /* 0x3F11566A, 0xAF25DE2C */
P4   = -1.65339022054652515390e-06, /* 0xBEBBBD41, 0xC5D26BF1 */
P5   =  4.13813679705723846039e-08; /* 0x3E663769, 0x72BEA4D0 */

/*
* Definitions generated from Newton
*/
typedef double bmx055xAcceleration;

#ifdef __STDC__
double __ieee754_exp(bmx055xAcceleration x)	/* default IEEE double exp */
#else
double __ieee754_exp(x)	/* default IEEE double exp */
        bmx055xAcceleration x;
#endif
{
#ifdef ASSUME
    __builtin_assume(x > -16 && x < 16);
#endif
    double y,hi,lo,c,t;
    int k,xsb;
    unsigned hx;

    hx  = __HI(x);	/* high word of x */
    xsb = (hx>>31)&1;		/* sign bit of x */
    hx &= 0x7fffffff;		/* high word of |x| */

    /* filter out non-finite argument */
    if(hx >= 0x40862E42) {			/* if |x|>=709.78... */
        if(hx>=0x7ff00000) {
            if(((hx&0xfffff)|__LO(x))!=0)
                return x+x; 		/* NaN */
            else return (xsb==0)? x:0.0;	/* exp(+-inf)={inf,0} */
        }
        if(x > o_threshold) return huge*huge; /* overflow */
        if(x < u_threshold) return twom1000*twom1000; /* underflow */
    }

    /* argument reduction */
    if(hx > 0x3fd62e42) {		/* if  |x| > 0.5 ln2 ~= 0.34657359 */
        if(hx < 0x3FF0A2B2) {	/* and |x| < 1.5 ln2 ~= 1.039720771 */
            hi = x-ln2HI[xsb]; lo=ln2LO[xsb]; k = 1-xsb-xsb;
        } else {
            k  = (int)(invln2*x+halF[xsb]);
            t  = k;
            hi = x - t*ln2HI[0];	/* t*ln2HI is exact here */
            lo = t*ln2LO[0];
        }
        x  = hi - lo;
    }
    else if(hx < 0x3e300000)  {	/* when |x|<2**-28 */
        if(huge+x>one) return one+x;/* trigger inexact */
    }
    else k = 0;

    /* x is now in primary range */
    t  = x*x;
    c  = x - t*(P1+t*(P2+t*(P3+t*(P4+t*P5))));
    if(k==0) 	return one-((x*c)/(c-2.0)-x);
    else 		y = one-((lo-(x*c)/(2.0-c))-hi);
    if(k >= -1021) {
        __HI(y) += (k<<20);	/* add k to y's exponent */
        return y;
    } else {
        __HI(y) += ((k+1000)<<20);/* add k to y's exponent */
        return y*twom1000;
    }
}

// clang ../c-files/e_exp.c -D DEBUG -D ASSUME -O3 -o e_exp_assume
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
        result[idx] = __ieee754_exp(xOps[idx]);
    }

    toc(&timer, "computation delay");

    printf("results: %f\t%f\t%f\t%f\t%f\n", result[0], result[1], result[2], result[3], result[4]);

    return 0;
}
#endif
