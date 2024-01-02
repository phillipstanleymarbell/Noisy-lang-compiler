/*
	Authored 2022. Pei Mu.

	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	*	Redistributions of source code must retain the above
		copyright notice, this list of conditions and the following
		disclaimer.

	*	Redistributions in binary form must reproduce the above
		copyright notice, this list of conditions and the following
		disclaimer in the documentation and/or other materials
		provided with the distribution.

	*	Neither the name of the author nor the names of its
		contributors may be used to endorse or promote products
		derived from this software without specific prior written
		permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>
#include "../c-files/perf_test_api.h"
#include "../c-files/fdlibm.h"

/***************************************
 * Timer functions of the test framework
 ***************************************/

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

/**********************************************
 * Random value generator of the test framework
 **********************************************/

static bmx055xMagneto
randomInt(bmx055xMagneto min, bmx055xMagneto max)
{
    bmx055xMagneto randIntValue = (rand() % max) + 1;
    return randIntValue;
}

static bmx055yMagneto
randomInt_8(bmx055yMagneto min, bmx055yMagneto max)
{
    bmx055yMagneto randIntValue = (rand() % max) + 1;
    return randIntValue;
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

static bmx055fAcceleration
randomFloat(bmx055fAcceleration min, bmx055fAcceleration max)
{
    bmx055fAcceleration randDbValue = min + 1.0 * rand() / RAND_MAX * (max - min);
    return randDbValue;
}

/*
 * random integer array, [min, max]
 * */
static void
randomIntArr(bmx055xMagneto *randIntValue, bmx055xMagneto min, bmx055xMagneto max)
{
    for (size_t idx = 0; idx < iteration_num; idx++) {
        randIntValue[idx] = (rand() % max) + 1;
    }
}

/*
 * random double array, [min, max]
 * */
static void
randomDoubleArr(bmx055zAcceleration *randDoubleValue, bmx055zAcceleration min, bmx055zAcceleration max)
{
    for (size_t idx = 0; idx < iteration_num; idx++) {
        randDoubleValue[idx] = min + 1.0 * rand() / RAND_MAX * (max - min);
    }
}

/*
 * random float array, [min, max]
 * */
static void
randomFloatArr(bmx055fAcceleration *randFloatValue, bmx055fAcceleration min, bmx055fAcceleration max)
{
    for (size_t idx = 0; idx < iteration_num; idx++) {
        randFloatValue[idx] = min + 1.0 * rand() / RAND_MAX * (max - min);
    }
}

/************************************
 * Main process of the test framework
 ************************************/

int
main(int argc, char** argv)
{
    double parameters[2];
    char* pEnd;
    if (argc == 3) {
        for (size_t idx = 0; idx < argc-1; idx++) {
            parameters[idx] = strtod(argv[idx+1], &pEnd);
        }
    }
    else {
        parameters[0] = 3.0;
        parameters[1] = 10.0;
    }
    double result[iteration_num];
    bmx055xAcceleration xOps[iteration_num];
    bmx055yAcceleration yOps[iteration_num];
    for (size_t idx = 0; idx < iteration_num; idx++) {
        xOps[idx] = randomDouble(parameters[0], parameters[1]);
        yOps[idx] = randomDouble(parameters[0] + 0.6, parameters[1] + 0.3);
    }

    bmx055fAcceleration fpResult[iteration_num];
    bmx055fAcceleration fpXOps[iteration_num];
    bmx055fAcceleration fpYOps[iteration_num];
    for (size_t idx = 0; idx < iteration_num; idx++) {
        fpXOps[idx] = randomFloat(parameters[0], parameters[1]);
        fpYOps[idx] = randomFloat(parameters[0] + 0.6, parameters[1] + 0.3);
    }

    bmx055xMagneto intResult[iteration_num];
    bmx055xMagneto intXOps[iteration_num];
    bmx055xMagneto intYOps[iteration_num];
    for (size_t idx = 0; idx < iteration_num; idx++) {
        intXOps[idx] = randomInt(0, 127);
        intYOps[idx] = randomInt(0, 127);
    }

    bmx055yMagneto int8Result[iteration_num];
    bmx055yMagneto int8XOps[iteration_num];
    bmx055yMagneto int8YOps[iteration_num];
    for (size_t idx = 0; idx < iteration_num; idx++) {
        int8XOps[idx] = randomInt_8(0, 127);
        int8YOps[idx] = randomInt_8(0, 127);
    }

    // pre-processing of quantization
    int fixedResult[iteration_num];
    int fixedLeftOps[iteration_num];
    int fixedRightOps[iteration_num];
    for (size_t idx = 0; idx < iteration_num; idx++) {
#if defined(BENCHMARK_SUITE_QUANT)
        fixedLeftOps[idx] = (int) (intXOps[idx] * (1 << Q) + 0.5);
        fixedRightOps[idx] = (int) (intYOps[idx] * (1 << Q) + 0.5);
#elif defined(BENCHMARK_SUITE_FIXEDPOINT)
        fixedLeftOps[idx] = (int) (intXOps[idx] / 0.98 + 0.5);
        fixedRightOps[idx] = (int) (intYOps[idx] / 0.98 + 0.5);
#endif
    }

    /*
	 * I try to pass the function name from command line to make it more automatic,
	 * but it's seemingly forbidden in C/C++.
	 * So we need to write the function name manually here.
	 * */
    timespec timer = tic();
#if defined(CONTROL_FLOW_FUNC)
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = controlFlowFunc(xOps[idx]);
    }
#elif defined(LIBC_EXP)
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = __ieee754_exp(xOps[idx]);
    }
#elif defined(LIBC_LOG)
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = __ieee754_log(xOps[idx]);
    }
#elif defined(LIBC_ACOSH)
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = __ieee754_acosh(xOps[idx]);
    }
#elif defined(LIBC_J0)
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = __ieee754_j0(xOps[idx]);
    }
#elif defined(LIBC_Y0)
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = __ieee754_y0(xOps[idx]);
    }
#elif defined(LIBC_REM_PIO2)
    bmx055xAcceleration y[2];
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = __ieee754_rem_pio2(xOps[idx], y);
    }
#elif defined(LIBC_SINCOSF)
    float sinp, cosp;
    for (size_t idx = 0; idx < iteration_num; idx++) {
        sinp = cosp = 0;
        libc_sincosf(xOps[idx], &sinp, &cosp);
        result[idx] = sinp;
    }
#elif defined(FLOAT64_ADD)
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = float64_add((uint64_t)(xOps[idx]), (uint64_t)(yOps[idx]));
    }
#elif defined(FLOAT64_DIV)
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = float64_div((uint64_t)(xOps[idx]), (uint64_t)(yOps[idx]));
    }
#elif defined(FLOAT64_MUL)
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = float64_mul((uint64_t)(xOps[idx]), (uint64_t)(yOps[idx]));
    }
#elif defined(FLOAT64_SIN)
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = float64_sin((uint64_t)(xOps[idx]), (uint64_t)(yOps[idx]));
    }
#elif defined(BENCHMARK_SUITE_INT)
    int32_add_test(intXOps, intYOps, intResult);
#elif defined(BENCHMARK_SUITE_INT_8)
    int8_add_test(int8XOps, int8YOps, int8Result);
#elif defined(BENCHMARK_SUITE_DOUBLE)
    double_add_test(xOps, yOps, result);
#elif defined(BENCHMARK_SUITE_FLOAT)
    float_add_test(fpXOps, fpYOps, fpResult);
#elif defined(BENCHMARK_SUITE_ASUINT)
    asUint_add_test(xOps, yOps, result);
#elif defined(BENCHMARK_SUITE_QUANT)
    quant_add_test(fixedLeftOps, fixedRightOps, fixedResult);
#elif defined(BENCHMARK_SUITE_FIXEDPOINT)
    fixed_point_add_test_simplified(fixedLeftOps, fixedRightOps, fixedResult);
#elif defined(FUNC_CALL)
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = funcA(xOps[idx], yOps[idx]);
    }
#elif defined(ARM_SQRT_Q15)
    for (size_t idx = 0; idx < iteration_num; idx++) {
        ///// part of the arm_cmplx_mag_q15, which call arm_sqrt_q15
//        int32_t x = xOps[idx] * yOps[idx] * 2;
        intResult[idx] = arm_sqrt_q15((int16_t)xOps[idx]);
    }
#else
	#error "Benchmark function not defined"
#endif
    toc(&timer, "computation delay");

    // post-processing of quantization
    for (size_t idx = 0; idx < iteration_num; idx++) {
#if defined(BENCHMARK_SUITE_QUANT)
        result[idx] = fixedResult[idx] * 0.98;
#elif defined(BENCHMARK_SUITE_FIXEDPOINT)
        result[idx] = (double)fixedResult[idx] / (1<<Q);
#endif
    }

    printf("results: %f\t%f\t%f\t%f\t%f\n", result[0], result[1], result[2], result[3], result[4]);
    printf("int results: %d\t%d\t%d\t%d\t%d\n", intResult[0], intResult[1], intResult[2], intResult[3], intResult[4]);

	return 0;
}
