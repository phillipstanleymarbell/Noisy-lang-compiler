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

#include "newton-parser.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton.h"
#include "newton-api.h"
#include "common-irHelpers.h"
#include "newton-check-pass.h"

extern int		gNewtonFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];

void
newtonCheckCompareOp(
    NoisyState* N,
    NoisyIrNode * leftExpression, 
    NoisyIrNode * rightExpression,
    NoisyIrNodeType compareOpType,
    NewtonAPIReport * report
) {
    switch(compareOpType)
    {
        case kNewtonIrNodeType_Tge:
            report->satisfiesValueConstraint = leftExpression->value >= rightExpression->value;
            report->satisfiesDimensionConstraint = (newtonIsConstant(leftExpression->physics) && newtonIsConstant(rightExpression->physics)) ||\
			  (leftExpression->physics->id == rightExpression->physics->id);
            if (!report->satisfiesValueConstraint)
                sprintf(report->valueErrorMessage, "%f should be >= %f", leftExpression->value, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;
        case kNewtonIrNodeType_Tgt:
            report->satisfiesValueConstraint = leftExpression->value > rightExpression->value;
            report->satisfiesDimensionConstraint = (newtonIsConstant(leftExpression->physics) && newtonIsConstant(rightExpression->physics)) ||\
			  (leftExpression->physics->id == rightExpression->physics->id);
            if (!report->satisfiesValueConstraint)
                sprintf(report->valueErrorMessage, "%f should be > %f", leftExpression->value, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;
        case kNewtonIrNodeType_Tle:
            report->satisfiesValueConstraint = leftExpression->value <= rightExpression->value;
            report->satisfiesDimensionConstraint = (newtonIsConstant(leftExpression->physics) && newtonIsConstant(rightExpression->physics)) ||\
                (leftExpression->physics->id == rightExpression->physics->id);
            if (!report->satisfiesValueConstraint)
                sprintf(report->valueErrorMessage, "%f should be <= %f", leftExpression->value, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;
        case kNewtonIrNodeType_Tlt:
            report->satisfiesValueConstraint = leftExpression->value < rightExpression->value;
            report->satisfiesDimensionConstraint = (newtonIsConstant(leftExpression->physics) && newtonIsConstant(rightExpression->physics)) ||\
                (leftExpression->physics->id == rightExpression->physics->id);
            if (!report->satisfiesValueConstraint)
                sprintf(report->valueErrorMessage, "%f should be < %f", leftExpression->value, rightExpression->value);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;
        case kNewtonIrNodeType_Tproportionality:
            /* TODO figure out how to check proportionality */
            break;
        case kNewtonIrNodeType_Tequivalent:
            /* TODO change == to incorporate an epsilon */
            report->satisfiesValueConstraint = leftExpression->value == rightExpression->value;
            report->satisfiesDimensionConstraint = (newtonIsConstant(leftExpression->physics) && newtonIsConstant(rightExpression->physics)) ||\
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
}

void
newtonCheckBinOp(
    NoisyState* N,
    NoisyIrNode * left,
    NoisyIrNode * right,
    NoisyIrNodeType binOpType,
    NewtonAPIReport * report
) {
	if (left == NULL || right == NULL)
	    noisyFatal(N, "newton-check-pass.c:newtonCheckBinOp: left or right is null");
    switch(binOpType)
    {
        case kNewtonIrNodeType_Tplus:
            left->value += right->value;
            report->satisfiesDimensionConstraint =\
                (newtonIsConstant(left->physics) && newtonIsConstant(right->physics)) ||\
			  (left->physics->id == right->physics->id);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;

        case kNewtonIrNodeType_Tminus:
            left->value -= right->value;
            report->satisfiesDimensionConstraint =\
                (newtonIsConstant(left->physics) && newtonIsConstant(right->physics)) ||\
			  (left->physics->id == right->physics->id);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;

        /* 
         * Note that we don't need to copy over any dimensions as in Parsing
         * because they are already filled in. Same for division and exponents
         */
        case kNewtonIrNodeType_Tmul:
            assert(left->value != 0); /* TODO this is not true, if it's actually 0. remove after debugging */
            left->value *= right->value;
            report->satisfiesDimensionConstraint =\
                (newtonIsConstant(left->physics) && newtonIsConstant(right->physics)) ||\
			  (left->physics->id == right->physics->id);
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
            break;

        case kNewtonIrNodeType_Tdiv:
            left->value /= right->value;
            report->satisfiesValueConstraint = right->value != 0;
            report->satisfiesDimensionConstraint =\
                (newtonIsConstant(left->physics) && newtonIsConstant(right->physics)) ||\
			  (left->physics->id == right->physics->id);
            if (!report->satisfiesValueConstraint)
                sprintf(report->valueErrorMessage, "division by zero");
            if (!report->satisfiesDimensionConstraint)
                sprintf(report->dimensionErrorMessage, "dimensions do not match");
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
            noisyFatal(N, "newton-api.c:newtonCheckCompareOp: no lowBinOp detected");
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

    if (constraintTreeRoot->irRightChild != NULL)
	  	iterateConstraints(N, constraintTreeRoot->irRightChild, parameterTreeRoot, report);

    if (constraintTreeRoot->irLeftChild != NULL)
	  	iterateConstraints(N, constraintTreeRoot->irLeftChild, parameterTreeRoot, report);
}

void
checkSingleConstraint(NoisyState * N, NoisyIrNode * constraintTreeRoot, NoisyIrNode* parameterTreeRoot, NewtonAPIReport* report)
{
    int expressionIndex = 0;
    
    /* check LHS expression */
    NoisyIrNode* leftExpression = findNthIrNodeOfType(
        N,
        constraintTreeRoot,
        kNewtonIrNodeType_PquantityTerm,
        &expressionIndex
    );
    expressionIndex++;
    checkQuantityExpression(
        N,
        leftExpression,
        parameterTreeRoot,
        report
    );

    /* find the compareOp between LHS and RHS*/
    NoisyIrNode* compareOp = findNthIrNodeOfTypes(
        N,
        constraintTreeRoot,
        kNewtonIrNodeType_PcompareOp,
		gNewtonFirsts,
        0
    );

    /* There can be only one compareOp per constraint */
    if (compareOp != NULL)
    {
        NoisyIrNode* rightExpression = findNthIrNodeOfType(
            N,
            constraintTreeRoot,
            kNewtonIrNodeType_PquantityTerm,
            &expressionIndex
        );
        expressionIndex++;
        checkQuantityExpression(
            N,
            rightExpression,
            parameterTreeRoot,
            report
        );

        newtonCheckCompareOp(
            N,
            leftExpression,
            rightExpression,
            compareOp->token->type,
            report
        );
    }
}

void
checkQuantityExpression(
    NoisyState * N,
    NoisyIrNode * expressionRoot,
    NoisyIrNode * parameterTreeRoot,
    NewtonAPIReport * report
) {
    int termIndex = 0;
    NoisyIrNode* leftTerm = findNthIrNodeOfTypes(
        N,
        expressionRoot,
        kNewtonIrNodeType_PquantityTerm,
        gNewtonFirsts,
        &termIndex
    );
    termIndex++;
    assert(leftTerm != NULL);
    checkQuantityTerm(
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
        &lowBinOpIndex
    );
    lowBinOpIndex++;

    while (lowBinOpNode != NULL)
    {
        NoisyIrNode* rightTerm = findNthIrNodeOfTypes(
            N,
            expressionRoot,
            kNewtonIrNodeType_PquantityTerm,
			gNewtonFirsts,
            &termIndex
        );
        termIndex++;
	    assert(	rightTerm != NULL );
        checkQuantityTerm(
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
            &lowBinOpIndex
        );
        lowBinOpIndex++;
    }
}

void
checkQuantityTerm(
    NoisyState * N,
    NoisyIrNode * termRoot,
    NoisyIrNode * parameterTreeRoot,
    NewtonAPIReport * report
) {
    int factorIndex = 0;
    NoisyIrNode* leftFactor = findNthIrNodeOfTypes(
        N,
        termRoot,
        kNewtonIrNodeType_PquantityFactor,
        gNewtonFirsts,
        &factorIndex
    );
    factorIndex++;
    assert(leftFactor != NULL);
    checkQuantityFactor(
        N,
        leftFactor,
        parameterTreeRoot,
        report
    );

    int midBinOpIndex = 0;
    NoisyIrNode* midBinOpNode = findNthIrNodeOfTypes(
        N,
        termRoot,
        kNewtonIrNodeType_PmidPrecedenceBinaryOp,
		gNewtonFirsts,
        &midBinOpIndex
    );
    midBinOpIndex++;

    while (midBinOpNode != NULL)
    {
        NoisyIrNode* rightFactor = findNthIrNodeOfTypes(
            N,
            termRoot,
            kNewtonIrNodeType_PquantityFactor,
			gNewtonFirsts,
            &factorIndex
        );
        factorIndex++;
        checkQuantityFactor(
            N,
            rightFactor,
            parameterTreeRoot,
            report
        );

        midBinOpNode = findNthIrNodeOfTypes(
            N,
            termRoot,
            kNewtonIrNodeType_PmidPrecedenceBinaryOp,
			gNewtonFirsts,
            &midBinOpIndex
        );
        midBinOpIndex++;
    }
}

void
checkQuantityFactor(
    NoisyState * N,
    NoisyIrNode * factorRoot,
    NoisyIrNode * parameterTreeRoot,
    NewtonAPIReport * report
) {
    // find a factor and then look it up in parameters
    // to assign a value
    //
    // if there is an highBinOp, check Expression
  	NoisyIrNode * factor = findNthIrNodeOfTypes(
	    N,
	    factorRoot,
	    kNewtonIrNodeType_PquantityFactor,
	    gNewtonFirsts,
		0
	);
	NoisyIrNode * matchingParameter = newtonParseFindNodeByPhysicsId(N, parameterTreeRoot, factor->physics->id);
	if (matchingParameter == NULL)
	{
	    sprintf(report->dimensionErrorMessage, "newton-check-pass.c:checkQuantityFactor: did not find a parameter with physics id %llu", factor->physics->id);
	    return; 
	}

	int highBinOpIndex = 0;
	NoisyIrNode* highBinOpNode = findNthIrNodeOfTypes(
		N,
		factorRoot,
		kNewtonIrNodeType_PhighPrecedenceBinaryOp,
		gNewtonFirsts,
		&highBinOpIndex
	);
	highBinOpIndex++;

	int expressionIndex = 0;
	while  (highBinOpNode != NULL)
	{
	    NoisyIrNode* expression = findNthIrNodeOfTypes(
	        N,
	        factorRoot,
	        kNewtonIrNodeType_PquantityTerm,
			gNewtonFirsts,
	        &expressionIndex
		);
	    expressionIndex++;
		checkQuantityExpression(	N,
									expression,
									parameterTreeRoot,
									report);
		highBinOpNode = findNthIrNodeOfTypes(
			N,
			factorRoot,
			kNewtonIrNodeType_PhighPrecedenceBinaryOp,
			gNewtonFirsts,
			&highBinOpIndex
		);
		highBinOpIndex++;
	}
}













