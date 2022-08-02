/* @(#)e_acosh.c 5.1 93/09/24 */
/*
 * ====================================================
 * Copyright (C) 1993 by Sun Microsystems, Inc. All rights reserved.
 *
 * Developed at SunPro, a Sun Microsystems, Inc. business.
 * Permission to use, copy, modify, and distribute this
 * software is freely granted, provided that this notice
 * is preserved.
 * ====================================================
 *
 * ====================================================
 * @Name: Pei Mu
 * @Modifications:
 * 1. change the type of `hx` into
 * a physical type `bmx055xMagneto`
 * 2. rename the function as `libc_acosh` so that
 * it does not conflict with system libraries
 * @SHA: 8a0efa53e44919bcf5ccb1d3353618a82afdf8bc
 * ====================================================
 */

#include "fdlibm.h"

#ifndef _DOUBLE_IS_32BITS

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
typedef __uint32_t bmx055xMagneto;
typedef double bmx055xAcceleration;

#ifdef __STDC__
double libc_acosh(bmx055xAcceleration x)
#else
double libc_acosh(x)
        bmx055xAcceleration x;
#endif
{
    double t;
    __uint32_t hx;
    __uint32_t lx;
    EXTRACT_WORDS(hx,lx,x);
    if(hx<0x3ff00000) {		/* hx < 1072693248; x < 1 */
        return (x-x)/(x-x);
    } else if(hx >=0x41b00000) {	/* hx >= 1102053376; x > 2**28 */
        if(hx >=0x7ff00000) {	/* hx >= 2146435072; x is inf of NaN */
            return x+x;
        } else
            return log(x)+ln2;	/* acosh(huge)=log(2x) */
    } else if(((hx-0x3ff00000)|lx)==0) {
        return 0.0;			/* acosh(1) = 0 */
    } else if (hx > 0x40000000) {	/* hx > 1073741824; 2**28 > x > 2 */
        t=x*x;
        return log(2.0*x-one/(x+sqrt(t-one)));
    } else {			/* 1<x<2 */
        t = x-one;
        return log1p(t+sqrt(2.0*t+t*t));
    }
}

#endif /* defined(_DOUBLE_IS_32BITS) */
