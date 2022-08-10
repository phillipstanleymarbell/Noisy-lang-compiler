/*
	Authored 2022. Pei Mu.

	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	*	Redistributions of source code must retain the above
		copyright notice, this list of conditions and the following
		disclaimer.

	*	Redistributions in binary form must reproduce the above
		copyright notice, this list of conditions and the following
		disclaimer in the documentation and/or other materials
		provided with the distribution.

	*	Neither the name of the author nor the names of its
		contributors may be used to endorse or promote products
		derived from this software without specific prior written
		permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>

/*
 * Definitions generated from Newton
 */
typedef double bmx055xAcceleration;  // [-16, 16]

/*
 * random floating point, [min, max]
 * */
bmx055xAcceleration
randomFloat(float min, float max)
{
    bmx055xAcceleration randFpValue = min + 1.0 * rand() / RAND_MAX * (max - min);
	return randFpValue;
}

int
main()
{
	double result = 0;
	/*
	 * I try to pass the function name from command line to make it more automatic,
	 * but it's seemingly forbidden in C/C++.
	 * So we need to write the function name manually here.
	 * */
	for (int i = 0; i < 1000000; i++)
	{
#ifdef CONTROL_FLOW_FUNC
		result = controlFlowFunc(randomFloat(-16.0, 16.0));
#elif defined(LIBC_EXP)
        result = libc_exp(randomFloat(3.0, 10.0));
#elif defined(LIBC_LOG)
        result = libc_log(randomFloat(3.0, 10.0));
#elif defined(LIBC_ACOSH)
		result = libc_acosh(randomFloat(3.0, 10.0));
#elif defined(LIBC_J0)
        result = libc_j0(randomFloat(3.0, 10.0));
#elif defined(LIBC_Y0)
		result = libc_y0(randomFloat(3.0, 10.0));
#elif defined(LIBC_SINCOSF)
        result = libc_sincosf(randomFloat(3.0, 10.0));
#elif defined(FLOAT64_ADD)
        result = float64_add(randomFloat(3.0, 10.0));
#elif defined(FLOAT64_DIV)
        result = float64_div(randomFloat(3.0, 10.0));
#elif defined(FLOAT64_MUL)
        result = float64_mul(randomFloat(3.0, 10.0));
#elif defined(FLOAT64_SIN)
        result = float64_sin(randomFloat(3.0, 10.0));
#else
	#error "Benchmark function not defined"
#endif
	}

	return 0;
}
