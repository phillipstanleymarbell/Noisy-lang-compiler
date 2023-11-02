/* ----------------------------------------------------------------------
* Copyright (C) 2010-2014 ARM Limited. All rights reserved.
*
* $Date:        12. March 2014
* $Revision: 	V1.4.4
*
* Project:      CMSIS DSP Library
* Title:		arm_sqrt_q15.c
*
* Description:	Q15 square root function.
*
* Target Processor: Cortex-M4/Cortex-M3/Cortex-M0
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*   - Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
*   - Redistributions in binary form must reproduce the above copyright
*     notice, this list of conditions and the following disclaimer in
*     the documentation and/or other materials provided with the
*     distribution.
*   - Neither the name of ARM LIMITED nor the names of its contributors
*     may be used to endorse or promote products derived from this
*     software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
* ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
* -------------------------------------------------------------------- */

#include "arm_sqrt_q15.h"

static uint32_t __CLZ(
        q31_t data)
{
    uint32_t count = 0;
    uint32_t mask = 0x80000000;

    while((data & mask) == 0)
    {
        count += 1u;
        mask = mask >> 1u;
    }

    return (count);

}

/**
 * @brief  Q15 square root function.
 * @param[in]   in     input value.  The range of the input value is [0 +1) or 0x0000 to 0x7FFF.
 * @return The function returns ARM_MATH_SUCCESS if the input value is positive
 * and ARM_MATH_ARGUMENT_ERROR if the input is negative.  For
 * negative inputs, the function returns 0.
 */
q15_t arm_sqrt_q15_func(q15_t number) {
    q15_t temp1, var1, signBits1, half;
    q31_t bits_val1;
    float32_t temp_float1;
    union
    {
        q31_t fracval;
        float32_t floatval;
    } tempconv;

/* If the input is a positive number then compute the signBits. */
    if(number > 0)
    {
        signBits1 = __CLZ(number) - 17; // todo: if it's a constant, we can get the result in theory. 14

        /* Shift by the number of signBits1 */
        if((signBits1 % 2) == 0)
        {
            number = number << signBits1;
        }
        else
        {
            number = number << (signBits1 - 1);
        }

        /* Calculate half value of the number */
        half = number >> 1;
        /* Store the number for later use */
        temp1 = number;
        //printf("in: %d\t", in);
        //printf("signBits1: %d\t", signBits1);
        //printf("number: %d\t", number);
        //printf("half: %d\t", half);

        /*Convert to float */
        temp_float1 = number * 3.051757812500000e-005f;
        /*Store as integer */
        tempconv.floatval = temp_float1;
        bits_val1 = tempconv.fracval;
        /* Subtract the shifted value from the magic number to give intial guess */
        bits_val1 = 0x5f3759df - (bits_val1 >> 1);  // gives initial guess
        /* Store as float */
        tempconv.fracval = bits_val1;
        temp_float1 = tempconv.floatval;
        /* Convert to integer format */
        var1 = (q31_t) (temp_float1 * 16384);

        /* 1st iteration */
        var1 = ((q15_t) ((q31_t) var1 * (0x3000 -
                                         ((q15_t)
                                                 ((((q15_t)
                                                         (((q31_t) var1 * var1) >> 15)) *
                                                   (q31_t) half) >> 15))) >> 15)) << 2;
        /* 2nd iteration */
        var1 = ((q15_t) ((q31_t) var1 * (0x3000 -
                                         ((q15_t)
                                                 ((((q15_t)
                                                         (((q31_t) var1 * var1) >> 15)) *
                                                   (q31_t) half) >> 15))) >> 15)) << 2;
        /* 3rd iteration */
        var1 = ((q15_t) ((q31_t) var1 * (0x3000 -
                                         ((q15_t)
                                                 ((((q15_t)
                                                         (((q31_t) var1 * var1) >> 15)) *
                                                   (q31_t) half) >> 15))) >> 15)) << 2;

        /* Multiply the inverse square root with the original value */
        var1 = ((q15_t) (((q31_t) temp1 * var1) >> 15)) << 1;
        ////printf("var1=%d\t", var1);

        /* Shift the output down accordingly */
        if((signBits1 % 2) == 0)
        {
            var1 = var1 >> (signBits1 / 2);
        }
        else
        {
            var1 = var1 >> ((signBits1 - 1) / 2);
        }
        //printf("var1=%d\n", var1);

        return var1;
    }
        /* If the number is a negative number then store zero as its square root value */
    else
    {
        return 0;
    }
}

q15_t arm_sqrt_q15(bmx055yMagneto in)
{
    q15_t number = (in*in*2 >> 17) % 5;

    return arm_sqrt_q15_func(number);
}

/**
 * @} end of SQRT group
 */
