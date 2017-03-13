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

static int numberOfConstraintsPassed(NewtonAPIReport* newtonReport);

static int
numberOfConstraintsPassed(NewtonAPIReport* newtonReport)
{
  int count = 0;
  ConstraintReport* current = newtonReport->firstConstraintReport;

  while (current != NULL)
    {
      // printf("satisfiesValueConstraint %d\n", current->satisfiesValueConstraint);
      // printf("satisfiesDimensionConstraint %d\n\n", current->satisfiesDimensionConstraint);
      // printf("valueErrorMessage %s\n", current->valueErrorMessage);
      // printf("dimensionErrorMessage %s\n\n", current->dimensionErrorMessage);
      if (current->satisfiesValueConstraint && current->satisfiesDimensionConstraint)
        count++;
      current = current->next;
    }

  return count;
}

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
    State* N = newtonApiInit("../Examples/invariants.nt");
	mu_assert(
        "test_newtonApiInit_notNullInvariant: invariantList is NULL!",
         N->invariantList != NULL
    );
    return 0;
}

char * test_newtonApiGetPhysicsTypeByName_Valid()
{
    State* N = newtonApiInit("../Examples/invariants.nt");
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
    State* N = newtonApiInit("../Examples/invariants.nt");

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

/*
 * This example shows how a host language compiler (e.g. Noisy compiler) would check 
 * if two IR nodes are dimensionally equivalent during the parsing step.
 * Let's say the Noisy compiler encounters two variables of type distance and time.
 * The Noisy compiler can look up Physics * struct's corresponding to distance and time
 * and then compare the ID's to see if they should be equal.
 */
char * test_newtonApiPhysicsTypeUsageExample()
{
    State * noisy = noisyInit(kNoisyModeDefault);
    State * newton = newtonApiInit("../Examples/invariants.nt");

    IrNode * distanceNode = makeNoisyIrNodeSetValue(
        noisy,
        kNoisyIrNodeType_Tidentifier,
        "distance",
        0.0
    );

    IrNode * timeNode = makeNoisyIrNodeSetValue(
        noisy,
        kNoisyIrNodeType_Tidentifier,
        "time",
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
  return 0;
}


IrNode *
makeTestParameterTuple(State * newton)
{
  IrNode *	root = genNoisyIrNode(newton,	kNewtonIrNodeType_PparameterTuple,
									  NULL /* left child */,
									  NULL /* right child */,
									  NULL /* source info */);
  IrNode * distanceParameter = makeNoisyIrNodeSetValue(
													   newton,
                             kNewtonIrNodeType_Pparameter,
                             "distance",
                             5
													   );
  distanceParameter->physics = newtonApiGetPhysicsTypeByName(newton, distanceParameter->token->identifier);
  newtonApiAddLeaf(newton, root, distanceParameter);

  IrNode * timeParameter = makeNoisyIrNodeSetValue(
												   newton,
												   kNewtonIrNodeType_Pparameter,
												   "time",
                           6.6
												   );
  timeParameter->physics = newtonApiGetPhysicsTypeByName(newton, timeParameter->token->identifier);
  newtonApiAddLeafWithChainingSeqNoLexer(newton, root, timeParameter);

  newtonApiNumberParametersZeroToN(newton, root);
  return root;
}

IrNode *
makeNoisyIrNodeSetValue(
    State * N,
    IrNodeType nodeType,
    char * identifier,
    double realConst
) {
	IrNode * node = genNoisyIrNode(
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
                                      NULL  /* stringConst	*/,
                                      NULL	/* sourceInfo	*/
                                      );

  node->value = node->token->integerConst + node->token->realConst; /* this works because either one is always zero */

    return node;
}
