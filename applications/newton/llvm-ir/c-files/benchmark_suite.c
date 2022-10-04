/*
 * Benchmark Suite for the time consumption of LLVM instructions
 * */
#include <stdio.h>
#include <stdint.h>

static const uint64_t iteration_num = 100;

/*
 * Type Benchmark Suite
 * */
typedef int64_t bmx055xMagneto;

bmx055xMagneto* uint8_add_test(bmx055xMagneto* leftOp, bmx055xMagneto* rightOp) {
    bmx055xMagneto result[iteration_num];
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = leftOp[idx] + rightOp[idx];
        auto x = leftOp[idx] + 12;
        auto y = rightOp[idx] + 15;
        auto z = x + y;
        result[idx] = result[idx] * (z%100);
    }
    return result;
}

typedef double bmx055zAcceleration;

bmx055zAcceleration* double_add_test(bmx055zAcceleration* leftOp, bmx055zAcceleration* rightOp) {
    bmx055zAcceleration result[iteration_num];
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = leftOp[idx] + rightOp[idx];
        auto x = leftOp[idx] + 12.789;
        auto y = rightOp[idx] + 15.653;
        auto z = x + y;
        result[idx] = result[idx] * ((int)z%100);
    }
    return result;
}

typedef float bmx055fAcceleration;

bmx055fAcceleration* float_add_test(bmx055fAcceleration* leftOp, bmx055fAcceleration* rightOp) {
    bmx055fAcceleration result[iteration_num];
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = leftOp[idx] + rightOp[idx];
    }
    return result;
}
