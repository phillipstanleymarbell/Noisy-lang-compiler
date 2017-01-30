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
#include "noisy-timeStamps.h"
#include "noisy-errors.h"
#include "noisy.h"
#include "noisy-irHelpers.h"
#include "noisy-parser.h"
#include "newton-parser-expression.h"
#include "newton-parser.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton-firstAndFollow.h"


extern char *		gNewtonAstNodeStrings[];
extern int		gNewtonFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];

extern void		noisyFatal(NoisyState *  N, const char *  msg);
extern void		noisyError(NoisyState *  N, const char *  msg);

/*
 * ParseNumericExpression is only used to parse expressions of numbers and dimensionless constants inside exponents.
 * It was inconvenient just to use ParseQuantityExpression for the following reason.
 * Although we do not want to evaluate expressions at compile time, evaluating
 * expressions inside exponents is necessary for compile time dimensional checking.
 * e.g.) The expression, mass ** 2, yields two "mass" dimensions in numeratorDimensions.
 * If we use ParseQuantityExpression, then sometimes not all the terms and factors have
 * numeric values known.
 * e.g.) Pi == 3.14 but mass might not have a numeric value.
 *
 * We use kNewtonIrNodeType_PquantityTerm and kNewtonIrNodeType_PquantityFactor because
 * constant physics structs are essentially quantityFactors.
 */
NoisyIrNode *
newtonParseNumericExpression(NoisyState * N, NoisyScope * currentScope)
{
    NoisyIrNode * leftTerm;
    NoisyIrNode * rightTerm;

    if (newtonInFirst(N, kNewtonIrNodeType_PquantityTerm))
    {
        leftTerm = newtonParseNumericTerm(N, currentScope);

        while (newtonInFirst(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp))
        {
            NoisyIrNode * binOp = newtonParseLowPrecedenceBinaryOp(N, currentScope);
            addLeaf(N, leftTerm, binOp);
            
            rightTerm = newtonParseNumericTerm(N, currentScope);
            addLeafWithChainingSeqNewton(N, leftTerm, rightTerm);

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
        noisyParserSyntaxError(N, kNewtonIrNodeType_PquantityExpression, kNoisyIrNodeTypeMax);
        noisyFatal(N, Esanity);
    }
    
    return leftTerm;
}

NoisyIrNode *
newtonParseNumericTerm(NoisyState * N, NoisyScope * currentScope)
{
    NoisyIrNode *   intermediate = genNoisyIrNode(N,   kNewtonIrNodeType_PquantityTerm,
                        NULL /* left child */,
                        NULL /* right child */,
                        newtonLexPeek(N, 1)->sourceInfo /* source info */);
    intermediate->value = 1;
    if (newtonInFirst(N, kNewtonIrNodeType_PunaryOp))
    {
        addLeaf(N, intermediate, newtonParseUnaryOp(N, currentScope));
        intermediate->value *= -1;
    }
    
    NoisyIrNode * leftFactor = newtonParseNumericFactor(N, currentScope);
    intermediate->value *= leftFactor->value;

    addLeafWithChainingSeqNewton(N, intermediate, leftFactor);
    
    while (newtonInFirst(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp))
    {
        NoisyIrNode * binOp = newtonParseMidPrecedenceBinaryOp(N, currentScope);
        addLeafWithChainingSeqNewton(N, intermediate, binOp);
        
        NoisyIrNode * rightFactor = newtonParseNumericFactor(N, currentScope);
        addLeafWithChainingSeqNewton(N, intermediate, rightFactor);
        
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

NoisyIrNode *
newtonParseNumericFactor(NoisyState * N, NoisyScope * currentScope)
{
    NoisyIrNode *   node;

    if (peekCheckNewton(N, 1, kNewtonIrNodeType_Tidentifier))
    {
        node = newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
        assert(newtonIsConstant(node->physics));

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
        noisyFatal(N, "newtonParseQuantityFactor: missed a case in factor\n");
    }

    if (newtonInFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp))
    {
        addLeaf(N, node, newtonParseHighPrecedenceBinaryOp(N, currentScope));
        
        /* exponents are automatically just one integer unless wrapped in parens */
        NoisyIrNode * exponentExpression = peekCheckNewton(N, 1, kNewtonIrNodeType_TleftParen) ? 
            newtonParseNumericExpression(N, currentScope) : 
            newtonParseInteger(N, currentScope);
        addLeaf(N, node, exponentExpression);
        
        /* 0 ** 0 in mathematics is indeterminate */
        assert(node->value != 0 || exponentExpression->value != 0);
        node->value = pow(node->value, exponentExpression->value);
    }

    return node;
}

NoisyIrNode *
newtonParseQuantityExpression(NoisyState * N, NoisyScope * currentScope)
{
    NoisyIrNode * leftTerm;
    NoisyIrNode * rightTerm;

    if (newtonInFirst(N, kNewtonIrNodeType_PquantityTerm))
    {
        leftTerm = newtonParseQuantityTerm(N, currentScope);

        while (newtonInFirst(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp))
        {
            addLeaf(N, leftTerm, newtonParseLowPrecedenceBinaryOp(N, currentScope));
            
            rightTerm = newtonParseQuantityTerm(N, currentScope);
            addLeafWithChainingSeqNewton(N, leftTerm, rightTerm);
            
            // compare LHS and RHS prime numbers and make sure they're equal
            assert(leftTerm->physics->numeratorPrimeProduct == rightTerm->physics->numeratorPrimeProduct);
            assert(leftTerm->physics->denominatorPrimeProduct == rightTerm->physics->denominatorPrimeProduct);
        }
    }
    else
    {
        noisyParserSyntaxError(N, kNewtonIrNodeType_PquantityExpression, kNoisyIrNodeTypeMax);
        noisyFatal(N, Esanity);
    }
    
    return leftTerm;
}


NoisyIrNode *
newtonParseQuantityTerm(NoisyState * N, NoisyScope * currentScope)
{
    NoisyIrNode *   intermediate = genNoisyIrNode(N,   kNewtonIrNodeType_PquantityTerm,
                        NULL /* left child */,
                        NULL /* right child */,
                        newtonLexPeek(N, 1)->sourceInfo /* source info */);

    intermediate->physics = (Physics *) calloc(1, sizeof(Physics));
    intermediate->physics->numeratorPrimeProduct = 1;
    intermediate->physics->denominatorPrimeProduct = 1;


    if (newtonInFirst(N, kNewtonIrNodeType_PunaryOp))
    {
        addLeaf(N, intermediate, newtonParseUnaryOp(N, currentScope));
    }

    bool isPhysics /*not a number*/ = peekCheckNewton(N, 1, kNewtonIrNodeType_Tidentifier);
    NoisyIrNode * leftFactor = newtonParseQuantityFactor(N, currentScope);
    addLeafWithChainingSeqNewton(N, intermediate, leftFactor);
    
    int numVectorsInTerm = 0;

    if (isPhysics)
    {
        if (leftFactor->physics->numeratorDimensions)
            newtonPhysicsCopyNumeratorDimensions(N, intermediate->physics, leftFactor->physics);
        if (leftFactor->physics->denominatorDimensions)
            newtonPhysicsCopyDenominatorDimensions(N, intermediate->physics, leftFactor->physics);

        /*
         * If either LHS or RHS is a vector (not both), then the resultant is a vector
         */
        if (leftFactor->physics->isVector)
        {
            intermediate->physics->isVector = true;
            numVectorsInTerm++;
        }
    }
    
    NoisyIrNode * rightFactor;

    while (newtonInFirst(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp))
    {
        NoisyIrNode * binOp = newtonParseMidPrecedenceBinaryOp(N, currentScope);
        addLeafWithChainingSeqNewton(N, intermediate, binOp);
        
        bool isPhysics = peekCheckNewton(N, 1, kNewtonIrNodeType_Tidentifier);
        rightFactor = newtonParseQuantityFactor(N, currentScope);
        addLeafWithChainingSeqNewton(N, intermediate, rightFactor);
        
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
            if (rightFactor->physics->numeratorDimensions)
                newtonPhysicsCopyNumeratorDimensions(N, intermediate->physics, rightFactor->physics);
            if (rightFactor->physics->denominatorDimensions)
                newtonPhysicsCopyDenominatorDimensions(N, intermediate->physics, rightFactor->physics);
        }
        else if (isPhysics && binOp->type == kNewtonIrNodeType_Tdiv)
        {
            if (rightFactor->physics->denominatorDimensions)
                newtonPhysicsCopyNumeratorToDenominatorDimensions(N, intermediate->physics, rightFactor->physics);
            if (rightFactor->physics->numeratorDimensions)
                newtonPhysicsCopyDenominatorToNumeratorDimensions(N, intermediate->physics, rightFactor->physics);
        }
    }

    return intermediate;
}

NoisyIrNode *
newtonParseQuantityFactor(NoisyState * N, NoisyScope * currentScope)
{
    NoisyIrNode *   node;

    if (peekCheckNewton(N, 1, kNewtonIrNodeType_Tidentifier))
    {
        node = newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
        if (newtonInFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp))
        {
            addLeaf(N, node, newtonParseHighPrecedenceBinaryOp(N, currentScope));
            
            /* exponents are automatically just one integer unless wrapped in parens */
            NoisyIrNode * exponentExpression = peekCheckNewton(N, 1, kNewtonIrNodeType_TleftParen) ? 
                newtonParseNumericExpression(N, currentScope) : 
                newtonParseInteger(N, currentScope);
            addLeaf(N, node, exponentExpression);
        }
    }
    else if (peekCheck(N, 1, kNewtonIrNodeType_Tnumber))
    {
        node = newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope);
    }
    else if (newtonInFirst(N, kNewtonIrNodeType_PtimeOp))
    {
        node = newtonParseTimeOp(N, currentScope);
    }
    else if (newtonInFirst(N, kNewtonIrNodeType_PvectorOp) && peekCheck(N, 2, kNewtonIrNodeType_TleftParen) && peekCheck(N, 4, kNewtonIrNodeType_Tcomma))
    {
		node = newtonParseVectorOp(N, currentScope);
    }
    else if (peekCheck(N, 1, kNewtonIrNodeType_TleftParen))
    {
        newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
        node = newtonParseQuantityExpression(N, currentScope);
        newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);
        
        /*
         * e.g.) (acceleration * mass) ** (3 + 5)
         */
        if (newtonInFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp))
        {
            addLeaf(N, node, newtonParseHighPrecedenceBinaryOp(N, currentScope));
            /* exponents are automatically just one integer unless wrapped in parens */
            NoisyIrNode * exponentExpression = peekCheckNewton(N, 1, kNewtonIrNodeType_TleftParen) ? 
                newtonParseNumericExpression(N, currentScope) : 
                newtonParseInteger(N, currentScope);
            addLeaf(N, node, exponentExpression);
        }
    }
    else
    {
        noisyFatal(N, "newtonParseQuantityFactor: missed a case in factor\n");
    }

    return node;
}

NoisyIrNode *
newtonParseVectorOp(NoisyState *  N, NoisyScope * currentScope)
{
    NoisyIrNode *   intermediate = genNoisyIrNode(N,   kNewtonIrNodeType_PvectorOp,
                        NULL /* left child */,
                        NULL /* right child */,
                        newtonLexPeek(N, 1)->sourceInfo /* source info */);

    intermediate->physics = (Physics *) calloc(1, sizeof(Physics));
    intermediate->physics->numeratorPrimeProduct = 1;
    intermediate->physics->denominatorPrimeProduct = 1;
    
    bool addAngleToDenominator = false;
    
    if (peekCheck(N, 1, kNewtonIrNodeType_Tdot))
    {
        addLeaf(N, intermediate, newtonParseTerminal(N, kNewtonIrNodeType_Tdot, currentScope));
    } 
    else if (peekCheck(N, 1, kNewtonIrNodeType_Tcross))
    {
        addLeaf(N, intermediate, newtonParseTerminal(N, kNewtonIrNodeType_Tcross, currentScope));
        addAngleToDenominator = true;
    } 
    else 
    {
        noisyFatal(N, "newtonParseVectorOp: op is not dot or cross\n");
    }
    
    newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
    
    NoisyIrNode * left;
    left = newtonParseQuantityExpression(N, currentScope);
    addLeafWithChainingSeqNewton(N, intermediate, left);

    newtonPhysicsCopyNumeratorDimensions(N, intermediate->physics, left->physics);
    newtonPhysicsCopyDenominatorDimensions(N, intermediate->physics, left->physics);
    
    newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
    
    NoisyIrNode * right;
    right = newtonParseQuantityExpression(N, currentScope);
    addLeafWithChainingSeqNewton(N, intermediate, right);

    assert(left->physics->isVector && right->physics->isVector);

    newtonPhysicsCopyNumeratorDimensions(N, intermediate->physics, right->physics);
    newtonPhysicsCopyDenominatorDimensions(N, intermediate->physics, right->physics);

    if (addAngleToDenominator) 
    {
        Dimension* angle = newtonDimensionTableDimensionForIdentifier(N, currentScope, "rad");
        newtonPhysicsAddDenominatorDimension(N, intermediate->physics, angle);
    } 
    
    newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);

    return intermediate;
}


NoisyIrNode *
newtonParseLowPrecedenceBinaryOp(NoisyState *  N, NoisyScope * currentScope)
{
    NoisyIrNode *   n;

    if (peekCheckNewton(N, 1, kNewtonIrNodeType_Tplus))
    {
        n = newtonParseTerminal(N, kNewtonIrNodeType_Tplus, currentScope);
    }
    else if (peekCheckNewton(N, 1, kNewtonIrNodeType_Tminus))
    {
        n = newtonParseTerminal(N, kNewtonIrNodeType_Tminus, currentScope);
    }
    else
    {
        noisyParserSyntaxError(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp, kNoisyIrNodeTypeMax);
        // noisyParserErrorRecovery(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp);
        return NULL;
    }

    return n;
}

NoisyIrNode *
newtonParseUnaryOp(NoisyState *  N, NoisyScope * currentScope)
{
    NoisyIrNode *   n = NULL;

    if (peekCheckNewton(N, 1, kNewtonIrNodeType_Tminus))
    {
        n = newtonParseTerminal(N, kNewtonIrNodeType_Tminus, currentScope);
    }
    else
    {
        noisyParserSyntaxError(N, kNewtonIrNodeType_PunaryOp, kNoisyIrNodeTypeMax);
    }

    return n;
}

NoisyIrNode *
newtonParseTimeOp(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(
        N,
        kNewtonIrNodeType_PtimeOp,
		NULL /* left child */,
		NULL /* right child */,
		newtonLexPeek(N, 1)->sourceInfo /* source info */
    );
    
    NoisyIrNodeType type;
    if ((type = newtonLexPeek(N, 1)->type) == kNewtonIrNodeType_Tintegral || 
         type == kNewtonIrNodeType_Tderivative)
    {
		addLeaf(N, node, newtonParseTerminal(N, type, currentScope));
    }
    else
    {
        noisyParserSyntaxError(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, kNoisyIrNodeTypeMax);
    }

    while ((type = newtonLexPeek(N, 1)->type) == kNewtonIrNodeType_Tintegral || 
            type == kNewtonIrNodeType_Tderivative)
    {
		addLeafWithChainingSeqNewton(N, node, newtonParseTerminal(N, type, currentScope));
        addLeafWithChainingSeqNewton(N, node, newtonParseQuantityExpression(N, currentScope));
    }


    return node;
}

NoisyIrNode *
newtonParseCompareOp(NoisyState * N, NoisyScope * currentScope)
{
    
    NoisyIrNodeType type;
    if ((type = newtonLexPeek(N, 1)->type) == kNewtonIrNodeType_Tlt || 
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
        noisyFatal(N, "newton-parser-expression.c:newtonParseCompareOp op is not a compare op");
    }
}

NoisyIrNode *
newtonParseHighPrecedenceBinaryOp(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(
        N,
        kNewtonIrNodeType_PhighPrecedenceBinaryOp,
		NULL /* left child */,
		NULL /* right child */,
		newtonLexPeek(N, 1)->sourceInfo /* source info */
    );
    
    if (peekCheckNewton(N, 1, kNewtonIrNodeType_Texponent))
    {
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Texponent, currentScope));
    }
    else
    {
        noisyParserSyntaxError(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, kNoisyIrNodeTypeMax);
    }
    return node;
}

NoisyIrNode *
newtonParseMidPrecedenceBinaryOp(NoisyState *  N, NoisyScope * currentScope)
{
    NoisyIrNode *   n;

    if (peekCheckNewton(N, 1, kNewtonIrNodeType_Tmul))
    {
        n = newtonParseTerminal(N, kNewtonIrNodeType_Tmul, currentScope);
    }
    else if (peekCheckNewton(N, 1, kNewtonIrNodeType_Tdiv))
    {
        n = newtonParseTerminal(N, kNewtonIrNodeType_Tdiv, currentScope);
    }
    else
    {
        noisyParserSyntaxError(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp, kNoisyIrNodeTypeMax);
        noisyFatal(N, "newton-parser-expression.c: newtonParseMidPrecedenceBinaryOp not a mid precedence binop\n");
    }

    return n;
}

NoisyIrNode *
newtonParseInteger(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(N,	kNewtonIrNodeType_Pinteger,
						NULL /* left child */,
						NULL /* right child */,
						newtonLexPeek(N, 1)->sourceInfo /* source info */);
    
    if (newtonInFirst(N, kNewtonIrNodeType_PunaryOp))
    {
        addLeaf(N, node, newtonParseUnaryOp(N, currentScope));
        node->value = -1;
    }

    NoisyIrNode * number = newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope);
    addLeaf(N, node, number);
    node->value = node->value == -1 ? node->value * number->value : number->value;
        
    assert(node->value != 0); // TODO remove this assertion later bc value MIGHT be 0

    return node;
}
