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
#include "common-irHelpers.h"
#include "common-lexers-helpers.h"
#include "newton-parser.h"
#include "newton-parser-expression.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton-api.h"

#include "minunit.h"
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
    NoisyState* N = newtonApiInit("../Examples/invariants.nt");
	mu_assert(
        "test_newtonApiInit_notNullInvariant: invariantList is NULL!",
         N->invariantList != NULL
    );
    return 0;
}

char * test_newtonApiGetPhysicsTypeByName_Valid()
{
    NoisyState* N = newtonApiInit("../Examples/invariants.nt");
    mu_assert(
        "newtonApiGetPhysicsTypeByName: distance not found",
        !strcmp(
            newtonApiGetPhysicsTypeByName(N, "distance")->identifier,
            "distance"
        )
    );

    mu_assert(
        "newtonApiGetPhysicsTypeByName: acceleration not found",
        !strcmp(
            newtonApiGetPhysicsTypeByName(N, "acceleration")->identifier,
            "acceleration"
        )
    );

    return 0;
}

char * test_newtonApiGetInvariantByParameters_Valid()
{
    NoisyState* N = newtonApiInit("../Examples/invariants.nt");

    mu_assert(
        "test_newtonApiGetInvariantByParameters: the invariant is named SimplePendulum",
        !strcmp(
            newtonApiGetInvariantByParameters(
                N,
                N->invariantList->parameterList
            )->identifier,
            "SimplePendulum"
        )
    );
    return 0;
}

char * test_newtonCheckSingleInvariant()
{
  NoisyState * newton = newtonApiInit("../Examples/invariants.nt");
  NoisyIrNode* parameterTree = makeTestParameterTuple(newton);
  NewtonAPIReport* report = newtonApiSatisfiesConstraints(
													   newton,
													   parameterTree
													   );
  mu_assert(
			"test_newtonCheckSingleInvariant: the report should say success",
			report->satisfiesDimensionConstraint && report->satisfiesValueConstraint
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
    NoisyState * noisy = noisyInit(kNoisyModeDefault);
    NoisyState * newton = newtonApiInit("../Examples/invariants.nt");

    NoisyIrNode * distanceNode = makeNoisyIrNodeSetToken(
        noisy,
        kNoisyIrNodeType_Tidentifier,
        "distance",
        NULL,
        0.0
    );

    NoisyIrNode * timeNode = makeNoisyIrNodeSetToken(
        noisy,
        kNoisyIrNodeType_Tidentifier,
        "time",
        NULL,
        0.0
    );

    distanceNode->physics = newtonApiGetPhysicsTypeByName(newton, distanceNode->token->identifier);
    timeNode->physics = newtonApiGetPhysicsTypeByName(newton, timeNode->token->identifier);

    mu_assert(
        "test_newtonApiTypeExpressionExample: time and distance id's should be different and cannot be used in add or subtract",
        distanceNode->physics->id != timeNode->physics->id
    );

    return 0;
}


NoisyIrNode *
makeTestParameterTuple(NoisyState * newton)
{
  NoisyIrNode *	root = genNoisyIrNode(newton,	kNewtonIrNodeType_PparameterTuple,
									  NULL /* left child */,
									  NULL /* right child */,
									  NULL /* source info */);
  NoisyIrNode * firstParameter = genNoisyIrNode(newton,	kNewtonIrNodeType_Pparameter,
												NULL /* left child */,
												NULL /* right child */,
												NULL /* source info */);
  NoisyIrNode * distanceIdentifier = makeNoisyIrNodeSetToken(
													   newton,
													   kNoisyIrNodeType_Tidentifier,
													   "L",
													   NULL,
													   5
													   );
  NoisyIrNode * distanceNode = makeNoisyIrNodeSetToken(
													   newton,
													   kNoisyIrNodeType_Tidentifier,
													   "distance",
													   NULL,
													   0.0
													   );
  distanceNode->physics = newtonApiGetPhysicsTypeByName(newton, distanceNode->token->identifier);
  addLeaf(newton, firstParameter, distanceIdentifier);
  addLeaf(newton, firstParameter, distanceNode);

  NoisyIrNode * secondParameter = genNoisyIrNode(newton,	kNewtonIrNodeType_Pparameter,
												 NULL /* left child */,
												 NULL /* right child */,
												 NULL /* source info */);
  NoisyIrNode * timeIdentifier = makeNoisyIrNodeSetToken(
															 newton,
															 kNoisyIrNodeType_Tidentifier,
															 "period",
															 NULL,
															 6.6
															 );
  NoisyIrNode * timeNode = makeNoisyIrNodeSetToken(
												   newton,
												   kNoisyIrNodeType_Tidentifier,
												   "time",
												   NULL,
												   0.0
												   );
  timeNode->physics = newtonApiGetPhysicsTypeByName(newton, timeNode->token->identifier);
  addLeaf(newton, secondParameter, timeIdentifier);
  addLeaf(newton, secondParameter, timeNode);

  addLeaf(newton, root, firstParameter);
  addLeafWithChainingSeqNoLexer(newton, root, secondParameter);

  return root;
}

NoisyIrNode *
makeNoisyIrNodeSetToken(
    NoisyState * N,
    NoisyIrNodeType nodeType,
    char * identifier,
    char * stringConst,
    double realConst
) {
	NoisyIrNode * node = genNoisyIrNode(
        N,
        nodeType,
	    NULL /* left child */,
	    NULL /* right child */,
	    NULL /* source info */
    );

  node->token = noisyLexAllocateToken(
                                      N,
                                      nodeType /* type */,
                                      identifier /* identifier */,
                                      0	/* integerConst	*/,
                                      realConst	/* realConst	*/,
                                      stringConst /* stringConst	*/,
                                      NULL	/* sourceInfo	*/
                                      );

    node->value = node->token->integerConst || node->token->realConst;

    return node;
}

