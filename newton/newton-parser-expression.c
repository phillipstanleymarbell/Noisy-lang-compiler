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
#include "common-timeStamps.h"
#include "common-errors.h"
#include "data-structures.h"
#include "common-irHelpers.h"
#include "noisy-parser.h"
#include "newton-parser-expression.h"
#include "newton-parser.h"
#include "common-lexers-helpers.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "common-firstAndFollow.h"


extern char *		gNewtonAstNodeStrings[];
extern int		gNewtonFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];

extern void		fatal(State *  N, const char *  msg);
extern void		error(State *  N, const char *  msg);

/*
 * ParseNumericExpression is only used to parse expressions of numbers and dimensionless constants inside exponents.
 * It was inconvenient just to use ParseQuantityExpression for the following reason.
 * Although we do not want to evaluate expressions at compile time, evaluating
 * expressions inside exponents is necessary for compile time dimensional checking.
 * If we use ParseQuantityExpression, then sometimes not all the terms and factors have
 * numeric values known. To distinguish the two cases, we can either pass in a flag to quantity parsing methods
 * or just use ParseNumericExpression. 
 * e.g.) Pi == 3.14 but mass might not have a numeric value.
 *
 * We use kNewtonIrNodeType_PquantityTerm and kNewtonIrNodeType_PquantityFactor because
 * constant physics structs are essentially quantityFactors.
 */
IrNode *
newtonParseNumericExpression(State * N, Scope * currentScope)
{
    IrNode * leftTerm;
    IrNode * rightTerm;

    if (inFirst(N, kNewtonIrNodeType_PquantityTerm, gNewtonFirsts))
    {
        leftTerm = newtonParseNumericTerm(N, currentScope);

        while (inFirst(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp, gNewtonFirsts))
        {
            IrNode * binOp = newtonParseLowPrecedenceBinaryOp(N, currentScope);
            addLeaf(N, leftTerm, binOp);

            rightTerm = newtonParseNumericTerm(N, currentScope);
            addLeafWithChainingSeq(N, leftTerm, rightTerm);

            if (binOp->type == kNewtonIrNodeType_Tplus) 
            {
                leftTerm->value += rightTerm->value;
            }
            else if (binOp->type == kNewtonIrNodeType_Tminus)
            {
                leftTerm->value -= rightTerm->value;
            }
        }
    }
    else
    {
        fatal(N, Esanity);
    }

    return leftTerm;
}

IrNode *
newtonParseNumericTerm(State * N, Scope * currentScope)
{
    IrNode *   intermediate = genIrNode(N,   kNewtonIrNodeType_PquantityTerm,
                        NULL /* left child */,
                        NULL /* right child */,
                        lexPeek(N, 1)->sourceInfo /* source info */);
    intermediate->value = 1;
    if (inFirst(N, kNewtonIrNodeType_PunaryOp, gNewtonFirsts))
    {
        addLeaf(N, intermediate, newtonParseUnaryOp(N, currentScope));
        intermediate->value *= -1;
    }

    IrNode * leftFactor = newtonParseNumericFactor(N, currentScope);
    intermediate->value *= leftFactor->value;

    addLeafWithChainingSeq(N, intermediate, leftFactor);

    while (inFirst(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp, gNewtonFirsts))
    {
        IrNode * binOp = newtonParseMidPrecedenceBinaryOp(N, currentScope);
        addLeafWithChainingSeq(N, intermediate, binOp);

        IrNode * rightFactor = newtonParseNumericFactor(N, currentScope);
        addLeafWithChainingSeq(N, intermediate, rightFactor);

        if (binOp->type == kNewtonIrNodeType_Tmul) 
        {
            intermediate->value *= rightFactor->value;
        }
        else if (binOp->type == kNewtonIrNodeType_Tdiv)
        {
            intermediate->value /= rightFactor->value;
        }
    }

    return intermediate;
}

IrNode *
newtonParseNumericFactor(State * N, Scope * currentScope)
{
    IrNode *   node;

    if (peekCheck(N, 1, kNewtonIrNodeType_Tidentifier))
    {
        node = newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
        assert(node->physics->isConstant);
    }
    else if (peekCheck(N, 1, kNewtonIrNodeType_Tnumber))
    {
        node = newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope);
    }
    else if (peekCheck(N, 1, kNewtonIrNodeType_TleftParen))
    {
        newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
        node = newtonParseNumericExpression(N, currentScope);
        newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);
    }
    else
    {
        fatal(N, "newtonParseQuantityFactor: missed a case in factor\n");
    }

    if (inFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, gNewtonFirsts))
    {
        addLeaf(N, node, newtonParseHighPrecedenceBinaryOp(N, currentScope));

        /* exponents are automatically just one integer unless wrapped in parens */
        IrNode * exponentExpression = peekCheck(N, 1, kNewtonIrNodeType_TleftParen) ? 
            newtonParseNumericExpression(N, currentScope) : 
            newtonParseInteger(N, currentScope);
        addLeaf(N, node, exponentExpression);

        /* 0 ** 0 in mathematics is indeterminate */
        assert(node->value != 0 || exponentExpression->value != 0);
        node->value = pow(node->value, exponentExpression->value);
    }

    return node;
}

IrNode *
newtonParseQuantityExpression(State * N, Scope * currentScope)
{
    IrNode *   expression = genIrNode(N,   kNewtonIrNodeType_PquantityExpression,
                                                  NULL /* left child */,
                                                  NULL /* right child */,
                                                  lexPeek(N, 1)->sourceInfo /* source info */);

    expression->physics = newtonInitPhysics(N, currentScope, NULL);

    N->currentParameterNumber = 0;

    if (inFirst(N, kNewtonIrNodeType_PquantityTerm, gNewtonFirsts))
	{
        IrNode * leftTerm = newtonParseQuantityTerm(N, currentScope);
        expression->value = leftTerm->value;
        expression->physics = leftTerm->physics;
        addLeaf(N, expression, leftTerm);


        while (inFirst(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp, gNewtonFirsts))
		{
            addLeafWithChainingSeq(N, expression, newtonParseLowPrecedenceBinaryOp(N, currentScope));

            IrNode * rightTerm = newtonParseQuantityTerm(N, currentScope);
            addLeafWithChainingSeq(N, leftTerm, rightTerm);
            expression->value += rightTerm->value;

            assert(areTwoPhysicsEquivalent(N, leftTerm->physics, rightTerm->physics));
		}
	}
    else
	{
        fatal(N, Esanity);
	}

    return expression;
}


IrNode *
newtonParseQuantityTerm(State * N, Scope * currentScope)
{
    IrNode *   intermediate = genIrNode(N,   kNewtonIrNodeType_PquantityTerm,
                        NULL /* left child */,
                        NULL /* right child */,
                        lexPeek(N, 1)->sourceInfo /* source info */);

    intermediate->physics = newtonInitPhysics(N, currentScope, NULL);
    intermediate->value = 1;

    bool isUnary = false;

    if (inFirst(N, kNewtonIrNodeType_PunaryOp, gNewtonFirsts))
    {
        addLeaf(N, intermediate, newtonParseUnaryOp(N, currentScope));
        isUnary = true;
    }

    bool hasNumberInTerm = false;
    bool isPhysics /*not a number*/ = peekCheck(N, 1, kNewtonIrNodeType_Tidentifier);
    IrNode * leftFactor = newtonParseQuantityFactor(N, currentScope);
    addLeafWithChainingSeq(N, intermediate, leftFactor);
    hasNumberInTerm = hasNumberInTerm || leftFactor->physics == NULL || leftFactor->physics->isConstant;
    if (hasNumberInTerm)
	{
        intermediate->value = isUnary ? leftFactor->value * -1 : leftFactor->value;
	}

    int numVectorsInTerm = 0;

    if (isPhysics)
    {
		newtonPhysicsAddExponents(N, intermediate->physics, leftFactor->physics);

        /*
         * If either LHS or RHS is a vector (not both), then the resultant is a vector
         */
        if (leftFactor->physics->isVector)
        {
            intermediate->physics->isVector = true;
            numVectorsInTerm++;
        }
    }

    IrNode * rightFactor;

    while (inFirst(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp, gNewtonFirsts))
    {
        IrNode * binOp = newtonParseMidPrecedenceBinaryOp(N, currentScope);
        addLeafWithChainingSeq(N, intermediate, binOp);

        bool isPhysics = peekCheck(N, 1, kNewtonIrNodeType_Tidentifier);
        rightFactor = newtonParseQuantityFactor(N, currentScope);
        addLeafWithChainingSeq(N, intermediate, rightFactor);
        hasNumberInTerm = hasNumberInTerm || leftFactor->physics == NULL || leftFactor->physics->isConstant;

        if (hasNumberInTerm)
          {
            if (binOp->type == kNewtonIrNodeType_Tmul)
              {
                intermediate->value = rightFactor->value == 0 ? intermediate->value : intermediate->value * rightFactor->value;
              }
            else if (binOp->type == kNewtonIrNodeType_Tdiv)
              {
                intermediate->value = rightFactor->value == 0 ? intermediate->value : intermediate->value / rightFactor->value;
              }
          }

        // TODO double check this logic when I'm more awake
        if (isPhysics && rightFactor->physics->isVector)
        {
            intermediate->physics->isVector = true;
            numVectorsInTerm++;

            /*
             * Cannot perform multiply or divide operations on two vectors
             * e.g.) vector * scalar * scalar / vector is illegal because
             * it boils down to vector / vector which is illegal
             */
            assert(numVectorsInTerm < 2);
        }

        if (isPhysics && binOp->type == kNewtonIrNodeType_Tmul)
        {
			newtonPhysicsAddExponents(N, intermediate->physics, rightFactor->physics);
        }
        else if (isPhysics && binOp->type == kNewtonIrNodeType_Tdiv)
        {
			newtonPhysicsSubtractExponents(N, intermediate->physics, rightFactor->physics);
        }
    }

    if (! hasNumberInTerm)
		intermediate->value = 0;

    return intermediate;
}

IrNode *
newtonParseQuantityFactor(State * N, Scope * currentScope)
{
    IrNode *   factor;

    if (peekCheck(N, 1, kNewtonIrNodeType_Tidentifier))
    {
        factor = newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
        factor->physics = deepCopyPhysicsNode(factor->physics);
        factor->value = factor->physics->value;

        assert(factor->tokenString != NULL);

        /* Is a matchable parameter corresponding the invariant parameter */
        if (!newtonIsDimensionless(factor->physics) && !factor->physics->isConstant && newtonDimensionTableDimensionForIdentifier(N, N->newtonIrTopScope, factor->tokenString) == NULL)
        {
          factor->parameterNumber = N->currentParameterNumber++;
        }
    }
    else if (peekCheck(N, 1, kNewtonIrNodeType_Tnumber))
    {
        factor = newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope);
    }
    else if (peekCheck(N, 1, kNewtonIrNodeType_TleftParen))
    {
        newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
        factor = newtonParseQuantityExpression(N, currentScope);
        newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);
    }
    else
    {
        fatal(N, "newtonParseQuantityFactor: missed a case in factor\n");
    }

    /*
     * e.g.) (acceleration * mass) ** (3 + 5)
     */
    if (inFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, gNewtonFirsts))
    {
        addLeaf(N, factor, newtonParseHighPrecedenceBinaryOp(N, currentScope));

        IrNode * exponentialExpression = newtonParseExponentialExpression(N, currentScope, factor);
        addLeafWithChainingSeq(N, factor, exponentialExpression);

        if (factor->value != 0)
		{
			factor->value = pow(factor->value, exponentialExpression->value);
		}
    }

    return factor;
}

IrNode *
newtonParseExponentialExpression(State * N, Scope * currentScope, IrNode * baseNode)
{
    /* exponents are automatically just one integer unless wrapped in parens */
    IrNode * exponent = peekCheck(N, 1, kNewtonIrNodeType_TleftParen) ?
        newtonParseNumericExpression(N, currentScope) :
        newtonParseInteger(N, currentScope);

    if (exponent->value == 0)
    {
        baseNode->physics->value = 1;
		newtonPhysicsZeroExponents(N, baseNode->physics);

        return exponent;
    }
    else
    {
        baseNode->physics->value = pow(baseNode->physics->value, exponent->value);
    }

    /* If the base is a Physics quantity, the exponent must be an integer */
    assert(exponent->value == (int) exponent->value);
	newtonPhysicsMultiplyExponents(N, baseNode->physics, exponent->value);

    return exponent;
}



IrNode *
newtonParseLowPrecedenceBinaryOp(State *  N, Scope * currentScope)
{
    IrNode *   n;

    if (peekCheck(N, 1, kNewtonIrNodeType_Tplus))
    {
        n = newtonParseTerminal(N, kNewtonIrNodeType_Tplus, currentScope);
    }
    else if (peekCheck(N, 1, kNewtonIrNodeType_Tminus))
    {
        n = newtonParseTerminal(N, kNewtonIrNodeType_Tminus, currentScope);
    }
    else
    {
        // noisyParserErrorRecovery(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp);
        return NULL;
    }

    return n;
}

IrNode *
newtonParseUnaryOp(State *  N, Scope * currentScope)
{
    IrNode *   n = NULL;

    if (peekCheck(N, 1, kNewtonIrNodeType_Tminus))
    {
        n = newtonParseTerminal(N, kNewtonIrNodeType_Tminus, currentScope);
    }
    else
    {
        fatal(N, "newton-parser-expression.c: newtonParseUnaryOp: did not detect minus as unary op\n");
    }

    return n;
}


IrNode *
newtonParseCompareOp(State * N, Scope * currentScope)
{
    IrNodeType type;
    if ((type = lexPeek(N, 1)->type) == kNewtonIrNodeType_Tlt ||
         type == kNewtonIrNodeType_Tle ||
         type == kNewtonIrNodeType_Tge ||
         type == kNewtonIrNodeType_Tgt ||
         type == kNewtonIrNodeType_Tproportionality ||
         type == kNewtonIrNodeType_Tequivalent
       )
    {
		return newtonParseTerminal(N, type, currentScope);
    }
    else
    {
        fatal(N, "newton-parser-expression.c:newtonParseCompareOp op is not a compare op");
    }
}

IrNode *
newtonParseHighPrecedenceBinaryOp(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(
        N,
        kNewtonIrNodeType_PhighPrecedenceBinaryOp,
		NULL /* left child */,
		NULL /* right child */,
		lexPeek(N, 1)->sourceInfo /* source info */
    );

    if (peekCheck(N, 1, kNewtonIrNodeType_Texponent))
    {
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Texponent, currentScope));
    }
    else
    {
        fatal(N, "newton-parser-expression.c:newtonParseHighPrecedenceBinaryOp: no exponent token\n");
    }
    return node;
}

IrNode *
newtonParseMidPrecedenceBinaryOp(State *  N, Scope * currentScope)
{
    IrNode *   n;

    if (peekCheck(N, 1, kNewtonIrNodeType_Tmul))
    {
        n = newtonParseTerminal(N, kNewtonIrNodeType_Tmul, currentScope);
    }
    else if (peekCheck(N, 1, kNewtonIrNodeType_Tdiv))
    {
        n = newtonParseTerminal(N, kNewtonIrNodeType_Tdiv, currentScope);
    }
    else
    {
        fatal(N, "newton-parser-expression.c: newtonParseMidPrecedenceBinaryOp not a mid precedence binop\n");
    }

    return n;
}

IrNode *
newtonParseInteger(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pinteger,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);
    
    if (inFirst(N, kNewtonIrNodeType_PunaryOp, gNewtonFirsts))
    {
        addLeaf(N, node, newtonParseUnaryOp(N, currentScope));
        node->value = -1;
    }

    IrNode * number = newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope);
    addLeaf(N, node, number);
    node->value = node->value == -1 ? node->value * number->value : number->value;
        
    assert(node->value != 0); // TODO remove this assertion later bc value MIGHT be 0

    return node;
}
