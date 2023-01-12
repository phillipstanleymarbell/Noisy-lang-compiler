/*
 * Unit testcase of function call
 * */
#include <stdio.h>
#include <stdlib.h>

#include "fdlibm.h"

typedef double bmx055xAcceleration; // [3 mjf, 10 mjf]
typedef double bmx055yAcceleration; // [15 mjf, 36 mjf]

double testCallee(double param1) {
    double ret;
    if (param1 > 15) {
        ret = param1 + 100;
    } else {
        ret = param1 * 100;
    }
    return ret;
}

double test(bmx055xAcceleration x, double y) {
    double ret1 = testCallee(x);
    double ret2 = testCallee(y+x);
    double ret3 = testCallee(x+15.786);
    return ret1 + ret2 + ret3;
}

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

#ifdef __STDC__
double __ieee754_exp(double x)	/* default IEEE double exp */
#else
double __ieee754_exp(x)	/* default IEEE double exp */
        double x;
#endif
{
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

#ifdef __STDC__
double __ieee754_exp_opt(double x)	/* default IEEE double exp */
#else
double __ieee754_exp_opt(x)	/* default IEEE double exp */
        double x;
#endif
{
    double c,t;

    /* x is now in primary range */
    t  = x*x;
    c  = x - t*(P1+t*(P2+t*(P3+t*(P4+t*P5))));
    return one-((x*c)/(c-2.0)-x);
}

double funcA(double x, double y) {
    double ret1, ret2;
//    for (size_t i = 0; i < 1000000; i++) {
//        ret1 += __ieee754_exp_opt(x+0.0000001*i)/10000;
//        ret2 += __ieee754_exp(y+0.0000001*i)/10000;
//    }
    ret1 += __ieee754_exp_opt(x)/10000;
    ret2 += __ieee754_exp(y)/10000;
    return ret1 + ret2;
}

int main(int argc, char** argv) {
    char* pEnd;
    bmx055xAcceleration x;
    bmx055yAcceleration y;
    if (argc == 3) {
        x = strtod(argv[1], &pEnd);
        y = strtod(argv[2], &pEnd);
    } else {
        x = 0.15;
        y = 15.781;
    }
    double res = funcA(x, y);
    printf("res = %f\n", res);
    return res;
}
