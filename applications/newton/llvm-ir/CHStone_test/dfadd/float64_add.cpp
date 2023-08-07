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
/*
 * Copyright (C) 2008
 * Y. Hara, H. Tomiyama, S. Honda, H. Takada and K. Ishii
 * Nagoya University, Japan
 * All rights reserved.
 *
 * Disclaimer of Warranty
 *
 * These software programs are available to the user without any license fee or
 * royalty on an "as is" basis. The authors disclaims any and all warranties, 
 * whether express, implied, or statuary, including any implied warranties or 
 * merchantability or of fitness for a particular purpose. In no event shall the
 * copyright-holder be liable for any incidental, punitive, or consequential damages
 * of any kind whatsoever arising from the use of these programs. This disclaimer
 * of warranty extends to the user of these programs and user's customers, employees,
 * agents, transferees, successors, and assigns.
 *
 */
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

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

#include "include/softfloat.c"
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
#define N 46
const float64 a_input[N] = {
  0x7FF8000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x4000000000000000ULL,	/* 2.0 */
  0x4000000000000000ULL,	/* 2.0 */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x0000000000000000ULL,	/* 0.0 */
  0x3FF8000000000000ULL,	/* 1.5 */
  0x7FF8000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x0000000000000000ULL,	/* 0.0 */
  0x3FF8000000000000ULL,	/* 1.5 */
  0xFFF8000000000000ULL,	/* nan */
  0xFFF0000000000000ULL,	/* -inf */
  0xC000000000000000ULL,	/* -2.0 */
  0xC000000000000000ULL,	/* -2.0 */
  0xBFF0000000000000ULL,	/* -1.0 */
  0xBFF0000000000000ULL,	/* -1.0 */
  0x8000000000000000ULL,	/* -0.0 */
  0xBFF8000000000000ULL,	/* -1.5 */
  0xFFF8000000000000ULL,	/* nan */
  0xFFF0000000000000ULL,	/* -inf */
  0x8000000000000000ULL,	/* -0.0 */
  0xBFF8000000000000ULL,	/* -1.5 */
  0x7FF8000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x0000000000000000ULL,	/* 0.0 */
  0x3FF8000000000000ULL,	/* 1.5 */
  0x7FF8000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x4000000000000000ULL,	/* 2.0 */
  0xFFF0000000000000ULL,	/* -inf */
  0xFFF0000000000000ULL,	/* -inf */
  0xBFF0000000000000ULL,	/* -1.0 */
  0xBFF0000000000000ULL,	/* -1.0 */
  0xBFF0000000000000ULL,	/* -1.0 */
  0x8000000000000000ULL,	/* -0.0 */
  0xBFF8000000000000ULL,	/* -1.5 */
  0xFFF8000000000000ULL,	/* nan */
  0xFFF0000000000000ULL,	/* -inf */
  0xBFF0000000000000ULL,	/* -1.0 */
  0xC000000000000000ULL		/* -2.0 */
};

const float64 b_input[N] = {
  0x3FF0000000000000ULL,	/* 1.0 */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x0000000000000000ULL,	/* 0.0 */
  0x3FF8000000000000ULL,	/* 1.5 */
  0x7FF8000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x4000000000000000ULL,	/* 2.0 */
  0x4000000000000000ULL,	/* 2.0 */
  0x7FF0000000000000ULL,	/* inf */
  0x7FF0000000000000ULL,	/* inf */
  0x0000000000000000ULL,	/* 0.0 */
  0x3FF0000000000000ULL,	/* 1.0 */
  0xBFF0000000000000ULL,	/* -1.0 */
  0xBFF0000000000000ULL,	/* -1.0 */
  0x8000000000000000ULL,	/* -0.0 */
  0xBFF8000000000000ULL,	/* -1.5 */
  0xFFF8000000000000ULL,	/* nan */
  0xFFF0000000000000ULL,	/* -inf */
  0xC000000000000000ULL,	/* -2.0 */
  0xC000000000000000ULL,	/* -2.0 */
  0xFFF0000000000000ULL,	/* -inf */
  0xFFF0000000000000ULL,	/* -inf */
  0x8000000000000000ULL,	/* -inf */
  0xBFF0000000000000ULL,	/* -1.0 */
  0xFFF0000000000000ULL,	/* -inf */
  0xFFF0000000000000ULL,	/* -inf */
  0xBFF0000000000000ULL,	/* -1.0 */
  0xFFF8000000000000ULL,	/* nan */
  0xFFF0000000000000ULL,	/* -inf */
  0xBFF0000000000000ULL,	/* -1.0 */
  0xC000000000000000ULL,	/* -2.0 */
  0xBFF0000000000000ULL,	/* -1.0 */
  0xBFF0000000000000ULL,	/* -1.0 */
  0x8000000000000000ULL,	/* -0.0 */
  0xBFF8000000000000ULL,	/* -1.5 */
  0x7FF8000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x7FF8000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x4000000000000000ULL,	/* 2.0 */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x0000000000000000ULL,	/* 0.0 */
  0x3FF8000000000000ULL		/* 1.5 */
};

const float64 z_output[N] = {
  0x7FF8000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x4000000000000000ULL,	/* 2.0 */
  0x400C000000000000ULL,	/* 3.5 */
  0x7FF8000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x4000000000000000ULL,	/* 2.0 */
  0x400C000000000000ULL,	/* 3.5 */
  0x7FF8000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x0000000000000000ULL,	/* 0.0 */
  0x4004000000000000ULL,	/* 2.5 */
  0xFFF8000000000000ULL,	/* nan */
  0xFFF0000000000000ULL,	/* -inf */
  0xC000000000000000ULL,	/* -2.0 */
  0xC00C000000000000ULL,	/* -3.5 */
  0xFFF8000000000000ULL,	/* nan */
  0xFFF0000000000000ULL,	/* -inf */
  0xC000000000000000ULL,	/* -2.0 */
  0xC00C000000000000ULL,	/* -3.5 */
  0xFFF8000000000000ULL,	/* nan */
  0xFFF0000000000000ULL,	/* -inf */
  0x8000000000000000ULL,	/* -0.0 */
  0xC004000000000000ULL,	/* -2.5 */
  0x7FF8000000000000ULL,	/* nan */
  0x7FFFFFFFFFFFFFFFULL,	/* nan */
  0x0000000000000000ULL,	/* 0.0 */
  0xFFF8000000000000ULL,	/* nan */
  0xFFF0000000000000ULL,	/* -inf */
  0xBFF0000000000000ULL,	/* -1.0 */
  0xBFE0000000000000ULL,	/* -0.5 */
  0x7FF8000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x3FE0000000000000ULL,	/* 0.5 */
  0x7FF8000000000000ULL,	/* nan */
  0x7FFFFFFFFFFFFFFFULL,	/* nan */
  0x0000000000000000ULL,	/* 0.0 */
  0x7FF8000000000000ULL,	/* nan */
  0x7FF0000000000000ULL,	/* inf */
  0x3FF0000000000000ULL,	/* 1.0 */
  0x3FE0000000000000ULL,	/* 0.5 */
  0xFFF8000000000000ULL,	/* nan */
  0xFFF0000000000000ULL,	/* -inf */
  0xBFF0000000000000ULL,	/* -1.0 */
  0xBFE0000000000000ULL		/* -0.5 */
};

//int
//main ()
//{
//  int main_result;
//  int i;
//  float64 x1, x2;
//      main_result = 0;
//      for (i = 0; i < N; i++)
//	{
//	  float64 result;
//	  x1 = a_input[i];
//	  x2 = b_input[i];
//	  result = float64_add (x1, x2);
//	  main_result += (result != z_output[i]);
//
//	  printf
//	    ("a_input=%016llx b_input=%016llx expected=%016llx output=%016llx\n",
//	     a_input[i], b_input[i], z_output[i], result);
//	}
//      printf ("%d\n", main_result);
//      return main_result;
//    }

// clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D ASSUME -D lowerBound=3 -D upperBound=10 -O3 -o float64_add_assume -lm
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
        result[idx] = float64_add(*(bmx055xAcceleration*)(&xOps[idx]), *(bmx055xAcceleration*)(&yOps[idx]));
    }

    toc(&timer, "computation delay");

    printf("results: %llx, %llx, %llx, %llx, %llx\n", result[0], result[1], result[2], result[3], result[4]);
//    printf("results: %f\t%f\t%f\t%f\t%f\n", *(double*)(&result[0]), *(double*)(&result[1]),
//           *(double*)(&result[2]), *(double*)(&result[3]), *(double*)(&result[4]));

    return 0;
}
#endif
