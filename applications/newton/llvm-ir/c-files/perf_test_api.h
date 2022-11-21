//
// Created by pei on 16/11/22.
//

#ifndef NOISY_LANG_COMPILER_BENCHMARK_SUITE_H
#define NOISY_LANG_COMPILER_BENCHMARK_SUITE_H

/*
 * Definitions generated from Newton
 */
typedef double bmx055xAcceleration;  // [-16, 16]
typedef double bmx055zAcceleration;  // [0, 127]
typedef float bmx055fAcceleration;  // [0, 127]
typedef int64_t bmx055xMagneto;      // [0, 127]

#define iteration_num 5

uint64_t asuint(double t);
double asdouble(uint64_t t);

void uint8_add_test(bmx055xMagneto* leftOp, bmx055xMagneto* rightOp,
                    bmx055xMagneto* result);
void double_add_test(bmx055zAcceleration* leftOp, bmx055zAcceleration* rightOp,
                     bmx055zAcceleration* result);
void asuint_add_test(uint64_t* leftOp, uint64_t* rightOp, uint64_t* result);
void float_add_test(bmx055fAcceleration* leftOp, bmx055fAcceleration* rightOp,
                    bmx055fAcceleration* result);

#endif //NOISY_LANG_COMPILER_BENCHMARK_SUITE_H
