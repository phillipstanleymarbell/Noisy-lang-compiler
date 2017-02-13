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
#include "common-timeStamps.h"
#include "common-errors.h"
#include "data-structures.h"
#include "common-irHelpers.h"
#include "newton-parser.h"
#include "newton-parser-expression.h"
#include "common-lexers-helpers.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "common-firstAndFollow.h"


extern unsigned long int bigNumberOffset;
extern int primeNumbers[168];
extern const char * gNewtonTokenDescriptions[kNoisyIrNodeTypeMax];
extern char *		gNewtonAstNodeStrings[];
extern int		gNewtonFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];

extern void		noisyFatal(NoisyState *  N, const char *  msg);
extern void		noisyError(NoisyState *  N, const char *  msg);



NoisyIrNode *
newtonParse(NoisyState *  N, NoisyScope *  currentScope)
{
	return newtonParseFile(N, currentScope);
}


/*
 *	kNoisyIrNodeType_PruleList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PdimensionTypeNameScope
 *		node.right	= Xseq of more scopes
 */
NoisyIrNode *
newtonParseFile(NoisyState *  N, NoisyScope *  currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(
        N,
        kNewtonIrNodeType_PnewtonFile,
		NULL /* left child */,
		NULL /* right child */,
		noisyLexPeek(N, 1)->sourceInfo /* source info */
    );

	addLeaf(N, node, newtonParseRuleList(N, currentScope));

    assert(noisyLexPeek(N, 1)->type == kNewtonIrNodeType_Zeof);
	N->tokenList = N->tokenList->next; /* skip eof token without using noisyLexGet*/

	return node;
}

NoisyIrNode *
newtonParseRuleList(NoisyState *  N, NoisyScope *  currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(
        N,
        kNewtonIrNodeType_PruleList,
		NULL /* left child */,
		NULL /* right child */,
		noisyLexPeek(N, 1)->sourceInfo /* source info */
    );
    
    if (noisyInFirst(N, kNewtonIrNodeType_Prule, gNewtonFirsts))
	{
		addLeaf(N, node, newtonParseRule(N, currentScope));
	}

    while (noisyInFirst(N, kNewtonIrNodeType_Prule, gNewtonFirsts))
	{
		addLeafWithChainingSeq(N, node, newtonParseRule(N, currentScope));
	}

    return node;
}

NoisyIrNode *
newtonParseRule(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(
        N,
        kNewtonIrNodeType_Prule,
		NULL /* left child */,
		NULL /* right child */,
		noisyLexPeek(N, 1)->sourceInfo /* source info */
    );
	
    currentScope->begin = noisyLexPeek(N, 1)->sourceInfo;
    
    switch(noisyLexPeek(N, 3)->type)
    {
        case kNewtonIrNodeType_Tsignal:
            newtonParseBaseSignal(N, currentScope);
            break;
        case kNewtonIrNodeType_Tconstant:
            newtonParseConstant(N, currentScope);
            break;
        case kNewtonIrNodeType_Tinvariant:
            newtonParseInvariant(N, currentScope);
            break;
        default:
            noisyFatal(N, "newton-parser.c:newtonParseRule neither signal, constant, nor invariant\n");
    }

    currentScope->end = noisyLexPeek(N, 1)->sourceInfo;

    return node;
}

NoisyIrNode *
newtonParseInvariant(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(N,	kNewtonIrNodeType_Pinvariant,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


    Invariant * invariant = (Invariant *) calloc(1, sizeof(Invariant));
    
    NoisyIrNode * invariantName = newtonParseIdentifier(N, currentScope);
    addLeaf(N, node, invariantName);
    invariant->identifier = invariantName->tokenString;
    
    newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tinvariant, currentScope);

    invariant->parameterList = newtonParseParameterTuple(N, currentScope);

    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
    NoisyIrNode * scopeBegin = newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, currentScope);
	NoisyScope * newScope	= newtonSymbolTableOpenScope(N, currentScope, scopeBegin);
    newScope->invariantParameterList = invariant->parameterList;

	addLeafWithChainingSeq(N, node, newtonParseConstraint(N, newScope));
    while (peekCheck(N, 1, kNewtonIrNodeType_Tcomma))
	{
        newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, newScope);
		addLeafWithChainingSeq(N, node, newtonParseConstraint(N, newScope));
	}
    
    invariant->constraints = node->irRightChild;
    invariant->id = newtonGetInvariantIdByParameters(N, invariant->parameterList, 1);

	NoisyIrNode *	scopeEnd = newtonParseTerminal(N, kNewtonIrNodeType_TrightBrace, newScope);
	newtonSymbolTableCloseScope(N, newScope, scopeEnd);

    newtonAddInvariant(N, invariant);
    
    return node;
}

NoisyIrNode *
newtonParseConstraint(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(N,	kNewtonIrNodeType_Pconstraint,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);
    
    if (noisyInFirst(N, kNewtonIrNodeType_PquantityExpression, gNewtonFirsts))
    {
        addLeaf(N, node, newtonParseQuantityExpression(N, currentScope));
        addLeafWithChainingSeq(N, node, newtonParseCompareOp(N, currentScope));
        addLeafWithChainingSeq(N, node, newtonParseQuantityExpression(N, currentScope));
        
        // TODO do some epic type checking here
    }
    else
    {
        noisyFatal(N, "newton-parser.c:newtonParseConstraint not a first of quantityExpression\n");
    }

    return node;
}

NoisyIrNode *
newtonParseParameterTuple(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(N,	kNewtonIrNodeType_PparameterTuple,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);
    
	newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
    addLeaf(N, node, newtonParseParameter(N, currentScope));
    while (peekCheck(N, 1, kNewtonIrNodeType_Tcomma))
	{
        newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
		addLeafWithChainingSeq(N, node, newtonParseParameter(N, currentScope));
	}
	newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);

    return node;
}

NoisyIrNode *
newtonParseParameter(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(N,	kNewtonIrNodeType_Pparameter,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);

    /*
     * By convention the left child is the name, and the right child is the name of the Physics
     */
    addLeaf(N, node, newtonParseIdentifier(N, currentScope));
    newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
    addLeaf(N, node, newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));

    return node;
}


NoisyIrNode *
newtonParseConstant(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(N,	kNewtonIrNodeType_Pconstant,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);
    
    NoisyIrNode * constantIdentifier = newtonParseIdentifier(N, currentScope);

    newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tconstant, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);

    addLeaf(N, node, constantIdentifier);
    Physics * constantPhysics = newtonPhysicsTableAddPhysicsForToken(N, currentScope, constantIdentifier->token);

    if (noisyInFirst(N, kNewtonIrNodeType_PquantityExpression, gNewtonFirsts))
    {
        NoisyIrNode * constantExpression = newtonParseQuantityExpression(N, currentScope);
        newtonPhysicsCopyNumeratorDimensions(N, constantPhysics, constantExpression->physics);
        newtonPhysicsCopyDenominatorDimensions(N, constantPhysics, constantExpression->physics);

        /*
         * If LHS is declared a vector in vectorScalarPairScope, then 
         * the expression must evaluate to a vector.
         */
        if (constantPhysics->isVector)
        {
            assert(constantExpression->physics->isVector);
        }
    }
    else
    {
        noisyFatal(N, "newton-parser.c: newtonParseConstant after equal sign, there is no quantity expression");
    }
    
    newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

    return node;
}

NoisyIrNode *
newtonParseBaseSignal(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(N,	kNewtonIrNodeType_PbaseSignal,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);

    NoisyIrNode * basicPhysicsIdentifier = newtonParseIdentifier(N, currentScope);
    addLeaf(N, node, basicPhysicsIdentifier);
    Physics * newPhysics = newtonPhysicsTableAddPhysicsForToken(N, currentScope, basicPhysicsIdentifier->token);
    
    newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tsignal, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, currentScope);
	
    NoisyIrNode * unitName = newtonParseName(N, currentScope);
    addLeafWithChainingSeq(N, node, unitName);
    NoisyIrNode * unitAbbreviation = newtonParseSymbol(N, currentScope);
    addLeafWithChainingSeq(N, node, unitAbbreviation);
    
    NoisyIrNode * derivationExpression = newtonParseDerivation(N, currentScope)->irLeftChild;
    addLeafWithChainingSeq(N, node, derivationExpression);
    
    /*
     * These are the derived signals
     */
    if (derivationExpression->type != kNewtonIrNodeType_Tnone)
    {
        newtonPhysicsCopyNumeratorDimensions(N, newPhysics, derivationExpression->physics);
        newtonPhysicsCopyDenominatorDimensions(N, newPhysics, derivationExpression->physics);
    }
    /*
     * These are the base signals
     */
    else
    {
        newtonPhysicsAddNumeratorDimension(
            N, 
            newPhysics, 
            newtonDimensionTableAddDimensionForToken(
                N, 
                currentScope, 
                unitName->token, 
                unitAbbreviation->token
            )
        );
    }
    
    newPhysics->dimensionAlias = unitName->token->stringConst; /* e.g.) meter, Pascal*/
    newPhysics->dimensionAliasAbbreviation = unitAbbreviation->token->stringConst; /* e.g.) m, Pa*/

    newPhysics->id = newtonGetPhysicsId(N, newPhysics);
    assert(newPhysics->id > 1);
	
    newtonParseTerminal(N, kNewtonIrNodeType_TrightBrace, currentScope);

    return node;
}

NoisyIrNode *
newtonParseName(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(N,	kNewtonIrNodeType_Pname,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);

    addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tname, currentScope));
    addLeafWithChainingSeq(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope));
    
    NoisyIrNode * baseSignalName = newtonParseTerminal(N, kNewtonIrNodeType_TstringConst, currentScope);
    node->token = baseSignalName->token;
    
    addLeafWithChainingSeq(N, node, baseSignalName);
    
    if (noisyLexPeek(N, 1)->type == kNewtonIrNodeType_TEnglish || 
        noisyLexPeek(N, 1)->type == kNewtonIrNodeType_TSpanish)
	{
		addLeafWithChainingSeq(N, node, newtonParseTerminal(N, noisyLexPeek(N, 1)->type, currentScope));
	}
    else
    {
        noisyFatal(N, "newton-parser.c:newtonParseName no language setting\n");
    }

    return node;
}

NoisyIrNode *
newtonParseSymbol(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(N,	kNewtonIrNodeType_Psymbol,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);
    
    newtonParseTerminal(N, kNewtonIrNodeType_Tsymbol, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
    
    NoisyIrNode * baseSignalAbbreviation = newtonParseTerminal(N, kNewtonIrNodeType_TstringConst, currentScope);
    node->token = baseSignalAbbreviation->token;
    
    addLeaf(N, node, baseSignalAbbreviation);
    newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

    return node;
}

NoisyIrNode *
newtonParseDerivation(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(N,	kNewtonIrNodeType_Pderivation,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);
    
    newtonParseTerminal(N, kNewtonIrNodeType_Tderivation, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
    
    if (noisyLexPeek(N, 1)->type == kNewtonIrNodeType_Tnone)
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tnone, currentScope));
	}
    else
    {
		addLeaf(N, node, newtonParseQuantityExpression(N, currentScope));
    }
    
    newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

    return node;
}



/*
 *  Simply remove a terminal
 */
NoisyIrNode *
newtonParseTerminal(NoisyState *  N, NoisyIrNodeType expectedType, NoisyScope * currentScope)
{
    if (!peekCheck(N, 1, expectedType))
    {
        noisyFatal(N, "newton-parser.c: newtonParseTerminal");
    }

    NoisyToken *  t = noisyLexGet(N, gNewtonTokenDescriptions);
    NoisyIrNode * n = genNoisyIrNode(N, t->type,
                        NULL /* left child */,
                        NULL /* right child */,
                        t->sourceInfo /* source info */);
    
    n->token = t;
    n->tokenString = t->identifier;

    if (t->type == kNewtonIrNodeType_Tnumber)
    {
        n->value = 0.0;
        if (t->integerConst != 0)
            n->value = t->integerConst;
        else if (t->realConst != 0)
            n->value = t->realConst;
    }
   
    return n;
}


/*
 *	kNewtonIrNodeType_PidentifierOrNil
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNewtonIrNodeType_Tidentifier
 *		node.right	= Xseq of kNewtonIrNodeType_PfieldSelect
 */
NoisyIrNode *
newtonParseIdentifier(NoisyState *  N, NoisyScope *  currentScope)
{
	NoisyIrNode *	n;

	if (peekCheck(N, 1, kNewtonIrNodeType_Tidentifier))
	{
		n = newtonParseIdentifierDefinitionTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
	    return n;
	}
	else
	{
        noisyFatal(N, "newton-parser.c:newtonParseIdentifier: not an identifier token");
	}
    return NULL;
}

char *
newtonParseGetPhysicsTypeStringByBoundIdentifier(NoisyState * N, NoisyIrNode * root, char* boundVariableIdentifier)
{
    // do DFS and find the node whose left child node has given identifier
    // and return the right node's identifier
    if (root->type == kNewtonIrNodeType_Pparameter)
    {
        assert(root->irLeftChild != NULL && root->irRightChild != NULL);
		if (!strcmp(root->irLeftChild->tokenString, boundVariableIdentifier))
        {
            return root->irRightChild->tokenString;
        }
    }

    char * stringResult = "";
    
    if (root->irLeftChild != NULL)
        stringResult = newtonParseGetPhysicsTypeStringByBoundIdentifier(N, root->irLeftChild, boundVariableIdentifier);

    if (strcmp(stringResult, ""))
        return stringResult;
    
    if (root->irRightChild != NULL)
        stringResult = newtonParseGetPhysicsTypeStringByBoundIdentifier(N, root->irRightChild, boundVariableIdentifier);

    if (strcmp(stringResult, ""))
        return stringResult;

    return "";
}

/*
 * The caller of this function passes in 1 for invariantId
 * TODO: move this method, newtonParseGetPhysicsTypeStringByBoundIdentifier, and newtonIsConstant
 * to a helper file, don't put them here
 */
unsigned long long int
newtonGetInvariantIdByParameters(NoisyState * N, NoisyIrNode * parameterTreeRoot, unsigned long long int invariantId)
{
    if (parameterTreeRoot->type == kNewtonIrNodeType_Pparameter)
    {
        assert(parameterTreeRoot->irLeftChild != NULL && parameterTreeRoot->irRightChild != NULL);
        assert(parameterTreeRoot->irRightChild->physics->id > 1);
        
        return invariantId * parameterTreeRoot->irRightChild->physics->id;
    }

    if (parameterTreeRoot->irLeftChild != NULL)
        invariantId *= newtonGetInvariantIdByParameters(N, parameterTreeRoot->irLeftChild, 1);

    if (parameterTreeRoot->irRightChild != NULL)
        invariantId *= newtonGetInvariantIdByParameters(N, parameterTreeRoot->irRightChild, 1);

    return invariantId;
}


/*
 *  Remove an identifier _usage_ terminal, performing symtab lookup
 */
NoisyIrNode *
newtonParseIdentifierUsageTerminal(NoisyState *  N, NoisyIrNodeType expectedType, NoisyScope *  scope)
{
    if (!peekCheck(N, 1, expectedType))
    {
        noisyFatal(N, "newton-parser.c:newtonParseIdentifierUsageTerminal: not an expected type\n");
        return NULL;
    }

    NoisyToken *    t = noisyLexGet(N, gNewtonTokenDescriptions);
    NoisyIrNode *   n = genNoisyIrNode(N,   t->type,
                        NULL /* left child */,
                        NULL /* right child */,
                        t->sourceInfo /* source info */);

    n->token = t;
    n->tokenString = t->identifier;
    
    // TODO rewrite this logic in a cleaner way.... make a new method or something
    Physics * physicsSearchResult;
    if ((physicsSearchResult = newtonPhysicsTablePhysicsForIdentifier(N, scope, t->identifier)) == NULL)
    {
        physicsSearchResult = newtonPhysicsTablePhysicsForDimensionAlias(N, scope, t->identifier);
    } 

    if (physicsSearchResult == NULL)
    {
        char * physicsTypeString = newtonParseGetPhysicsTypeStringByBoundIdentifier(N, scope->invariantParameterList, t->identifier);
        if ((physicsSearchResult = newtonPhysicsTablePhysicsForIdentifier(N, scope, physicsTypeString)) == NULL)
        {
            physicsSearchResult = newtonPhysicsTablePhysicsForDimensionAlias(N, scope, physicsTypeString);
        }
    }
    
    if (physicsSearchResult == NULL)
    {
        errorUseBeforeDefinition(N, t->identifier);
    }
    else 
    {
        // n->physics = physicsSearchResult;
        
        /* defensive copying to keep the Physics list in NoisyState immutable */
        n->physics = deepCopyPhysicsNode(physicsSearchResult);
    }

    return n;
}

/*
 *	Remove an identifier _definition_ terminal, performing symtab insertion
 */
NoisyIrNode *
newtonParseIdentifierDefinitionTerminal(NoisyState *  N, NoisyIrNodeType  expectedType, NoisyScope *  scope)
{
	if (!peekCheck(N, 1, expectedType))
	{
        noisyFatal(N, "newton-parser.c:newtonParseIdentifierDefinitionTerminal: not an expected type\n");
	}

	NoisyToken *	t = noisyLexGet(N, gNewtonTokenDescriptions);
	NoisyIrNode *	n = genNoisyIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

    n->token = t;
	n->tokenString = t->identifier;

	return n;
}

bool
newtonIsConstant(Physics * physics)
{
    /* sanity check */
    if (physics->numeratorDimensions == NULL)
        assert(physics->numberOfNumerators == 0 && physics->numeratorPrimeProduct == 1);
    if (physics->denominatorDimensions == NULL)
        assert(physics->numberOfDenominators == 0 && physics->denominatorPrimeProduct == 1);
    
    return physics->numeratorDimensions == NULL && physics->denominatorDimensions;
}

int 
newtonGetPhysicsId(NoisyState * N, Physics * physics)
{
    /* 
     * Prime number id's are assigned to Dimension struct's, and
     * Physics struct's construct its own id as a multiple of all of 
     * its numerator Dimension's and denominator Dimension's.
     * However, if Physics is a constant and does not have any Dimension's
     * we just assign the next prime number from our prime numbers array.
     */
    if (physics->numeratorDimensions == NULL && physics->denominatorDimensions == NULL)
    {
        return primeNumbers[N->primeNumbersIndex++];
    }

    /* 
     * Basic Physics struct's don't have offsets applied to their id calculation
     * because we want to see the effect of bigNumberOffset only once for
     * derived Physics id calculation. 
     * This is why we set the product to 0 in basic cases.
     */
    int numeratorIdProduct = physics->numeratorPrimeProduct > 1 ? 1 : 0;
    Dimension * current = physics->numeratorDimensions;
    while (current != NULL)
    {
        numeratorIdProduct *= current->primeNumber;
        current = current->next;
    }

    int denominatorIdProduct = physics->denominatorPrimeProduct > 1 ? 1 : 0;
    current = physics->denominatorDimensions;
    while (current != NULL)
    {
        denominatorIdProduct *= current->primeNumber;
        current = current->next;
    }
    
    /* 
     * TODO: This is not a robust design... no guarantee that bigNumberOffset is
     * bigger than numeratorIdProduct in all cases (for most Newton files, yes). 
     * Could replace a single number
     * id with an id tuple scheme, but we could just make bigNumberOffset bigger.
     * Data type of Physics->id is unsigned long long int which can contain up to
     * this number: 18,446,744,073,709,551,615.
     */
    return numeratorIdProduct + bigNumberOffset * denominatorIdProduct;
}

