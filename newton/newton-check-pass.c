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
extern char* gNewtonAstNodeStrings[kNoisyIrNodeTypeMax];
extern const char *	gNewtonTokenDescriptions[kNoisyIrNodeTypeMax];



void
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
    State* N,
    IrNode * leftExpression,
    IrNode * rightExpression,
    char * leftErrorMessage,
    char * rightErrorMessage,
    IrNodeType compareOpType,
    ConstraintReport * report
) {

  assert(gNewtonAstNodeStrings[leftExpression->type] != NULL);
  assert(gNewtonAstNodeStrings[rightExpression->type] != NULL);

    switch(compareOpType)
    {
        case kNewtonIrNodeType_Tge:
            report->satisfiesValueConstraint = leftExpression->value >= rightExpression->value;
            report->satisfiesDimensionConstraint = report->satisfiesDimensionConstraint &&
			  ((newtonIsDimensionless(leftExpression->physics) &&
				newtonIsDimensionless(rightExpression->physics)) ||
			   (leftExpression->physics->id == rightExpression->physics->id));
            if (!report->satisfiesValueConstraint)
			  sprintf(report->valueErrorMessage, "LHS %s of value %f should be >= RHS %s of value %f", leftErrorMessage, leftExpression->value, rightErrorMessage, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
			  sprintf(report->dimensionErrorMessage, "dimensions of LHS %s and RHS %s do not match", leftErrorMessage, rightErrorMessage);
            break;

        case kNewtonIrNodeType_Tgt:
            report->satisfiesValueConstraint = leftExpression->value > rightExpression->value;
            report->satisfiesDimensionConstraint = report->satisfiesDimensionConstraint &&
			  ((newtonIsDimensionless(leftExpression->physics) &&
				newtonIsDimensionless(rightExpression->physics)) ||
			   (leftExpression->physics->id == rightExpression->physics->id));
            if (!report->satisfiesValueConstraint)
			  sprintf(report->valueErrorMessage, "LHS %s of value %f should be > RHS %s of value %f", leftErrorMessage, leftExpression->value, rightErrorMessage, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
			  sprintf(report->dimensionErrorMessage, "dimensions of LHS %s and RHS %s do not match", leftErrorMessage, rightErrorMessage);
            break;

        case kNewtonIrNodeType_Tle:
            report->satisfiesValueConstraint = leftExpression->value <= rightExpression->value;
            report->satisfiesDimensionConstraint = report->satisfiesDimensionConstraint &&
			  ((newtonIsDimensionless(leftExpression->physics) &&
				newtonIsDimensionless(rightExpression->physics)) ||
			   (leftExpression->physics->id == rightExpression->physics->id));
            if (!report->satisfiesValueConstraint)
			  sprintf(report->valueErrorMessage, "LHS %s of value %f should be <= RHS %s of value %f", leftErrorMessage, leftExpression->value, rightErrorMessage, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
			  sprintf(report->dimensionErrorMessage, "dimensions of LHS %s and RHS %s do not match", leftErrorMessage, rightErrorMessage);
            break;

        case kNewtonIrNodeType_Tlt:
            report->satisfiesValueConstraint = leftExpression->value < rightExpression->value;
            report->satisfiesDimensionConstraint = report->satisfiesDimensionConstraint &&
			  ((newtonIsDimensionless(leftExpression->physics) &&
				newtonIsDimensionless(rightExpression->physics)) ||
			   (leftExpression->physics->id == rightExpression->physics->id));
            if (!report->satisfiesValueConstraint)
			  sprintf(report->valueErrorMessage, "LHS %s of value %f should be < RHS %s of value %f", leftErrorMessage, leftExpression->value, rightErrorMessage, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
			  sprintf(report->dimensionErrorMessage, "dimensions of LHS %s and RHS %s do not match", leftErrorMessage, rightErrorMessage);
            break;

        case kNewtonIrNodeType_Tproportionality:
            /* TODO figure out how to check proportionality */
		  report->satisfiesValueConstraint = true;
		  report->satisfiesDimensionConstraint = true;
			break;

        case kNewtonIrNodeType_Tequivalent:
            /* TODO change == to incorporate an epsilon */
            report->satisfiesValueConstraint = leftExpression->value == rightExpression->value;
            report->satisfiesDimensionConstraint = report->satisfiesDimensionConstraint &&
			  ((newtonIsDimensionless(leftExpression->physics) &&
				newtonIsDimensionless(rightExpression->physics)) ||
			   (leftExpression->physics->id == rightExpression->physics->id));
            if (!report->satisfiesValueConstraint)
			  sprintf(report->valueErrorMessage, "LHS %s of value %f should be == RHS %s of value %f", leftErrorMessage, leftExpression->value, rightErrorMessage, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
			  sprintf(report->dimensionErrorMessage, "dimensions of LHS %s and RHS %s do not match", leftErrorMessage, rightErrorMessage);
            break;

        default:
            fatal(N, "newton-api.c:newtonCheckCompareOp: no compareOp detected");
            break;
    }
}

void
newtonCheckBinOp(
    State* N,
    IrNode * left,
    IrNode * right,
    IrNodeType binOpType,
    ConstraintReport * report
) {
	if (left == NULL || right == NULL)
	    fatal(N, "newton-check-pass.c:newtonCheckBinOp: left or right is null");
    switch(binOpType)
    {
        case kNewtonIrNodeType_Tplus:
            left->value += right->value;
            report->satisfiesDimensionConstraint = report->satisfiesDimensionConstraint &&
			    ((newtonIsDimensionless(left->physics) && newtonIsDimensionless(right->physics)) || \
				 (areTwoPhysicsEquivalent(N, left->physics, right->physics)));

            break;

        case kNewtonIrNodeType_Tminus:
            left->value -= right->value;
            report->satisfiesDimensionConstraint = report->satisfiesDimensionConstraint &&
			  ((newtonIsDimensionless(left->physics) && newtonIsDimensionless(right->physics)) || \
			   areTwoPhysicsEquivalent(N, left->physics, right->physics));
            break;

        /*
         * Note that we don't need to copy over any dimensions as in Parsing
         * because they are already filled in. Same for division and exponents
         */
        case kNewtonIrNodeType_Tmul:
			left->value = left->value == 0 ? 1 : left->value;
			right->value = right->value == 0 ? 1 : right->value;
            left->value *= right->value;
            break;

        case kNewtonIrNodeType_Tdiv:
		  /* The drawback of this approach is that we can't catch divide by 0 */
		  left->value = left->value == 0 ? 1 : left->value;
		  right->value = right->value == 0 ? 1 : right->value;
            left->value /= right->value;
            break;

        case kNewtonIrNodeType_Texponent:
            /*
             * left is base and right is exponential expression.
             * The value of the exponential expression is always dimensionless and should
             * already have been calculated by Newton description parser.
             */
		    report->satisfiesValueConstraint = report->satisfiesValueConstraint && (left->value != 0 || right->value != 0);
			if (!report->satisfiesValueConstraint)
			  {
				fatal(N, "newton-check-pass.c:newtonCheckBinOp: cannot raise 0 to 0 power");
			  }
            left->value = pow(left->value, right->value);

            break;
        default:
            fatal(N, "newton-api.c:newtonCheckCompareOp: no binOp detected");
            break;
    }

	/*
	 * This should not fail because asserts inside Newton parser must have caught this error.
	 */
	assert(report->satisfiesDimensionConstraint);
}

void
iterateConstraints(State * N, IrNode * constraintTreeRoot, IrNode* parameterTreeRoot, NewtonAPIReport* report)
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
checkSingleConstraint(State * N, IrNode * constraintTreeRoot, IrNode* parameterTreeRoot, NewtonAPIReport* newtonReport)
{
    int expressionIndex = 0;
	ConstraintReport * constraintReport = (ConstraintReport*) calloc(1, sizeof(ConstraintReport));
	constraintReport->satisfiesValueConstraint = true;
	constraintReport->satisfiesDimensionConstraint = true;
	char leftDetailMessage[1024];
	char rightDetailMessage[1024];

    /* check LHS expression */
    IrNode* leftExpression = findNthIrNodeOfType(
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
		leftDetailMessage,
        constraintReport
    );

    /* find the compareOp between LHS and RHS*/
    int compareOpIndex = 0;
    IrNode* compareOp = findNthIrNodeOfTypes(
        N,
        constraintTreeRoot,
        kNewtonIrNodeType_PcompareOp,
        gNewtonFirsts,
        compareOpIndex
    );
    /* There must be one compareOp per constraint */
    assert(compareOp != NULL);

    IrNode* rightExpression = findNthIrNodeOfType(
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
		rightDetailMessage,
        constraintReport
    );

    newtonCheckCompareOp(
        N,
        leftExpression,
        rightExpression,
		leftDetailMessage,
		rightDetailMessage,
        compareOp->token->type,
        constraintReport
    );

	addConstraintReportToNewtonAPIReport(newtonReport, constraintReport);
}

double
checkQuantityExpression(
    State * N,
    IrNode * expressionRoot,
    IrNode * parameterTreeRoot,
	char * errorMessage,
    ConstraintReport * report
) {
    int termIndex = 0;
    IrNode* leftTerm = findNthIrNodeOfType(
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
		errorMessage,
        report
    );

    int lowBinOpIndex = 0;
    IrNode* lowBinOpNode = findNthIrNodeOfTypes(
        N,
        expressionRoot,
        kNewtonIrNodeType_PlowPrecedenceBinaryOp,
        gNewtonFirsts,
        lowBinOpIndex
    );
    lowBinOpIndex++;

    while (lowBinOpNode != NULL)
    {
	    strcat(errorMessage, gNewtonTokenDescriptions[lowBinOpNode->type]);
        IrNode* rightTerm = findNthIrNodeOfType(
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
			errorMessage,
            report
        );
		strcat(errorMessage, gNewtonTokenDescriptions[lowBinOpNode->type]);

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
    State * N,
    IrNode * termRoot,
    IrNode * parameterTreeRoot,
    char * errorMessage,
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

	IrNode* unaryOp = findNthIrNodeOfTypes(
		N,
		termRoot,
		kNewtonIrNodeType_PunaryOp,
		gNewtonFirsts,
		factorIndex
		);
    IrNode* leftFactor = findNthIrNodeOfTypes(
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
		errorMessage,
        report
    );

	if (unaryOp != NULL)
	{
		leftFactor->value *= -1;
	}

	bool noFactorHasValueSet = leftFactor->value == 0;

    int midBinOpIndex = 0;
    IrNode* midBinOpNode = findNthIrNodeOfTypes(
        N,
        termRoot,
        kNewtonIrNodeType_PmidPrecedenceBinaryOp,
        gNewtonFirsts,
        midBinOpIndex
    );
    midBinOpIndex++;

    while (midBinOpNode != NULL)
    {

	    strcat(errorMessage, gNewtonTokenDescriptions[midBinOpNode->type]);

        IrNode* rightFactor = findNthIrNodeOfTypes(
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
			errorMessage,
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
    State * N,
    IrNode * factorRoot,
    IrNode * parameterTreeRoot,
	char * errorMessage,
    ConstraintReport * report
) {
  // find a factor and then look it up in parameters
  // to assign a value
  //
  // if there is an highBinOp, check Expression
  int factorIndex = 0;
  IrNode * factor = findNthIrNodeOfTypes(
	  N,
	  factorRoot,
	  kNewtonIrNodeType_PquantityFactor,
	  gNewtonFirsts,
      factorIndex
  );

  if (!newtonIsDimensionless(factor->physics) && !factor->physics->isConstant && newtonDimensionTableDimensionForIdentifier(N, N->newtonIrTopScope, factor->tokenString) == NULL)
	{
	  /*
	   * Suppose the caller of the API has supplied a IrNode Tidentifier "L" with numeric value 5.
	   * The constraint is L < 2 * meter.
	   *
	   * Currently, searching the symbol table for a unit returns that IrNode with its corresponding Physics struct.
	   * A matching parameter must correspond to the Physics struct bound by the token string (e.g. L : distance),
	   * but we do not want to raise an error a node that just is a unit (e.g. meter), not a Physics.
	   */
		IrNode * matchingParameter = newtonParseFindNodeByParameterNumberAndSubindex(N, parameterTreeRoot, factor->parameterNumber, factor->physics->subindex);
	  if (matchingParameter == NULL)
		{
		  sprintf(report->dimensionErrorMessage, "newton-check-pass.c:checkQuantityFactor: did not find a parameter with physics id %llu", factor->physics->id);

		  fatal(N, "newton-check-pass.c:checkQuantity: matchingParameter is null\n");
		  return;
		}
	  else
		{
		  factor->value = matchingParameter->value;
		}
	}

  char factorDetailMessage[1024];
  if (factor->physics)
	{
	  if (newtonDimensionTableDimensionForIdentifier(N, N->newtonIrTopScope, factor->tokenString) == NULL)
		{
		  sprintf(factorDetailMessage,
				  " (%s : %f) ",
				  (factor->physics != NULL ? factor->tokenString: ""),
				  factor->value);
		}
	  else
		{
		  sprintf(factorDetailMessage,
				  " %s ",
				  (factor->physics != NULL ? factor->tokenString: ""));
		}
	}
  else
	{
	  sprintf(factorDetailMessage,
			  " %f ",
			  factor->value);
	}
    strcat(errorMessage, factorDetailMessage);


	int highBinOpIndex = 0;
	IrNode* highBinOpNode = findNthIrNodeOfTypes(
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
	  strcat(errorMessage, gNewtonTokenDescriptions[highBinOpNode->type]);
	  strcat(errorMessage, "(");

	  IrNode* expression = findNthIrNodeOfType(
                                                   N,
                                                   factorRoot,
                                                   kNewtonIrNodeType_PquantityExpression,
                                                   expressionIndex
                                                   );

      expressionIndex++;
	  checkQuantityExpression(N,
                              expression,
                              parameterTreeRoot,
							  errorMessage,
                              report);

	  strcat(errorMessage, ")");

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
