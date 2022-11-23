/*
 * Benchmark Suite for the time consumption of LLVM instructions
 * */
#include "perf_test_api.h"

#define DB_FRACTION_BIT 52
#define DB_EXPONENT_BIT 11
#define SIGN_BIT        1

/* we assume to use bmx055xMagneto here,
 * which means 12 bits in Fraction of IEEE 754 double-precision format
 * */
const int useful_bit_num = 10;

static inline uint64_t asUint(double t) {
    union {
        double f;
        uint64_t k;
    } u = {t};
    return u.k;
}

uint32_t asUint32(float t) {
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

/*
 * a common case testsuite,
 * this example test code wants to show the performance of computation sensitive program.
 * the magic numbers and operators in this function are random and meaningless.
 * */
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

static inline bool extractSign(uint64_t long_word) {
    bool sign = long_word >> (DB_FRACTION_BIT + DB_EXPONENT_BIT);
    return sign;
}

static inline uint16_t extractExponent(uint64_t long_word) {
    // remove sign bit
    uint64_t exponent = long_word << SIGN_BIT;
    exponent = exponent >> (SIGN_BIT + DB_FRACTION_BIT);
    return exponent;
}

static inline uint16_t extractFraction(uint64_t long_word) {
//    printf("long_word: %lx\n", long_word);
    // remove sign bit and exponent bits
    uint64_t shift_left_word = long_word << (SIGN_BIT + DB_EXPONENT_BIT);
//    printf("shift left word: %lx\n", shift_left_word);
    uint16_t fraction = shift_left_word >> (64-useful_bit_num);
//    printf("fraction: %x\n", fraction);
    return fraction;
}

/*
 * split a double value into several parts based on IEEE 754 (https://en.wikipedia.org/wiki/IEEE_754).
 * double: 1 bit sign + 11 bits exponent + 52 bits fraction
 * */
partsOfFP splitDouble(double value) {
    partsOfFP res;
    uint64_t intValue = asUint(value);
    res.signBit = extractSign(intValue);
    res.fractionPart = extractFraction(intValue);
    res.exponentPart = extractExponent(intValue);
    return res;
}

/*
 * combine three parts to a double value based on IEEE 754 (https://en.wikipedia.org/wiki/IEEE_754).
 * double = (-1)^sign * (1.fraction) * 2^(exponent-1023)
 * */
double combineRealNumber(partsOfFP value) {
    bool sign = value.signBit;
    uint16_t exponent = value.exponentPart;
    uint16_t fraction = value.fractionPart;
    int integer_part =(1-sign*2) * (2 << (exponent - 1023 - 1));
//    printf("integer_part = %d\n", integer_part);
    double fraction_part = (double)fraction / (1 << 12) + 1;
//    printf("fraction_part = %f\n", fraction_part);
    return integer_part * fraction_part;
}

static inline partsOfFP partsOfFPMul(partsOfFP lhs, partsOfFP rhs) {
    partsOfFP res;
    res.signBit = lhs.signBit * rhs.signBit;
    res.fractionPart = lhs.fractionPart * rhs.fractionPart;
    res.exponentPart = lhs.exponentPart + rhs.exponentPart;
    return res;
}

static inline partsOfFP partsOfFPAdd(partsOfFP lhs, partsOfFP rhs) {
    partsOfFP res;
    if (lhs.signBit == rhs.signBit) {

    } else {
        /*
         * need to check
         * */
    }
    return res;
}

/*
 * nonsense, it's just another version of soft-float arithmetic
 * */
void asUint_add_test(bmx055zAcceleration* leftOp, bmx055zAcceleration* rightOp,
                     bmx055zAcceleration* result) {
    for (size_t idx = 0; idx < iteration_num; idx++) {
        partsOfFP partsOfLeft = splitDouble(leftOp[idx]);
        partsOfFP partsOfRight = splitDouble(rightOp[idx]);
        partsOfFP partsOfConst1 = splitDouble(12.789);
        partsOfFP partsOfConst2 = splitDouble(15.653);
        partsOfFP res = partsOfFPAdd(partsOfLeft, partsOfRight);
        partsOfFP x = partsOfFPAdd(partsOfLeft, partsOfConst1);
        partsOfFP y = partsOfFPAdd(partsOfRight, partsOfConst2);
        partsOfFP z = partsOfFPAdd(x, y);
        int int_z = (int) combineRealNumber(z) % 100;
        z = splitDouble((double)int_z);
        res = partsOfFPMul(res, z);
        result[idx] = combineRealNumber(res);
    }
    return;
}

/*
 * a quantization testsuite for the "double_add_test"
 * they have same computation pattern.
 * */
void quant_add_test(int* leftOp, int* rightOp,
                    int* result) {
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = leftOp[idx] + rightOp[idx];
        /* the resolution of 'bmx055zAcceleration' is 0.98, so
         * (int)(12.789/0.98) = 13;
         * (int)(15.653/0.98) = 16;
         * */
        int x = leftOp[idx] + 13;
        int y = rightOp[idx] + 16;
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
