//
// Created by pei on 23/02/23.
//

#include <stdint.h>
#include <stdio.h>

typedef double bmx055xAcceleration;
typedef double bmx055yAcceleration;

int32_t testFunc(bmx055xAcceleration a, bmx055yAcceleration b) {
    printf("%f, %f\n", a, b);
    int64_t res1 = (int64_t)b >> 3;
    printf("res1 = %ld\n", res1);
    int32_t res2 = (int32_t)a << 4;
    printf("res2 = %d\n", res2);
    int16_t res3 = (int16_t)a >> (int8_t)(b+40);
    printf("res3 = %d\n", res3);
    int32_t res4 = (uint64_t)a >> 52;
    printf("res4 = %d\n", res4);
    return res1 + res2 + res3 + res4;
}

int main() {
    int32_t res = testFunc(-532.4, -37.9);
    printf("res = %d\n", res);
}