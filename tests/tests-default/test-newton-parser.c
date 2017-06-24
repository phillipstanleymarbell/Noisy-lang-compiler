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

