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

#ifdef __STDC__
double libc_acosh(double x)
#else
double libc_acosh(x)
        double x;
#endif
{
    double t;
    bmx055xMagneto hx;
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
