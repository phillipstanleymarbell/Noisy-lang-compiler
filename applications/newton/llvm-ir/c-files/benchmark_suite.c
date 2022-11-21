/*
 * Benchmark Suite for the time consumption of LLVM instructions
 * */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "perf_test_api.h"

uint64_t asuint(double t) {
    union {
        double f;
        uint64_t k;
    } u = {t};
    return u.k;
}

uint32_t asuint32(float t) {
    union {
        float f;
        uint32_t k;
    } u = {t};
    return u.k;
}

double asdouble(uint64_t t) {
    union {
        uint64_t k;
        double f;
    } u = {t};
    return u.f;
}

float asfloat(uint32_t t) {
    union {
        uint32_t k;
        float f;
    } u = {t};
    return u.f;
}

void uint8_add_test(bmx055xMagneto* leftOp, bmx055xMagneto* rightOp,
                    bmx055xMagneto* result) {
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = leftOp[idx] + rightOp[idx];
        int64_t x = leftOp[idx] + 12;
        int64_t y = rightOp[idx] + 15;
        int64_t z = x + y;
        result[idx] = result[idx] * (z%100);
    }
    return;
}

void double_add_test(bmx055zAcceleration* leftOp, bmx055zAcceleration* rightOp,
                     bmx055zAcceleration* result) {
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = leftOp[idx] + rightOp[idx];
        double x = leftOp[idx] + 12.789;
        double y = rightOp[idx] + 15.653;
        double z = x + y;
        result[idx] = result[idx] * ((int)z%100);
    }
    return;
}

uint32_t extractMSW(uint64_t long_word) {
    uint32_t msw = long_word >> (32 * 1);
    return msw;
}

uint32_t extractLSW(uint64_t long_word) {
    uint32_t lsw = long_word >> (32 * 0);
    return lsw;
}

uint64_t combineWord(uint32_t msw, uint32_t lsw) {
    uint64_t tmp_msw = msw;
    uint64_t long_word = (tmp_msw << 32) + lsw;
    return long_word;
}

bool extractSign(uint64_t long_word) {
    bool sign = long_word >> 63;
    return sign;
}

uint16_t extractExponent(uint64_t long_word) {
    uint16_t exponent = long_word >> 52;
    exponent = exponent << 5;
    exponent = exponent >> 5;
    return exponent;
}

uint16_t extractFraction(uint64_t long_word) {
    // we assume to use bmx055xMagneto here,
    // which means 12 bits in Fraction of IEEE 754 double-precision format
    const int useful_bit_num = 12;
//    printf("long_word: %lx\n", long_word);
    // remove sign bit and exponent bits
    uint64_t shift_left_word = long_word << 12;
//    printf("shift left word: %lx\n", shift_left_word);
    uint16_t fraction = shift_left_word >> (64-useful_bit_num);
//    printf("fraction: %x\n", fraction);
    return fraction;
}

double combineRealNumber(bool sign, uint16_t exponent, uint16_t fraction) {
    // (-1)^sign * (1.fraction) * 2^(exponent-1023)
    int integer_part =(1-sign*2) * (2 << (exponent - 1023 - 1));
//    printf("integer_part = %d\n", integer_part);
    double fraction_part = (double)fraction / (1 << 12) + 1;
//    printf("fraction_part = %f\n", fraction_part);
    return integer_part * fraction_part;
}

void asuint_add_test(uint64_t* leftOp, uint64_t* rightOp, uint64_t* result) {
    uint64_t long_word = asuint(12.789);
    bool sign = extractSign(long_word);
    uint16_t exponent = extractExponent(long_word);
    uint16_t fraction = extractFraction(long_word);
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = leftOp[idx] + rightOp[idx];
//        uint32_t test_msw = extractMSW(long_word);
//        uint32_t test_lsw = extractLSW(long_word);
//        uint64_t test_long_word = combineWord(test_msw, test_lsw);
//        printf("test::::::%x\t%x\t%lx\t%f\n", test_msw, test_lsw, test_long_word,
//               asdouble(test_long_word));
//        printf("convert:::::%f\t%f\n", asfloat(test_msw), asfloat(test_lsw));
//        printf("extract:::::%u\t%u\t%u\t%f\n", sign, exponent, fraction,
//               combineRealNumber(sign, exponent, fraction));

        bool leftOpSign = extractSign(leftOp[idx]);
        uint16_t leftOpExponent = extractExponent(leftOp[idx]);
        uint16_t leftOpFraction = extractFraction(leftOp[idx]);
//        printf("extract:::::%f\t%u\t%u\t%u\t%f\n", asdouble(leftOp[idx]), leftOpSign, leftOpExponent,
//               leftOpFraction, combineRealNumber(leftOpSign, leftOpExponent, leftOpFraction));

//        printf("test:::::%x\t%x\n", ((1<<12)+fraction)<<(exponent-1023),
//               ((1<<12)+leftOpFraction)<<(leftOpExponent-1023));
        uint16_t exponent_res = exponent + leftOpExponent;
        uint16_t fraction_res = fraction + leftOpFraction;
        uint64_t x = combineRealNumber(sign, exponent_res, fraction_res);


//        uint64_t x = leftOp[idx] + asuint(12.789);
        uint64_t y = rightOp[idx] + asuint(15.653);
        uint64_t z = x + y;
        result[idx] = result[idx] * ((int)z%100);
//        printf("%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\t%lu\n",
//               leftOp[idx], rightOp[idx], asuint(12.789), asuint(15.653),
//               x, y, z, result[idx]);
//        printf("%f\t%f\t%f\t%f\t%f\t%f\t%f\t%f\n", asdouble(leftOp[idx]),
//               asdouble(rightOp[idx]), asdouble(asuint(12.789)),
//               asdouble(asuint(15.653)), asdouble(x), asdouble(y),
//               asdouble(z), asdouble(result[idx]));
    }
    return;
}

void quant_add_test(int* leftOp, int* rightOp,
                    int* result) {
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = leftOp[idx] + rightOp[idx];
        int x = leftOp[idx] + (int)(12.789 / 0.98 + 0.5);
        int y = rightOp[idx] + (int)(15.653 / 0.98 + 0.5);
        int z = x + y;
        result[idx] = result[idx] * ((int)z%100);
    }
    return;
}

void float_add_test(bmx055fAcceleration* leftOp, bmx055fAcceleration* rightOp,
                    bmx055fAcceleration* result) {
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = leftOp[idx] + rightOp[idx];
        float x = leftOp[idx] + 12.789;
        float y = rightOp[idx] + 15.653;
        float z = x + y;
        result[idx] = result[idx] * ((int)z%100);
    }
    return;
}
