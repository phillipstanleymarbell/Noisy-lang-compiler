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
#include "newton-parser.h"
#include "newton-parser-expression.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton-api.h"

#include "minunit.h"
#include "test-newton-parser.h"

extern unsigned long int bigNumberOffset;

char * test_getPhysicsTypeByName_Valid() 
{
    NoisyState* N = newtonApiInit("../Examples/invariants.nt");
    mu_assert(
        "getPhysicsTypeByName: distance not found",
        !strcmp(
            getPhysicsTypeByName(N, "distance")->identifier,
            "distance"
        )
    );

    mu_assert(
        "getPhysicsTypeByName: acceleration not found",
        !strcmp(
            getPhysicsTypeByName(N, "acceleration")->identifier,
            "acceleration"
        )
    );

    return 0;
}

char * test_newtonGetPhysicsId()
{
    NoisyState* N = newtonApiInit("../Examples/invariants.nt");
    mu_assert(
        "test_newtonGetPhysicsId: first Physics struct should have first prime number as id",
        getPhysicsTypeByName(N, "time")->id == 2
    );
    
    mu_assert(
        "test_newtonGetPhysicsId: second Physics struct should have second prime number as id",
        getPhysicsTypeByName(N, "distance")->id == 3
    );
    
    mu_assert(
        "test_newtonGetPhysicsId: speed should have distance->id + offset * time->id as id",
        getPhysicsTypeByName(N, "speed")->id == 3 + bigNumberOffset * 2
    );
    
    mu_assert(
        "test_newtonGetPhysicsId: acceleration should have distance->id + offset * time->id * time->id as id",
        getPhysicsTypeByName(N, "acceleration")->id == 3 + bigNumberOffset * 2 * 2
    );
    
    mu_assert(
        "test_newtonGetPhysicsId: force should have mass * distance + offset * time * time as id",
        getPhysicsTypeByName(N, "force")->id == 3 * 5 + bigNumberOffset * 2 * 2
    );
    return 0;
}

char * test_newtonGetInvariantIdByParameters()
{
    NoisyState* N = newtonApiInit("../Examples/invariants.nt");

    mu_assert(
        "test_newtonGetInvariantIdByParameters: invariant id should be distance * time",
        newtonGetInvariantIdByParameters(
            N, 
            N->invariantList->parameterList,
            1
        ) == 3 * 2
    );
    return 0;
}

