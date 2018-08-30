/*
	Authored 2017. Jonathan Lim.  Modified 2018, Phillip Stanley-Marbell.

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
#include <inttypes.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "version.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "newton-data-structures.h"

#include "newton-parser.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton.h"
#include "newton-api.h"
#include "common-irHelpers.h"
#include "newton-check-pass.h"

extern int		gNewtonFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];
extern char *    gNewtonAstNodeStrings[kNoisyIrNodeTypeMax];
extern const char *     gNewtonTokenDescriptions[kNoisyIrNodeTypeMax];

void
updateDestinationTrackerIndicesFromSource(Indices * dest, Indices * source)
{
	dest->expressionIndex += source->expressionIndex;
	dest->termIndex += source->termIndex;
	dest->factorIndex += source->factorIndex;
	dest->lowBinOpIndex += source->lowBinOpIndex;
	dest->midBinOpIndex += source->midBinOpIndex;
	dest->highBinOpIndex += source->highBinOpIndex;
}

void
addConstraintReportToNewtonAPIReport(NewtonAPIReport * newtonReport, ConstraintReport * constraintReport)
{
	if (newtonReport->firstConstraintReport == NULL)
	{
		newtonReport->firstConstraintReport = constraintReport;
		return;
	}

	ConstraintReport * current = newtonReport->firstConstraintReport;
	while (current->next != NULL)
	{
		current = current->next;
	}

	current->next = constraintReport;
}

void
newtonCheckCompareOp(
    State * N,
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
				areTwoPhysicsEquivalent(N, leftExpression->physics, rightExpression->physics));
            if (!report->satisfiesValueConstraint)
			{
			    sprintf(report->valueErrorMessage,
						"LHS %s of value %f should be >= RHS %s of value %f",
						leftErrorMessage,
						leftExpression->value,
						rightErrorMessage,
						rightExpression->value);
			}
            if (!report->satisfiesDimensionConstraint)
			{
				sprintf(report->dimensionErrorMessage,
						"dimensions of LHS %s and RHS %s do not match",
						leftErrorMessage,
						rightErrorMessage);
			}
            break;

        case kNewtonIrNodeType_Tgt:
            report->satisfiesValueConstraint = leftExpression->value > rightExpression->value;
            report->satisfiesDimensionConstraint = report->satisfiesDimensionConstraint &&
			    ((newtonIsDimensionless(leftExpression->physics) &&
				newtonIsDimensionless(rightExpression->physics)) ||
			    areTwoPhysicsEquivalent(N, leftExpression->physics, rightExpression->physics));
            if (!report->satisfiesValueConstraint)
			{
				sprintf(report->valueErrorMessage,
						"LHS %s of value %f should be > RHS %s of value %f",
						leftErrorMessage,
						leftExpression->value,
						rightErrorMessage,
						rightExpression->value);
			}
            if (!report->satisfiesDimensionConstraint)
			{
				sprintf(report->dimensionErrorMessage,
						"dimensions of LHS %s and RHS %s do not match",
						leftErrorMessage,
						rightErrorMessage);
			}
            break;

        case kNewtonIrNodeType_Tle:
            report->satisfiesValueConstraint = leftExpression->value <= rightExpression->value;
            report->satisfiesDimensionConstraint = report->satisfiesDimensionConstraint &&
			    ((newtonIsDimensionless(leftExpression->physics) &&
				newtonIsDimensionless(rightExpression->physics)) ||
			    areTwoPhysicsEquivalent(N, leftExpression->physics, rightExpression->physics));
            if (!report->satisfiesValueConstraint)
			{
				sprintf(report->valueErrorMessage,
						"LHS %s of value %f should be <= RHS %s of value %f",
						leftErrorMessage,
						leftExpression->value,
						rightErrorMessage,
						rightExpression->value);
			}
            if (!report->satisfiesDimensionConstraint)
			{
				sprintf(report->dimensionErrorMessage,
						"dimensions of LHS %s and RHS %s do not match",
						leftErrorMessage,
						rightErrorMessage);
			}
            break;

        case kNewtonIrNodeType_Tlt:
            report->satisfiesValueConstraint = leftExpression->value < rightExpression->value;
            report->satisfiesDimensionConstraint = report->satisfiesDimensionConstraint &&
			    ((newtonIsDimensionless(leftExpression->physics) &&
				newtonIsDimensionless(rightExpression->physics)) ||
			    areTwoPhysicsEquivalent(N, leftExpression->physics, rightExpression->physics));
            if (!report->satisfiesValueConstraint)
			{
				sprintf(report->valueErrorMessage,
						"LHS %s of value %f should be < RHS %s of value %f",
						leftErrorMessage,
						leftExpression->value,
						rightErrorMessage,
						rightExpression->value);
			}
            if (!report->satisfiesDimensionConstraint)
			{
				sprintf(report->dimensionErrorMessage,
						"dimensions of LHS %s and RHS %s do not match",
						leftErrorMessage,
						rightErrorMessage);
			}
            break;

        case kNewtonIrNodeType_Tproportional:
            /* 
             *  TODO figure out how to check proportionality 
             */
		    report->satisfiesValueConstraint = true;
		    report->satisfiesDimensionConstraint = true;
			break;

        case kNewtonIrNodeType_Tequivalent:
            /* 
             *  TODO change == to incorporate an epsilon 
             */
            report->satisfiesValueConstraint = leftExpression->value == rightExpression->value;
            report->satisfiesDimensionConstraint = report->satisfiesDimensionConstraint &&
			    ((newtonIsDimensionless(leftExpression->physics) &&
				newtonIsDimensionless(rightExpression->physics)) ||
			    areTwoPhysicsEquivalent(N, leftExpression->physics, rightExpression->physics));
            if (!report->satisfiesValueConstraint)
			{
				sprintf(report->valueErrorMessage,
						"LHS %s of value %f should be == RHS %s of value %f",
						leftErrorMessage,
						leftExpression->value,
						rightErrorMessage,
						rightExpression->value);
			}
            if (!report->satisfiesDimensionConstraint)
			{
				sprintf(report->dimensionErrorMessage,
						"dimensions of LHS %s and RHS %s do not match",
						leftErrorMessage,
						rightErrorMessage);
			}
            break;

        default:
            fatal(N, "newton-api.c:newtonCheckCompareOp: no compareOp detected");
            break;
    }
}

void
newtonCheckBinOp(
    State * N,
	IrNode * parent,
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
            parent->value += right->value;

            report->satisfiesDimensionConstraint = report->satisfiesDimensionConstraint &&
			    ((newtonIsDimensionless(left->physics) && newtonIsDimensionless(right->physics)) || \
				(areTwoPhysicsEquivalent(N, left->physics, right->physics)));
            break;

        case kNewtonIrNodeType_Tminus:
            parent->value = left->value - right->value;

            report->satisfiesDimensionConstraint = report->satisfiesDimensionConstraint &&
			    ((newtonIsDimensionless(left->physics) && newtonIsDimensionless(right->physics)) || \
			    areTwoPhysicsEquivalent(N, left->physics, right->physics));
            break;

        /*
         *  Note that we don't need to copy over any dimensions as in Parsing
         *  because they are already filled in. Same for division and exponents
         */
        case kNewtonIrNodeType_Tmul:
			left->value = left->value == 0 ? 1 : left->value;
			right->value = right->value == 0 ? 1 : right->value;
			parent->value = left->value * right->value;
            break;

        case kNewtonIrNodeType_Tdiv:
		    /* 
             *  The drawback of this approach is that we can't catch divide by 0 
             */
		    left->value = left->value == 0 ? 1 : left->value;
		    right->value = right->value == 0 ? 1 : right->value;
		    parent->value = left->value / right->value;
            break;

        case kNewtonIrNodeType_Texponent:
            /*
             *  left is base and right is exponential expression.
             *  The value of the exponential expression is always dimensionless and should
             *  already have been calculated by Newton description parser.
             */
		    report->satisfiesValueConstraint = report->satisfiesValueConstraint && (left->value != 0 || right->value != 0);
			if (!report->satisfiesValueConstraint)
			{
				fatal(N, "newton-check-pass.c:newtonCheckBinOp: cannot raise 0 to 0 power");
			}
            parent->value = pow(left->value, right->value);
            break;

        default:
            fatal(N, "newton-api.c:newtonCheckCompareOp: no binOp detected");
            break;
    }

	/*
	 *  This should not fail because asserts inside Newton parser must have caught this error.
	 */
	assert(report->satisfiesDimensionConstraint);
}

void
iterateConstraints(State * N, IrNode * constraintTreeRoot, IrNode * parameterTreeRoot, NewtonAPIReport * report)
{
    if (constraintTreeRoot->type == kNewtonIrNodeType_Pconstraint)
    {
        checkSingleConstraint(N, constraintTreeRoot, parameterTreeRoot, report);
    }

    if (constraintTreeRoot->irLeftChild != NULL)
	{
	  	iterateConstraints(N, constraintTreeRoot->irLeftChild, parameterTreeRoot, report);
	}

    if (constraintTreeRoot->irRightChild != NULL)
	{
	  	iterateConstraints(N, constraintTreeRoot->irRightChild, parameterTreeRoot, report);
	}
}

void
checkSingleConstraint(State * N, IrNode * constraintTreeRoot, IrNode * parameterTreeRoot, NewtonAPIReport * newtonReport)
{
    Indices * tracker = (Indices *) calloc(1, sizeof(Indices));
	ConstraintReport * constraintReport = (ConstraintReport *) calloc(1, sizeof(ConstraintReport));
	constraintReport->satisfiesValueConstraint = true;
	constraintReport->satisfiesDimensionConstraint = true;
	char leftDetailMessage[1024];
	char rightDetailMessage[1024];

    /*
     *  check LHS expression 
     */
    IrNode* leftExpression = findNthIrNodeOfType(
        N,
        constraintTreeRoot,
        kNewtonIrNodeType_PquantityExpression,
        tracker->expressionIndex
    );
    tracker->expressionIndex++;
	tracker->factorIndex++;
    Indices* leftTracker = checkQuantityExpression(
        N,
        leftExpression,
        parameterTreeRoot,
		leftDetailMessage,
        constraintReport
    );
	updateDestinationTrackerIndicesFromSource(tracker, leftTracker);

    /* 
     *  find the compareOp between LHS and RHS
     */
    int compareOpIndex = 0;
    IrNode* compareOp = findNthIrNodeOfTypes(
        N,
        constraintTreeRoot,
        kNewtonIrNodeType_PcompareOp,
        gNewtonFirsts,
        compareOpIndex
    );
    /* 
     *  There must be one compareOp per constraint 
     */
    assert(compareOp != NULL);

    IrNode* rightExpression = findNthIrNodeOfType(
        N,
        constraintTreeRoot,
        kNewtonIrNodeType_PquantityExpression,
        tracker->expressionIndex
    );
    tracker->expressionIndex++;
	tracker->factorIndex++;
    Indices * rightTracker = checkQuantityExpression(
        N,
        rightExpression,
        parameterTreeRoot,
		rightDetailMessage,
        constraintReport
    );
	updateDestinationTrackerIndicesFromSource(tracker, rightTracker);

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

	free(tracker);
	free(leftTracker);
	free(rightTracker);
}

Indices *
checkQuantityExpression(
    State * N,
    IrNode * expressionRoot,
    IrNode * parameterTreeRoot,
	char * errorMessage,
    ConstraintReport * report
) {
	Indices * tracker = (Indices *) calloc(1, sizeof(Indices));

    IrNode * leftTerm = findNthIrNodeOfType(
        N,
        expressionRoot,
        kNewtonIrNodeType_PquantityTerm,
        tracker->termIndex
    );
	tracker->termIndex++;
    assert(leftTerm != NULL);

    Indices * leftTracker = checkQuantityTerm(
        N,
        leftTerm,
        parameterTreeRoot,
		errorMessage,
        report
    );
	expressionRoot->value = leftTerm->value;
	updateDestinationTrackerIndicesFromSource(tracker, leftTracker);

    IrNode * lowBinOpNode = findNthIrNodeOfTypes(
        N,
        expressionRoot,
        kNewtonIrNodeType_PlowPrecedenceBinaryOp,
        gNewtonFirsts,
        tracker->lowBinOpIndex
    );

    while (lowBinOpNode != NULL)
    {
		tracker->lowBinOpIndex++;

	    strcat(errorMessage, gNewtonTokenDescriptions[lowBinOpNode->type]);
        IrNode * rightTerm = findNthIrNodeOfType(
            N,
            expressionRoot,
            kNewtonIrNodeType_PquantityTerm,
            tracker->termIndex
        );
		tracker->termIndex++;
        assert(	rightTerm != NULL );

        Indices * rightTracker = checkQuantityTerm(
            N,
            rightTerm,
            parameterTreeRoot,
			errorMessage,
            report
        );
		updateDestinationTrackerIndicesFromSource(tracker, rightTracker);
		strcat(errorMessage, gNewtonTokenDescriptions[lowBinOpNode->type]);

        newtonCheckBinOp(
            N,
			expressionRoot,
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
            tracker->lowBinOpIndex
        );

		free(rightTracker);
    }

	free(leftTracker);

	return tracker;
}

Indices * 
checkQuantityTerm(
    State * N,
    IrNode * termRoot,
    IrNode * parameterTreeRoot,
    char * errorMessage,
    ConstraintReport * report
) {
	Indices * tracker = (Indices *) calloc(1, sizeof(Indices));

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

	IrNode * unaryOp = findNthIrNodeOfTypes(
		N,
		termRoot,
		kNewtonIrNodeType_PunaryOp,
		gNewtonFirsts,
		tracker->lowBinOpIndex
		);
	if (unaryOp != NULL)
	{
		tracker->lowBinOpIndex++;
	}

    IrNode * leftFactor = findNthIrNodeOfTypes(
        N,
        termRoot,
        kNewtonIrNodeType_PquantityFactor,
        gNewtonFirsts,
        tracker->factorIndex
    );
    tracker->factorIndex++;
    assert(leftFactor != NULL);
    Indices * leftTracker = checkQuantityFactor(
        N,
        leftFactor,
        parameterTreeRoot,
		errorMessage,
        report
    );
	termRoot->value = leftFactor->value;
	updateDestinationTrackerIndicesFromSource(tracker, leftTracker);

	if (unaryOp != NULL)
	{
		termRoot->value *= -1;
	}

	bool noFactorHasValueSet = termRoot->value == 0;

    IrNode * midBinOpNode = findNthIrNodeOfTypes(
        N,
        termRoot,
        kNewtonIrNodeType_PmidPrecedenceBinaryOp,
        gNewtonFirsts,
        tracker->midBinOpIndex
    );

    while (midBinOpNode != NULL)
    {
		tracker->midBinOpIndex++;
	    strcat(errorMessage, gNewtonTokenDescriptions[midBinOpNode->type]);

        IrNode * rightFactor = findNthIrNodeOfTypes(
            N,
            termRoot,
            kNewtonIrNodeType_PquantityFactor,
            gNewtonFirsts,
            tracker->factorIndex
        );
		tracker->factorIndex++;
        Indices * rightTracker = checkQuantityFactor(
            N,
            rightFactor,
            parameterTreeRoot,
			errorMessage,
            report
        );
		updateDestinationTrackerIndicesFromSource(tracker, rightTracker);
		noFactorHasValueSet = noFactorHasValueSet && (rightFactor->value == 0);

		/* 
         *  checking for high precedence binop just sets the values by multiplying or dividing 
         */
        newtonCheckBinOp(
			N,
			termRoot,
			leftFactor,
			rightFactor,
			midBinOpNode->type,
			report
		);

        midBinOpNode = findNthIrNodeOfTypes(
            N,
            termRoot,
            kNewtonIrNodeType_PmidPrecedenceBinaryOp,
            gNewtonFirsts,
            tracker->midBinOpIndex
        );
		free(rightTracker);
    }

	if (!noFactorHasValueSet)
		assert(termRoot->value != 0);

	if (noFactorHasValueSet)
		termRoot->value = 0;

	free(leftTracker);

	return tracker;
}

Indices *
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
    Indices * tracker = (Indices *) calloc(1, sizeof(Indices));

    IrNode * factor = findNthIrNodeOfTypes(
    	N,
    	factorRoot,
    	kNewtonIrNodeType_PquantityFactor,
    	gNewtonFirsts,
        tracker->factorIndex
    );


	if (factorRoot->type == kNewtonIrNodeType_PquantityExpression)
	{
		tracker->expressionIndex++;

		Indices * rightTracker = checkQuantityExpression(
			N,
			factorRoot,
			parameterTreeRoot,
			errorMessage,
			report);
		updateDestinationTrackerIndicesFromSource(tracker, rightTracker);
	}
    else if (
		!newtonIsDimensionless(factor->physics) &&
		!factor->physics->isConstant &&
		newtonPhysicsTablePhysicsForDimensionAliasAbbreviation(N, N->newtonIrTopScope, factor->tokenString) == NULL &&
		newtonPhysicsTablePhysicsForDimensionAlias(N, N->newtonIrTopScope, factor->tokenString) == NULL)
    {
        /*
         *  Suppose the caller of the API has supplied a IrNode Tidentifier "L" with numeric value 5.
         *  The constraint is L < 2 * meter.
         *  
         *  Currently, searching the symbol table for a unit returns that IrNode with its corresponding Physics struct.
         *  A matching parameter must correspond to the Physics struct bound by the token string (e.g. L : distance),
         *  but we do not want to raise an error a node that just is a unit (e.g. meter), not a Physics.
         */
        IrNode * matchingParameter = newtonParseFindNodeByParameterNumberAndSubindex(
			N,
			parameterTreeRoot,
			factor->parameterNumber,
			factor->physics->subindex);
        if (matchingParameter == NULL)
        {
		    sprintf(
				report->dimensionErrorMessage,
				"newton-check-pass.c:checkQuantityFactor: did not find a parameter with physics id %" PRIu64, factor->physics->id);

		    fatal(N, "newton-check-pass.c:checkQuantity: matchingParameter is null\n");
		    return NULL;
        }
        else
        {
        	factor->value = matchingParameter->value;
        }
    }

    char factorDetailMessage[1024];
    if (factor->type == kNewtonIrNodeType_Tidentifier && factor->physics)
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

    IrNode * highBinOpNode = findNthIrNodeOfTypes(
    	N,
    	factorRoot,
    	kNewtonIrNodeType_PhighPrecedenceBinaryOp,
    	gNewtonFirsts,
    	tracker->highBinOpIndex
    );

	/*
     *  if, not while, because a factor can have maximum one exponential expression at a time 
     */
    if (highBinOpNode != NULL)
    {
        tracker->highBinOpIndex++;
        strcat(errorMessage, gNewtonTokenDescriptions[highBinOpNode->type]);
        strcat(errorMessage, "(");

        IrNode * expression = findNthIrNodeOfType(
		    N,
			factorRoot,
			kNewtonIrNodeType_PquantityExpression,
			tracker->expressionIndex
			);
        tracker->expressionIndex++;
		tracker->factorIndex++;

        Indices * rightTracker = checkQuantityExpression(
			N,
			expression,
			parameterTreeRoot,
			errorMessage,
			report);
		updateDestinationTrackerIndicesFromSource(tracker, rightTracker);

        strcat(errorMessage, ")");

        newtonCheckBinOp(
			N,
			factor,
			factor,
			expression,
			highBinOpNode->type,
			report);
    }

	return tracker;
}
