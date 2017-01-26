#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
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

NoisyIrNode *
newtonParseUnitExpression(NoisyState * N, NoisyScope * currentScope)
{
    return newtonParseUnitTerm(N, currentScope);
}

NoisyIrNode *
newtonParseUnitTerm(NoisyState * N, NoisyScope * currentScope)
{
    NoisyIrNode *   intermediate = genNoisyIrNode(N,  kNewtonIrNodeType_PunitTerm,
                        NULL /* left child */,
                        NULL /* right child */,
                        newtonLexPeek(N, 1)->sourceInfo /* source info */);


    if (newtonInFirst(N, kNewtonIrNodeType_PunaryOp))
    {
        addLeaf(N, intermediate, newtonParseUnaryOp(N, currentScope));
    }

    bool hasDimensions = peekCheckNewton(N, 1, kNewtonIrNodeType_Tidentifier);
    NoisyIrNode * leftFactor = newtonParseUnitFactor(N, currentScope);
    addLeaf(N, intermediate, leftFactor);

    if (hasDimensions)
    {
        intermediate->physics = (Physics *) calloc(1, sizeof(Physics));
        intermediate->physics->numeratorPrimeProduct = 1;
        intermediate->physics->denominatorPrimeProduct = 1;

        newtonPhysicsCopyNumeratorDimensions(N, intermediate->physics, leftFactor->physics);
        newtonPhysicsCopyDenominatorDimensions(N, intermediate->physics, leftFactor->physics);
        
        if (leftFactor->physics->isVector)
        {
            noisyFatal(N, "newton-parser-expression.c: newtonParseUnitTerm leftFactor shouldn't be a vector");
        }
    }
    
    NoisyIrNode * rightFactor;

    while (newtonInFirst(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp))
    {
        NoisyIrNode * binOp = newtonParseMidPrecedenceBinaryOp(N, currentScope);
        addLeafWithChainingSeqNewton(N, intermediate, binOp);
        
        hasDimensions = peekCheckNewton(N, 1, kNewtonIrNodeType_Tidentifier);
        rightFactor = newtonParseUnitFactor(N, currentScope);
        addLeafWithChainingSeqNewton(N, intermediate, rightFactor);
        
        if (rightFactor->physics->isVector)
        {
            noisyFatal(N, "newton-parser-expression.c: newtonParseUnitTerm rightFactor shouldn't be a vector");
        }

        if (hasDimensions)
        {
            if (binOp->type == kNewtonIrNodeType_Tmul) 
            {
                newtonPhysicsCopyNumeratorDimensions(N, intermediate->physics, rightFactor->physics);
                newtonPhysicsCopyDenominatorDimensions(N, intermediate->physics, rightFactor->physics);
            }
            else if (binOp->type == kNewtonIrNodeType_Tdiv)
            {
                newtonPhysicsCopyNumeratorToDenominatorDimensions(N, intermediate->physics, rightFactor->physics);
                newtonPhysicsCopyDenominatorToNumeratorDimensions(N, intermediate->physics, rightFactor->physics);
            }
        }
    }

    return intermediate;
}

NoisyIrNode *
newtonParseUnitFactor(NoisyState * N, NoisyScope * currentScope)
{
    NoisyIrNode *   n;

    if (peekCheckNewton(N, 1, kNewtonIrNodeType_Tidentifier))
    {
        n = newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
        if (newtonInFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp))
        {
            addLeaf(N, n, newtonParseHighPrecedenceBinaryOp(N, currentScope));

            // TODO https://github.com/phillipstanleymarbell/Noisy-lang-compiler/issues/74
            // this should technically be something other than newtonParseUnitExpression
            // Make a new expression for only numbers
            // because we shouldn't allow meter ^ (something other than expression of numbers)
            addLeaf(N, n, newtonParseInteger(N, currentScope));
        }
    }
    else if (peekCheck(N, 1, kNewtonIrNodeType_Tnumber))
    {
        n = newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope);
    }
    else if (peekCheck(N, 1, kNewtonIrNodeType_TleftParen))
    {
        newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
        n = newtonParseUnitExpression(N, currentScope);
        newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);
    }
    else
    {
        noisyFatal(N, "newtonParseQuantityFactor: missed a case in factor\n");
    }

    return n;
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

    NoisyIrNode * leftFactor = newtonParseQuantityFactor(N, currentScope);
    addLeaf(N, intermediate, leftFactor);

    newtonPhysicsCopyNumeratorDimensions(N, intermediate->physics, leftFactor->physics);
    newtonPhysicsCopyDenominatorDimensions(N, intermediate->physics, leftFactor->physics);
    
    int numVectorsInTerm = 0;
    /*
     * If either LHS or RHS is a vector (not both), then the resultant is a vector
     */
    if (leftFactor->physics->isVector)
    {
        intermediate->physics->isVector = true;
        numVectorsInTerm++;
    }
    
    NoisyIrNode * rightFactor;

    while (newtonInFirst(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp))
    {
        NoisyIrNode * binOp = newtonParseMidPrecedenceBinaryOp(N, currentScope);
        addLeafWithChainingSeq(N, intermediate, binOp);
        
        rightFactor = newtonParseQuantityFactor(N, currentScope);
        addLeafWithChainingSeq(N, intermediate, rightFactor);
        
        if (rightFactor->physics->isVector)
        {
            intermediate->physics->isVector = true;
            numVectorsInTerm++;
        }

        /*
         * Cannot perform multiply or divide operations on two vectors
         * e.g.) vector * scalar * scalar / vector is illegal because
         * it boils down to vector / vector which is illegal
         */
        assert(numVectorsInTerm < 2);

        if (binOp->type == kNewtonIrNodeType_Tmul) 
        {
            newtonPhysicsCopyNumeratorDimensions(N, intermediate->physics, rightFactor->physics);
            newtonPhysicsCopyDenominatorDimensions(N, intermediate->physics, rightFactor->physics);
        }
        else if (binOp->type == kNewtonIrNodeType_Tdiv)
        {
            newtonPhysicsCopyNumeratorToDenominatorDimensions(N, intermediate->physics, rightFactor->physics);
            newtonPhysicsCopyDenominatorToNumeratorDimensions(N, intermediate->physics, rightFactor->physics);
        }
    }

    return intermediate;
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
    addLeafWithChainingSeq(N, intermediate, left);
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
newtonParseQuantityFactor(NoisyState * N, NoisyScope * currentScope)
{
    NoisyIrNode *   node;

    if (peekCheckNewton(N, 1, kNewtonIrNodeType_Tidentifier))
    {
        node = newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
    }
    else if (peekCheck(N, 1, kNewtonIrNodeType_Tnumber))
    {
        node = newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope);
    }
    else if (newtonInFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp))
    {
        node = newtonParseHighPrecedenceBinaryOp(N, currentScope);
        addLeafWithChainingSeqNewton(N, node, newtonParseQuantityExpression(N, currentScope));
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
    }
    else
    {
        noisyFatal(N, "newtonParseQuantityFactor: missed a case in factor\n");
    }

    return node;
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
newtonParseInteger(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(N,	kNewtonIrNodeType_Pinteger,
						NULL /* left child */,
						NULL /* right child */,
						newtonLexPeek(N, 1)->sourceInfo /* source info */);
    
    if (newtonInFirst(N, kNewtonIrNodeType_PunaryOp))
    {
        addLeaf(N, node, newtonParseUnaryOp(N, currentScope));
    }

    addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope));

    return node;
}
