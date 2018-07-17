/*
	Authored 2017. Jonathan Lim.

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
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"

#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "newton-data-structures.h"
#include "newton-parser.h"
#include "newton-parser-expression.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton-api.h"

#include "minunit.h"
#include "test-newton-api.h"
#include "test-newton-parser.h"
#include "test-utils.h"

extern unsigned long int bigNumberOffset;
extern int primeNumbers[168];
extern int tests_run;


char * test_newtonGetPhysicsId()
{
    State* newton = newtonApiInit("../../Examples/invariants.nt");
    mu_assert(
        "test_newtonGetPhysicsId: first Physics struct should have the first prime number as id",
        newtonApiGetPhysicsTypeByName(newton, "time")->id == 2
    );

    mu_assert(
        "test_newtonGetPhysicsId: second Physics struct should have the second prime number as id",
        newtonApiGetPhysicsTypeByName(newton, "distance")->id == 3
    );

	assert(newtonApiGetPhysicsTypeByName(newton, "speed")->id == 13);
    mu_assert(
        "test_newtonGetPhysicsId: speed should have the sixth prime number as id",
        newtonApiGetPhysicsTypeByName(newton, "speed")->id == 13
    );

    mu_assert(
        "test_newtonGetPhysicsId: acceleration should have the ninth prime number as id",
        newtonApiGetPhysicsTypeByName(newton, "acceleration")->id == 23
    );

    mu_assert(
        "test_newtonGetPhysicsId: force should have the twelvth prime number as id",
        newtonApiGetPhysicsTypeByName(newton, "force")->id == 37
    );
    return 0;
}

char * test_newtonGetInvariantIdByParameters()
{
    State* newton = newtonApiInit("../../Examples/invariants.nt");

    mu_assert(
        "test_newtonGetInvariantIdByParameters: invariant id should be distance * time",
        newtonGetInvariantIdByParameters(
            newton,
            makeTestParameterTuple(newton),
            1
        ) == primeNumbers[0] * primeNumbers[1]
    );

	assert(
        newtonGetInvariantIdByParameters(
            newton,
            makeTestParameterTuplePendulumCase(),
            1
			) == primeNumbers[7] * primeNumbers[8] * primeNumbers[9]
		);

    mu_assert(
        "test_newtonGetInvariantIdByParameters: invariant id should be accelerationX * Y * Z",
        newtonGetInvariantIdByParameters(
            newton,
            makeTestParameterTuplePendulumCase(),
            1
			) == primeNumbers[7] * primeNumbers[8] * primeNumbers[9]
		);
    return 0;
}

