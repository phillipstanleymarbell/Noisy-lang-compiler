#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
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

extern unsigned long int bigNumberOffset;
extern int tests_run;


char * test_newtonGetPhysicsId()
{
    State* N = newtonApiInit("../Examples/invariants.nt");
    mu_assert(
        "test_newtonGetPhysicsId: first Physics struct should have the first prime number as id",
        newtonApiGetPhysicsTypeByName(N, "time")->id == 2
    );

    mu_assert(
        "test_newtonGetPhysicsId: second Physics struct should have the second prime number as id",
        newtonApiGetPhysicsTypeByName(N, "distance")->id == 3
    );

    mu_assert(
        "test_newtonGetPhysicsId: speed should have the fourth prime number as id",
        newtonApiGetPhysicsTypeByName(N, "speed")->id == 7
    );

    mu_assert(
        "test_newtonGetPhysicsId: acceleration should have the fifth prime number as id",
        newtonApiGetPhysicsTypeByName(N, "acceleration")->id == 11
    );

    mu_assert(
        "test_newtonGetPhysicsId: force should have the sixth prime number as id",
        newtonApiGetPhysicsTypeByName(N, "force")->id == 13
    );
    return 0;
}

char * test_newtonGetInvariantIdByParameters()
{
    State* newton = newtonApiInit("../Examples/invariants.nt");

    mu_assert(
        "test_newtonGetInvariantIdByParameters: invariant id should be distance * time",
        newtonGetInvariantIdByParameters(
            newton,
            makeTestParameterTuple(newton),
            1
        ) == 3 * 2
    );
    return 0;
}

