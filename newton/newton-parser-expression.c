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
#include "general-irHelpers.h"
#include "noisy-irHelpers.h"
#include "noisy-parser.h"
#include "newton-parser-expression.h"
#include "newton-parser.h"
#include "noisy-lexers-helpers.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "noisy-firstAndFollow.h"


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
 * numeric values known. To distinguish the two cases, we can either pass in a flag to quantity parsing methods
 * or just use ParseNumericExpression. 
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

    if (noisyInFirst(N, kNewtonIrNodeType_PquantityTerm, gNewtonFirsts))
    {
        leftTerm = newtonParseNumericTerm(N, currentScope);

        while (noisyInFirst(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp, gNewtonFirsts))
        {
            NoisyIrNode * binOp = newtonParseLowPrecedenceBinaryOp(N, currentScope);
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
                        noisyLexPeek(N, 1)->sourceInfo /* source info */);
    intermediate->value = 1;
    if (noisyInFirst(N, kNewtonIrNodeType_PunaryOp, gNewtonFirsts))
    {
        addLeaf(N, intermediate, newtonParseUnaryOp(N, currentScope));
        intermediate->value *= -1;
    }
    
    NoisyIrNode * leftFactor = newtonParseNumericFactor(N, currentScope);
    intermediate->value *= leftFactor->value;

    addLeafWithChainingSeq(N, intermediate, leftFactor);
    
    while (noisyInFirst(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp, gNewtonFirsts))
    {
        NoisyIrNode * binOp = newtonParseMidPrecedenceBinaryOp(N, currentScope);
        addLeafWithChainingSeq(N, intermediate, binOp);
        
        NoisyIrNode * rightFactor = newtonParseNumericFactor(N, currentScope);
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

NoisyIrNode *
newtonParseNumericFactor(NoisyState * N, NoisyScope * currentScope)
{
    NoisyIrNode *   node;

    if (peekCheck(N, 1, kNewtonIrNodeType_Tidentifier))
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

    if (noisyInFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, gNewtonFirsts))
    {
        addLeaf(N, node, newtonParseHighPrecedenceBinaryOp(N, currentScope));
        
        /* exponents are automatically just one integer unless wrapped in parens */
        NoisyIrNode * exponentExpression = peekCheck(N, 1, kNewtonIrNodeType_TleftParen) ? 
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

    if (noisyInFirst(N, kNewtonIrNodeType_PquantityTerm, gNewtonFirsts))
    {
        leftTerm = newtonParseQuantityTerm(N, currentScope);

        while (noisyInFirst(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp, gNewtonFirsts))
        {
            addLeaf(N, leftTerm, newtonParseLowPrecedenceBinaryOp(N, currentScope));
            
            rightTerm = newtonParseQuantityTerm(N, currentScope);
            addLeafWithChainingSeq(N, leftTerm, rightTerm);
            
            // compare LHS and RHS prime numbers and make sure they're equal
            assert(leftTerm->physics->numeratorPrimeProduct == rightTerm->physics->numeratorPrimeProduct);
            assert(leftTerm->physics->denominatorPrimeProduct == rightTerm->physics->denominatorPrimeProduct);
        }
    }
    else
    {
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
                        noisyLexPeek(N, 1)->sourceInfo /* source info */);

    intermediate->physics = (Physics *) calloc(1, sizeof(Physics));
    intermediate->physics->numeratorPrimeProduct = 1;
    intermediate->physics->denominatorPrimeProduct = 1;


    if (noisyInFirst(N, kNewtonIrNodeType_PunaryOp, gNewtonFirsts))
    {
        addLeaf(N, intermediate, newtonParseUnaryOp(N, currentScope));
    }

    bool isPhysics /*not a number*/ = peekCheck(N, 1, kNewtonIrNodeType_Tidentifier);
    NoisyIrNode * leftFactor = newtonParseQuantityFactor(N, currentScope);
    addLeafWithChainingSeq(N, intermediate, leftFactor);
    
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

    while (noisyInFirst(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp, gNewtonFirsts))
    {
        NoisyIrNode * binOp = newtonParseMidPrecedenceBinaryOp(N, currentScope);
        addLeafWithChainingSeq(N, intermediate, binOp);
        
        bool isPhysics = peekCheck(N, 1, kNewtonIrNodeType_Tidentifier);
        rightFactor = newtonParseQuantityFactor(N, currentScope);
        addLeafWithChainingSeq(N, intermediate, rightFactor);
        
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
            if (rightFactor->physics->numeratorDimensions)
                newtonPhysicsCopyNumeratorToDenominatorDimensions(N, intermediate->physics, rightFactor->physics);
            if (rightFactor->physics->denominatorDimensions)
                newtonPhysicsCopyDenominatorToNumeratorDimensions(N, intermediate->physics, rightFactor->physics);
        }
    }

    return intermediate;
}

NoisyIrNode *
newtonParseQuantityFactor(NoisyState * N, NoisyScope * currentScope)
{
    NoisyIrNode *   intermediate;

    // intermediate->physics = (Physics *) calloc(1, sizeof(Physics));
    // intermediate->physics->numeratorPrimeProduct = 1;
    // intermediate->physics->denominatorPrimeProduct = 1;

    if (peekCheck(N, 1, kNewtonIrNodeType_Tidentifier))
    {
        intermediate = newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
        intermediate->physics = deepCopyPhysicsNode(intermediate->physics);
    }
    else if (peekCheck(N, 1, kNewtonIrNodeType_Tnumber))
    {
        intermediate = newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope);
    }
    // else if (noisyInFirst(N, kNewtonIrNodeType_PtimeOp, gNewtonFirsts))
    // {
    //     intermediate = newtonParseTimeOp(N, currentScope);
    // }
    // else if (noisyInFirst(N, kNewtonIrNodeType_PvectorOp, gNewtonFirsts) && peekCheck(N, 2, kNewtonIrNodeType_TleftParen) && peekCheck(N, 4, kNewtonIrNodeType_Tcomma))
    // {
	// 	intermediate = newtonParseVectorOp(N, currentScope);
    // }
    else if (peekCheck(N, 1, kNewtonIrNodeType_TleftParen))
    {
        newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
        intermediate = newtonParseQuantityExpression(N, currentScope);
        newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);
    }
    else
    {
        noisyFatal(N, "newtonParseQuantityFactor: missed a case in factor\n");
    }

    /*
     * e.g.) (acceleration * mass) ** (3 + 5)
     */
    if (noisyInFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, gNewtonFirsts))
    {
        addLeaf(N, intermediate, newtonParseHighPrecedenceBinaryOp(N, currentScope));
        addLeafWithChainingSeq(N, intermediate, newtonParseExponentialExpression(N, currentScope, intermediate));
    }

    return intermediate;
}

NoisyIrNode *
newtonParseExponentialExpression(NoisyState * N, NoisyScope * currentScope, NoisyIrNode * baseNode)
{
    /* exponents are automatically just one integer unless wrapped in parens */
    NoisyIrNode * exponent = peekCheck(N, 1, kNewtonIrNodeType_TleftParen) ? 
        newtonParseNumericExpression(N, currentScope) : 
        newtonParseInteger(N, currentScope);
    Physics * newExponentBase = shallowCopyPhysicsNode(baseNode->physics);

    if (exponent->value == 0)
    {
        /* any dimension raised to zero power has dimensions removed */
        newExponentBase->value = 1;
        baseNode->physics = newExponentBase;

        return exponent;
    }

    /*
     * This copying is necessary because we don't want to append the same node multiple times
     * to the numerator or denominator linked list
     */
    Physics* copy = deepCopyPhysicsNode(baseNode->physics);

    if (baseNode->physics->numberOfNumerators > 0)
    {
        /* If the base is a Physics quantity, the exponent must be an integer */
        assert(exponent->value == (int) exponent->value); 
        
        /* e.g.) mass ** -2 : mass is copied to denominator, not numerator */
        if (exponent->value < 0)
        {
            for (int power = 0; power > exponent->value; power--)
            {
                newtonPhysicsCopyNumeratorToDenominatorDimensions(N, newExponentBase, copy);
                copy = deepCopyPhysicsNode(copy);
            }
        }
        else
        {
            for (int power = 0; power < exponent->value; power++)
            {
                newtonPhysicsCopyNumeratorDimensions(N, newExponentBase, copy);
                copy = deepCopyPhysicsNode(copy);
            }
        }
    }
    
    if (baseNode->physics->numberOfDenominators > 0)
    {
        assert(exponent->value == (int) exponent->value); 
        
        if (exponent->value < 0)
        {
            for (int power = 0; power > exponent->value; power--)
            {
                newtonPhysicsCopyDenominatorToNumeratorDimensions(N, newExponentBase, copy);
                copy = deepCopyPhysicsNode(copy);
            }
        }
        else
        {
            for (int power = 0; power < exponent->value; power++)
            {
                newtonPhysicsCopyDenominatorDimensions(N, newExponentBase, copy);
                copy = deepCopyPhysicsNode(copy);
            }
        }
    }

    baseNode->physics = newExponentBase;

    return exponent;
}

NoisyIrNode *
newtonParseVectorOp(NoisyState *  N, NoisyScope * currentScope)
{
    NoisyIrNode *   intermediate = genNoisyIrNode(N,   kNewtonIrNodeType_PvectorOp,
                        NULL /* left child */,
                        NULL /* right child */,
                        noisyLexPeek(N, 1)->sourceInfo /* source info */);

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
    addLeafWithChainingSeq(N, intermediate, left);

    newtonPhysicsCopyNumeratorDimensions(N, intermediate->physics, left->physics);
    newtonPhysicsCopyDenominatorDimensions(N, intermediate->physics, left->physics);
    
    newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
    
    NoisyIrNode * right;
    right = newtonParseQuantityExpression(N, currentScope);
    addLeafWithChainingSeq(N, intermediate, right);

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

NoisyIrNode *
newtonParseUnaryOp(NoisyState *  N, NoisyScope * currentScope)
{
    NoisyIrNode *   n = NULL;

    if (peekCheck(N, 1, kNewtonIrNodeType_Tminus))
    {
        n = newtonParseTerminal(N, kNewtonIrNodeType_Tminus, currentScope);
    }
    else
    {
        noisyFatal(N, "newton-parser-expression.c: newtonParseUnaryOp: did not detect minus as unary op\n");
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
		noisyLexPeek(N, 1)->sourceInfo /* source info */
    );
    
    NoisyIrNodeType type;
    if ((type = noisyLexPeek(N, 1)->type) == kNewtonIrNodeType_Tintegral || 
         type == kNewtonIrNodeType_Tderivative)
    {
		addLeaf(N, node, newtonParseTerminal(N, type, currentScope));
    }
    else
    {
        noisyFatal(N, "newton-parser-expression.c:newtonParseTimeOp: did not detect derivative or integral\n");
    }

    while ((type = noisyLexPeek(N, 1)->type) == kNewtonIrNodeType_Tintegral || 
            type == kNewtonIrNodeType_Tderivative)
    {
		addLeafWithChainingSeq(N, node, newtonParseTerminal(N, type, currentScope));
        addLeafWithChainingSeq(N, node, newtonParseQuantityExpression(N, currentScope));
    }


    return node;
}

NoisyIrNode *
newtonParseCompareOp(NoisyState * N, NoisyScope * currentScope)
{
    
    NoisyIrNodeType type;
    if ((type = noisyLexPeek(N, 1)->type) == kNewtonIrNodeType_Tlt || 
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
		noisyLexPeek(N, 1)->sourceInfo /* source info */
    );
    
    if (peekCheck(N, 1, kNewtonIrNodeType_Texponent))
    {
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Texponent, currentScope));
    }
    else
    {
        noisyFatal(N, "newton-parser-expression.c:newtonParseHighPrecedenceBinaryOp: no exponent token\n");
    }
    return node;
}

NoisyIrNode *
newtonParseMidPrecedenceBinaryOp(NoisyState *  N, NoisyScope * currentScope)
{
    NoisyIrNode *   n;

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
						noisyLexPeek(N, 1)->sourceInfo /* source info */);
    
    if (noisyInFirst(N, kNewtonIrNodeType_PunaryOp, gNewtonFirsts))
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
