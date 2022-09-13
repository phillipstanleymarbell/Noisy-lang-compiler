/*
 * Benchmark Suite for the time consumption of LLVM instructions
 * */
#include <stdio.h>
#include <stdint.h>

static const uint64_t iteration_num = 100;

/*
 * Type Benchmark Suite
 * */
typedef int bmx055xMagneto;

bmx055xMagneto* uint8_add_test(bmx055xMagneto* leftOp, bmx055xMagneto* rightOp) {
    bmx055xMagneto result[iteration_num];
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = leftOp[idx] + rightOp[idx];
    }
    return result;
}
