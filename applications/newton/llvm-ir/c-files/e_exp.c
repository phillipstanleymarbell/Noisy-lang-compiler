/* @(#)e_exp.c 5.1 93/09/24 */
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
 * 2. rename the function as `libc_exp` so that
 * it does not conflict with system libraries
 * @SHA: 12ad9a46dff86b7750aa9c8f8fca7349a0925114
 * ====================================================
 */

#include "fdlibm.h"
#include "math_config.h"
//#if __OBSOLETE_MATH

#ifndef _DOUBLE_IS_32BITS

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
typedef __uint32_t bmx055xMagneto;
typedef double bmx055xAcceleration;

#ifdef __STDC__
	double libc_exp(bmx055xAcceleration x)	/* default IEEE double exp */
#else
	double libc_exp(x)	/* default IEEE double exp */
    bmx055xAcceleration x;
#endif
{
	double y,hi,lo,c,t;
	__int32_t k = 0,xsb;
	__uint32_t hx;

	GET_HIGH_WORD(hx,x);
	xsb = (hx>>31)&1;		/* sign bit of x */
	hx &= 0x7fffffff;		/* high word of |x| */

    /* filter out non-finite argument */
	if(hx >= 0x40862E42) {			/* if |x|>=709.78... */
        if(hx>=0x7ff00000) {
	        __uint32_t lx;
		GET_LOW_WORD(lx,x);
		if(((hx&0xfffff)|lx)!=0) 
		     return x+x; 		/* NaN */
		else return (xsb==0)? x:0.0;	/* exp(+-inf)={inf,0} */
	    }
	    if(x > o_threshold) return -1; /* overflow */
	    if(x < u_threshold) return -2; /* underflow */
	}

    /* argument reduction */
	if(hx > 0x3fd62e42) {		/* if  |x| > 0.5 ln2; hx > 1,071,001,154 */
	    if(hx < 0x3FF0A2B2) {	/* and |x| < 1.5 ln2; hx < 1,072,734,898 */
		hi = x-ln2HI[xsb]; lo=ln2LO[xsb]; k = 1-xsb-xsb;
	    } else {
		k  = invln2*x+halF[xsb];
		t  = k;
		hi = x - t*ln2HI[0];	/* t*ln2HI is exact here */
		lo = t*ln2LO[0];
	    }
	    x  = hi - lo;
	} 
	else if(hx < 0x3df00000)  {	/* when |x|<2**-32 */
	    if(huge+x>one) return one+x;/* trigger inexact */
	}

    /* x is now in primary range */
	t  = x*x;
	c  = x - t*(P1+t*(P2+t*(P3+t*(P4+t*P5))));
	if(k==0) 	return one-((x*c)/(c-2.0)-x); 
	else 		y = one-((lo-(x*c)/(2.0-c))-hi);
	if(k >= -1021) {
	    __uint32_t hy;
	    GET_HIGH_WORD(hy,y);
	    SET_HIGH_WORD(y,hy+(k<<20));	/* add k to y's exponent */
	    return y;
	} else {
	    __uint32_t hy;
	    GET_HIGH_WORD(hy,y);
	    SET_HIGH_WORD(y,hy+((k+1000)<<20));	/* add k to y's exponent */
	    return y*twom1000;
	}
}

#endif /* defined(_DOUBLE_IS_32BITS) */
//#endif /* __OBSOLETE_MATH */
