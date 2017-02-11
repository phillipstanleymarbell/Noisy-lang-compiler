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
#include "test-newton-api.h"

int tests_run = 0;

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
    NoisyState* N = newtonApiInit("../Examples/invariants.nt");
	mu_assert(
        "test_newtonApiInit_notNullInvariant: invariantList is NULL!", 
         N->invariantList != NULL
    );
    return 0;
}

