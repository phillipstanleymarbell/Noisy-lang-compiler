/*
	Authored 2016. Jonathan Lim.

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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <sys/time.h>
#include <getopt.h>
#include <setjmp.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "version.h"
#include "common-timeStamps.h"
#include "data-structures.h"
#include "newton-data-structures.h"

#include "newton-parser.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton.h"
#include "newton-api.h"
#include "common-irHelpers.h"
#include "newton-check-pass.h"

extern int		gNewtonFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];

static void
addConstraintReportToNewtonAPIReport(NewtonAPIReport * newtonReport, ConstraintReport * constraintReport);


static void
addConstraintReportToNewtonAPIReport(NewtonAPIReport * newtonReport, ConstraintReport * constraintReport)
{
  if (newtonReport->firstConstraintReport == NULL)
	{
	  newtonReport->firstConstraintReport = constraintReport;
	  return;
	}

  ConstraintReport* current = newtonReport->firstConstraintReport;
  while (current->next != NULL)
	{
	  current = current->next;
	}

  current->next = constraintReport;
}


void
newtonCheckCompareOp(
    NoisyState* N,
    NoisyIrNode * leftExpression,
    NoisyIrNode * rightExpression,
    NoisyIrNodeType compareOpType,
    ConstraintReport * report
) {

    switch(compareOpType)
    {
        case kNewtonIrNodeType_Tge:
            report->satisfiesValueConstraint = leftExpression->value >= rightExpression->value;
            report->satisfiesDimensionConstraint = (newtonIsDimensionless(leftExpression->physics) && newtonIsDimensionless(rightExpression->physics)) ||\
              (leftExpression->physics->id == rightExpression->physics->id);
            if (!report->satisfiesValueConstraint)
                sprintf(report->valueErrorMessage, "%f should be >= %f", leftExpression->value, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;
        case kNewtonIrNodeType_Tgt:
            report->satisfiesValueConstraint = leftExpression->value > rightExpression->value;
            report->satisfiesDimensionConstraint = (newtonIsDimensionless(leftExpression->physics) && newtonIsDimensionless(rightExpression->physics)) ||\
			  (leftExpression->physics->id == rightExpression->physics->id);
            if (!report->satisfiesValueConstraint)
                sprintf(report->valueErrorMessage, "%f should be > %f", leftExpression->value, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;
        case kNewtonIrNodeType_Tle:
            report->satisfiesValueConstraint = leftExpression->value <= rightExpression->value;
            report->satisfiesDimensionConstraint = (newtonIsDimensionless(leftExpression->physics) && newtonIsDimensionless(rightExpression->physics)) ||\
                (leftExpression->physics->id == rightExpression->physics->id);
            if (!report->satisfiesValueConstraint)
                sprintf(report->valueErrorMessage, "%f should be <= %f", leftExpression->value, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;
        case kNewtonIrNodeType_Tlt:
            report->satisfiesValueConstraint = leftExpression->value < rightExpression->value;
            report->satisfiesDimensionConstraint = (newtonIsDimensionless(leftExpression->physics) && newtonIsDimensionless(rightExpression->physics)) ||\
                (leftExpression->physics->id == rightExpression->physics->id);
            if (!report->satisfiesValueConstraint)
                sprintf(report->valueErrorMessage, "%f should be < %f", leftExpression->value, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;
        case kNewtonIrNodeType_Tproportionality:
            /* TODO figure out how to check proportionality */
		  report->satisfiesValueConstraint = true;
		  report->satisfiesDimensionConstraint = true;
			break;
        case kNewtonIrNodeType_Tequivalent:
            /* TODO change == to incorporate an epsilon */
            report->satisfiesValueConstraint = leftExpression->value == rightExpression->value;
            report->satisfiesDimensionConstraint = (newtonIsDimensionless(leftExpression->physics) && newtonIsDimensionless(rightExpression->physics)) ||\
                (leftExpression->physics->id == rightExpression->physics->id);
            if (!report->satisfiesValueConstraint)
                sprintf(report->valueErrorMessage, "%f should be == %f", leftExpression->value, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;
        default:
            noisyFatal(N, "newton-api.c:newtonCheckCompareOp: no compareOp detected");
            break;
    }
	// assert(constraintReport->satisfiesValueConstraint);
	// assert(constraintReport->satisfiesDimensionConstraint);
}

void
newtonCheckBinOp(
    NoisyState* N,
    NoisyIrNode * left,
    NoisyIrNode * right,
    NoisyIrNodeType binOpType,
    ConstraintReport * report
) {
	if (left == NULL || right == NULL)
	    noisyFatal(N, "newton-check-pass.c:newtonCheckBinOp: left or right is null");
    switch(binOpType)
    {
        case kNewtonIrNodeType_Tplus:
            left->value += right->value;
            report->satisfiesDimensionConstraint =
                (newtonIsDimensionless(left->physics) && newtonIsDimensionless(right->physics)) ||\
                (left->physics->id == right->physics->id);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;

        case kNewtonIrNodeType_Tminus:
            left->value -= right->value;
            report->satisfiesDimensionConstraint =
                (newtonIsDimensionless(left->physics) && newtonIsDimensionless(right->physics)) ||\
			  (left->physics->id == right->physics->id);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;

        /*
         * Note that we don't need to copy over any dimensions as in Parsing
         * because they are already filled in. Same for division and exponents
         */
        case kNewtonIrNodeType_Tmul:
			left->value = left->value == 0 ? 1 : left->value;
			right->value = right->value == 0 ? 1 : right->value;
            left->value *= right->value;
            report->satisfiesDimensionConstraint = true;
            break;

        case kNewtonIrNodeType_Tdiv:
		  left->value = left->value == 0 ? 1 : left->value;
		  right->value = right->value == 0 ? 1 : right->value;
            left->value /= right->value;
            report->satisfiesDimensionConstraint = true; /* The drawback of this approach is that we can't catch divide by 0 */
            break;

        case kNewtonIrNodeType_Texponent:
            /*
             * left is base and right is exponential expression.
             * The value of the exponential expression is always dimensionless and should
             * already have been calculated by Newton description parser.
             */
            report->satisfiesValueConstraint = left->value != 0 || right->value != 0;
            left->value = pow(left->value, right->value);

            break;
        default:
            noisyFatal(N, "newton-api.c:newtonCheckCompareOp: no binOp detected");
            break;
    }
}

void
iterateConstraints(NoisyState * N, NoisyIrNode * constraintTreeRoot, NoisyIrNode* parameterTreeRoot, NewtonAPIReport* report)
{
    if (constraintTreeRoot->type == kNewtonIrNodeType_Pconstraint)
    {
      checkSingleConstraint(N, constraintTreeRoot, parameterTreeRoot, report);
    }

    if (constraintTreeRoot->irLeftChild != NULL)
	  	iterateConstraints(N, constraintTreeRoot->irLeftChild, parameterTreeRoot, report);

    if (constraintTreeRoot->irRightChild != NULL)
	  	iterateConstraints(N, constraintTreeRoot->irRightChild, parameterTreeRoot, report);
}

void
checkSingleConstraint(NoisyState * N, NoisyIrNode * constraintTreeRoot, NoisyIrNode* parameterTreeRoot, NewtonAPIReport* newtonReport)
{
    int expressionIndex = 0;
	ConstraintReport * constraintReport = (ConstraintReport*) calloc(1, sizeof(ConstraintReport));

    /* check LHS expression */
    NoisyIrNode* leftExpression = findNthIrNodeOfType(
        N,
        constraintTreeRoot,
        kNewtonIrNodeType_PquantityExpression,
        expressionIndex
    );
    expressionIndex++;
    leftExpression->value = checkQuantityExpression(
        N,
        leftExpression,
        parameterTreeRoot,
        constraintReport
    );

    /* find the compareOp between LHS and RHS*/
    int compareOpIndex = 0;
    NoisyIrNode* compareOp = findNthIrNodeOfTypes(
        N,
        constraintTreeRoot,
        kNewtonIrNodeType_PcompareOp,
        gNewtonFirsts,
        compareOpIndex
    );
    /* There must be one compareOp per constraint */
    assert(compareOp != NULL);

    NoisyIrNode* rightExpression = findNthIrNodeOfType(
        N,
        constraintTreeRoot,
        kNewtonIrNodeType_PquantityExpression,
        expressionIndex
    );
    expressionIndex++;
    rightExpression->value = checkQuantityExpression(
        N,
        rightExpression,
        parameterTreeRoot,
        constraintReport
    );

    newtonCheckCompareOp(
        N,
        leftExpression,
        rightExpression,
        compareOp->token->type,
        constraintReport
    );

	addConstraintReportToNewtonAPIReport(newtonReport, constraintReport);
}

double
checkQuantityExpression(
    NoisyState * N,
    NoisyIrNode * expressionRoot,
    NoisyIrNode * parameterTreeRoot,
    ConstraintReport * report
) {
    int termIndex = 0;
    NoisyIrNode* leftTerm = findNthIrNodeOfType(
        N,
        expressionRoot,
        kNewtonIrNodeType_PquantityTerm,
        termIndex
    );
    termIndex++;
    assert(leftTerm != NULL);
    leftTerm->value = checkQuantityTerm(
        N,
        leftTerm,
        parameterTreeRoot,
        report
    );

    int lowBinOpIndex = 0;
    NoisyIrNode* lowBinOpNode = findNthIrNodeOfTypes(
        N,
        expressionRoot,
        kNewtonIrNodeType_PlowPrecedenceBinaryOp,
        gNewtonFirsts,
        lowBinOpIndex
    );
    lowBinOpIndex++;

    while (lowBinOpNode != NULL)
    {
        NoisyIrNode* rightTerm = findNthIrNodeOfType(
            N,
            expressionRoot,
            kNewtonIrNodeType_PquantityTerm,
            termIndex
        );
        termIndex++;
        assert(	rightTerm != NULL );
        rightTerm->value = checkQuantityTerm(
            N,
            rightTerm,
            parameterTreeRoot,
            report
        );

        newtonCheckBinOp(
            N,
            leftTerm,
            rightTerm,
            lowBinOpNode->type,
            report
        );

        lowBinOpNode = findNthIrNodeOfTypes(
            N,
            expressionRoot,
            kNewtonIrNodeType_PlowPrecedenceBinaryOp,
            gNewtonFirsts,
            lowBinOpIndex
        );
        lowBinOpIndex++;
    }
	return leftTerm->value;
}

double
checkQuantityTerm(
    NoisyState * N,
    NoisyIrNode * termRoot,
    NoisyIrNode * parameterTreeRoot,
    ConstraintReport * report
) {
    int factorIndex = 0;

	/*
	 * This is part of value propagation. Some factor may have a value set, some may not.
	 * For example, in the term  3 * meter, 3 has a value of 3 and meter has value of 0.
	 * If at least one factor in a term has a value set, then the value for the entire term must be set as well.
	 * If nothing has any value set, set the value of the entire term to 0.
	 * Note that having a value set to 0 and not having any value set at all will have the same effect
	 * since the value for the entire term will be 0.
	 *
	 * This is also how the values are propagated in newtonParseQuantityExpression.
	 */

    NoisyIrNode* leftFactor = findNthIrNodeOfTypes(
        N,
        termRoot,
        kNewtonIrNodeType_PquantityFactor,
        gNewtonFirsts,
        factorIndex
    );
    factorIndex++;
    assert(leftFactor != NULL);

    checkQuantityFactor(
        N,
        leftFactor,
        parameterTreeRoot,
        report
    );
	bool noFactorHasValueSet = leftFactor->value == 0;

    int midBinOpIndex = 0;
    NoisyIrNode* midBinOpNode = findNthIrNodeOfTypes(
        N,
        termRoot,
        kNewtonIrNodeType_PmidPrecedenceBinaryOp,
        gNewtonFirsts,
        midBinOpIndex
    );
    midBinOpIndex++;

    while (midBinOpNode != NULL)
    {
        NoisyIrNode* rightFactor = findNthIrNodeOfTypes(
            N,
            termRoot,
            kNewtonIrNodeType_PquantityFactor,
            gNewtonFirsts,
            factorIndex
        );
        factorIndex++;
        checkQuantityFactor(
            N,
            rightFactor,
            parameterTreeRoot,
            report
        );
		noFactorHasValueSet = noFactorHasValueSet && (rightFactor->value == 0);

		/* checking for high precedence binop just sets the values by multiplying or dividing */
        newtonCheckBinOp(N, leftFactor, rightFactor, midBinOpNode->type, report);

        midBinOpNode = findNthIrNodeOfTypes(
            N,
            termRoot,
            kNewtonIrNodeType_PmidPrecedenceBinaryOp,
            gNewtonFirsts,
            midBinOpIndex
        );
        midBinOpIndex++;
    }

	assert(!noFactorHasValueSet);
	if (!noFactorHasValueSet)
	  assert(leftFactor->value != 0);

	if (noFactorHasValueSet)
	  leftFactor->value = 0;

	return leftFactor->value;
}

void
checkQuantityFactor(
    NoisyState * N,
    NoisyIrNode * factorRoot,
    NoisyIrNode * parameterTreeRoot,
    ConstraintReport * report
) {
  // find a factor and then look it up in parameters
  // to assign a value
  //
  // if there is an highBinOp, check Expression
  int factorIndex = 0;
  NoisyIrNode * factor = findNthIrNodeOfTypes(
	  N,
	  factorRoot,
	  kNewtonIrNodeType_PquantityFactor,
	  gNewtonFirsts,
    factorIndex
  );

  if (!newtonIsDimensionless(factor->physics) && !factor->physics->isConstant && newtonDimensionTableDimensionForIdentifier(N, N->newtonIrTopScope, factor->tokenString) == NULL)
	{
	  /*
	   * Suppose the caller of the API has supplied a NoisyIrNode Tidentifier "L" with numeric value 5.
	   * The constraint is L < 2 * meter.
	   *
	   * Currently, searching the symbol table for a unit returns that NoisyIrNode with its corresponding Physics struct.
	   * A matching parameter must correspond to the Physics struct bound by the token string (e.g. L : distance),
	   * but we do not want to raise an error a node that just is a unit (e.g. meter), not a Physics.
	   */
	  NoisyIrNode * matchingParameter = newtonParseFindNodeByParameterNumber(N, parameterTreeRoot, factor->parameterNumber);
	  if (matchingParameter == NULL)
		{
		  sprintf(report->dimensionErrorMessage, "newton-check-pass.c:checkQuantityFactor: did not find a parameter with physics id %llu", factor->physics->id);

		  noisyFatal(N, "newton-check-pass.c:checkQuantity: matchingParameter is null\n");
		  return;
		}
	  else
		{
		  factor->value = matchingParameter->value;
		}
	}


	int highBinOpIndex = 0;
	NoisyIrNode* highBinOpNode = findNthIrNodeOfTypes(
		N,
		factorRoot,
		kNewtonIrNodeType_PhighPrecedenceBinaryOp,
		gNewtonFirsts,
		highBinOpIndex
	);
	highBinOpIndex++;

	int expressionIndex = 0;
	while  (highBinOpNode != NULL)
	{
	  NoisyIrNode* expression = findNthIrNodeOfType(
                                                   N,
                                                   factorRoot,
                                                   kNewtonIrNodeType_PquantityExpression,
                                                   expressionIndex
                                                   );
    expressionIndex++;
		checkQuantityExpression(	N,
                              expression,
                              parameterTreeRoot,
                              report);

    newtonCheckBinOp(N, factor, expression, highBinOpNode->type, report);

		highBinOpNode = findNthIrNodeOfTypes(
                                         N,
                                         factorRoot,
                                         kNewtonIrNodeType_PhighPrecedenceBinaryOp,
                                         gNewtonFirsts,
                                         highBinOpIndex
                                         );
		highBinOpIndex++;
	}
}
