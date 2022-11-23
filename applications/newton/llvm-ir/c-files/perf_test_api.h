//
// Created by pei on 16/11/22.
//

#ifndef NOISY_LANG_COMPILER_BENCHMARK_SUITE_H
#define NOISY_LANG_COMPILER_BENCHMARK_SUITE_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

/*
 * Definitions generated from Newton
 */
typedef double bmx055xAcceleration;  // [-16, 16]
typedef double bmx055zAcceleration;  // [0, 127]
typedef float bmx055fAcceleration;  // [0, 127]
typedef int64_t bmx055xMagneto;      // [0, 127]

typedef struct partsOfFP {
    bool signBit;
    uint16_t fractionPart;
    uint16_t exponentPart;
} partsOfFP;

#define iteration_num 5

//uint64_t asUint(double t);
//double asdouble(uint64_t t);

void uint8_add_test(bmx055xMagneto* leftOp, bmx055xMagneto* rightOp,
                    bmx055xMagneto* result);
void double_add_test(bmx055zAcceleration* leftOp, bmx055zAcceleration* rightOp,
                     bmx055zAcceleration* result);
void asUint_add_test(bmx055zAcceleration* leftOp, bmx055zAcceleration* rightOp,
                     bmx055zAcceleration* result);
void float_add_test(bmx055fAcceleration* leftOp, bmx055fAcceleration* rightOp,
                    bmx055fAcceleration* result);

partsOfFP splitDouble(double value);
double combineRealNumber(partsOfFP value);

#endif //NOISY_LANG_COMPILER_BENCHMARK_SUITE_H
