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

#include "common-timeStamps.h"
#include "data-structures.h"
#include "newton-data-structures.h"
#include "common-irHelpers.h"
#include "common-lexers-helpers.h"
#include "newton-parser.h"
#include "newton-parser-expression.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton-api.h"

#include "minunit.h"
#include "test-utils.h"
#include "test-newton-api.h"

extern int tests_run;



char * test_newtonApiInit_notNull()
{
	mu_assert(
        "test_newtonApiInit_notNull: newtonApiInit returns NULL!",
        newtonApiInit("../Examples/invariants.nt") != NULL
    );
    return 0;
}

char * test_newtonApiInit_notNullInvariant()
{
    State* newton = newtonApiInit("../Examples/invariants.nt");
	mu_assert(
        "test_newtonApiInit_notNullInvariant: invariantList for invariants.nt is NULL!",
         newton->invariantList != NULL
    );

    newton = newtonApiInit("../Examples/pendulum_acceleration.nt");
	mu_assert(
        "test_newtonApiInit_notNullInvariant: invariantList is pendulum_acceleration.nt is NULL!",
		newton->invariantList != NULL
		);
    return 0;
}

char * test_newtonApiGetPhysicsTypeByNameAndSubindex_Valid()
{
    State* newton = newtonApiInit("../Examples/pendulum_acceleration.nt");
    mu_assert(
        "newtonApiGetPhysicsTypeByNameAndSubindex: distance on Y axis not found",
        newtonApiGetPhysicsTypeByNameAndSubindex(newton, "distance", 1) != NULL
		);
    mu_assert(
        "newtonApiGetPhysicsTypeByNameAndSubindex: acceleration on Z axis not found",
        newtonApiGetPhysicsTypeByNameAndSubindex(newton, "acceleration", 2) != NULL
		);

    return 0;
}

char * test_newtonApiGetPhysicsTypeByName_Valid()
{
    State* newton = newtonApiInit("../Examples/invariants.nt");
    mu_assert(
        "newtonApiGetPhysicsTypeByName: distance not found",
        !strcmp(
            newtonApiGetPhysicsTypeByName(newton, "distance")->identifier,
            "distance"
        )
    );
    mu_assert(
        "newtonApiGetPhysicsTypeByName: acceleration not found",
        !strcmp(
            newtonApiGetPhysicsTypeByName(newton, "acceleration")->identifier,
            "acceleration"
        )
    );

    return 0;
}

char * test_newtonApiGetInvariantByParameters_Valid()
{
    State* newton = newtonApiInit("../Examples/invariants.nt");

    mu_assert(
        "test_newtonApiGetInvariantByParameters: the invariant is named SimplePendulum",
        !strcmp(
            newtonApiGetInvariantByParameters(
                newton,
                newton->invariantList->parameterList
            )->identifier,
            "SimplePendulum"
        )
    );

    newton = newtonApiInit("../Examples/pendulum_acceleration.nt");
    mu_assert(
        "test_newtonApiGetInvariantByParameters: the invariant is named AccelerationRange",
        !strcmp(
            newtonApiGetInvariantByParameters(
                newton,
                newton->invariantList->parameterList
				)->identifier,
            "AccelerationRange"
			)
		);
    return 0;
}

char * test_newtonCheckSingleInvariant()
{
	State * newton = newtonApiInit("../Examples/invariants.nt");
	IrNode* parameterTree = makeTestParameterTuple(newton);
	NewtonAPIReport* newtonReport = newtonApiSatisfiesConstraints(
		newton,
		parameterTree
		);
	int numberPassed = numberOfConstraintsPassed(newtonReport);

	mu_assert(
		"test_newtonCheckSingleInvariant: number passed should be 5",
		numberPassed == 5
		);
	return 0;
}

char * test_newtonApiDimensionCheckTree()
{
	State * newton = newtonApiInit("../Examples/invariants.nt");

	ConstraintReport* report = newtonApiDimensionCheckTree(newton, makeSampleCorrectTestStatement());
	assert(report->satisfiesDimensionConstraint);
	mu_assert(
		"correct test statements should pass dimension constraint",
		report->satisfiesDimensionConstraint
		);

	report = newtonApiDimensionCheckTree(newton, makeSampleIncorrectTestStatement());
	assert(!report->satisfiesDimensionConstraint);
	mu_assert(
		"incorrect test statements should not pass dimension constraint",
		!report->satisfiesDimensionConstraint
		);

	return 0;
}

/*
 * This example shows how a host language compiler (e.g. Noisy compiler) would check 
 * if two IR nodes are dimensionally equivalent during the parsing step.
 * Let's say the Noisy compiler encounters two variables of type distance and time.
 * The Noisy compiler can look up Physics * struct's corresponding to distance and time
 * and then compare the ID's to see if they should be equal.
 */
char * test_newtonApiPhysicsTypeUsageExample()
{
    State * noisy = init(kNoisyModeDefault);
    State * newton = newtonApiInit("../Examples/invariants.nt");

    IrNode * distanceNode = makeIrNodeSetValue(
        noisy,
        kNoisyIrNodeType_Tidentifier,
        "distance",
        0.0
    );

    IrNode * timeNode = makeIrNodeSetValue(
        noisy,
        kNoisyIrNodeType_Tidentifier,
        "time",
        0.0
    );

    distanceNode->physics = newtonApiGetPhysicsTypeByName(newton, distanceNode->token->identifier);
    timeNode->physics = newtonApiGetPhysicsTypeByName(newton, timeNode->token->identifier);

    mu_assert(
        "test_newtonApiTypeExpressionExample invariants.nt: time and distance id's should be different and cannot be used in add or subtract",
        distanceNode->physics->id != timeNode->physics->id
    );

    newton = newtonApiInit("../Examples/pendulum_acceleration.nt");
    distanceNode->physics = newtonApiGetPhysicsTypeByName(newton, distanceNode->token->identifier);
    timeNode->physics = newtonApiGetPhysicsTypeByName(newton, timeNode->token->identifier);

    mu_assert(
        "test_newtonApiTypeExpressionExample pendulum_acceleration.nt: time and distance id's should be different and cannot be used in add or subtract",
        distanceNode->physics->id != timeNode->physics->id
		);


    return 0;
}


char * test_newtonApiNumberParametersZeroToN()
{
	State * newton = newtonApiInit("../Examples/invariants.nt");
	IrNode* parameterTree = makeTestParameterTuple(newton);
	mu_assert(
		"test_newtonApiNumberParametersZeroToN: the first left child should have number of 0",
		parameterTree->irLeftChild->parameterNumber == 0
		);
	mu_assert(
		"test_newtonApiNumberParametersZeroToN: the first right child should have number of 1",
		parameterTree->irRightChild->irLeftChild->parameterNumber == 1
		);

	parameterTree = makeTestParameterTuplePendulumCase();
	mu_assert(
		"test_newtonApiNumberParametersZeroToN: the first left child should have number of 0",
		parameterTree->irLeftChild->parameterNumber == 0
		);
	mu_assert(
		"test_newtonApiNumberParametersZeroToN: the first right child should have number of 1",
		parameterTree->irRightChild->irLeftChild->parameterNumber == 1
		);
	return 0;
}

