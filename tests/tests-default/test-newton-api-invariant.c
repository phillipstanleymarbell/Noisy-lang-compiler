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
#include "newton-parser-expression.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton-api.h"

#include "minunit.h"
#include "test-utils.h"
#include "test-newton-api.h"
#include "probes.h"

extern int tests_run;



char * test_newtonApiInit_notNull()
{
	mu_assert(
        "test_newtonApiInit_notNull: newtonApiInit returns NULL!",
        newtonApiInit("../../Examples/reactor_rod.nt") != NULL
    );
    return 0;
}

char * test_newtonApiInit_notNullInvariant()
{
    State* newton = newtonApiInit("../../Examples/reactor_rod.nt");
	mu_assert(
        "test_newtonApiInit_notNullInvariant: invariantList for reactor_rod.nt is NULL!",
         newton->invariantList != NULL
    );

    return 0;
}

char * test_newtonApiGetPhysicsTypeByNameAndSubindex_Valid()
{
    State* newton = newtonApiInit("../../Examples/reactor_rod.nt");
    mu_assert(
        "newtonApiGetPhysicsTypeByNameAndSubindex: temperature second subdimension not found",
        newtonApiGetPhysicsTypeByNameAndSubindex(newton, "temperature", 1) != NULL
		);

    return 0;
}

char * test_newtonApiGetPhysicsTypeByName_Valid()
{
    State* newton = newtonApiInit("../../Examples/reactor_rod.nt");

    mu_assert(
        "newtonApiGetPhysicsTypeByName: time not found",
        !strcmp(
            newtonApiGetPhysicsTypeByName(newton, "time")->identifier,
            "time"
        )
    );

    mu_assert(
        "newtonApiGetPhysicsTypeByName: temperature not found",
        !strcmp(
            newtonApiGetPhysicsTypeByName(newton, "temperature")->identifier,
            "temperature"
        )
    );

    return 0;
}

char * test_newtonApiGetInvariantByParameters_Valid()
{
    State* newton = newtonApiInit("../../Examples/reactor_rod.nt");

    mu_assert(
        "test_newtonApiGetInvariantByParameters: the invariant is named RodCooling",
        !strcmp(
            newtonApiGetInvariantByParameters(
                newton,
                newton->invariantList->parameterList
            )->identifier,
            "RodCooling"
        )
    );

    return 0;
}

char * test_newtonCheckSingleInvariant()
{
	State * newton = newtonApiInit("../../Examples/reactor_rod.nt");
    IrNode* parameter = makeTestParameterTuple(newton);
    NEWTON_NEWTON_START();
    NewtonAPIReport * report = newtonApiSatisfiesConstraints(
				newton,
                parameter
				);
    NEWTON_NEWTON_DONE();
	mu_assert(
		"test_newtonCheckSingleInvariant reactor_rod.nt: number passed should be 1",
	    numberOfConstraintsPassed(
            report
			) == 1
		);

	return 0;
}


char * test_newtonApiDimensionCheckTree()
{
	State * newton = newtonApiInit("../../Examples/invariants.nt");

	ConstraintReport* report = newtonApiDimensionCheckTree(newton, makeSampleCorrectTestStatement());
	assert(report->satisfiesDimensionConstraint);
	mu_assert(
		"invariants.nt: correct test statements should pass dimension constraint",
		report->satisfiesDimensionConstraint
		);

	report = newtonApiDimensionCheckTree(newton, makeSampleIncorrectTestStatement());
	assert(!report->satisfiesDimensionConstraint);
	mu_assert(
		"invariants.nt: incorrect test statements should not pass dimension constraint",
		!report->satisfiesDimensionConstraint
		);

	newton = newtonApiInit("../../Examples/pendulum_acceleration.nt");
	report = newtonApiDimensionCheckTree(newton, makeSampleCorrectTestStatementPendulumCase());
	assert(report->satisfiesDimensionConstraint);
	mu_assert(
		"pendulum_acceleration.nt: correct test statements should pass dimension constraint",
		report->satisfiesDimensionConstraint
		);

	report = newtonApiDimensionCheckTree(newton, makeSampleIncorrectTestStatementPendulumCase());
	assert(!report->satisfiesDimensionConstraint);
	mu_assert(
		"pendulum_acceleration.nt: incorrect test statements should not pass dimension constraint",
		!report->satisfiesDimensionConstraint
		);

	newton = newtonApiInit("../../Examples/pressure_sensors.nt");
	report = newtonApiDimensionCheckTree(newton, makeSampleCorrectTestStatementPressureCase());
	assert(report->satisfiesDimensionConstraint);
	mu_assert(
		"pressure_sensors.nt: correct test statements should pass dimension constraint",
		report->satisfiesDimensionConstraint
		);

	report = newtonApiDimensionCheckTree(newton, makeSampleIncorrectTestStatementPressureCase());
	assert(!report->satisfiesDimensionConstraint);
	mu_assert(
		"pressure_sensors.nt: incorrect test statements should not pass dimension constraint",
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
    State * newton = newtonApiInit("../../Examples/invariants.nt");

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

    newton = newtonApiInit("../../Examples/pendulum_acceleration.nt");
    distanceNode->physics = newtonApiGetPhysicsTypeByName(newton, distanceNode->token->identifier);
    timeNode->physics = newtonApiGetPhysicsTypeByName(newton, timeNode->token->identifier);

    mu_assert(
        "test_newtonApiTypeExpressionExample pendulum_acceleration.nt: time and distance id's should be different and cannot be used in add or subtract",
        distanceNode->physics->id != timeNode->physics->id
		);


    IrNode * pressure = makeIrNodeSetValue(
        noisy,
        kNoisyIrNodeType_Tidentifier,
        "pressure",
        0.0
		);

    IrNode * temperature= makeIrNodeSetValue(
        noisy,
        kNoisyIrNodeType_Tidentifier,
        "temperature",
        0.0
		);

    newton = newtonApiInit("../../Examples/pressure_sensors.nt");
    pressure->physics = newtonApiGetPhysicsTypeByName(newton, pressure->token->identifier);
    temperature->physics = newtonApiGetPhysicsTypeByName(newton, temperature->token->identifier);

    mu_assert(
        "test_newtonApiTypeExpressionExample pendulum_acceleration.nt: time and distance id's should be different and cannot be used in add or subtract",
        pressure->physics->id != temperature->physics->id
		);

    return 0;
}


char * test_newtonApiNumberParametersZeroToN()
{
	State * newton = newtonApiInit("../../Examples/invariants.nt");
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
		"test_newtonApiNumberParametersZeroToN: Pendulum Case the first left child should have number of 0",
		parameterTree->irLeftChild->parameterNumber == 0
		);
	mu_assert(
		"test_newtonApiNumberParametersZeroToN: Pendulum Case the first right child should have number of 1",
		parameterTree->irRightChild->irLeftChild->parameterNumber == 1
		);

	IrNode* parameterTrees[3] = {
		makeTestParameterTuplePressureCaseBoyles(),
		makeTestParameterTuplePressureCaseGayLussac(),
		makeTestParameterTuplePressureCaseAvogadro(),
	};

	for (int index = 0; index < 3; index++)
	{
		mu_assert(
			"test_newtonApiNumberParametersZeroToN: Pressure Case the first left child should have number of 0",
			parameterTrees[index]->irLeftChild->parameterNumber == 0
			);
		mu_assert(
			"test_newtonApiNumberParametersZeroToN: Pressure Case the first right child should have number of 1",
			parameterTrees[index]->irRightChild->irLeftChild->parameterNumber == 1
			);
	}
	return 0;
}
