/*
 * Benchmark Suite for the time consumption of LLVM instructions
 * */
#include "perf_test_api.h"
#include <math.h>
#include <assert.h>

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

void int32_add_test(bmx055xMagneto* leftOp, bmx055xMagneto* rightOp,
                    bmx055xMagneto* result) {
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = leftOp[idx] + rightOp[idx];
        int32_t x = leftOp[idx] + 12;
        int32_t y = rightOp[idx] + 15;
        int32_t z = x + y;
        result[idx] = result[idx] * (z%100);
    }
    return;
}

void int8_add_test(bmx055yMagneto* leftOp, bmx055yMagneto* rightOp,
                    bmx055yMagneto* result) {
    for (size_t idx = 0; idx < iteration_num; idx++) {
        result[idx] = leftOp[idx] + rightOp[idx];
        int8_t x = leftOp[idx] + 12;
        int8_t y = rightOp[idx] + 15;
        int8_t z = x + y;
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
void quant_add_test(int* leftOp, int* rightOp, int* result) {
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

// saturate to range of int16_t
int16_t sat16(int32_t x)
{
    if (x > INT16_MAX || x < INT16_MIN)
        /*
         * In the real code, it changes to a higher bit number
         * */
        assert(false && "overflow!!!");
    else
        return (int16_t)x;
}

// saturate to range of int32_t
int32_t sat32(int64_t x)
{
    if (x > INT32_MAX || x < INT32_MIN)
        /*
         * In the real code, it changes to a higher bit number
         * */
        assert(false && "overflow!!!");
    else
        return (int32_t)x;
}

int16_t q_add_sat(int16_t a, int16_t b)
{
    int32_t tmp;

    tmp = (int32_t)a + (int32_t)b;

    return sat16(tmp);
}

int16_t q_sub(int16_t a, int16_t b)
{
    return (int16_t)(a - b);
}

int16_t q_mul(int16_t a, int16_t b)
{
    int16_t result;
    int32_t temp;

    temp = (int32_t)a * (int32_t)b; // result type is operand's type
    // Rounding; mid-values are rounded up
    /*
     * Rounding is possible by adding a 'rounding addend' of
     * half of the scaling factor before shifting;
     *
     * The proof:
     * Res = round(x/y) = (int)(x/y +/- 0.5) = (int)((x +/- y/2)/y)
     * let y = 2^Q,
     * Res = (int)((x +/- 2^(Q-1))/2^Q) = (x +/- 2^(Q-1)) >> Q
     * */
    if ((temp >> 31))
        temp -= K;
    else
        temp += K;
    // Correct by dividing by base and saturate result
    result = sat16(temp >> Q);

    return result;
}

int32_t q_mul_32(int32_t a, int32_t b)
{
    int32_t result;
    int64_t temp;

    temp = (int64_t)a * (int64_t)b; // result type is operand's type
    // Rounding; mid-values are rounded up
    /*
     * Rounding is possible by adding a 'rounding addend' of
     * half of the scaling factor before shifting;
     *
     * The proof:
     * Res = round(x/y) = (int)(x/y +/- 0.5) = (int)((x +/- y/2)/y)
     * let y = 2^Q,
     * Res = (int)((x +/- 2^(Q-1))/2^Q) = (x +/- 2^(Q-1)) >> Q
     * */
    if ((temp >> 63))
        temp -= K;
    else
        temp += K;
    // Correct by dividing by base and saturate result
    result = sat32(temp >> Q);

    return result;
}

int16_t q_div(int16_t a, int16_t b)
{
    /* pre-multiply by the base (Upscale to Q16 so that the result will be in Q8 format) */
    int32_t temp = (int32_t)a << Q;
    /* Rounding: mid-values are rounded up (down for negative values). */
    /*
     * The proof:
     * Res = round(a/b)
     * Because trunc negative remove the fraction, e.g. (int)(-1.833) = -1
     * if a and b have the same sign bit, Res = (int)(a/b + 0.5) = floor((a+b/2)/b)
     * else Res = (int)(a/b - 0.5) = floor((a-b/2)/b)
     * */
    if ((temp >> 31) ^ (b >> 15)) {
        temp -= (b >> 1);
    } else {
        temp += (b >> 1);
    }
    return (int16_t)(temp / b);
}

/*
 * a testsuite for q-format-fixed-point-quantization,
 * this example test code wants to show the performance of computation sensitive program.
 * the magic numbers and operators in this function are random and meaningless.
 * */
void fixed_point_add_test(bmx055zAcceleration* leftOp, bmx055zAcceleration* rightOp,
                          bmx055zAcceleration* result) {
    int16_t c1 = (int16_t)round(12.789 * (1 << Q));
    int16_t c2 = (int16_t)round(15.653 * (1 << Q));
    for (size_t idx = 0; idx < iteration_num; idx++) {
        int16_t leftOpQuant = (int16_t)round(leftOp[idx] * (1 << Q));
        int16_t rightOpQuant = (int16_t)round(rightOp[idx] * (1 << Q));
        int16_t resultQuant = q_add_sat(leftOpQuant, rightOpQuant);
        int16_t x = q_add_sat(leftOpQuant, c1);
        int16_t y = q_add_sat(rightOpQuant, c2);
        int16_t z = q_add_sat(x, y);
        /*
         * For non-linear operator, change back to floating-point
         * */
        double fp_z = (double)(z >> Q);
        int16_t temp_z = (int)fp_z%100;
        temp_z = temp_z << Q;
        /*
         * When the range analyzer find the range of result might need 32 bits,
         * it changes to upper precision
         * */
        int32_t resultQuant_32 = q_mul_32(resultQuant, temp_z);
        result[idx] = (double)resultQuant_32 / (1<<Q);
    }
    return;
}

/*
 * simplified version of fixed_point_add_test function.
 * */
const int c1 = (int)(12.789 * (1 << Q)+0.5);
const int c2 = (int)(15.653 * (1 << Q)+0.5);

void fixed_point_add_test_simplified(const int* leftOp, const int* rightOp, int* result) {
    printf("left: %d\tright: %d\n", leftOp[0], rightOp[0]);
    for (size_t idx = 0; idx < iteration_num; idx++) {
        int result_round = leftOp[idx] + rightOp[idx];
        int x = leftOp[idx] + c1;
        int y = rightOp[idx] + c2;
        int z = x + y;
        /*
         * For non-linear operator, change back to floating-point
         * */
        int temp_z = (z >> Q)%100;
        temp_z = temp_z << Q;
        /*
         * When the range analyzer find the range of result might need 32 bits,
         * it changes to upper precision
         * */
        result[idx] = (result_round * temp_z + K) >> Q;
    }
    printf("result: %d\n", result[0]);
}