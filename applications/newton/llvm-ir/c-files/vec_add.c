/*
 * compile with 'clang --target=aarch64-arm-none-eabi -O1 vec_add.c -o vec_add -fvectorize'
 * */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

typedef float bmx055fAcceleration;

#define NUM 102400

void vec_add(bmx055fAcceleration *vec_A, bmx055fAcceleration *vec_B, bmx055fAcceleration *vec_C, int len_vec) {
    int i;
    for (i=0; i<len_vec; i++) {
        vec_C[i] = vec_A[i] + vec_B[i];
    }
}

int main() {
    float x[NUM], y[NUM], z[NUM];
    for (size_t idx = 0; idx < NUM; idx++) {
        x[idx] = rand() % INT8_MAX;
        y[idx] = rand() % INT8_MAX;
    }
//    timespec timer = tic();
    vec_add(x, y, z, NUM);
//    toc(&timer, "computation delay");
    for (size_t idx = 0; idx < NUM; idx++) {
        printf("value of z[%d]=%f, ", idx, z[idx]);
    }
    return 0;
}
