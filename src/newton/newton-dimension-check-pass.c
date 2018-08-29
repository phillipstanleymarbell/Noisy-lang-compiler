/*
	Authored 2017. Jonathan Lim. Modified 2018, Phillip Stanley-Marbell.

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
#include <math.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "version.h"
#include "common-errors.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "newton-data-structures.h"
#include "common-irHelpers.h"
#include "noisy-parser.h"
#include "newton-parser.h"
#include "common-lexers-helpers.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "common-firstAndFollow.h"
#include "newton-dimension-check-pass.h"


extern int		gNewtonFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];
extern char* gNewtonAstNodeStrings[kNoisyIrNodeTypeMax];
extern const char *	gNewtonTokenDescriptions[kNoisyIrNodeTypeMax];


extern void		fatal(State *  N, const char *  msg);
extern void		error(State *  N, const char *  msg);

ConstraintReport *
newtonDimensionCheckExpressionOrStatement(
	State * N,
	IrNode * tree
) {
    int     expressionIndex = 0;
	ConstraintReport *      constraintReport = (ConstraintReport *) calloc(1, sizeof(ConstraintReport));
	constraintReport->satisfiesValueConstraint = true;
	constraintReport->satisfiesDimensionConstraint = true;
	char leftDetailMessage[1024];
	char rightDetailMessage[1024];

    /* 
     *  check LHS expression 
     */
    IrNode *    leftExpression = findNthIrNodeOfType(
        N,
        tree,
        kNewtonIrNodeType_PquantityExpression,
        expressionIndex
		);
    expressionIndex++;
    newtonDimensionCheckQuantityExpression(
        N,
        leftExpression,
		leftDetailMessage,
        constraintReport
		);

    /*
     *  find the compareOp between LHS and RHS
     */
    int     compareOrAssignOpIndex = 0;
	IrNode *    compareOrAssignOpNode;
    IrNode *    compareOp = findNthIrNodeOfTypes(
        N,
        tree,
        kNewtonIrNodeType_PcompareOp,
        gNewtonFirsts,
        compareOrAssignOpIndex
		);
	compareOrAssignOpNode = compareOp != NULL ?
		compareOp :
		findNthIrNodeOfType(
			N,
			tree,
			kNewtonIrNodeType_Tequals,
			compareOrAssignOpIndex
			);
    assert(compareOrAssignOpNode != NULL);

    IrNode *    rightExpression = findNthIrNodeOfType(
        N,
        tree,
        kNewtonIrNodeType_PquantityExpression,
        expressionIndex
		);
    expressionIndex++;
    newtonDimensionCheckQuantityExpression(
        N,
        rightExpression,
		rightDetailMessage,
        constraintReport
		);

	if (!areTwoPhysicsEquivalent(N, leftExpression->physics, rightExpression->physics))
	{
		constraintReport->satisfiesDimensionConstraint = false;
		sprintf(constraintReport->dimensionErrorMessage, "leftExpression and rightExpression do not have the same dimensions. Additional errors from left expression: %s.\n Additional errors from right expression: %s", leftDetailMessage, rightDetailMessage);
	}

	return constraintReport;
}

/*
 *  This file basically follows the format of newton-parser-expression.c except for a few differences
 *  First, inFirst method searches the first productions in the IR tree
 *  given, instead of in the token stream.
 *  Second, this file needs to evaluate compareOps and assignOps.
 *  Third, it does not create new nodes. This file assumes that the tree is constructed correctly.
 */
void
newtonDimensionCheckQuantityExpression(
	State * N,
	IrNode * expressionRoot,
	char * errorMessage,
	ConstraintReport * report
) {

    int     termIndex = 0;
    IrNode *    leftTerm = findNthIrNodeOfType(
        N,
        expressionRoot,
        kNewtonIrNodeType_PquantityTerm,
        termIndex
		);
	termIndex++;

    if (leftTerm == NULL)
	{
		report->satisfiesDimensionConstraint = false;
		sprintf(errorMessage, "did not find a quantityTerm in expression. Expression tree is incorrectly constructed\n");
		return;
	}

	newtonDimensionCheckQuantityTerm(
		N,
		leftTerm,
		errorMessage,
		report
		);


    int     lowBinOpIndex = 0;
    IrNode *    lowBinOpNode = findNthIrNodeOfTypes(
        N,
        expressionRoot,
        kNewtonIrNodeType_PlowPrecedenceBinaryOp,
        gNewtonFirsts,
        lowBinOpIndex
		);
    lowBinOpIndex++;

    expressionRoot->physics = newtonInitPhysics(N, NULL, NULL);
	newtonPhysicsAddExponents(N, expressionRoot->physics, leftTerm->physics);

    while (lowBinOpNode != NULL)
	{
		IrNode *    rightTerm = findNthIrNodeOfType(
			N,
			expressionRoot,
			kNewtonIrNodeType_PquantityTerm,
			termIndex
			);
		termIndex++;
		newtonDimensionCheckQuantityTerm(
			N,
			rightTerm,
			errorMessage,
			report
			);

		if (!areTwoPhysicsEquivalent(N, leftTerm->physics, rightTerm->physics))
		{
			report->satisfiesDimensionConstraint = false;
			sprintf(errorMessage, "leftTerm and rightTerm do not have the same dimensions.\n");
		}

		lowBinOpNode = findNthIrNodeOfTypes(
			N,
			expressionRoot,
			kNewtonIrNodeType_PlowPrecedenceBinaryOp,
			gNewtonFirsts,
			lowBinOpIndex
			);
		lowBinOpIndex++;
	}
}

void
newtonDimensionCheckQuantityTerm(State * N, IrNode * termRoot, char * errorMessage, ConstraintReport * report)
{
    int     factorIndex = 0;

	/*
	 *  We do not check for unary op because
	 *  unary ops do not have an impact on dimensions
	 */

    IrNode *    leftFactor = findNthIrNodeOfTypes(
        N,
        termRoot,
        kNewtonIrNodeType_PquantityFactor,
        gNewtonFirsts,
        factorIndex
		);
	factorIndex++;
    termRoot->physics = newtonInitPhysics(N, NULL, NULL);

    newtonDimensionCheckQuantityFactor(
		N,
		leftFactor,
		errorMessage,
		report);
    if (leftFactor == NULL)
	{
		report->satisfiesDimensionConstraint = false;
		sprintf(errorMessage, "did not find a quantityFactor in expression. Expression tree is incorrectly constructed\n");
		return;
	}

    int     numVectorsInTerm = 0;

	assert(leftFactor->physics != NULL);
	newtonPhysicsAddExponents(N, termRoot->physics, leftFactor->physics);

    /*
     *  If either LHS or RHS is a vector (not both), then the resultant is a vector
     */
    if (leftFactor->physics->isVector)
    {
        termRoot->physics->isVector = true;
        numVectorsInTerm++;
    }

	int     midBinOpIndex = 0;
	IrNode *    midBinOp = findNthIrNodeOfTypes(
		N,
		termRoot,
		kNewtonIrNodeType_PmidPrecedenceBinaryOp,
		gNewtonFirsts,
		midBinOpIndex
		);
	midBinOpIndex++;

    while (midBinOp != NULL)
    {
		IrNode *    rightFactor = findNthIrNodeOfTypes(
			N,
			termRoot,
			kNewtonIrNodeType_PquantityFactor,
			gNewtonFirsts,
			factorIndex
			);
		factorIndex++;
        newtonDimensionCheckQuantityFactor(
			N,
			rightFactor,
			errorMessage,
			report
			);
		assert(rightFactor->physics != NULL);

        if (rightFactor->physics->isVector)
        {
            termRoot->physics->isVector = true;
            numVectorsInTerm++;

            /*
             *  Cannot perform multiply or divide operations on two vectors
             *  e.g.) vector * scalar * scalar / vector is illegal because
             *  it boils down to vector / vector which is illegal
             */
            assert(numVectorsInTerm < 2);
        }

        if (midBinOp->type == kNewtonIrNodeType_Tmul)
        {
			newtonPhysicsAddExponents(N, termRoot->physics, rightFactor->physics);
        }
        else if (midBinOp->type == kNewtonIrNodeType_Tdiv)
        {
			newtonPhysicsSubtractExponents(N, termRoot->physics, rightFactor->physics);
        }

		midBinOp = findNthIrNodeOfTypes(
			N,
			termRoot,
			kNewtonIrNodeType_PmidPrecedenceBinaryOp,
			gNewtonFirsts,
			midBinOpIndex
			);
		midBinOpIndex++;
    }
}

void
newtonDimensionCheckQuantityFactor(
	State * N,
	IrNode * factorRoot,
	char * errorMessage,
	ConstraintReport * report
	) {
	char factorDetailMessage[1024];

	int     factorIndex = 0;
	IrNode *    factor = findNthIrNodeOfTypes(
		N,
		factorRoot,
		kNewtonIrNodeType_PquantityFactor,
		gNewtonFirsts,
		factorIndex
		);
	assert(factor->physics != NULL);

	if (newtonIsDimensionless(factor->physics))
	{
		if (newtonDimensionTableDimensionForName(N, N->newtonIrTopScope, factor->tokenString) == NULL)
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

	int     highBinOpIndex = 0;
	IrNode *    highBinOpNode = findNthIrNodeOfTypes(
		N,
		factorRoot,
		kNewtonIrNodeType_PhighPrecedenceBinaryOp,
		gNewtonFirsts,
		highBinOpIndex
		);
	highBinOpIndex++;

	int     expressionIndex = 0;
	while (highBinOpNode != NULL)
	{
		strcat(errorMessage, gNewtonTokenDescriptions[highBinOpNode->type]);
		strcat(errorMessage, "(");

		IrNode *    expression = findNthIrNodeOfType(
			N,
			factorRoot,
			kNewtonIrNodeType_PquantityExpression,
			expressionIndex
			);

		expressionIndex++;
		newtonDimensionCheckExponentialExpression(
			N,
			expression,
			factor,
			errorMessage,
			report);
		strcat(errorMessage, ")");
		if (!newtonIsDimensionless(expression->physics))
		{
			strcat(errorMessage, "An exponential expression must be dimensionless\n");
			return;
		}

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

void
newtonDimensionCheckExponentialExpression(
	State * N,
	IrNode * expressionRoot,
	IrNode * baseNode,
	char * errorMessage,
	ConstraintReport * report
	)
{
	double      exponentValue = newtonDimensionCheckNumericExpression(
		N,
		expressionRoot,
		errorMessage,
		report);

    /* 
     *  If the base is a Physics quantity, the exponent must be an integer 
     */
    assert(exponentValue == (int) exponentValue);
	assert(baseNode->physics != NULL);
	newtonPhysicsMultiplyExponents(N, baseNode->physics, exponentValue);
}

double
newtonDimensionCheckNumericExpression(
    State * N,
    IrNode * expressionRoot,
    char * errorMessage,
    ConstraintReport * report
	) {

    int     termIndex = 0;
    IrNode *    leftTerm = findNthIrNodeOfType(
        N,
        expressionRoot,
        kNewtonIrNodeType_PquantityTerm,
        termIndex
		);
	termIndex++;

    leftTerm->value = newtonDimensionCheckNumericTerm(
        N,
        leftTerm,
		errorMessage,
        report
		);
	expressionRoot->value = leftTerm->value;

    expressionRoot->physics = newtonInitPhysics(N, NULL, NULL);

    int lowBinOpIndex = 0;
    IrNode *    lowBinOpNode = findNthIrNodeOfTypes(
        N,
        expressionRoot,
        kNewtonIrNodeType_PlowPrecedenceBinaryOp,
        gNewtonFirsts,
        lowBinOpIndex
		);
    lowBinOpIndex++;

    while (lowBinOpNode != NULL)
	{
		IrNode *    rightTerm = findNthIrNodeOfType(
			N,
			expressionRoot,
			kNewtonIrNodeType_PquantityTerm,
			termIndex
			);
		termIndex++;
		newtonDimensionCheckNumericTerm(
			N,
			rightTerm,
			errorMessage,
			report
			);

		if (lowBinOpNode->token->type == kNewtonIrNodeType_Tplus)
		{
			expressionRoot->value += rightTerm->value;
		}
		else if (lowBinOpNode->token->type == kNewtonIrNodeType_Tminus)
		{
			expressionRoot->value -= rightTerm->value;
		}
		else
		{
			fatal(N, "lowBinOpNode type is neither plus nor minus\n");
		}

		if (!areTwoPhysicsEquivalent(N, leftTerm->physics, rightTerm->physics))
		{
			report->satisfiesDimensionConstraint = false;
			sprintf(errorMessage, "in the exponential expression: leftTerm and rightTerm do not have the same dimensions.\n");
			return 0;
		}

		lowBinOpNode = findNthIrNodeOfTypes(
			N,
			expressionRoot,
			kNewtonIrNodeType_PlowPrecedenceBinaryOp,
			gNewtonFirsts,
			lowBinOpIndex
			);
		lowBinOpIndex++;
	}

	return expressionRoot->value;
}

double
newtonDimensionCheckNumericTerm(
    State * N,
    IrNode * termRoot,
    char * errorMessage,
    ConstraintReport * report
	) {
    int     factorIndex = 0;

	IrNode *    unaryOp = findNthIrNodeOfTypes(
		N,
		termRoot,
		kNewtonIrNodeType_PunaryOp,
		gNewtonFirsts,
		0
		);

    IrNode *    leftFactor = findNthIrNodeOfTypes(
        N,
        termRoot,
        kNewtonIrNodeType_PquantityFactor,
        gNewtonFirsts,
        factorIndex
		);
    termRoot->physics = newtonInitPhysics(N, NULL, NULL);

    newtonDimensionCheckNumericFactor(
		N,
		leftFactor,
		errorMessage,
		report);

	termRoot->value = leftFactor->value;

	assert(termRoot->value != 0);
	if (unaryOp != NULL)
	{
		termRoot->value *= -1;
	}

    int     numVectorsInTerm = 0;

	assert(leftFactor->physics != NULL);
	newtonPhysicsAddExponents(N, termRoot->physics, leftFactor->physics);

    /*
     *  If either LHS or RHS is a vector (not both), then the resultant is a vector
     */
    if (leftFactor->physics->isVector)
    {
        termRoot->physics->isVector = true;
        numVectorsInTerm++;
    }

	int     midBinOpIndex = 0;
	IrNode *    midBinOp = findNthIrNodeOfTypes(
		N,
		termRoot,
		kNewtonIrNodeType_PmidPrecedenceBinaryOp,
		gNewtonFirsts,
		midBinOpIndex
		);
	midBinOpIndex++;

    while (midBinOp != NULL)
    {
		IrNode *    rightFactor = findNthIrNodeOfTypes(
			N,
			termRoot,
			kNewtonIrNodeType_PquantityFactor,
			gNewtonFirsts,
			factorIndex
			);
		factorIndex++;
		newtonDimensionCheckNumericFactor(
			N,
			rightFactor,
			errorMessage,
			report);

		assert(newtonIsDimensionless(rightFactor->physics));


        if (rightFactor->physics->isVector)
        {
            termRoot->physics->isVector = true;
            numVectorsInTerm++;

            /*
             *  Cannot perform multiply or divide operations on two vectors
             *  e.g.) vector * scalar * scalar / vector is illegal because
             *  it boils down to vector / vector which is illegal
             */
            assert(numVectorsInTerm < 2);
        }

        if (midBinOp->type == kNewtonIrNodeType_Tmul)
        {
			termRoot->value *= rightFactor->value;
			newtonPhysicsAddExponents(N, termRoot->physics, rightFactor->physics);
        }
        else if (midBinOp->type == kNewtonIrNodeType_Tdiv)
        {
			if (rightFactor->value == 0)
			{
				strcat(report->dimensionErrorMessage, "Do not divide by zero!!");
				return 0;
			}
			termRoot->value /= rightFactor->value;
			newtonPhysicsSubtractExponents(N, termRoot->physics, rightFactor->physics);
        }
		else
		{
			strcat(report->dimensionErrorMessage, "midBinOp neither mul nor div\n");
			return 0;
		}


		midBinOp = findNthIrNodeOfTypes(
			N,
			termRoot,
			kNewtonIrNodeType_PmidPrecedenceBinaryOp,
			gNewtonFirsts,
			midBinOpIndex
			);
		midBinOpIndex++;
    }

	return termRoot->value;
}

void
newtonDimensionCheckNumericFactor(
    State * N,
    IrNode * factorRoot,
    char * errorMessage,
    ConstraintReport * report
	) {

	char factorDetailMessage[1024];
	int     factorIndex = 0;
	IrNode *    factor = findNthIrNodeOfTypes(
		N,
		factorRoot,
		kNewtonIrNodeType_PquantityFactor,
		gNewtonFirsts,
		factorIndex
		);
	if (factor->physics != NULL)
	{
		factor->physics = newtonInitPhysics(N, NULL, NULL);
	}
	assert(newtonIsDimensionless(factor->physics));
	assert(factor->type == kNewtonIrNodeType_TnumericConst);

	if (newtonDimensionTableDimensionForName(N, N->newtonIrTopScope, factor->tokenString) == NULL)
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
    strcat(errorMessage, factorDetailMessage);

	int     highBinOpIndex = 0;
	IrNode *    highBinOpNode = findNthIrNodeOfTypes(
		N,
		factorRoot,
		kNewtonIrNodeType_PhighPrecedenceBinaryOp,
		gNewtonFirsts,
		highBinOpIndex
		);
	highBinOpIndex++;

	int     expressionIndex = 0;
	while  (highBinOpNode != NULL)
	{
		strcat(errorMessage, gNewtonTokenDescriptions[highBinOpNode->type]);
		strcat(errorMessage, "(");

		IrNode *    expression = findNthIrNodeOfType(
			N,
			factorRoot,
			kNewtonIrNodeType_PquantityExpression,
			expressionIndex
			);

		expressionIndex++;
		newtonDimensionCheckNumericExpression(
			N,
			expression,
			errorMessage,
			report);
		strcat(errorMessage, ")");
		if (!newtonIsDimensionless(expression->physics))
		{
			strcat(errorMessage, "An exponential expression must be dimensionless\n");
			return;
		}

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
