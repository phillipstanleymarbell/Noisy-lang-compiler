/*
	Authored 2017. Jonathan Lim.

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
	ANY WAY OUT OF THE USE OF THIS SOF    expressionIndex++;
TWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/
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
#include "newton-parser-expression.h"
#include "common-lexers-helpers.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "common-firstAndFollow.h"
#include "newton-parser.h"


extern unsigned long int bigNumberOffset;
extern int primeNumbers[168];
extern const char * gNewtonTokenDescriptions[kNoisyIrNodeTypeMax];
extern char *		gNewtonAstNodeStrings[];
extern int		gNewtonFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];

extern void		fatal(State *  N, const char *  msg);
extern void		noisyError(State *  N, const char *  msg);



IrNode *
newtonParse(State *  N, Scope *  currentScope)
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
IrNode *
newtonParseFile(State *  N, Scope *  currentScope)
{
	IrNode *	node = genIrNode(
        N,
        kNewtonIrNodeType_PnewtonFile,
		NULL /* left child */,
		NULL /* right child */,
		lexPeek(N, 1)->sourceInfo /* source info */
    );

	addLeaf(N, node, newtonParseRuleList(N, currentScope));

    assert(lexPeek(N, 1)->type == kNewtonIrNodeType_Zeof);
	N->tokenList = N->tokenList->next; /* skip eof token without using lexGet*/

	return node;
}

IrNode *
newtonParseRuleList(State *  N, Scope *  currentScope)
{
	IrNode *	node = genIrNode(
        N,
        kNewtonIrNodeType_PruleList,
		NULL /* left child */,
		NULL /* right child */,
		lexPeek(N, 1)->sourceInfo /* source info */
    );
    
    if (inFirst(N, kNewtonIrNodeType_Prule, gNewtonFirsts))
	{
		addLeaf(N, node, newtonParseRule(N, currentScope));
	}

    while (inFirst(N, kNewtonIrNodeType_Prule, gNewtonFirsts))
	{
		addLeafWithChainingSeq(N, node, newtonParseRule(N, currentScope));
	}

    return node;
}

IrNode *
newtonParseRule(State * N, Scope * currentScope)
{
	  IrNode *	node;

    currentScope->begin = lexPeek(N, 1)->sourceInfo;

    switch(lexPeek(N, 3)->type)
    {
        case kNewtonIrNodeType_Tsignal:
            node = newtonParseBaseSignal(N, currentScope);
            break;
        case kNewtonIrNodeType_Tconstant:
            node = newtonParseConstant(N, currentScope);
            break;
        case kNewtonIrNodeType_Tinvariant:
            node = newtonParseInvariant(N, currentScope);
            break;
        default:
            fatal(N, "newton-parser.c:newtonParseRule neither signal, constant, nor invariant\n");
    }

    currentScope->end = lexPeek(N, 1)->sourceInfo;

    return node;
}

IrNode *
newtonParseInvariant(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pinvariant,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


    Invariant * invariant = (Invariant *) calloc(1, sizeof(Invariant));

    IrNode * invariantName = newtonParseIdentifier(N, currentScope);
    addLeaf(N, node, invariantName);
    invariant->identifier = invariantName->tokenString;

    newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tinvariant, currentScope);

    invariant->parameterList = newtonParseParameterTuple(N, currentScope);

    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
    IrNode * scopeBegin = newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, currentScope);
	Scope * newScope	= newtonSymbolTableOpenScope(N, currentScope, scopeBegin);
    newScope->invariantParameterList = invariant->parameterList;

	addLeafWithChainingSeq(N, node, newtonParseConstraint(N, newScope));
    while (peekCheck(N, 1, kNewtonIrNodeType_Tcomma))
	{
        newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, newScope);
		addLeafWithChainingSeq(N, node, newtonParseConstraint(N, newScope));
	}

    invariant->constraints = node->irRightChild;
    invariant->id = newtonGetInvariantIdByParameters(N, invariant->parameterList, 1);

	IrNode *	scopeEnd = newtonParseTerminal(N, kNewtonIrNodeType_TrightBrace, newScope);
	newtonSymbolTableCloseScope(N, newScope, scopeEnd);

    newtonAddInvariant(N, invariant);

    return node;
}

IrNode *
newtonParseSubindex(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Psubindex,
								 NULL /* left child */,
								 NULL /* right child */,
								 lexPeek(N, 1)->sourceInfo /* source info */);

	addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);

	node->subindexStart = newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope)->value;
	newtonParseTerminal(N, kNewtonIrNodeType_Tto, currentScope);
	node->subindexEnd = newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope)->value;

	return node;
}

IrNode *
newtonParseSubindexTuple(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PsubindexTuple,
								 NULL /* left child */,
								 NULL /* right child */,
								 lexPeek(N, 1)->sourceInfo /* source info */);

	newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);

	IrNode * subindexNode = newtonParseSubindex(N, currentScope);
	addLeaf(N, node, subindexNode);

	node->subindexStart = subindexNode->subindexStart;
	node->subindexEnd = subindexNode->subindexEnd;

	newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);

	return node;
}

IrNode *
newtonParseParameterTuple(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PparameterTuple,
                                      NULL /* left child */,
                                      NULL /* right child */,
                                      lexPeek(N, 1)->sourceInfo /* source info */);

	newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);

  int parameterNumber = 0;
  addLeaf(N, node, newtonParseParameter(N, currentScope, parameterNumber++));

  while (peekCheck(N, 1, kNewtonIrNodeType_Tcomma))
    {
      newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
      addLeafWithChainingSeq(N, node, newtonParseParameter(N, currentScope, parameterNumber++));
    }
	newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);

  return node;
}

IrNode *
newtonParseParameter(State * N, Scope * currentScope, int parameterNumber)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pparameter,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

    /*
     * By convention the left child is the name, and the right child is the name of the Physics
     */
    addLeaf(N, node, newtonParseIdentifier(N, currentScope));
    newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
    IrNode * physicsName = newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
    addLeaf(N, node, physicsName);

	if (lexPeek(N, 1)->type == kNewtonIrNodeType_TatSign)
	{
		newtonParseTerminal(N, kNewtonIrNodeType_TatSign, currentScope);

		newtonParseResetPhysicsWithCorrectSubindex(
			N,
			physicsName,
			currentScope,
			physicsName->token->identifier,
			newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope)->value
			);
	}

    node->parameterNumber = parameterNumber;
    node->physics = physicsName->physics;

    return node;
}


IrNode *
newtonParseConstant(State * N, Scope * currentScope)
{
	  IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pconstant,
				NULL /* left child */,
				NULL /* right child */,
				lexPeek(N, 1)->sourceInfo /* source info */);

    IrNode * constantIdentifier = newtonParseIdentifier(N, currentScope);

    newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tconstant, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);

    addLeaf(N, node, constantIdentifier);
    Physics * constantPhysics = newtonPhysicsTableAddPhysicsForToken(N, currentScope, constantIdentifier->token);

    if (inFirst(N, kNewtonIrNodeType_PquantityExpression, gNewtonFirsts))
    {
        IrNode * constantExpression = newtonParseQuantityExpression(N, currentScope);
        constantPhysics->value = constantExpression->value;
        constantPhysics->isConstant = true;

        node->value = constantExpression->value;
        assert(node->value != 0); // TODO remove later

		newtonPhysicsAddExponents(N, constantPhysics, constantExpression->physics);

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
        fatal(N, "newton-parser.c: newtonParseConstant after equal sign, there is no quantity expression");
    }

    newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

    return node;
}

IrNode *
newtonParseBaseSignal(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PbaseSignal,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

    IrNode * basicPhysicsIdentifier = newtonParseIdentifier(N, currentScope);
    addLeaf(N, node, basicPhysicsIdentifier);
	Physics * newPhysics = newtonPhysicsTableAddPhysicsForToken(N, currentScope, basicPhysicsIdentifier->token);

    newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tsignal, currentScope);

	/*
	 * TODO do some i: 0 to 2 parsing here
	 */
	int subindexStart = 0;
	int subindexEnd = 0;
	IrNode * subindexNode;
	if (lexPeek(N, 5)->type == kNewtonIrNodeType_Tto)
	{
		subindexNode = newtonParseSubindexTuple(N, currentScope);
		subindexStart = subindexNode->subindexStart;
		subindexEnd = subindexNode->subindexEnd;

		assert(subindexEnd > 0);
	}


    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, currentScope);

	/*
	 * name syntax is optional
	 */
	IrNode * unitName = NULL;
	if (inFirst(N, kNewtonIrNodeType_Pname, gNewtonFirsts))
	{
		unitName = newtonParseName(N, currentScope);
		addLeafWithChainingSeq(N, node, unitName);
		newPhysics->dimensionAlias = unitName->token->stringConst; /* e.g.) meter, Pascal*/
	}

	/*
	 * abbreviation syntax is also optional
	 */
	IrNode * unitAbbreviation = NULL;
	if (inFirst(N, kNewtonIrNodeType_Psymbol, gNewtonFirsts))
	{
		unitAbbreviation = newtonParseSymbol(N, currentScope);
		addLeafWithChainingSeq(N, node, unitAbbreviation);
		newPhysics->dimensionAliasAbbreviation = unitAbbreviation->token->stringConst; /* e.g.) m, Pa*/
	}

	/*
	 * derivation syntax is required
	 */
	IrNode * derivationExpression = newtonParseDerivation(N, currentScope)->irLeftChild;
	addLeafWithChainingSeq(N, node, derivationExpression);

	if (derivationExpression->type != kNewtonIrNodeType_Tnone)
	{
		newtonPhysicsAddExponents(N, newPhysics, derivationExpression->physics);
	}
	else
	{
		assert(basicPhysicsIdentifier->token->identifier != NULL);
		assert(unitName != NULL && unitName->token);
		newtonPhysicsIncrementExponent(
			N,
			newPhysics,
			newtonDimensionTableDimensionForIdentifier(N, N->newtonIrTopScope, unitName->token->stringConst)
			);
	}

	newPhysics->id = newtonGetPhysicsId(N, newPhysics);
	newPhysics->subindex = currentScope->currentSubindex;

	assert(newPhysics->id > 1);

	for (currentScope->currentSubindex = subindexStart + 1; currentScope->currentSubindex <= subindexEnd; currentScope->currentSubindex++)
	{
		Physics * newSubindexPhysics = newtonPhysicsTableCopyAndAddPhysics(N, currentScope, newPhysics);
		assert(newSubindexPhysics != newPhysics);

		newSubindexPhysics->id = newtonGetPhysicsId(N, newSubindexPhysics);
		newSubindexPhysics->subindex = currentScope->currentSubindex;

		assert(newSubindexPhysics->id > 1);
		assert(newSubindexPhysics->subindex > 0);
	}

	newtonParseTerminal(N, kNewtonIrNodeType_TrightBrace, currentScope);
	currentScope->currentSubindex = 0;

    return node;
}

IrNode *
newtonParseName(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pname,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

    addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tname, currentScope));
    addLeafWithChainingSeq(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope));

    IrNode * baseSignalName = newtonParseTerminal(N, kNewtonIrNodeType_TstringConst, currentScope);
    node->token = baseSignalName->token;

    addLeafWithChainingSeq(N, node, baseSignalName);

    if (lexPeek(N, 1)->type == kNewtonIrNodeType_TEnglish || 
        lexPeek(N, 1)->type == kNewtonIrNodeType_TSpanish)
	{
		addLeafWithChainingSeq(N, node, newtonParseTerminal(N, lexPeek(N, 1)->type, currentScope));
	}
    else
    {
        fatal(N, "newton-parser.c:newtonParseName no language setting\n");
    }

    return node;
}

IrNode *
newtonParseSymbol(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Psymbol,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

    newtonParseTerminal(N, kNewtonIrNodeType_Tsymbol, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);

    IrNode * baseSignalAbbreviation = newtonParseTerminal(N, kNewtonIrNodeType_TstringConst, currentScope);
    node->token = baseSignalAbbreviation->token;

    addLeaf(N, node, baseSignalAbbreviation);
    newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

    return node;
}

IrNode *
newtonParseDerivation(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pderivation,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

    newtonParseTerminal(N, kNewtonIrNodeType_Tderivation, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);

    if (lexPeek(N, 1)->type == kNewtonIrNodeType_Tnone)
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
IrNode *
newtonParseTerminal(State *  N, IrNodeType expectedType, Scope * currentScope)
{
    if (!peekCheck(N, 1, expectedType))
    {
        fatal(N, "newton-parser.c: newtonParseTerminal");
    }

    Token *  t = lexGet(N, gNewtonTokenDescriptions);
    IrNode * n = genIrNode(N, t->type,
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
IrNode *
newtonParseIdentifier(State *  N, Scope *  currentScope)
{
	IrNode *	n;

	if (peekCheck(N, 1, kNewtonIrNodeType_Tidentifier))
	{
		n = newtonParseIdentifierDefinitionTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
	    return n;
	}
	else
	{
        fatal(N, "newton-parser.c:newtonParseIdentifier: not an identifier token");
	}
    return NULL;
}

/*
 * This method is used by the Newton API to search through the parameters
 * that correspond to each Physics node in the invariant tree.
 * TODO: can add nth like findNthNodeType method to accommodate multiple 
 * parameters with same Physics
 */
IrNode *
newtonParseFindNodeByPhysicsId(State *N, IrNode * root, int physicsId)
{
    // do DFS and find the node whose right child node has given identifier
    // and return the left node's identifier
    if (root->physics != NULL)
    {
        assert(root->physics->id > 1);
		if (root->physics->id == physicsId)
        {
            return root;
        }
    }

    IrNode* targetNode = NULL;

    if (root->irLeftChild != NULL)
        targetNode = newtonParseFindNodeByPhysicsId(N, root->irLeftChild, physicsId);

    if (targetNode != NULL)
        return targetNode;

    if (root->irRightChild != NULL)
        targetNode = newtonParseFindNodeByPhysicsId(N, root->irRightChild, physicsId);

    if (targetNode != NULL)
        return targetNode;

    return targetNode;
}

IrNode *
newtonParseFindNodeByParameterNumberAndSubindex(State *N, IrNode * root, int parameterNumber, int subindex)
{
	if (root->type == kNewtonIrNodeType_Pparameter &&
		root->parameterNumber == parameterNumber &&
		root->physics != NULL &&
		root->physics->subindex == subindex
		)
    {
		return root;
    }

	IrNode* targetNode = NULL;

	if (root->irLeftChild != NULL)
		targetNode = newtonParseFindNodeByParameterNumberAndSubindex(N, root->irLeftChild, parameterNumber, subindex);

	if (targetNode != NULL)
		return targetNode;

	if (root->irRightChild != NULL)
		targetNode = newtonParseFindNodeByParameterNumberAndSubindex(N, root->irRightChild, parameterNumber, subindex);

	if (targetNode != NULL)
		return targetNode;

	return targetNode;
}

IrNode *
newtonParseFindParameterByTokenString(State *N, IrNode * root, char* tokenString)
{
    if (root->type == kNewtonIrNodeType_Pparameter)
    {
        assert(root->irLeftChild != NULL && root->irRightChild != NULL);
		if (!strcmp(root->irLeftChild->tokenString, tokenString))
        {
			assert(root->irRightChild->physics != NULL);
			return root;
        }
    }

	IrNode* targetNode = NULL;

	if (root->irLeftChild != NULL)
		targetNode = newtonParseFindParameterByTokenString(N, root->irLeftChild, tokenString);

	if (targetNode != NULL)
		return targetNode;

	if (root->irRightChild != NULL)
		targetNode = newtonParseFindParameterByTokenString(N, root->irRightChild, tokenString);

	if (targetNode != NULL)
		return targetNode;

	return targetNode;
}

Physics* 
newtonParseGetPhysicsByBoundIdentifier(State * N, IrNode * root, char* boundVariableIdentifier)
{
    // do DFS and find the node whose left child node has given identifier
    // and return the right node's identifier
    if (root->type == kNewtonIrNodeType_Pparameter)
    {
        assert(root->irLeftChild != NULL && root->irRightChild != NULL);
		if (!strcmp(root->irLeftChild->tokenString, boundVariableIdentifier))
        {
			assert(root->irRightChild->physics != NULL);
            return root->irRightChild->physics;
        }
    }

	Physics* result = NULL;

    if (root->irLeftChild != NULL)
        result = newtonParseGetPhysicsByBoundIdentifier(N, root->irLeftChild, boundVariableIdentifier);

    if (result != NULL)
        return result;

    if (root->irRightChild != NULL)
        result = newtonParseGetPhysicsByBoundIdentifier(N, root->irRightChild, boundVariableIdentifier);

    if (result != NULL)
        return result;

    return NULL;
}

/*
 * The caller of this function passes in 1 for invariantId
 * TODO: move this method, newtonParseGetPhysicsTypeStringByBoundIdentifier, and newtonIsConstant
 * to a helper file, don't put them here
 * TODO: this is not a robust design. Even unsigned long long can overflow
 */
unsigned long long int
newtonGetInvariantIdByParameters(State * N, IrNode * parameterTreeRoot, unsigned long long int invariantId)
{
    if (parameterTreeRoot->type == kNewtonIrNodeType_Pparameter)
    {
        assert(parameterTreeRoot->physics->id != 0);

        return invariantId * parameterTreeRoot->physics->id;
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
IrNode *
newtonParseIdentifierUsageTerminal(State *  N, IrNodeType expectedType, Scope *  scope)
{
    if (!peekCheck(N, 1, expectedType))
    {
        fatal(N, "newton-parser.c:newtonParseIdentifierUsageTerminal: not an expected type\n");
        return NULL;
    }

    Token *    t = lexGet(N, gNewtonTokenDescriptions);
    IrNode *   n = genIrNode(N,   t->type,
                        NULL /* left child */,
                        NULL /* right child */,
                        t->sourceInfo /* source info */);

    n->token = t;
    n->tokenString = t->identifier;
	assert(!strcmp(n->token->identifier, n->tokenString));

    // TODO rewrite this logic in a cleaner way.... make a new method or something
    Physics * physicsSearchResult;

    if ((physicsSearchResult = newtonPhysicsTablePhysicsForIdentifier(N, scope, t->identifier)) == NULL)
    {
        physicsSearchResult = newtonPhysicsTablePhysicsForDimensionAlias(N, scope, t->identifier);
    }
	if (physicsSearchResult == NULL)
    {
        physicsSearchResult = newtonPhysicsTablePhysicsForDimensionAliasAbbreviation(N, scope, t->identifier);
    }

    if (physicsSearchResult == NULL)
    {
		// TODO remove later
		if (!strcmp(t->identifier, "h")) {
			assert(true);
		}
        physicsSearchResult = newtonParseGetPhysicsByBoundIdentifier(N, scope->invariantParameterList, t->identifier);
    }

	assert(physicsSearchResult != NULL);

    n->physics = deepCopyPhysicsNode(physicsSearchResult);
	assert(n->physics->dimensions != NULL);

    return n;
}

void newtonParseResetPhysicsWithCorrectSubindex(
	State * N,
	IrNode * node,
	Scope * scope,
	char * identifier,
	int subindex)
{
    Physics * physicsSearchResult = newtonPhysicsTablePhysicsForIdentifierAndSubindex(N, scope, identifier, subindex);

	assert(physicsSearchResult != NULL);

    /* defensive copying to keep the Physics list in State immutable */
    node->physics = deepCopyPhysicsNode(physicsSearchResult);
	assert(node->physics->dimensions != NULL);
}

IrNode *
newtonParseConstraint(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pconstraint,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

    if (inFirst(N, kNewtonIrNodeType_PquantityExpression, gNewtonFirsts))
    {
        addLeaf(N, node, newtonParseQuantityExpression(N, currentScope));
        addLeafWithChainingSeq(N, node, newtonParseCompareOp(N, currentScope));
        addLeafWithChainingSeq(N, node, newtonParseQuantityExpression(N, currentScope));

        // TODO do some epic type checking here
    }
    else
    {
        fatal(N, "newton-parser.c:newtonParseConstraint not a first of quantityExpression\n");
    }

    return node;
}


/*
 *	Remove an identifier _definition_ terminal, performing symtab insertion
 */
IrNode *
newtonParseIdentifierDefinitionTerminal(State *  N, IrNodeType  expectedType, Scope *  scope)
{
	if (!peekCheck(N, 1, expectedType))
	{
        fatal(N, "newton-parser.c:newtonParseIdentifierDefinitionTerminal: not an expected type\n");
	}

	Token *	t = lexGet(N, gNewtonTokenDescriptions);
	IrNode *	n = genIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

    n->token = t;
	n->tokenString = t->identifier;

	return n;
}

bool
newtonIsDimensionless(Physics * physics)
{
	if (physics == NULL)
		return true;

	bool isDimensionless = true;
	Dimension* current = physics->dimensions;
	while (current != NULL)
    {
		assert(current->exponent == (int) current->exponent);
		isDimensionless = isDimensionless && (current->exponent == 0);

		current = current->next;
    }

	return isDimensionless;
}

int
newtonGetPhysicsId(State * N, Physics * physics)
{
	return primeNumbers[N->primeNumbersIndex++];
    // /*
    //  * Prime number id's are assigned to Dimension struct's, and
    //  * Physics struct's construct its own id as a multiple of all of 
    //  * its numerator Dimension's and denominator Dimension's.
    //  * However, if Physics is a constant and does not have any Dimension's
    //  * we just assign the next prime number from our prime numbers array.
    //  */
    // if (newtonIsDimensionless(physics))
    // {
    //     return primeNumbers[N->primeNumbersIndex++];
    // }

    // /*
    //  * Important: Assumes a determinate order of Physics struct's in the newtonIrTopScope->firstDimension
    //  * This is how Java does hash functions of Double List.
    //  * Taken from https://docs.oracle.com/javase/7/docs/api/java/util/List.html#hashCode()
    //  * There will be a collision, but hopefully the probability of collision is low.
    //  */
    // int id = 0;
    // int base = 31;
    // int offset = 1;

    // Dimension * current = physics->numeratorDimensions;
    // while (current != NULL)
    // {
    //     assert(current->exponent == (int) current->exponent);
    //     id += pow(base, offset) * getHashCode(pow(current->primeNumber, current->exponent));
    //     current = current->next;
    // }

    // return id;
}

/*
 * Taken from
 * http://stackoverflow.com/questions/31327789/how-do-the-gethashcode-methods-work-for-the-value-types-in-c
 */
// int getHashCode(double number) {
//     double d = number;
// 	if (d == 0) {
// 	    // Ensure that 0 and -0 have the same hash code
// 	    return 0;
//     }
// 	long value = *(long*)(&d);
// 	return ((int)value) ^ ((int)(value >> 32));
// }
