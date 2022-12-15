/*
 * element-wise test
 * */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM 102400

void element_add(const float* x, const float* y, float* z, size_t len) {
    for (size_t idx = 0; idx <= len; idx++) {
        z[idx] = x[idx] + y[idx];
    }
}

int main() {
    float x[NUM], y[NUM], z[NUM];
    for (size_t idx = 0; idx < NUM; idx++) {
        x[idx] = rand() % INT8_MAX;
        y[idx] = rand() % INT8_MAX;
    }
    element_add(x, y, z, NUM);
    return 0;
}