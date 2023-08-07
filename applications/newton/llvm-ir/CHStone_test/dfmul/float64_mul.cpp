/*
+--------------------------------------------------------------------------+
| CHStone : a suite of benchmark programs for C-based High-Level Synthesis |
| ======================================================================== |
|                                                                          |
| * Collected and Modified : Y. Hara, H. Tomiyama, S. Honda,               |
|                            H. Takada and K. Ishii                        |
|                            Nagoya University, Japan                      |
|                                                                          |
| * Remark :                                                               |
|    1. This source code is modified to unify the formats of the benchmark |
|       programs in CHStone.                                               |
|    2. Test vectors are added for CHStone.                                |
|    3. If "main_result" is 0 at the end of the program, the program is    |
|       correctly executed.                                                |
|    4. Please follow the copyright of each benchmark program.             |
+--------------------------------------------------------------------------+
*/
/*============================================================================

This C source file is part of the SoftFloat IEC/IEEE Floating-point Arithmetic
Package, Release 2b.

Written by John R. Hauser.  This work was made possible in part by the
International Computer Science Institute, located at Suite 600, 1947 Center
Street, Berkeley, California 94704.  Funding was partially provided by the
National Science Foundation under grant MIP-9311980.  The original version
of this code was written as part of a project to build a fixed-point vector
processor in collaboration with the University of California at Berkeley,
overseen by Profs. Nelson Morgan and John Wawrzynek.  More information
is available through the Web page `http://www.cs.berkeley.edu/~jhauser/
arithmetic/SoftFloat.html'.

THIS SOFTWARE IS DISTRIBUTED AS IS, FOR FREE.  Although reasonable effort has
been made to avoid it, THIS SOFTWARE MAY CONTAIN FAULTS THAT WILL AT TIMES
RESULT IN INCORRECT BEHAVIOR.  USE OF THIS SOFTWARE IS RESTRICTED TO PERSONS
AND ORGANIZATIONS WHO CAN AND WILL TAKE FULL RESPONSIBILITY FOR ALL LOSSES,
COSTS, OR OTHER PROBLEMS THEY INCUR DUE TO THE SOFTWARE, AND WHO FURTHERMORE
EFFECTIVELY INDEMNIFY JOHN HAUSER AND THE INTERNATIONAL COMPUTER SCIENCE
INSTITUTE (possibly via similar legal warning) AGAINST ALL LOSSES, COSTS, OR
OTHER PROBLEMS INCURRED BY THEIR CUSTOMERS AND CLIENTS DUE TO THE SOFTWARE.

Derivative works are acceptable, even for commercial purposes, so long as
(1) the source code for the derivative work includes prominent notice that
the work is derivative, and (2) the source code includes prominent notice with
these four paragraphs for those parts of this code that are retained.

=============================================================================*/


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "include/milieu.h"
#include "include/softfloat.h"

/*----------------------------------------------------------------------------
| Floating-point rounding mode, extended double-precision rounding precision,
| and exception flags.
*----------------------------------------------------------------------------*/
int8 float_rounding_mode = float_round_nearest_even;
int8 float_exception_flags = 0;

/*----------------------------------------------------------------------------
| Primitive arithmetic functions, including multi-word arithmetic, and
| division and square root approximations.  (Can be specialized to target if
| desired.)
*----------------------------------------------------------------------------*/
#include "include/softfloat-macros"

/*----------------------------------------------------------------------------
| Functions and definitions to determine:  (1) whether tininess for underflow
| is detected before or after rounding by default, (2) what (if anything)
| happens when exceptions are raised, (3) how signaling NaNs are distinguished
| from quiet NaNs, (4) the default generated quiet NaNs, and (5) how NaNs
| are propagated from function inputs to output.  These details are target-
| specific.
*----------------------------------------------------------------------------*/
#include "include/softfloat-specialize"


/*----------------------------------------------------------------------------
| Returns the fraction bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
bits64 extractFloat64Frac (float64 a) __attribute__ ((always_inline));
bits64 extractFloat64Frac (float64 a) {
  return a & LIT64 (0x000FFFFFFFFFFFFF);
}
#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
| Returns the exponent bits of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
int16 extractFloat64Exp (float64 a) __attribute__ ((always_inline));
int16 extractFloat64Exp (float64 a) {
  return (a >> 52) & 0x7FF;
}
#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
| Returns the sign bit of the double-precision floating-point value `a'.
*----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
flag extractFloat64Sign (float64 a) __attribute__ ((always_inline));
flag extractFloat64Sign (float64 a) {
  return a >> 63;
}
#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
| Normalizes the subnormal double-precision floating-point value represented
| by the denormalized significand `aSig'.  The normalized exponent and
| significand are stored at the locations pointed to by `zExpPtr' and
| `zSigPtr', respectively.
*----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
void normalizeFloat64Subnormal (bits64 aSig, int16 * zExpPtr, bits64 * zSigPtr) {
  int8 shiftCount;

  shiftCount = countLeadingZeros64 (aSig) - 11;
  *zSigPtr = aSig << shiftCount;
  *zExpPtr = 1 - shiftCount;

}
#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
| Packs the sign `zSign', exponent `zExp', and significand `zSig' into a
| double-precision floating-point value, returning the result.  After being
| shifted into the proper positions, the three fields are simply added
| together to form the result.  This means that any integer portion of `zSig'
| will be added into the exponent.  Since a properly normalized significand
| will have an integer portion equal to 1, the `zExp' input should be 1 less
| than the desired result exponent whenever `zSig' is a complete, normalized
| significand.
*----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
float64 packFloat64 (flag zSign, int16 zExp, bits64 zSig) __attribute__ ((always_inline));
float64 packFloat64 (flag zSign, int16 zExp, bits64 zSig) {
  return (((bits64) zSign) << 63) + (((bits64) zExp) << 52) + zSig;
}
#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
| Takes an abstract floating-point value having sign `zSign', exponent `zExp',
| and significand `zSig', and returns the proper double-precision floating-
| point value corresponding to the abstract input.  Ordinarily, the abstract
| value is simply rounded and packed into the double-precision format, with
| the inexact exception raised if the abstract input cannot be represented
| exactly.  However, if the abstract value is too large, the overflow and
| inexact exceptions are raised and an infinity or maximal finite value is
| returned.  If the abstract value is too small, the input value is rounded
| to a subnormal number, and the underflow and inexact exceptions are raised
| if the abstract input cannot be represented exactly as a subnormal double-
| precision floating-point number.
|     The input significand `zSig' has its binary point between bits 62
| and 61, which is 10 bits to the left of the usual location.  This shifted
| significand must be normalized or smaller.  If `zSig' is not normalized,
| `zExp' must be 0; in that case, the result returned is a subnormal number,
| and it must not require rounding.  In the usual case that `zSig' is
| normalized, `zExp' must be 1 less than the ``true'' floating-point exponent.
| The handling of underflow and overflow follows the IEC/IEEE Standard for
| Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif
float64 roundAndPackFloat64 (flag zSign, int16 zExp, bits64 zSig) {
  int8 roundingMode;
  flag roundNearestEven, isTiny;
  int16 roundIncrement, roundBits;

  roundingMode = float_rounding_mode;
  roundNearestEven = (roundingMode == float_round_nearest_even);
  roundIncrement = 0x200;
  if (!roundNearestEven)
    {
      if (roundingMode == float_round_to_zero)
	{
	  roundIncrement = 0;
	}
      else
	{
	  roundIncrement = 0x3FF;
	  if (zSign)
	    {
	      if (roundingMode == float_round_up)
		roundIncrement = 0;
	    }
	  else
	    {
	      if (roundingMode == float_round_down)
		roundIncrement = 0;
	    }
	}
    }
  roundBits = zSig & 0x3FF;
  if (0x7FD <= (bits16) zExp)
    {
      if ((0x7FD < zExp)
	  || ((zExp == 0x7FD) && ((sbits64) (zSig + roundIncrement) < 0)))
	{
	  float_raise (float_flag_overflow | float_flag_inexact);
	  return packFloat64 (zSign, 0x7FF, 0) - (roundIncrement == 0);
	}
      if (zExp < 0)
	{
	  isTiny = (float_detect_tininess == float_tininess_before_rounding)
	    || (zExp < -1)
	    || (zSig + roundIncrement < LIT64 (0x8000000000000000));
	  shift64RightJamming (zSig, -zExp, &zSig);
	  zExp = 0;
	  roundBits = zSig & 0x3FF;
	  if (isTiny && roundBits)
	    float_raise (float_flag_underflow);
	}
    }
  if (roundBits)
    float_exception_flags |= float_flag_inexact;
  zSig = (zSig + roundIncrement) >> 10;
  zSig &= ~(((roundBits ^ 0x200) == 0) & roundNearestEven);
  if (zSig == 0)
    zExp = 0;
  return packFloat64 (zSign, zExp, zSig);

}
#ifdef __cplusplus
}
#endif

/*----------------------------------------------------------------------------
| Returns the result of multiplying the double-precision floating-point values
| `a' and `b'.  The operation is performed according to the IEC/IEEE Standard
| for Binary Floating-Point Arithmetic.
*----------------------------------------------------------------------------*/
#ifdef __cplusplus
extern "C" {
#endif

typedef float64 bmx055xAcceleration;
typedef float64 bmx055yAcceleration;

#ifndef lowerBound
#define lowerBound 0
#endif
#ifndef upperBound
#define upperBound 16
#endif

float64 float64_mul (bmx055xAcceleration a, bmx055yAcceleration b) {
#ifdef ASSUME
    double aLowerBound = lowerBound, aUpperBound = upperBound;
    double bLowerBound = aLowerBound+0.6, bUpperBound = aUpperBound+0.3;
    bmx055xAcceleration llhs = *(bmx055xAcceleration*)(&aLowerBound);
    bmx055xAcceleration lrhs = *(bmx055xAcceleration*)(&aUpperBound);
    bmx055yAcceleration rlhs = *(bmx055xAcceleration*)(&bLowerBound);
    bmx055yAcceleration rrhs = *(bmx055xAcceleration*)(&bUpperBound);
    __builtin_assume(a > llhs && a < lrhs);
    __builtin_assume(b > rlhs && b < rrhs);
#endif
  flag aSign, bSign, zSign;
  int16 aExp, bExp, zExp;
  bits64 aSig, bSig, zSig0, zSig1;

  aSig = extractFloat64Frac (a);
  aExp = extractFloat64Exp (a);
  aSign = extractFloat64Sign (a);
  bSig = extractFloat64Frac (b);
  bExp = extractFloat64Exp (b);
  bSign = extractFloat64Sign (b);
  zSign = aSign ^ bSign;
  if (aExp == 0x7FF)
    {
      if (aSig || ((bExp == 0x7FF) && bSig))
	return propagateFloat64NaN (a, b);
      if ((bExp | bSig) == 0)
	{
	  float_raise (float_flag_invalid);
	  return float64_default_nan;
	}
      return packFloat64 (zSign, 0x7FF, 0);
    }
  if (bExp == 0x7FF)
    {
      if (bSig)
	return propagateFloat64NaN (a, b);
      if ((aExp | aSig) == 0)
	{
	  float_raise (float_flag_invalid);
	  return float64_default_nan;
	}
      return packFloat64 (zSign, 0x7FF, 0);
    }
  if (aExp == 0)
    {
      if (aSig == 0)
	return packFloat64 (zSign, 0, 0);
      normalizeFloat64Subnormal (aSig, &aExp, &aSig);
    }
  if (bExp == 0)
    {
      if (bSig == 0)
	return packFloat64 (zSign, 0, 0);
      normalizeFloat64Subnormal (bSig, &bExp, &bSig);
    }
  zExp = aExp + bExp - 0x3FF;
  aSig = (aSig | LIT64 (0x0010000000000000)) << 10;
  bSig = (bSig | LIT64 (0x0010000000000000)) << 11;
  mul64To128 (aSig, bSig, &zSig0, &zSig1);
  zSig0 |= (zSig1 != 0);
  if (0 <= (sbits64) (zSig0 << 1))
    {
      zSig0 <<= 1;
      --zExp;
    }
  return roundAndPackFloat64 (zSign, zExp, zSig0);

}
#ifdef __cplusplus
}
#endif

/*
+--------------------------------------------------------------------------+
| * Test Vectors (added for CHStone)                                       |
|     a_input, b_input : input data                                        |
|     z_output : expected output data                                      |
+--------------------------------------------------------------------------+
*/
#define N 20
const float64 a_input[N] = {
  0x7FF0000000000000ULL,	/* inf */
  0x7FFF000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x7FF0000000000000ULL,	/* inf */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x0000000000000000ULL,	/* 0.0 */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x0000000000000000ULL,	/* 0.0 */
  0x8000000000000000ULL,	/* -0.0 */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x4000000000000000ULL,	/* 2.0 */
  0x3FD0000000000000ULL,	/* 0.25 */
  0xC000000000000000ULL,	/* -2.0 */
  0xBFD0000000000000ULL,	/* -0.25 */
  0x4000000000000000ULL,	/* 2.0 */
  0xBFD0000000000000ULL,	/* -0.25 */
  0xC000000000000000ULL,	/* -2.0 */
  0x3FD0000000000000ULL,	/* 0.25 */
  0x0000000000000000ULL		/* 0.0 */
};

const float64 b_input[N] = {
  0xFFFFFFFFFFFFFFFFULL,	/* nan */
  0xFFF0000000000000ULL,	/* -inf */
  0x0000000000000000ULL,	/* nan */
  0x3FF0000000000000ULL,	/* -inf */
  0xFFFF000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x7FF0000000000000ULL,	/* inf */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x0000000000000000ULL,	/* 0.0 */
  0x8000000000000000ULL,	/* -0.0 */
  0x3FD0000000000000ULL,	/* 0.25 */
  0x4000000000000000ULL,	/* 2.0 */
  0xBFD0000000000000ULL,	/* -0.25 */
  0xC000000000000000ULL,	/* -2.0 */
  0xBFD0000000000000ULL,	/* -0.25 */
  0x4000000000000000ULL,	/* -2.0 */
  0x3FD0000000000000ULL,	/* 0.25 */
  0xC000000000000000ULL,	/* -2.0 */
  0x0000000000000000ULL		/* 0.0 */
};

const float64 z_output[N] = {
  0xFFFFFFFFFFFFFFFFULL,	/* nan */
  0x7FFF000000000000ULL,	/* nan */
  0x7FFFFFFFFFFFFFFFULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0xFFFF000000000000ULL,	/* nan */
  0x7FFFFFFFFFFFFFFFULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x0000000000000000ULL,	/* 0.0 */
  0x8000000000000000ULL,	/* -0.0 */
  0x0000000000000000ULL,	/* 0.0 */
  0x8000000000000000ULL,	/* -0.0 */
  0x3FE0000000000000ULL,	/* 0.5 */
  0x3FE0000000000000ULL,	/* 0.5 */
  0x3FE0000000000000ULL,	/* 0.5 */
  0x3FE0000000000000ULL,	/* 0.5 */
  0xBFE0000000000000ULL,	/* -0.5 */
  0xBFE0000000000000ULL,	/* -0.5 */
  0xBFE0000000000000ULL,	/* -0.5 */
  0xBFE0000000000000ULL,	/* -0.5 */
  0x0000000000000000ULL		/* 0.0 */
};

//int main(int argc, char **argv) {
//  int main_result;
//  int i;
//  float64 x1, x2;
//      main_result = 0;
//      for (i = 0; i < N; i++)
//	{
//	  float64 result;
//	  x1 = a_input[i];
//	  x2 = b_input[i];
//	  result = float64_mul (x1, x2);
//	  main_result += (result != z_output[i]);
//
//	  printf
//	    ("a_input=%016llx b_input=%016llx expected=%016llx output=%016llx\n",
//	     a_input[i], b_input[i], z_output[i], result);
//	}
//      printf ("%d\n", main_result);
//      return main_result;
//    }


// clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D ASSUME -D lowerBound=3 -D upperBound=10 -O3 -o float64_mul_assume -lm
#ifdef DEBUG

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define iteration_num 50000

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
static double
randomDouble(double min, double max)
{
    double randDbValue = min + 1.0 * rand() / RAND_MAX * (max - min);
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
    float64 result[iteration_num];
    double xOps[iteration_num];
    double yOps[iteration_num];
    for (size_t idx = 0; idx < iteration_num; idx++) {
        xOps[idx] = randomDouble(parameters[0], parameters[1]);
        yOps[idx] = randomDouble(parameters[0] + 0.6, parameters[1] + 0.3);
    }

    timespec timer = tic();
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = float64_mul(*(bmx055xAcceleration*)(&xOps[idx]), *(bmx055xAcceleration*)(&yOps[idx]));
    }

    toc(&timer, "computation delay");

    printf("results: %llx, %llx, %llx, %llx, %llx\n", result[0], result[1], result[2], result[3], result[4]);
//    printf("results: %f\t%f\t%f\t%f\t%f\n", *(double*)(&result[0]), *(double*)(&result[1]),
//           *(double*)(&result[2]), *(double*)(&result[3]), *(double*)(&result[4]));

    return 0;
}
#endif
