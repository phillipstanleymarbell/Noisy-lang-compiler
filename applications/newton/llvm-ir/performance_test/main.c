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
static bmx055xMagneto randIntValue[iteration_num];
bmx055xMagneto*
randomIntArr(bmx055xMagneto min, bmx055xMagneto max)
{
    for (size_t idx = 0; idx < iteration_num; idx++) {
        randIntValue[idx] = (rand() % max) + 1;
    }
    return randIntValue;
}

/*
 * random double array, [min, max]
 * */
static bmx055zAcceleration randDoubleValue[iteration_num];
bmx055zAcceleration*
randomDoubleArr(bmx055zAcceleration min, bmx055zAcceleration max)
{
    for (size_t idx = 0; idx < iteration_num; idx++) {
        randDoubleValue[idx] = min + 1.0 * rand() / RAND_MAX * (max - min);
    }
    return randDoubleValue;
}

/*
 * random float array, [min, max]
 * */
static bmx055fAcceleration randFloatValue[iteration_num];
bmx055fAcceleration*
randomFloatArr(bmx055fAcceleration min, bmx055fAcceleration max)
{
    for (size_t idx = 0; idx < iteration_num; idx++) {
        randFloatValue[idx] = min + 1.0 * rand() / RAND_MAX * (max - min);
    }
    return randFloatValue;
}

int
main(int argc, char** argv)
{
	double result = 0;
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
	/*
	 * I try to pass the function name from command line to make it more automatic,
	 * but it's seemingly forbidden in C/C++.
	 * So we need to write the function name manually here.
	 * */
	for (int i = 0; i < 1; i++)
	{
#ifdef CONTROL_FLOW_FUNC
		result = controlFlowFunc(randomFloat(-16.0, 16.0));
#elif defined(LIBC_EXP)
        result = __ieee754_exp(randomFloat(parameters[0], parameters[1]));
#elif defined(LIBC_LOG)
        result = __ieee754_log(randomFloat(parameters[0], parameters[1]));
#elif defined(LIBC_ACOSH)
		result = __ieee754_acosh(randomFloat(parameters[0], parameters[1]));
#elif defined(LIBC_J0)
        result = __ieee754_j0(randomFloat(parameters[0], parameters[1]));
#elif defined(LIBC_Y0)
		result = __ieee754_y0(randomFloat(parameters[0], parameters[1]));
#elif defined(LIBC_REM_PIO2)
        bmx055xAcceleration y[2];
        result = __ieee754_rem_pio2(randomFloat(parameters[0], parameters[1]), y);
#elif defined(LIBC_SINCOSF)
        float sinp, cosp;
        result = libc_sincosf(randomFloat(parameters[0], parameters[1]), &sinp, &cosp);
#elif defined(FLOAT64_ADD)
        result = float64_add(randomFloat(parameters[0], parameters[1]), randomFloat(parameters[0] + 0.6, parameters[1] + 0.3));
#elif defined(FLOAT64_DIV)
        result = float64_div(randomFloat(parameters[0], parameters[1]), randomFloat(parameters[0] + 0.6, parameters[1] + 0.3));
#elif defined(FLOAT64_MUL)
        result = float64_mul(randomFloat(parameters[0], parameters[1]), randomFloat(parameters[0] + 0.6, parameters[1] + 0.3));
#elif defined(FLOAT64_SIN)
        result = float64_sin(randomFloat(parameters[0], parameters[1]));
#elif defined(BENCHMARK_SUITE_INT)
        bmx055xMagneto result[iteration_num];
        bmx055xMagneto leftOps[iteration_num];
        bmx055xMagneto rightOps[iteration_num];
        for (size_t idx = 0; idx < iteration_num; idx++) {
            leftOps[idx] = randomInt(0, 127);
            rightOps[idx] = randomInt(0, 127);
        }
        timespec timer = tic();
        int32_add_test(leftOps, rightOps, result);
        toc(&timer, "computation delay");
        printf("%d\t%d\t%d\t%d\t%d\n", result[0], result[1], result[2], result[3], result[4]);
#elif defined(BENCHMARK_SUITE_INT_8)
        bmx055yMagneto result[iteration_num];
        bmx055yMagneto leftOps[iteration_num];
        bmx055yMagneto rightOps[iteration_num];
        for (size_t idx = 0; idx < iteration_num; idx++) {
            leftOps[idx] = randomInt_8(0, 127);
            rightOps[idx] = randomInt_8(0, 127);
        }
        timespec timer = tic();
        int8_add_test(leftOps, rightOps, result);
        toc(&timer, "computation delay");
        printf("%d\t%d\t%d\t%d\t%d\n", result[0], result[1], result[2], result[3], result[4]);
#elif defined(BENCHMARK_SUITE_DOUBLE)
        bmx055zAcceleration result[iteration_num];
        bmx055zAcceleration leftOps[iteration_num];
        bmx055zAcceleration rightOps[iteration_num];
        for (size_t idx = 0; idx < iteration_num; idx++) {
            leftOps[idx] = randomDouble(0, 127);
            rightOps[idx] = randomDouble(0, 127);
        }
        timespec timer = tic();
        double_add_test(leftOps, rightOps, result);
        toc(&timer, "computation delay");
        printf("%f\t%f\t%f\t%f\t%f\n", result[0], result[1], result[2], result[3], result[4]);
#elif defined(BENCHMARK_SUITE_FLOAT)
        bmx055fAcceleration result[iteration_num];
        bmx055fAcceleration leftOps[iteration_num];
        bmx055fAcceleration rightOps[iteration_num];
        for (size_t idx = 0; idx < iteration_num; idx++) {
            leftOps[idx] = randomFloat(0, 127);
            rightOps[idx] = randomFloat(0, 127);
        }
        timespec timer = tic();
        float_add_test(leftOps, rightOps, result);
        toc(&timer, "computation delay");
        printf("%f\t%f\t%f\t%f\t%f\n", result[0], result[1], result[2], result[3], result[4]);
#elif defined(BENCHMARK_SUITE_ASUINT)
        bmx055zAcceleration result[iteration_num];
        bmx055zAcceleration leftOps[iteration_num];
        bmx055zAcceleration rightOps[iteration_num];
        for (size_t idx = 0; idx < iteration_num; idx++) {
            leftOps[idx] = randomDouble(0, 127);
            rightOps[idx] = randomDouble(0, 127);
        }
        asUint_add_test(leftOps, rightOps, result);
//        printf("%f\t%f\t%f\t%f\t%f\n", result[0], result[1], result[2], result[3], result[4]);
#elif defined(BENCHMARK_SUITE_QUANT)
        int result[iteration_num];
        double result_res[iteration_num];
        int leftOps[iteration_num];
        int rightOps[iteration_num];
        for (size_t idx = 0; idx < iteration_num; idx++) {
            leftOps[idx] = (int)(randomDouble(0, 127) / 0.98 + 0.5);
            rightOps[idx] = (int)(randomDouble(0, 127) / 0.98 + 0.5);
        }
        quant_add_test(leftOps, rightOps, result);
        for (size_t idx = 0; idx < iteration_num; idx++) {
            result_res[idx] = result[idx] * 0.98;
        }
        printf("%f\t%f\t%f\t%f\t%f\n", result_res[0], result_res[1],
               result_res[2], result_res[3], result_res[4]);
//        printf("%f\t%f\t%f\t%f\t%f\n", result[0], result[1], result[2], result[3], result[4]);
#elif defined(BENCHMARK_SUITE_FIXEDPOINT)
        bmx055zAcceleration result[iteration_num];
        int fixed_result[iteration_num];
        bmx055zAcceleration leftOps[iteration_num];
        bmx055zAcceleration rightOps[iteration_num];
        int fixed_leftOps[iteration_num];
        int fixed_rightOps[iteration_num];
        for (size_t idx = 0; idx < iteration_num; idx++) {
            leftOps[idx] = randomDouble(0, 127);
            rightOps[idx] = randomDouble(0, 127);
            fixed_leftOps[idx] = (int) (leftOps[idx] * (1 << Q) + 0.5);
            fixed_rightOps[idx] = (int) (rightOps[idx] * (1 << Q) + 0.5);
        }
        timespec timer = tic();
//        fixed_point_add_test(leftOps, rightOps, result);
        fixed_point_add_test_simplified(fixed_leftOps, fixed_rightOps, fixed_result);
        toc(&timer, "computation delay");
        for (size_t idx = 0; idx < iteration_num; idx++) {
            result[idx] = (double)fixed_result[idx] / (1<<Q);
        }
        printf("%f\t%f\t%f\t%f\t%f\n", result[0], result[1], result[2], result[3], result[4]);
#elif defined(FUNC_CALL)
        bmx055xAcceleration x;
        bmx055yAcceleration y;
        double result[iteration_num];
        timespec timer = tic();
        for (size_t idx = 0; idx < iteration_num; idx++) {
            result[idx] = funcA(randomDouble(3, 10), randomDouble(15, 36));
        }
        toc(&timer, "computation delay");
        printf("%f\t%f\t%f\t%f\t%f\n", result[0], result[1], result[2], result[3], result[4]);
#else
	#error "Benchmark function not defined"
#endif
	}

	return 0;
}
