/*
	Authored 2017 by Jonathan Lim (based on Noisy parser, authored
	2015 -- 2017 by Phillip Stanley-Marbell).

	Modified 2018, Phillip Stanley-Marbell.

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

/*
 *	For asprintf()
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <stdarg.h>
#include <math.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "version.h"
#include "common-errors.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "common-irHelpers.h"
#include "common-lexers-helpers.h"
#include "newton-lexer.h"
#include "common-symbolTable.h"
#include "newton-symbolTable.h"
#include "common-firstAndFollow.h"
#include "newton-parser.h"


extern unsigned long int	bigNumberOffset;
extern int			primeNumbers[];
extern const char *		gNewtonTokenDescriptions[];
extern char *			gNewtonAstNodeStrings[];
extern char *			gProductionDescriptions[];
extern int			gNewtonFirsts[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax];
extern int			gNewtonFollows[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax];
extern void			fatal(State *  N, const char *  msg);


static char			kNewtonErrorTokenHtmlTagOpen[]	= "<span style=\"background-color:#FFCC00; color:#FF0000;\">";
static char			kNewtonErrorTokenHtmlTagClose[]	= "</span>";
static char			kNewtonErrorDetailHtmlTagOpen[]	= "<span style=\"background-color:#A9E9FF; color:#000000;\">";
static char			kNewtonErrorDetailHtmlTagClose[]= "</span>";


IrNode *
newtonParse(State *  N, Scope *  currentScope)
{
	return newtonParseFile(N, currentScope);
}

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

	/*
	 *	Before we start parsing, set begin source line of toplevel scope.
	 */
	currentScope->begin = lexPeek(N, 1)->sourceInfo;

	addLeaf(N, node, newtonParseStatementList(N, currentScope));

	if (lexPeek(N, 1)->type != kNewtonIrNodeType_Zeof)
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Zeof, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Zeof);
	}

	/*
	 *	Skip eof token without using lexGet
	 */
	N->tokenList = N->tokenList->next;

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PnewtonFile, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PnewtonFile, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnewtonFile);
	}
	*/

	return node;
}

IrNode *
newtonParseStatementList(State *  N, Scope *  currentScope)
{
	IrNode *	node = genIrNode(
						N,
						kNewtonIrNodeType_PstatementList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (inFirst(N, kNewtonIrNodeType_Pstatement, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseStatement(N, currentScope));
	}

	while (inFirst(N, kNewtonIrNodeType_Pstatement, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseStatement(N, currentScope));
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PstatementList, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PstatementList, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PstatementList);
	}
	*/

	return node;
}

IrNode *
newtonParseStatement(State * N, Scope * currentScope)
{
	IrNode *	node;

	currentScope->begin = lexPeek(N, 1)->sourceInfo;

	switch(lexPeek(N, 3)->type)
	{
		case kNewtonIrNodeType_Tsignal:
		{
			node = newtonParseBaseSignal(N, currentScope);
			break;
		}

		case kNewtonIrNodeType_Tconstant:
		{
			node = newtonParseConstant(N, currentScope);
			break;
		}

		case kNewtonIrNodeType_Tinvariant:
		{
			node = newtonParseInvariant(N, currentScope);
			break;
		}

		default:
		{
			newtonParserSyntaxError(N, kNewtonIrNodeType_Pstatement, kNewtonIrNodeTypeMax, gNewtonFirsts);
			newtonParserErrorRecovery(N, kNewtonIrNodeType_Pstatement);
		}
	}

	currentScope->end = lexPeek(N, 1)->sourceInfo;

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Pstatement, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pstatement, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pstatement);
	}
	*/

	return node;
}

IrNode *
newtonParseInvariant(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pinvariant,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	Invariant *	invariant	= (Invariant *) calloc(1, sizeof(Invariant));
	IrNode *	invariantName	= newtonParseIdentifier(N, currentScope);

	/*
	 *	Also add it to the AST
	 */
	addLeaf(N, node, invariantName);
	invariant->identifier = invariantName->tokenString;

	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);

	/*
	 *	The new scope begins at the `invariant` (and includes the parameter list)
	 */
	IrNode *	scopeBegin	= newtonParseTerminal(N, kNewtonIrNodeType_Tinvariant, currentScope);
	Scope *		newScope	= commonSymbolTableOpenScope(N, currentScope, scopeBegin);

	invariant->parameterList = newtonParseParameterTuple(N, newScope);
	newScope->invariantParameterList = invariant->parameterList;

	/*
	 *	Also add it to the AST
	 */
	addLeafWithChainingSeq(N, node, invariant->parameterList);

	newtonParseTerminal(N, kNewtonIrNodeType_Tequals, newScope);
	newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, newScope);

	if (inFirst(N, kNewtonIrNodeType_Pconstraint, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		IrNode *	firstConstraint = newtonParseConstraint(N, newScope);
		addLeafWithChainingSeq(N, node, firstConstraint);

		while (peekCheck(N, 1, kNewtonIrNodeType_Tcomma))
		{
			newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, newScope);
			addLeafWithChainingSeq(N, node, newtonParseConstraint(N, newScope));
		}

		invariant->constraints		= firstConstraint;
		invariant->id			= newtonGetInvariantIdByParameters(N, invariant->parameterList, 1);
	}

	IrNode *	scopeEnd	= newtonParseTerminal(N, kNewtonIrNodeType_TrightBrace, newScope);
	commonSymbolTableCloseScope(N, newScope, scopeEnd);
	newtonAddInvariant(N, invariant);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Pinvariant, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pinvariant, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pinvariant);
	}
	*/

	return node;
}

IrNode *
newtonParseSubindex(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Psubindex,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);

	node->subindexStart = newtonParseTerminal(N, kNewtonIrNodeType_TnumericConst, currentScope)->value;
	newtonParseTerminal(N, kNewtonIrNodeType_Tto, currentScope);
	node->subindexEnd = newtonParseTerminal(N, kNewtonIrNodeType_TnumericConst, currentScope)->value;

	if (node->subindexEnd <= 0)
	{
		char *	details;

		asprintf(&details, "%s\n", EsubindexEndMustBeNaturalnumber);
		newtonParserSemanticError(N, kNewtonIrNodeType_Psubindex, details);
		free(details);

		newtonParserErrorRecovery(N, kNewtonIrNodeType_Psubindex);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Psubindex, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Psubindex, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Psubindex);
	}
	*/

	return node;
}

IrNode *
newtonParseSubindexTuple(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PsubindexTuple,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);

	IrNode *	subindexNode = newtonParseSubindex(N, currentScope);
	addLeaf(N, node, subindexNode);

	node->subindexStart = subindexNode->subindexStart;
	node->subindexEnd = subindexNode->subindexEnd;

	newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PsubindexTuple, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PsubindexTuple, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsubindexTuple);
	}
	*/

	return node;
}

IrNode *
newtonParseParameterTuple(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PparameterTuple,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);

	int	parameterNumber = 0;
	addLeaf(N, node, newtonParseParameter(N, currentScope, parameterNumber++));

	while (peekCheck(N, 1, kNewtonIrNodeType_Tcomma))
	{
		newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
		addLeafWithChainingSeq(N, node, newtonParseParameter(N, currentScope, parameterNumber++));
	}
	newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PparameterTuple, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PparameterTuple, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PparameterTuple);
	}
	*/

	return node;
}

IrNode *
newtonParseParameter(State * N, Scope * currentScope, int parameterNumber)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pparameter,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	/*
	 *	By convention the left child is the name, and the right child is the name of the Physics
	 */
	addLeaf(N, node, newtonParseIdentifier(N, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
	IrNode *	physicsName = newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
	addLeaf(N, node, physicsName);

	if (lexPeek(N, 1)->type == kNewtonIrNodeType_TatSign)
	{
		newtonParseTerminal(N, kNewtonIrNodeType_TatSign, currentScope);

		newtonParseResetPhysicsWithCorrectSubindex(
			N,
			physicsName,
			currentScope,
			physicsName->token->identifier,
			newtonParseTerminal(N, kNewtonIrNodeType_TnumericConst, currentScope)->value
		);
	}

	node->parameterNumber = parameterNumber;
	node->physics = physicsName->physics;

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Pparameter, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pparameter, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pparameter);
	}
	*/

	return node;
}


IrNode *
newtonParseConstant(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pconstant,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	IrNode *	constantIdentifier = newtonParseIdentifier(N, currentScope);

	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tconstant, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);

	addLeaf(N, node, constantIdentifier);
	Physics *	constantPhysics = newtonPhysicsTableAddPhysicsForToken(N, currentScope, constantIdentifier->token);

	if (inFirst(N, kNewtonIrNodeType_PquantityExpression, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		IrNode *	constantExpression = newtonParseQuantityExpression(N, currentScope);
		constantPhysics->value = constantExpression->value;
		constantPhysics->isConstant = true;

		node->value = constantExpression->value;

		newtonPhysicsAddExponents(N, constantPhysics, constantExpression->physics);

		/*
		 *	If LHS is declared a vector in vectorScalarPairScope, then
		 *	the expression must evaluate to a vector.
		 */
		if (constantPhysics->isVector)
		{
			newtonParserSyntaxError(N, kNewtonIrNodeType_PquantityExpression, kNewtonIrNodeTypeMax, gNewtonFirsts);
			newtonParserErrorRecovery(N, kNewtonIrNodeType_PquantityExpression);	
		}
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PquantityExpression, kNewtonIrNodeTypeMax, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PquantityExpression);
	}

	newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Pconstant, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pconstant, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pconstant);
	}
	*/

	return node;
}

IrNode *
newtonParseBaseSignal(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PbaseSignal,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	IrNode *	basicPhysicsIdentifier = newtonParseIdentifier(N, currentScope);
	addLeaf(N, node, basicPhysicsIdentifier);
	Physics *	newPhysics = newtonPhysicsTableAddPhysicsForToken(N, currentScope, basicPhysicsIdentifier->token);

	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tsignal, currentScope);

	/*
	 *	TODO (Jonathan) do some i: 0 to 2 parsing here
	 */
	int		subindexStart = 0;
	int		subindexEnd = 0;
	IrNode *	subindexNode;
	if (lexPeek(N, 5)->type == kNewtonIrNodeType_Tto)
	{
		subindexNode = newtonParseSubindexTuple(N, currentScope);
		subindexStart = subindexNode->subindexStart;
		subindexEnd = subindexNode->subindexEnd;
	}

	newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, currentScope);

	/*
	 *	Name syntax is optional
	 */
	IrNode *	unitName = NULL;
	if (inFirst(N, kNewtonIrNodeType_Pname, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		unitName = newtonParseName(N, currentScope);
		addLeafWithChainingSeq(N, node, unitName);
		newPhysics->dimensionAlias = unitName->token->stringConst; /* e.g., meter, Pascal*/
	}

	/*
	 *	Abbreviation syntax is also optional
	 */
	IrNode *	unitAbbreviation = NULL;
	if (inFirst(N, kNewtonIrNodeType_Psymbol, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		unitAbbreviation = newtonParseSymbol(N, currentScope);
		addLeafWithChainingSeq(N, node, unitAbbreviation);

		/*
		 *	e.g., m, Pa
		 */
		newPhysics->dimensionAliasAbbreviation = unitAbbreviation->token->identifier;
	}

	/*
	 *	Derivation syntax is required
	 */
	IrNode *	derivationExpression = newtonParseDerivation(N, currentScope)->irLeftChild;
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
			newtonDimensionTableDimensionForName(N, N->newtonIrTopScope, unitName->token->stringConst)
			);
	}

	newPhysics->id = newtonGetPhysicsId(N, newPhysics);
	newPhysics->subindex = currentScope->currentSubindex;

	assert(newPhysics->id > 1);

	for (currentScope->currentSubindex = subindexStart + 1; currentScope->currentSubindex <= subindexEnd; currentScope->currentSubindex++)
	{
		Physics *   newSubindexPhysics = newtonPhysicsTableCopyAndAddPhysics(N, currentScope, newPhysics);
		assert(newSubindexPhysics != newPhysics);

		newSubindexPhysics->id = newtonGetPhysicsId(N, newSubindexPhysics);
		newSubindexPhysics->subindex = currentScope->currentSubindex;

		assert(newSubindexPhysics->id > 1);
		assert(newSubindexPhysics->subindex > 0);
	}

	newtonParseTerminal(N, kNewtonIrNodeType_TrightBrace, currentScope);
	currentScope->currentSubindex = 0;

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PbaseSignal, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PbaseSignal, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PbaseSignal);
	}
	*/

	return node;
}

IrNode *
newtonParseName(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pname,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tname, currentScope));
	addLeafWithChainingSeq(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope));

	IrNode *	baseSignalName = newtonParseTerminal(N, kNewtonIrNodeType_TstringConst, currentScope);
	node->token = baseSignalName->token;

	addLeafWithChainingSeq(N, node, baseSignalName);

	if (	lexPeek(N, 1)->type == kNewtonIrNodeType_TEnglish)
	{
		addLeafWithChainingSeq(N, node, newtonParseTerminal(N, lexPeek(N, 1)->type, currentScope));
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pname, kNewtonIrNodeTypeMax, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pname);
	}
	newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Pname, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pname, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pname);
	}
	*/

	return node;
}

IrNode *
newtonParseSymbol(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Psymbol,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_Tsymbol, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);

	IrNode *	baseSignalAbbreviation = newtonParseIdentifierDefinitionTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
	node->token = baseSignalAbbreviation->token;

	addLeaf(N, node, baseSignalAbbreviation);
	newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);


	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Psymbol, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Psymbol, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Psymbol);
	}
	*/

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
	else if (lexPeek(N, 1)->type == kNewtonIrNodeType_Tdimensionless)
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tdimensionless, currentScope));
	}
	else
	{
		addLeaf(N, node, newtonParseQuantityExpression(N, currentScope));
	}
	newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);


	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Pderivation, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pderivation, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pderivation);
	}
	*/

	return node;
}



/*
 *	Simply remove a terminal
 */
IrNode *
newtonParseTerminal(State *  N, IrNodeType expectedType, Scope * currentScope)
{
	if (!peekCheck(N, 1, expectedType))
	{
		newtonParserSyntaxError(N, expectedType, expectedType, gNewtonFirsts);
		newtonParserErrorRecovery(N, expectedType);
	}

	Token *		t = lexGet(N, gNewtonTokenDescriptions);
	IrNode *	n = genIrNode(N,	t->type,
					NULL /* left child */,
					NULL /* right child */,
					t->sourceInfo /* source info */
					);

	n->token = t;
	n->tokenString = t->identifier;

	if (t->type == kNewtonIrNodeType_TnumericConst)
	{
		n->value = 0.0;
		if (t->integerConst != 0)
		{
			n->value = t->integerConst;
		}
		else if (t->realConst != 0)
		{
			n->value = t->realConst;
		}
	}

	return n;
}

IrNode *
newtonParseIdentifier(State *  N, Scope *  currentScope)
{
	IrNode *	n;

	if (peekCheck(N, 1, kNewtonIrNodeType_Tidentifier))
	{
		n = newtonParseIdentifierDefinitionTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);

		return n;
	}

	newtonParserSyntaxError(N, kNewtonIrNodeType_Tidentifier, kNewtonIrNodeType_Tidentifier, gNewtonFirsts);
	newtonParserErrorRecovery(N, kNewtonIrNodeType_Tidentifier);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Tidentifier, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Tidentifier, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Tidentifier);
	}
	*/

	return NULL;
}






















/*
 *	The following functions were originally in newton-parser-expression.c.
 *	I've moved them in here, deleted newton-parser-expression.c, and
 *	started cleaning them up (removing assert()s that should be 
 *	newtonParserSemanticError()s, adding the !inFollow()s, etc.) as part
 *	of general hygiene. --- PSM.
 */

IrNode *
newtonParseQuantityExpression(State * N, Scope * currentScope)
{
	IrNode *	expression = genIrNode(N,   kNewtonIrNodeType_PquantityExpression,
					NULL /* left child */,
					NULL /* right child */,
					lexPeek(N, 1)->sourceInfo /* source info */);

	expression->physics = newtonInitPhysics(N, currentScope, NULL);

	if (inFirst(N, kNewtonIrNodeType_PquantityTerm, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		IrNode *	leftTerm = newtonParseQuantityTerm(N, currentScope);
		expression->value = leftTerm->value;
		expression->physics = leftTerm->physics;
		addLeaf(N, expression, leftTerm);

		while (inFirst(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax))
		{
			addLeafWithChainingSeq(N, expression, newtonParseLowPrecedenceBinaryOp(N, currentScope));

			IrNode *	rightTerm = newtonParseQuantityTerm(N, currentScope);
			addLeafWithChainingSeq(N, expression, rightTerm);
			expression->value += rightTerm->value;

			if(!areTwoPhysicsEquivalent(N, leftTerm->physics, rightTerm->physics))
			{
				newtonParserSemanticError(N, kNewtonIrNodeType_PquantityExpression, (char *)EexpressionPhysicsMismatch);
				newtonParserErrorRecovery(N, kNewtonIrNodeType_PquantityExpression);
			}
		}
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PquantityExpression, kNewtonIrNodeType_PquantityExpression, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PquantityExpression);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PquantityExpression, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PquantityExpression, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PquantityExpression);
	}
	*/

	return expression;
}

IrNode *
newtonParseQuantityTerm(State * N, Scope * currentScope)
{
	IrNode *   intermediate = genIrNode(N, kNewtonIrNodeType_PquantityTerm,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

	intermediate->physics = newtonInitPhysics(N, currentScope, NULL);
	intermediate->value = 1;

	bool isUnary = false;

	if (inFirst(N, kNewtonIrNodeType_PunaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, intermediate, newtonParseUnaryOp(N, currentScope));
		isUnary = true;
	}

	bool hasNumberInTerm = false;
	IrNode *	leftFactor = newtonParseQuantityFactor(N, currentScope);
	addLeafWithChainingSeq(N, intermediate, leftFactor);
	hasNumberInTerm = hasNumberInTerm || leftFactor->physics == NULL || leftFactor->physics->isConstant;
	if (hasNumberInTerm)
	{
		intermediate->value = (
					isUnary			?
					leftFactor->value * -1	:
					leftFactor->value
				);
	}

	int	numVectorsInTerm = 0;

	if (!newtonIsDimensionless(leftFactor->physics))
	{
		assert(leftFactor->physics != NULL);
		newtonPhysicsAddExponents(N, intermediate->physics, leftFactor->physics);

		/*
		 *	If either LHS or RHS is a vector (not both), then the resultant is a vector
		 */
		if (leftFactor->physics->isVector)
		{
			intermediate->physics->isVector = true;
			numVectorsInTerm++;
		}
	}

	IrNode *	rightFactor;

	while (inFirst(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		IrNode *	binOp = newtonParseMidPrecedenceBinaryOp(N, currentScope);
		addLeafWithChainingSeq(N, intermediate, binOp);


		rightFactor = newtonParseQuantityFactor(N, currentScope);

		addLeafWithChainingSeq(N, intermediate, rightFactor);
		hasNumberInTerm = hasNumberInTerm || leftFactor->physics == NULL || leftFactor->physics->isConstant;

		if (hasNumberInTerm)
		{
			if (binOp->type == kNewtonIrNodeType_Tmul)
			{
				intermediate->value = (
							rightFactor->value == 0 		?
							intermediate->value			:
							intermediate->value*rightFactor->value
						);
			}
			else if (binOp->type == kNewtonIrNodeType_Tdiv)
			{
				intermediate->value = (
							rightFactor->value == 0			?
							intermediate->value			:
							intermediate->value/rightFactor->value
						);
			}
		}

		/*
		 *	TODO (from Jonathan): double check this logic when I'm more awake
		 */
		if (!newtonIsDimensionless(rightFactor->physics) && rightFactor->physics->isVector)
		{
			intermediate->physics->isVector = true;
			numVectorsInTerm++;

			/*
			 *	Cannot perform multiply or divide operations on two vectors,
			 *	e.g., vector * scalar * scalar / vector is illegal because
			 *	it boils down to vector / vector which is illegal
			 */
			assert(numVectorsInTerm < 2);
		}

		if (!newtonIsDimensionless(rightFactor->physics) && binOp->type == kNewtonIrNodeType_Tmul)
		{
			newtonPhysicsAddExponents(N, intermediate->physics, rightFactor->physics);
		}
		else if (!newtonIsDimensionless(rightFactor->physics) && binOp->type == kNewtonIrNodeType_Tdiv)
		{
			newtonPhysicsSubtractExponents(N, intermediate->physics, rightFactor->physics);
		}
	}

	if (! hasNumberInTerm)
	{
		intermediate->value = 0;
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PquantityTerm, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PquantityTerm, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PquantityTerm);
	}
	*/

	return intermediate;
}

IrNode *
newtonParseQuantityFactor(State * N, Scope * currentScope)
{
	IrNode *	intermediate = genIrNode(N, kNewtonIrNodeType_PquantityFactor,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

	IrNode *	factor;
	if (peekCheck(N, 1, kNewtonIrNodeType_Tidentifier))
	{
		factor = newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
		factor->physics = deepCopyPhysicsNode(factor->physics);
		factor->value = factor->physics->value;

		assert(factor->tokenString != NULL);

		if (peekCheck(N, 1, kNewtonIrNodeType_TatSign))
		{
			newtonParseTerminal(N, kNewtonIrNodeType_TatSign, currentScope);
			newtonParseTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
			newtonParseResetPhysicsWithCorrectSubindex(
				N,
				factor,
				currentScope,
				factor->token->identifier,
				currentScope->currentSubindex);

		}

		/*
		 *	Is a matchable parameter corresponding the invariant parameter.
		 */
		if (!newtonIsDimensionless(factor->physics) &&
			!factor->physics->isConstant &&
			newtonPhysicsTablePhysicsForDimensionAliasAbbreviation(N, N->newtonIrTopScope, factor->tokenString) == NULL &&
			newtonPhysicsTablePhysicsForDimensionAlias(N, N->newtonIrTopScope, factor->tokenString) == NULL &&
			currentScope->invariantParameterList)
		{
			IrNode *	matchingParameter = newtonParseFindParameterByTokenString(N,
													currentScope->invariantParameterList,
													factor->token->identifier
												);
			factor->parameterNumber = matchingParameter->parameterNumber;
		}
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TnumericConst))
	{
		factor = newtonParseTerminal(N, kNewtonIrNodeType_TnumericConst, currentScope);
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TleftParen))
	{
		newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
		factor = newtonParseQuantityExpression(N, currentScope);
		newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PquantityFactor, kNewtonIrNodeType_PquantityFactor, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PquantityFactor);
	}
	addLeaf(N, intermediate, factor);
	intermediate->value = factor->value;
	intermediate->physics = factor->physics;

	/*
	 *	e.g., (acceleration * mass) ** (3 + 5)
	 */
	if (inFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, intermediate, newtonParseHighPrecedenceBinaryOp(N, currentScope));

		IrNode *	exponentialExpression = newtonParseExponentialExpression(N, currentScope, factor);
		assert(exponentialExpression->type == kNewtonIrNodeType_PquantityExpression);
		addLeafWithChainingSeq(N, intermediate, exponentialExpression);

		if (intermediate->value != 0)
		{
			intermediate->value = pow(intermediate->value, exponentialExpression->value);
		}
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PquantityFactor, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PquantityFactor, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PquantityFactor);
	}
	*/

	return intermediate;
}

IrNode *
newtonParseExponentialExpression(State * N, Scope * currentScope, IrNode * baseNode)
{
	IrNode *	expression = genIrNode(N, kNewtonIrNodeType_PquantityExpression,
							NULL /* left child */,
							NULL /* right child */,
							lexPeek(N, 1)->sourceInfo /* source info */);
	expression->physics = newtonInitPhysics(N, currentScope, NULL);

	/*
	 *	Exponents are just one integer unless wrapped in parenthesis
	 */
	IrNode *	exponent = (
					peekCheck(N, 1, kNewtonIrNodeType_TleftParen)	?
					newtonParseQuantityExpression(N, currentScope)	:
					newtonParseInteger(N, currentScope)
				);
	addLeaf(N, expression, exponent);

	expression->value = exponent->value;

	/*
	 *	If the base is a Physics quantity, the exponent must be an integer
	 */
	if (!newtonIsDimensionless(baseNode->physics))
	{
		baseNode->physics->value = pow(baseNode->physics->value, expression->value);
		newtonPhysicsMultiplyExponents(N, baseNode->physics, expression->value);

		/*
		 *	Can't raise a dimension to a non integer value
		 */
		if (exponent->value != (int)exponent->value)
		{
			char *	details;

			asprintf(&details, "%s\n", EraisingPhysicsQuantityToNonIntegerExponent);
			newtonParserSemanticError(N, kNewtonIrNodeType_PquantityExpression, details);
			free(details);

			newtonParserErrorRecovery(N, kNewtonIrNodeType_PquantityExpression);
		}
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PquantityExpression, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PquantityExpression, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PquantityExpression);
	}
	*/

	return expression;
}

IrNode *
newtonParseLowPrecedenceBinaryOp(State *  N, Scope * currentScope)
{
	IrNode *	n;

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
		newtonParserSyntaxError(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp, kNewtonIrNodeType_PlowPrecedenceBinaryOp, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp);
	}
	*/

	return n;
}

IrNode *
newtonParseUnaryOp(State *  N, Scope * currentScope)
{
	IrNode *	n = NULL;

	if (peekCheck(N, 1, kNewtonIrNodeType_Tminus))
	{
		n = newtonParseTerminal(N, kNewtonIrNodeType_Tminus, currentScope);
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tplus))
	{
		n = newtonParseTerminal(N, kNewtonIrNodeType_Tplus, currentScope);
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PunaryOp, kNewtonIrNodeType_PunaryOp, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PunaryOp);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PunaryOp, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PunaryOp, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PunaryOp);
	}
	*/

	return n;
}

IrNode *
newtonParseCompareOp(State * N, Scope * currentScope)
{
	IrNodeType	type = lexPeek(N, 1)->type;
	if (	
		type == kNewtonIrNodeType_Tlt		||
		type == kNewtonIrNodeType_Tle		||
		type == kNewtonIrNodeType_Tge		||
		type == kNewtonIrNodeType_Tgt		||
		type == kNewtonIrNodeType_Tproportional	||
		type == kNewtonIrNodeType_Tequivalent
		)
	{
		return newtonParseTerminal(N, type, currentScope);
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PcompareOp, kNewtonIrNodeType_PcompareOp, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PcompareOp);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PcompareOp, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PcompareOp, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PcompareOp);
	}
	*/
}

IrNode *
newtonParseHighPrecedenceBinaryOp(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp,
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
		newtonParserSyntaxError(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, kNewtonIrNodeType_PhighPrecedenceBinaryOp, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp);
	}
	*/

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
		newtonParserSyntaxError(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp, kNewtonIrNodeType_PmidPrecedenceBinaryOp, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp);
	}
	*/

	return n;
}

IrNode *
newtonParseInteger(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PquantityTerm,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

	bool	hasUnaryOp = false;
	if (inFirst(N, kNewtonIrNodeType_PunaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		int	unaryIntegerMultiplier;
		if (peekCheck(N, 1, kNewtonIrNodeType_Tplus))
		{
			unaryIntegerMultiplier = 1;
		}
		else if (peekCheck(N, 1, kNewtonIrNodeType_Tminus))
		{
			unaryIntegerMultiplier = -1;
		}
		else
		{
			/*
			 *	Flagging this syntax error is in principle redundant since it will be caught by newtonParseUnaryOp() below
			 */
			newtonParserSyntaxError(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp, kNewtonIrNodeType_PmidPrecedenceBinaryOp, gNewtonFirsts);
			newtonParserErrorRecovery(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp);
		}
		addLeafWithChainingSeq(N, node, newtonParseUnaryOp(N, currentScope));
		node->value = unaryIntegerMultiplier;
	}

	IrNode *	number = newtonParseTerminal(N, kNewtonIrNodeType_TnumericConst, currentScope);
	addLeaf(N, node, number);
	if (hasUnaryOp)
	{
		node->value *= number->value;
	}
	else
	{
		node->value = number->value;
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PquantityTerm, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PquantityTerm, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PquantityTerm);
	}
	*/

	return node;
}

IrNode *
newtonParseConstraint(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pconstraint,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (inFirst(N, kNewtonIrNodeType_Pconstraint, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseQuantityExpression(N, currentScope));
		addLeafWithChainingSeq(N, node, newtonParseCompareOp(N, currentScope));
		addLeafWithChainingSeq(N, node, newtonParseQuantityExpression(N, currentScope));

		/*
		 *	TODO; with the Newton grammar update, there are actually two cases to handle here.
		 */
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pconstraint, kNewtonIrNodeTypeMax, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pconstraint);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Pconstraint, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pconstraint, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pconstraint);
	}
	*/

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
		newtonParserSyntaxError(N, expectedType, expectedType, gNewtonFirsts);
		newtonParserErrorRecovery(N, expectedType);
	}

	Token *		t = lexGet(N, gNewtonTokenDescriptions);
	IrNode *	n = genIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */
					);

	n->token = t;
	n->tokenString = t->identifier;

	/*
	 *	NOTE: commonSymbolTableAddOrLookupSymbolForToken(N, scope, t) adds token 't' to scope 'scope'
	 */
	Symbol *	sym = commonSymbolTableAddOrLookupSymbolForToken(N, scope, t);
	if (sym->definition != NULL)
	{
		errorMultiDefinition(N, sym);
	}
	n->symbol = sym;

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Tidentifier, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Tidentifier, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Tidentifier);
	}
	*/

	return n;
}

/*
 *	Remove an identifier _usage_ terminal, performing symtab lookup
 */
IrNode *
newtonParseIdentifierUsageTerminal(State *  N, IrNodeType expectedType, Scope *  scope)
{
	if (!peekCheck(N, 1, expectedType))
	{
		newtonParserSyntaxError(N, expectedType, expectedType, gNewtonFirsts);
		newtonParserErrorRecovery(N, expectedType);
	
		return NULL;
	}

	Token *		t = lexGet(N, gNewtonTokenDescriptions);
	IrNode *	n = genIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */
					);

	n->token = t;
	n->tokenString = t->identifier;
	assert(!strcmp(n->token->identifier, n->tokenString));

	/*
	 *	TODO (Jonathan) rewrite this logic in a cleaner way.... make a new method or something
	 */
	Physics *   physicsSearchResult;

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
		/*
		 *	Identifier use when scope parameter list is empty
		 */	
		if (scope->invariantParameterList == NULL)
		{
			char *	details;

			asprintf(&details, "%s: \"%s\"\n", Eundeclared, t->identifier);
			newtonParserSemanticError(N, kNewtonIrNodeType_Tidentifier, details);
			free(details);

			newtonParserErrorRecovery(N, kNewtonIrNodeType_Tidentifier);
		}

		physicsSearchResult = newtonParseGetPhysicsByBoundIdentifier(N, scope->invariantParameterList, t->identifier);
	}

	if (physicsSearchResult == NULL)
	{
		/*
		 *	Identifier use before definition.
		 */
		char *	details;

		asprintf(&details, "%s: \"%s\"\n", Eundeclared, t->identifier);
		newtonParserSemanticError(N, kNewtonIrNodeType_Tidentifier, details);
		free(details);

		newtonParserErrorRecovery(N, kNewtonIrNodeType_Tidentifier);
	}

	n->physics = deepCopyPhysicsNode(physicsSearchResult);
	assert(n->physics->dimensions != NULL);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Tidentifier, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Tidentifier, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Tidentifier);
	}
	*/

	return n;
}






















/*
 *	Routines related to Newton API. TODO: Should be moved out of newton-parser.c into a separate (API-related) source file.
 */






















/*
 *	This method is used by the Newton API to search through the parameters
 *	that correspond to each Physics node in the invariant tree.
 *	TODO (Jonathan): can add nth like findNthNodeType method to accommodate multiple 
 *	parameters with same Physics
 */
IrNode *
newtonParseFindNodeByPhysicsId(State *N, IrNode * root, int physicsId)
{
	/*
	 *	Do DFS and find the node whose right child node has given identifier
	 *	and return the left node's identifier
	 */
	if (root->physics != NULL)
	{
		assert(root->physics->id > 1);
		if (root->physics->id == physicsId)
		{
			return root;
		}
	}

	IrNode *	targetNode = NULL;

	if (root->irLeftChild != NULL)
	{
		targetNode = newtonParseFindNodeByPhysicsId(N, root->irLeftChild, physicsId);
	}

	if (targetNode != NULL)
	{
		return targetNode;
	}

	if (root->irRightChild != NULL)
	{
		targetNode = newtonParseFindNodeByPhysicsId(N, root->irRightChild, physicsId);
	}

	if (targetNode != NULL)
	{
		return targetNode;
	}

	return targetNode;
}

IrNode *
newtonParseFindNodeByParameterNumberAndSubindex(State *N, IrNode * root, int parameterNumber, int subindex)
{
	if (	root->type == kNewtonIrNodeType_Pparameter	&&
		root->parameterNumber == parameterNumber	&&
		root->physics != NULL				&&
		root->physics->subindex == subindex
	)
	{
		return root;
	}

	IrNode *	targetNode = NULL;

	if (root->irLeftChild != NULL)
	{
		targetNode = newtonParseFindNodeByParameterNumberAndSubindex(N, root->irLeftChild, parameterNumber, subindex);
	}

	if (targetNode != NULL)
	{
		return targetNode; 
	}

	if (root->irRightChild != NULL)
	{
		targetNode = newtonParseFindNodeByParameterNumberAndSubindex(N, root->irRightChild, parameterNumber, subindex);
	}

	if (targetNode != NULL)
	{
		return targetNode;
	}

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

	IrNode *	targetNode = NULL;

	if (root->irLeftChild != NULL)
	{
		targetNode = newtonParseFindParameterByTokenString(N, root->irLeftChild, tokenString);
	}

	if (targetNode != NULL)
	{
		return targetNode;
	}

	if (root->irRightChild != NULL)
	{
		targetNode = newtonParseFindParameterByTokenString(N, root->irRightChild, tokenString);
	}

	if (targetNode != NULL)
	{
		return targetNode;
	}

	return targetNode;
}

Physics *
newtonParseGetPhysicsByBoundIdentifier(State * N, IrNode * root, char* boundVariableIdentifier)
{
	/*
	 *	Do DFS and find the node whose left child node has given identifier
	 *	and return the right node's identifier
	 */
	if (root->type == kNewtonIrNodeType_Pparameter)
	{
		assert(root->irLeftChild != NULL && root->irRightChild != NULL);
		if (!strcmp(root->irLeftChild->tokenString, boundVariableIdentifier))
		{
			assert(root->irRightChild->physics != NULL);
			return root->irRightChild->physics;
		}
	}

	Physics *	result = NULL;

	if (root->irLeftChild != NULL)
	{
		result = newtonParseGetPhysicsByBoundIdentifier(N, root->irLeftChild, boundVariableIdentifier);
	}

	if (result != NULL)
	{
		return result;
	}

	if (root->irRightChild != NULL)
	{
		result = newtonParseGetPhysicsByBoundIdentifier(N, root->irRightChild, boundVariableIdentifier);
	}

	if (result != NULL)
	{
		return result;
	}

	return NULL;
}

/*
 *	The caller of this function passes in 1 for invariantId
 *	TODO (Jonathan): move this method, newtonParseGetPhysicsTypeStringByBoundIdentifier, and newtonIsConstant
 *	to a helper file, don't put them here
 *	TODO (Jonathan): this is not a robust design. Even unsigned long long can overflow
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
	{
		invariantId *= newtonGetInvariantIdByParameters(N, parameterTreeRoot->irLeftChild, 1);
	}

	if (parameterTreeRoot->irRightChild != NULL)
	{
		invariantId *= newtonGetInvariantIdByParameters(N, parameterTreeRoot->irRightChild, 1);
	}

	return invariantId;
}

void newtonParseResetPhysicsWithCorrectSubindex(
	State * N,
	IrNode * node,
	Scope * scope,
	char * identifier,
	int subindex)
{
	Physics *   physicsSearchResult = newtonPhysicsTablePhysicsForIdentifierAndSubindex(N, scope, identifier, subindex);

	if (physicsSearchResult == NULL)
	{
		char *	details;

		asprintf(&details, "%s: \"%s\"@%d\n", Eundeclared, identifier, subindex);
		newtonParserSemanticError(N, kNewtonIrNodeType_Tidentifier, details);
		free(details);

		newtonParserErrorRecovery(N, kNewtonIrNodeType_Tidentifier);
	}

	/* 
	 *	Defensive copying to keep the Physics list in State immutable 
	 */
	node->physics = deepCopyPhysicsNode(physicsSearchResult);
	if (node->physics->dimensions == NULL)
	{
		fatal(N, Esanity);
	}
}

bool
newtonIsDimensionless(Physics * physics)
{
	if (physics == NULL)
	{
		return true;
	}

	bool		isDimensionless = true;
	Dimension *	current = physics->dimensions;
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
}







/*
 *	Exported non-parse routines
 */







void
newtonParserSyntaxAndSemanticPre(State *  N, IrNodeType currentlyParsingTokenOrProduction,
	const char *  string1, const char *  string2, const char *  string3, const char *  string4)
{
	flexprint(N->Fe, N->Fm, N->Fperr, "\n-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --\n");
	if (N->mode & kNoisyModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "<b>");
	}

	if (N->mode & kNoisyModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\t%s, line %d position %d, %s %s\"",
						string1,
						lexPeek(N, 1)->sourceInfo->lineNumber,
						lexPeek(N, 1)->sourceInfo->columnNumber,
						string4,
						kNewtonErrorTokenHtmlTagOpen);
		lexPrintToken(N, lexPeek(N, 1), gNewtonTokenDescriptions);
		flexprint(N->Fe, N->Fm, N->Fperr, "\"%s %s %s.<br><br>%s%s",
			kNewtonErrorTokenHtmlTagClose,
			string2,
			(
				currentlyParsingTokenOrProduction > kNewtonIrNodeType_TMax	?
				gProductionDescriptions[currentlyParsingTokenOrProduction]	:
				gNewtonTokenDescriptions[currentlyParsingTokenOrProduction]
			),
			kNewtonErrorDetailHtmlTagOpen,
			string3);
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\t%s, %s line %d position %d, %s \"",
						string1,
						lexPeek(N, 1)->sourceInfo->fileName,
						lexPeek(N, 1)->sourceInfo->lineNumber,
						lexPeek(N, 1)->sourceInfo->columnNumber,
						string4);
		lexPrintToken(N, lexPeek(N, 1), gNewtonTokenDescriptions);
		flexprint(N->Fe, N->Fm, N->Fperr, "\" %s %s.\n\n\t%s",
			string2,
			(
				currentlyParsingTokenOrProduction > kNewtonIrNodeType_TMax	?
				gProductionDescriptions[currentlyParsingTokenOrProduction]	:
				gNewtonTokenDescriptions[currentlyParsingTokenOrProduction]
			),
			string3);
	}
}

void
newtonParserSyntaxAndSemanticPost(State *  N)
{
	if (N->mode & kNoisyModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s</b>", kNewtonErrorDetailHtmlTagClose);
	}

	flexprint(N->Fe, N->Fm, N->Fperr, "\n-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --\n\n");	
}

void
newtonParserSyntaxError(State *  N, IrNodeType currentlyParsingTokenOrProduction, IrNodeType expectedProductionOrToken, int firstOrFollowsArray[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax])
{
	int		seen = 0;


	TimeStampTraceMacro(kNewtonTimeStampKeyParserSyntaxError);

	newtonParserSyntaxAndSemanticPre(N, currentlyParsingTokenOrProduction, EsyntaxA, EsyntaxB, EsyntaxC, EsyntaxD);

	if (((expectedProductionOrToken > kNewtonIrNodeType_TMax) && (expectedProductionOrToken < kNewtonIrNodeType_PMax)) || (expectedProductionOrToken == kNewtonIrNodeTypeMax))
	{
		flexprint(N->Fe, N->Fm, N->Fperr, " one of:\n\n\t\t");
		for (int i = 0; i < kNewtonIrNodeTypeMax && firstOrFollowsArray[currentlyParsingTokenOrProduction][i] != kNewtonIrNodeTypeMax; i++)
		{
			if (seen > 0)
			{
				flexprint(N->Fe, N->Fm, N->Fperr, ",\n\t\t");
			}

			flexprint(N->Fe, N->Fm, N->Fperr, "'%s'", gNewtonTokenDescriptions[firstOrFollowsArray[currentlyParsingTokenOrProduction][i]]);
			seen++;
		}
	}
	else if ((expectedProductionOrToken >= kNewtonIrNodeType_TMin) && (expectedProductionOrToken < kNewtonIrNodeType_TMax))
	{
		flexprint(N->Fe, N->Fm, N->Fperr, ":\n\n\t\t");
		flexprint(N->Fe, N->Fm, N->Fperr, "%s", gNewtonTokenDescriptions[expectedProductionOrToken]);
	}
	else
	{
		fatal(N, Esanity);
	}

	flexprint(N->Fe, N->Fm, N->Fperr, ".\n\n\tInstead, saw:\n\n");
	lexPeekPrint(N, 5, 0, gNewtonTokenDescriptions);
	
	newtonParserSyntaxAndSemanticPost(N);
}


void
newtonParserSemanticError(State *  N, IrNodeType currentlyParsingTokenOrProduction, char *  details)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParserSemanticError);
	newtonParserSyntaxAndSemanticPre(N, currentlyParsingTokenOrProduction, EsemanticsA, EsemanticsB, details, EsemanticsD);
	newtonParserSyntaxAndSemanticPost(N);
}


void
newtonParserErrorRecovery(State *  N, IrNodeType expectedProductionOrToken)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParserErrorRecovery);

	if (N->verbosityLevel & kNoisyVerbosityDebugParser)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "In newtonParserErrorRecovery(), about to discard tokens...\n");
	}

	/*
	while (!inFollow(N, expectedProductionOrToken, gNewtonFollows, kNewtonIrNodeTypeMax) && N->tokenList != NULL)
	{
		 *
		 *	Retrieve token and discard...
		 *
		Token *	token = lexGet(N);
		if (N->verbosityLevel & kNewtonVerbosityDebugParser)
		{
			lexDebugPrintToken(N, token);
		}
	}
	*/

	if ((N != NULL) && (N->jmpbufIsValid))
	{
		fprintf(stderr, "doing longjmp");

		/*
		 *	Could pass in case-specific info here, but just
		 *	pass 0.
		 *
		 *	TODO (Jonathan): We could, e.g., return info on which line
		 *	number of the input we have reached, and let, e.g.,
		 *	the CGI version highlight the point at which
		 *	processing stopped.
		 */
		longjmp(N->jmpbuf, 0);
	}

	/*
	 *	Not reached if N->jmpbufIsValid
	 */
	consolePrintBuffers(N);

	exit(EXIT_SUCCESS);
}
