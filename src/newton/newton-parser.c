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
#include "newton-irPass-constantFolding.h"


extern unsigned long int	bigNumberOffset;
extern int			primeNumbers[];
extern const char *		gNewtonTokenDescriptions[kCommonIrNodeTypeMax];
extern const char *		gNewtonAstNodeStrings[kCommonIrNodeTypeMax];
extern const char *		gProductionDescriptions[kCommonIrNodeTypeMax];
extern int			gNewtonFirsts[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax];
extern int			gNewtonFollows[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax];
extern void			fatal(State *  N, const char *  msg);


static char			kNewtonErrorTokenHtmlTagOpen[]	= "<span style=\"background-color:#FFCC00; color:#FF0000;\">";
static char			kNewtonErrorTokenHtmlTagClose[]	= "</span>";
static char			kNewtonErrorDetailHtmlTagOpen[]	= "<span style=\"background-color:#A9E9FF; color:#000000;\">";
static char			kNewtonErrorDetailHtmlTagClose[]= "</span>";

static void			setPhysicsOfBaseNode(State *  N, IrNode *  baseNode, IrNode *  expression);



IrNode *
newtonParse(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParse);

	return newtonParseFile(N, currentScope);
}



/*
 *	Grammar production:
 *
 *		newtonDescription		::=	ruleList .
 */
IrNode *
newtonParseFile(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParseFile);

	IrNode *	node = genIrNode(
						N,
						kNewtonIrNodeType_PnewtonDescription,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	/*
	 *	Before we start parsing, set begin source line of toplevel scope.
	 */
	currentScope->begin = lexPeek(N, 1)->sourceInfo;

	addLeaf(N, node, newtonParseRuleList(N, currentScope));

	if (lexPeek(N, 1)->type != kNewtonIrNodeType_Zeof)
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PnewtonDescription, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnewtonDescription);
	}

	/*
	 *	Skip eof token without using lexGet
	 */
	N->tokenList = N->tokenList->next;

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PnewtonDescription, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PnewtonDescription, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnewtonDescription);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		ruleList			::=	{rule} .
 */
IrNode *
newtonParseRuleList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParseRuleList);

	IrNode *	node = genIrNode(
						N,
						kNewtonIrNodeType_PruleList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (inFirst(N, kNewtonIrNodeType_Prule, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseRule(N, currentScope));
	}

	while (inFirst(N, kNewtonIrNodeType_Prule, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseRule(N, currentScope));
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PruleList, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PruleList, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PruleList);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		rule				::=	constantDefinition | invariantDefinition | baseSignalDefinition | sensorDefinition .
 */
IrNode *
newtonParseRule(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParseRule);

	IrNode *	node;

	currentScope->begin = lexPeek(N, 1)->sourceInfo;

	switch(lexPeek(N, 3)->type)
	{
		case kNewtonIrNodeType_Tsensor:
		{
			node = newtonParseSensorDefinition(N, currentScope);
			break;
		}

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
			newtonParserSyntaxError(N, kNewtonIrNodeType_Prule, kNewtonIrNodeType_Prule, gNewtonFirsts);
			newtonParserErrorRecovery(N, kNewtonIrNodeType_Prule);
		}
	}

	currentScope->end = lexPeek(N, 1)->sourceInfo;

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Prule, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Prule, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Prule);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		constantDefinition		::=	identifier ":" "constant" "=" numericFactor [unitFactor] ";" .
 */
IrNode *
newtonParseConstant(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParseConstant);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PconstantDefinition,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	IrNode *	constantIdentifier = newtonParseIdentifier(N, currentScope);

	addLeaf(N, node, constantIdentifier);
	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tconstant, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tassign, currentScope);

	Physics *	constantPhysics = newtonPhysicsTableAddPhysicsForToken(N, currentScope, constantIdentifier->token);
	IrNode *	constantExpression;

	if (inFirst(N, kNewtonIrNodeType_PnumericFactor, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		constantExpression = newtonParseNumericFactor(N, currentScope);
		constantPhysics->value = constantExpression->value;
		constantPhysics->isConstant = true;
		node->value = constantExpression->value;

		/*
		 *	If LHS is declared a vector in vectorScalarPairScope, then
		 *	the expression must evaluate to a vector.
		 */
		if (constantPhysics->isVector)
		{
			newtonParserSyntaxError(N, kNewtonIrNodeType_PconstantDefinition, kNewtonIrNodeTypeMax, gNewtonFirsts);
			newtonParserErrorRecovery(N, kNewtonIrNodeType_PconstantDefinition);
		}
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PconstantDefinition, kNewtonIrNodeType_PconstantDefinition, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PconstantDefinition);
	}

	if (inFirst(N, kNewtonIrNodeType_PunitFactor, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		IrNode *	unitFactorNode = newtonParseUnitFactor(N, currentScope);

		/*
		 *	The actual `unit` node is in the left child of the unitFactor node
		 */
		newtonPhysicsAddExponentsRecursively(N, constantPhysics, unitFactorNode);
		addLeafWithChainingSeq(N, node, unitFactorNode);
		node->irLeftChild->physics = deepCopyPhysicsNode(N, constantPhysics);
	}
	else
	{
		/*
		 *	The unitExpression is optional, so if its missing, that's OK, do nothing.
		 */
	}

	newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

	if ((node->irLeftChild->physics != NULL) && (N->verbosityLevel & kCommonVerbosityDebugParser))
	{
		flexprint(N->Fe, N->Fm, N->Fpinfo, "\tConstant identifier \"%s\"\n", node->irLeftChild->physics->identifier);
		printDimensionsOfNode(N, node->irLeftChild, N->Fpinfo);
		flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PconstantDefinition, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PconstantDefinition, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PconstantDefinition);
	}
	*/

  /*
	Dimension * tmpDimensionsNode;
	if (node->irLeftChild->physics != NULL)
  {
		flexprint(N->Fe, N->Fm, N->Fpinfo, "\t(Constant identifier is %s)\n", node->irLeftChild->physics->identifier);

		for (tmpDimensionsNode = node->irLeftChild->physics->dimensions; tmpDimensionsNode != NULL; tmpDimensionsNode = tmpDimensionsNode->next)
    {
			flexprint(N->Fe, N->Fm, N->Fpinfo, "\t(Unit is %s with exponent %f)\n", tmpDimensionsNode->name, tmpDimensionsNode->exponent);
		}
	}
	flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
*/

	return node;
}



/*
 *	Grammar production:
 *
 *		invariantDefinition		::=	identifier ":" "invariant" parameterTuple  "=" "{" [constraintList] "}" .
 */
IrNode *
newtonParseInvariant(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParseInvariant);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PinvariantDefinition,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	Invariant *	invariant	= (Invariant *) calloc(1, sizeof(Invariant));
	IrNode *	invariantName	= newtonParseIdentifier(N, currentScope);
	invariantName->irParent = node;

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
	newScope->scopeParameterList = invariant->parameterList;

	/*
	 *	Also add it to the AST
	 */
	addLeafWithChainingSeq(N, node, invariant->parameterList);

	newtonParseTerminal(N, kNewtonIrNodeType_Tassign, newScope);
	newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, newScope);

	if (inFirst(N, kNewtonIrNodeType_Pconstraint, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		invariant->constraints		= newtonParseConstraintList(N, newScope);

		/*
		 *	Also add it to the AST
		 */
		addLeafWithChainingSeq(N, node, invariant->constraints);

		invariant->id			= newtonGetInvariantIdByParameters(N, invariant->parameterList, 1);
	}

	IrNode *	scopeEnd	= newtonParseTerminal(N, kNewtonIrNodeType_TrightBrace, newScope);
	commonSymbolTableCloseScope(N, newScope, scopeEnd);
	newtonAddInvariant(N, invariant);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PinvariantDefinition, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PinvariantDefinition, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PinvariantDefinition);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		subdimensionTuple		::=	"(" identifier ":" numericExpression "to" numericExpression ")" .
 */
IrNode *
newtonParseSubindexTuple(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParseSubindexTuple);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PsubdimensionTuple,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
	addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);

	node->subindexStart = newtonParseNumericExpression(N, currentScope)->value;
	newtonParseTerminal(N, kNewtonIrNodeType_Tto, currentScope);
	node->subindexEnd = newtonParseNumericExpression(N, currentScope)->value;

	if (node->subindexEnd <= 0)
	{
		char *	details;

		asprintf(&details, "%s\n", EsubindexEndMustBeNaturalnumber);
		newtonParserSemanticError(N, kNewtonIrNodeType_PsubdimensionTuple, details);
		free(details);

		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsubdimensionTuple);
	}

	newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PsubdimensionTuple, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PsubdimensionTuple, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsubdimensionTuple);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		parameterTuple			::=	"(" parameter {"," parameter} ")" .
 */
IrNode *
newtonParseParameterTuple(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParseParameterTuple);

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



/*
 *	Grammar production:
 *
 *		parameter			::=	identifier ":" (identifier | numericExpression) {"[" integerConst "]"} .
 */
IrNode *
newtonParseParameter(State *  N, Scope *  currentScope, int parameterNumber)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParseParameter);

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

	Signal * signal = (Signal *) calloc(1, sizeof(Signal));
	signal->axis = 0;

	if (peekCheck(N, 1, kNewtonIrNodeType_Tidentifier))
	{
		IrNode *	physicsName = newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
		addLeaf(N, node, physicsName);

		if (lexPeek(N, 1)->type == kNewtonIrNodeType_TleftBracket)
		{
			// newtonParseTerminal(N, kNewtonIrNodeType_TatSign, currentScope);
			newtonParseTerminal(N, kNewtonIrNodeType_TleftBracket, currentScope);
			IrNode * integerConst = newtonParseTerminal(N, kNewtonIrNodeType_TintegerConst, currentScope);
			signal->axis = integerConst->value;

			newtonParseResetPhysicsWithCorrectSubindex(
					N,
					physicsName,
					currentScope,
					physicsName->token->identifier,
					integerConst->value
					);

			newtonParseTerminal(N, kNewtonIrNodeType_TrightBracket, currentScope);
		}


			node->physics = physicsName->physics;
			node->signal = signal;
	}
	else
	{
		addLeaf(N, node, newtonParseNumericExpression(N, currentScope));
	}

	node->parameterNumber = parameterNumber;

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



/*
 *	Grammar production: (***NOTE***: Since it doesn't make sense to add/subtract units (only to multiply or divide them), unitExpression degenerates to unitTerm.)
 *
 *		unitExpression			::=	unitTerm .
 */
IrNode *
newtonParseUnitExpression(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParseUnitExpression);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PunitExpression,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	addLeaf(N, node, newtonParseUnitTerm(N, currentScope));

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PunitExpression, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PunitExpression, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PunitExpression);
	}
	*/

	return node;
}



/*
 *	Grammar production: (**NOTE** Here, we are not really permitting all highPrecedenceQuantityOperator, only * and /, but for naming sake it makes it easier to think about)
 *
 *		unitTerm			::=	unitFactor {highPrecedenceQuantityOperator unitFactor} .
 */
IrNode *
newtonParseUnitTerm(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParseUnitTerm);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PunitTerm,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	addLeaf(N, node, newtonParseUnitFactor(N, currentScope));

	while (inFirst(N, kNewtonIrNodeType_PhighPrecedenceQuantityOperator, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseHighPrecedenceQuantityOperator(N, currentScope));
		addLeafWithChainingSeq(N, node, newtonParseUnitFactor(N, currentScope));
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PunitTerm, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PunitTerm, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PunitTerm);
	}
	*/

	return node;
}



static void
setPhysicsOfBaseNode(State *  N, IrNode *  baseNode, IrNode *  exponent)
{
	TimeStampTraceMacro(kNewtonTimeStampKeySetPhysicsOfBaseNode);

	/*
	 *	This is not necessary, but keeping it since an earlier implementation
	 *	(erroneously / pointlessly) set the value of the quantityExpression
	 *	to the value of its exponent. TODO: eventually discard:
	 */
	baseNode->value = exponent->value;

//fprintf(stderr, "\tin setPhysicsOfBaseNode(), baseNode->value = [%f],  exponent->value = [%f]\n", baseNode->value, exponent->value);
	/*
	 *	If the base is a Physics quantity, we used to check that the
	 *	exponent must be an integer. We no longer restrict this.
	 *	One use case is noise for many sensors (e.g., accelerometers)
	 *	which is derivation = 1E-6 * (acceleration / (frequency ** 0.5));
	 */
	if (!newtonIsDimensionless(N, baseNode->physics))
	{
		/*
		 *	TODO: get rid of this commented block during a future cleanup.
		 */
		/*
		if (exponent->value != (int)exponent->value)
		{
			char *	details;

			asprintf(&details, "%s\n", EraisingPhysicsQuantityToNonIntegerExponent);
			newtonParserSemanticError(N, kNewtonIrNodeType_PquantityExpression, details);
			free(details);

			newtonParserErrorRecovery(N, kNewtonIrNodeType_PquantityExpression);
		}
		*/

		baseNode->physics->value = pow(baseNode->physics->value, exponent->value);
		newtonPhysicsMultiplyExponents(N, baseNode->physics, exponent->value);
	}

	return;
}



/*
 *	Grammar production:
 *
 *		unitFactor			::=	(unit [exponentiationOperator numericFactor]) | "(" unitExpression ")" .
 */
IrNode *
newtonParseUnitFactor(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParseUnitFactor);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PunitFactor,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (inFirst(N, kNewtonIrNodeType_Punit, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		IrNode *	unitNode = newtonParseUnit(N, currentScope);
		addLeaf(N, node, unitNode);

		if (inFirst(N, kNewtonIrNodeType_PexponentiationOperator, gNewtonFirsts, kNewtonIrNodeTypeMax))
		{
			addLeafWithChainingSeq(N, node, newtonParseExponentiationOperator(N, currentScope));
			IrNode *	exponentValue = newtonParseNumericFactor(N, currentScope);
			addLeafWithChainingSeq(N, node, exponentValue);
			setPhysicsOfBaseNode(N, unitNode, exponentValue);
		}
	}
	else if (lexPeek(N, 1)->type == kNewtonIrNodeType_TleftParen)
	{
		newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
		addLeaf(N, node, newtonParseUnitExpression(N, currentScope));
		newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PunitFactor, kNewtonIrNodeType_PunitFactor, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PunitFactor);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PunitFactor, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PunitFactor, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PunitFactor);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		unit				::=	identifier .
 */
IrNode *
newtonParseUnit(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParseUnit);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Punit,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	node->physics = newtonInitPhysics(N, currentScope, NULL);
	addLeaf(N, node, newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));

	/*
	 *	Propagate the physics to the unit node
	 */
	newtonPhysicsAddExponents(N, node->physics, node->irLeftChild->physics);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Punit, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Punit, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Punit);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		numericExpression			::=	numericTerm {lowPrecedenceOperator numericTerm} .
 */
IrNode *
newtonParseNumericExpression(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PnumericExpression,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	IrNode *	firstChildTerm = newtonParseNumericTerm(N, currentScope);
	addLeaf(N, node, firstChildTerm);
	node->value = firstChildTerm->value;

	/*
	 *	We have to check whether the token after the operator is a numeric const.
	 *	If it is not, we should not eat the operator here, but rather end processing.
	 */
	//xxx TODO xxx

	while (inFirst(N, kNewtonIrNodeType_PlowPrecedenceOperator, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseLowPrecedenceOperator(N, currentScope));
		addLeafWithChainingSeq(N, node, newtonParseNumericTerm(N, currentScope));
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PnumericExpression, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PnumericExpression, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnumericExpression);
	}
	*/

	/*
	 *	Preemptively call constant-folding routine on the subtree to set the
	 *	root node ->value field, since physics annotation is currently
	 *	(unfortunately) wired into the parse phase.
	 */
	node->value = newtonIrPassConstantFoldingSubtreeEvaluate(N, node);
//fprintf(stderr, "In newtonParseNumericExpression(), newtonIrPassConstantFoldingSubtreeEvaluate() returned [%f]\n", node->value);

	return node;
}



/*
 *	Grammar production:
 *
 *		numericTerm			::=	numericFactor {highPrecedenceOperator numericFactor} .
 */
IrNode *
newtonParseNumericTerm(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PnumericTerm,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	IrNode *	firstChildFactor = newtonParseNumericFactor(N, currentScope);
	addLeaf(N, node, firstChildFactor);
	node->value = firstChildFactor->value;

	/*
	 *	We have to check whether the token after the operator is a numeric const.
	 *	If it is not, we should not eat the operator here, but rather end processing.
	 */
	//xxx TODO xxx

	while (inFirst(N, kNewtonIrNodeType_PhighPrecedenceQuantityOperator, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseHighPrecedenceQuantityOperator(N, currentScope));
		addLeafWithChainingSeq(N, node, newtonParseNumericFactor(N, currentScope));
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PnumericTerm, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PnumericTerm, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnumericTerm);
	}
	*/


	/*
	 *	Preemptively call constant-folding routine on the subtree to set the
	 *	root node ->value field, since physics annotation is currently
	 *	(unfortunately) wired into the parse phase.
	 */
	node->value = newtonIrPassConstantFoldingSubtreeEvaluate(N, node);
//fprintf(stderr, "In newtonParseNumericTerm(), newtonIrPassConstantFoldingSubtreeEvaluate() returned [%f]\n", node->value);

	return node;
}



/*
 *	Grammar production:
 *
 *		numericFactor			::=	(numericConst [exponentiationOperator numericConst]) | "(" numericExpression ")" .
 */
IrNode *
newtonParseNumericFactor(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PnumericFactor,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (inFirst(N, kNewtonIrNodeType_PnumericConst, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		IrNode *	constChildNode = newtonParseNumericConst(N, currentScope);
		addLeaf(N, node, constChildNode);
		node->value = constChildNode->value;
		node->integerValue = constChildNode->integerValue;

		/*
		 *	We have to check whether the token after the operator is a numeric const.
		 *	If it is not, we should not eat the operator here, but rather end processing.
		 */
		//xxx TODO xxx

		if (inFirst(N, kNewtonIrNodeType_PexponentiationOperator, gNewtonFirsts, kNewtonIrNodeTypeMax))
		{
			addLeafWithChainingSeq(N, node, newtonParseExponentiationOperator(N, currentScope));
			addLeafWithChainingSeq(N, node, newtonParseNumericConst(N, currentScope));
		}

	}
	else if (lexPeek(N, 1)->type == kNewtonIrNodeType_TleftParen)
	{
		newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
		addLeaf(N, node, newtonParseNumericExpression(N, currentScope));
		newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PnumericFactor, kNewtonIrNodeType_PnumericFactor, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnumericFactor);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PnumericFactor, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PnumericFactor, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnumericFactor);
	}
	*/

	/*
	 *	Preemptively call constant-folding routine on the subtree to set the
	 *	root node ->value field, since physics annotation is currently
	 *	(unfortunately) wired into the parse phase.
	 */
	node->value = newtonIrPassConstantFoldingSubtreeEvaluate(N, node);
//fprintf(stderr, "In newtonParseNumericFactor(), newtonIrPassConstantFoldingSubtreeEvaluate() returned [%f]\n", node->value);

	return node;
}



/*
 *	Grammar production:
 *
 *		baseSignalDefinition		::=	identifier ":" "signal" [subdimensionTuple] "=" "{" [nameStatement] symbolStatement derivationStatement "}" .
 */
IrNode *
newtonParseBaseSignal(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PbaseSignalDefinition,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	IrNode *	basicPhysicsIdentifier = newtonParseIdentifier(N, currentScope);
	addLeaf(N, node, basicPhysicsIdentifier);
	Physics *	newPhysics = newtonPhysicsTableAddPhysicsForToken(N, currentScope, basicPhysicsIdentifier->token);

	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tsignal, currentScope);

	int		subindexStart = 0;
	int		subindexEnd = 0;
	IrNode *	subindexNode;
	if (lexPeek(N, 5)->type == kNewtonIrNodeType_Tto)
	{
		subindexNode = newtonParseSubindexTuple(N, currentScope);
		subindexStart = subindexNode->subindexStart;
		subindexEnd = subindexNode->subindexEnd;
	}

	newtonParseTerminal(N, kNewtonIrNodeType_Tassign, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, currentScope);

	/*
	 *	Name syntax is optional
	 */
	IrNode *	unitName = NULL;
	if (inFirst(N, kNewtonIrNodeType_PnameStatement, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		unitName = newtonParseName(N, currentScope);
		addLeafWithChainingSeq(N, node, unitName);
		newPhysics->dimensionAlias = unitName->token->stringConst; /* e.g., meter, Pascal*/
	}

	/*
	 *	Uncertainty syntax is optional
	 */
	IrNode *	signalUncertainty = NULL;
	if (inFirst(N, kNewtonIrNodeType_PsignalUncertaintyStatement, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		signalUncertainty = newtonParseSignalUncertainty(N, currentScope);
		addLeafWithChainingSeq(N, node, signalUncertainty);
		/*
		 *	Based on the implementation of commonSymbolTableOpenScope()
		 *	the most recent subscope is in firstChild. 
		 *	Not sure why this is the behaviour, though. -- Orestis
		 */
		newPhysics->uncertaintyScope = currentScope->firstChild;
	}

	/*
	 *	Sensor syntax is optional
	 */
	IrNode *	signalSensor = NULL;
	if (inFirst(N, kNewtonIrNodeType_PsensorStatement, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		signalSensor = newtonParseSignalSensor(N, currentScope);
		addLeafWithChainingSeq(N, node, signalSensor);
	}

	/*
	 *	Abbreviation syntax is also optional // TODO: It is appear optional in the newton.grammar
	 */
	IrNode *	unitAbbreviation = NULL;
	if (inFirst(N, kNewtonIrNodeType_PsymbolStatement, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		unitAbbreviation = newtonParseSymbol(N, currentScope);
		addLeafWithChainingSeq(N, node, unitAbbreviation);

		/*
		 *	e.g., m, Pa
		 */
		newPhysics->dimensionAliasAbbreviation = unitAbbreviation->token->identifier;
	}

	/*
	 *	Derivation syntax is required. The `derivationExpression` is the irLeftChild of
	 *	the node returned from newtonParseDerivation() and will be a PquantityExpression.
	 */
	IrNode *	derivationExpression = newtonParseDerivation(N, currentScope)->irLeftChild;
	addLeafWithChainingSeq(N, node, derivationExpression);

	if (derivationExpression->type != kNewtonIrNodeType_Tnone)
	{
//fprintf(stderr, "in newtonParseBaseSignal(), about to set physics...\n");
		newtonPhysicsAddExponents(N, newPhysics, derivationExpression->physics);

/*
fprintf(stderr, "*1 In newtonParseBaseSignal for [%s], newPhysics dimensions exponents are:\n", basicPhysicsIdentifier->tokenString);
Dimension *p = newPhysics->dimensions;
while (p != NULL)
{
	fprintf(stderr, "\tdim name [%s], dim abbrv [%s], dim exponent [%f]\n", p->name, p->abbreviation, p->exponent);
	p = p->next;
}
*/
/*
fprintf(stderr, "*2 In newtonParseBaseSignal for [%s], derivationExpression->physics dimensions exponents are:\n", basicPhysicsIdentifier->tokenString);
p = derivationExpression->physics->dimensions;
while (p != NULL)
{
	fprintf(stderr, "\tdim name [%s], dim abbrv [%s], dim exponent [%f]\n", p->name, p->abbreviation, p->exponent);
	p = p->next;
}
*/
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
	if (!inFollow(N, kNewtonIrNodeType_PbaseSignalDefinition, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PbaseSignalDefinition, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PbaseSignalDefinition);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		nameStatement			::=	"name" "=" stringConst languageSetting ";" .
 */
IrNode *
newtonParseName(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PnameStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tname, currentScope));
	addLeafWithChainingSeq(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tassign, currentScope));

	IrNode *	baseSignalName = newtonParseTerminal(N, kNewtonIrNodeType_TstringConst, currentScope);
	node->token = baseSignalName->token;

	addLeafWithChainingSeq(N, node, baseSignalName);
	addLeafWithChainingSeq(N, node, newtonParseLanguageSetting(N, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PnameStatement, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PnameStatement, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnameStatement);
	}
	*/

	return node;
}


/*
 *	Grammar production:
 *
 *		signalUncertaintyStatement	::= "uncertainty" "=" distribution [parameterTuple] ";" .
 */
IrNode *
newtonParseSignalUncertainty(State * N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PsignalUncertaintyStatement,
					NULL /* left child */,
					NULL /* right child */,
					lexPeek(N, 1)->sourceInfo /* source info */
				);

	addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tuncertainty, currentScope));
	addLeafWithChainingSeq(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tassign, currentScope));

	IrNode *	scopeBegin = newtonParseDistribution(N, currentScope);
	Scope *	newScope = commonSymbolTableOpenScope(N, currentScope, scopeBegin);

	addLeafWithChainingSeq(N, node, scopeBegin);

	if (inFirst(N, kNewtonIrNodeType_PparameterTuple, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseParameterTuple(N, newScope));
	}

	IrNode *	scopeEnd = newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, newScope);
	commonSymbolTableCloseScope(N, newScope, scopeEnd);

	return node;
}


/*
 *	Grammar production:
 *
 *		sensorStatement			::=	"sensor" "=" identifier ";" .
 */
IrNode *
newtonParseSignalSensor(State * N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PsensorStatement,
					NULL /* left child */,
					NULL /* right child */,
					lexPeek(N, 1)->sourceInfo /* source info */
				);

	addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tsensor, currentScope));
	addLeafWithChainingSeq(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tassign, currentScope));
	addLeafWithChainingSeq(N, node, newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

	return node;
}

/*
 *	Grammar production:
 *
 *		languageSetting			::=	"English" .
 */
IrNode *
newtonParseLanguageSetting(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PlanguageSetting,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (	lexPeek(N, 1)->type == kNewtonIrNodeType_TEnglish)
	{
		addLeaf(N, node, newtonParseTerminal(N, lexPeek(N, 1)->type, currentScope));
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PlanguageSetting, kNewtonIrNodeTypeMax, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PlanguageSetting);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PlanguageSetting, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PlanguageSetting, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PlanguageSetting);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		symbolStatement			::=	"symbol" "=" identifier ";" .
 */
IrNode *
newtonParseSymbol(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PsymbolStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_Tsymbol, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tassign, currentScope);

	IrNode *	baseSignalAbbreviation = newtonParseIdentifierDefinitionTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
	node->token = baseSignalAbbreviation->token;

	addLeaf(N, node, baseSignalAbbreviation);
	newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PsymbolStatement, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PsymbolStatement, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsymbolStatement);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		derivationStatement		::=	"derivation" "=" ("none" | "dimensionless" | quantityExpression) ";" .
 */
IrNode *
newtonParseDerivation(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PderivationStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

	newtonParseTerminal(N, kNewtonIrNodeType_Tderivation, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tassign, currentScope);

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
	if (!inFollow(N, kNewtonIrNodeType_PderivationStatement, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PderivationStatement, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PderivationStatement);
	}
	*/

	return node;
}



/*
 *	Simply remove a terminal
 */
IrNode *
newtonParseTerminal(State *  N, IrNodeType expectedType, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

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

	if (t->type == kNewtonIrNodeType_TintegerConst)
	{
		n->integerValue = t->integerConst;
		n->value = (double) t->integerConst;
	}

	if (t->type == kNewtonIrNodeType_TrealConst)
	{
		n->value = t->realConst;
	}

	return n;
}



IrNode *
newtonParseIdentifier(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	n;

	if (peekCheck(N, 1, kNewtonIrNodeType_Tidentifier))
	{
		n = newtonParseIdentifierDefinitionTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
		n->symbol->typeTree = n;
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





/*
 *	Grammar production:
 *
 *		quantityExpression		::=	quantityTerm {lowPrecedenceOperator quantityTerm} .
 */
IrNode *
newtonParseQuantityExpression(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParseQuantityExpression);

	IrNode *	expression = genIrNode(N,	kNewtonIrNodeType_PquantityExpression,
							NULL /* left child */,
							NULL /* right child */,
							lexPeek(N, 1)->sourceInfo /* source info */);

//xxx re-check this. the dimensons are not getting set and parseQuantity* seems to be at fault...

	expression->physics = newtonInitPhysics(N, currentScope, NULL);

	if (inFirst(N, kNewtonIrNodeType_PquantityTerm, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		IrNode *	leftTerm = newtonParseQuantityTerm(N, currentScope);
		expression->value = leftTerm->value;
		expression->physics = leftTerm->physics;
		addLeaf(N, expression, leftTerm);

		while (inFirst(N, kNewtonIrNodeType_PlowPrecedenceOperator, gNewtonFirsts, kNewtonIrNodeTypeMax))
		{
			IrNode *	operatorProductionNode = newtonParseLowPrecedenceOperator(N, currentScope);
			addLeafWithChainingSeq(N, expression, operatorProductionNode);

			/*
			 *	Since the actual operator type node is somewhere in the left of the subtree, grab it
			 */
			IrNodeType	operatorType = getTypeFromOperatorSubtree(N, operatorProductionNode);

			IrNode *	rightTerm = newtonParseQuantityTerm(N, currentScope);
			addLeafWithChainingSeq(N, expression, rightTerm);

			if (operatorType == kNewtonIrNodeType_Tplus)
			{
				expression->value += rightTerm->value;
			}
			else if (operatorType == kNewtonIrNodeType_Tminus)
			{
				expression->value -= rightTerm->value;
			}
			else
			{
				newtonParserSyntaxError(N, kNewtonIrNodeType_PlowPrecedenceOperator, kNewtonIrNodeType_PlowPrecedenceOperator, gNewtonFirsts);
				newtonParserErrorRecovery(N, kNewtonIrNodeType_PlowPrecedenceOperator);
			}

			if(!areTwoPhysicsEquivalent(N, leftTerm->physics, rightTerm->physics))
			{
				flexprint(N->Fe, N->Fm, N->Fperr, "LHS:\n");
				printDimensionsOfNode(N, leftTerm, N->Fperr);
				flexprint(N->Fe, N->Fm, N->Fperr, "RHS:\n");
				printDimensionsOfNode(N, rightTerm, N->Fperr);

				newtonParserSemanticError(N, kNewtonIrNodeType_PlowPrecedenceOperator, (char *)EexpressionPhysicsMismatch);
				newtonParserErrorRecovery(N, kNewtonIrNodeType_PlowPrecedenceOperator);
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



/*
 *	Grammar production:
 *
 *		quantityTerm			::=	[unaryOp] quantityFactor {highPrecedenceQuantityOperator quantityFactor} .
 */
IrNode *
newtonParseQuantityTerm(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	bool		hasUnary = false;
	bool		hasNumberInTerm = false;
	int		numVectorsInTerm = 0;

//xxx re-check this. the dimensons are not getting set and parseQuantity* seems to be at fault...

	IrNode *	unaryNode;
	IrNode *	intermediate = genIrNode(N,	kNewtonIrNodeType_PquantityTerm,
							NULL /* left child */,
							NULL /* right child */,
							lexPeek(N, 1)->sourceInfo /* source info */);

	intermediate->physics = newtonInitPhysics(N, currentScope, NULL);

	if (inFirst(N, kNewtonIrNodeType_PunaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		unaryNode = newtonParseUnaryOp(N, currentScope);
		addLeaf(N, intermediate, unaryNode);
		hasUnary = true;
	}

	IrNode *	leftFactor = newtonParseQuantityFactor(N, currentScope);
	addLeafWithChainingSeq(N, intermediate, leftFactor);

	hasNumberInTerm = (leftFactor->physics == NULL) || leftFactor->physics->isConstant;
	if (hasNumberInTerm)
	{
		if (hasUnary && (unaryNode->type == kNewtonIrNodeType_Tminus))
		{
			intermediate->value = leftFactor->value * -1;
		}
		else
		{
			intermediate->value = leftFactor->value;
		}
	}

	if (!newtonIsDimensionless(N, leftFactor->physics))
	{
		assert(leftFactor->physics != NULL);
//fprintf(stderr, "in newtonParseQuantityTerm() case 1, about to set physics...\n");
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

	while (inFirst(N, kNewtonIrNodeType_PhighPrecedenceQuantityOperator, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		IrNode *	operatorProductionNode = newtonParseHighPrecedenceQuantityOperator(N, currentScope);
		addLeafWithChainingSeq(N, intermediate, operatorProductionNode);

		/*
		 *	Since the actual operator type node is somewhere in the left of the subtree, grab it
		 */
		IrNodeType	operatorType = getTypeFromOperatorSubtree(N, operatorProductionNode);

		rightFactor = newtonParseQuantityFactor(N, currentScope);

		addLeafWithChainingSeq(N, intermediate, rightFactor);
		hasNumberInTerm = hasNumberInTerm || leftFactor->physics == NULL || leftFactor->physics->isConstant;

		if (hasNumberInTerm)
		{
			if (operatorType == kNewtonIrNodeType_Tmul)
			{
				intermediate->value = (
							rightFactor->value == 0 		?
							intermediate->value			:
							intermediate->value*rightFactor->value
						);
			}
			else if (operatorType == kNewtonIrNodeType_Tdiv)
			{
				intermediate->value = (
							rightFactor->value == 0			?
							intermediate->value			:
							intermediate->value/rightFactor->value
						);
			}
		}

		if (!newtonIsDimensionless(N, rightFactor->physics) && rightFactor->physics->isVector)
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

		if (!newtonIsDimensionless(N, rightFactor->physics) && (operatorType == kNewtonIrNodeType_Tmul))
		{
//fprintf(stderr, "in newtonParseCQuantityTerm() case 2, about to set physics...\n");
			newtonPhysicsAddExponents(N, intermediate->physics, rightFactor->physics);
		}
		else if (!newtonIsDimensionless(N, rightFactor->physics) && (operatorType == kNewtonIrNodeType_Tdiv))
		{
			newtonPhysicsSubtractExponents(N, intermediate->physics, rightFactor->physics);
		}
		else
		{
			//TODO: check: is this option valid?!
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



/*
 *	Grammar production:
 *
 *		quantityFactor			::=	quantity [exponentiationOperator numericFactor]			|
 *							functionalOperator {functionalOperator} quantityFactor quantityFactor	|
 *							distribution "(" quantityExpression {"," quantityExpression} ")" 		|
 *							"(" quantityExpression ")" [exponentiationOperator numericFactor]	|
 *							"{" quantityExpression {"," quantityExpression} "}" .
 */
IrNode *
newtonParseQuantityFactor(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	intermediate = genIrNode(N, kNewtonIrNodeType_PquantityFactor,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

//xxx re-check this. the dimensons are not getting set and parseQuantity* seems to be at fault...

	if (inFirst(N, kNewtonIrNodeType_Pquantity, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		IrNode *	subnode = newtonParseQuantity(N, currentScope);

		addLeaf(N, intermediate, subnode);

		if (inFirst(N, kNewtonIrNodeType_PexponentiationOperator, gNewtonFirsts, kNewtonIrNodeTypeMax))
		{
			addLeafWithChainingSeq(N, intermediate, newtonParseExponentiationOperator(N, currentScope));

			IrNode *	exponentValue = newtonParseNumericFactor(N, currentScope);

			addLeafWithChainingSeq(N, intermediate, exponentValue);
			setPhysicsOfBaseNode(N, subnode, exponentValue);
		}

		intermediate->value = subnode->value;
		intermediate->physics = subnode->physics;
	}
	else if (inFirst(N, kNewtonIrNodeType_PfunctionalOperator, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		/*
		 *	NOTE: At least one timeOperator, followed by zero or more.
		 */
		addLeafWithChainingSeq(N, intermediate, newtonParseFunctionalOperator(N, currentScope));

		while (inFirst(N, kNewtonIrNodeType_PfunctionalOperator, gNewtonFirsts, kNewtonIrNodeTypeMax))
		{
			addLeafWithChainingSeq(N, intermediate, newtonParseFunctionalOperator(N, currentScope));
		}

		addLeafWithChainingSeq(N, intermediate, newtonParseQuantityFactor(N, currentScope));
		addLeafWithChainingSeq(N, intermediate, newtonParseQuantityFactor(N, currentScope));

		/*
		 *	Here, we don't set the physics since we'd have to compute the
		 *	derivative or integral of the physics dimension. In any case,
		 *	all the setting of physics should not have been done while parsing,
		 *	but rather in a subsequent tree-annotation pass.
		 *
		 *	Created GitHub issue to track moving all setting of physics from
		 *	parse-time to a dedicated to physics tree-annotation pass, #402.
		 */
	}
	else if (inFirst(N, kNewtonIrNodeType_Pdistribution, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, intermediate, newtonParseDistribution(N, currentScope));

		newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
		addLeafWithChainingSeq(N, intermediate, newtonParseQuantityExpression(N, currentScope));
		while (peekCheck(N, 1, kNewtonIrNodeType_Tcomma))
		{
			newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
			addLeafWithChainingSeq(N, intermediate, newtonParseQuantityExpression(N, currentScope));
		}
		newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);

	}
	else if (inFirst(N, kNewtonIrNodeType_Ptranscendental, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, intermediate, newtonParseTranscendental(N, currentScope));
		newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
		addLeafWithChainingSeq(N, intermediate, newtonParseQuantityExpression(N, currentScope));
		newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TleftParen))
	{
		newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
		IrNode *	subnode = newtonParseQuantityExpression(N, currentScope);
		addLeafWithChainingSeq(N, intermediate, subnode);
		newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);

		if (inFirst(N, kNewtonIrNodeType_PexponentiationOperator, gNewtonFirsts, kNewtonIrNodeTypeMax))
		{
			addLeafWithChainingSeq(N, intermediate, newtonParseExponentiationOperator(N, currentScope));

			IrNode *	exponentValue = newtonParseNumericFactor(N, currentScope);

			addLeafWithChainingSeq(N, intermediate, exponentValue);
			setPhysicsOfBaseNode(N, subnode, exponentValue);
		}

		intermediate->value = subnode->value;
		intermediate->physics = subnode->physics;
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TleftBrace))
	{
		newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, currentScope);
		addLeafWithChainingSeq(N, intermediate, newtonParseQuantityExpression(N, currentScope));

		while (peekCheck(N, 1, kNewtonIrNodeType_Tcomma))
		{
			newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
			addLeafWithChainingSeq(N, intermediate, newtonParseQuantityExpression(N, currentScope));
		}
		newtonParseTerminal(N, kNewtonIrNodeType_TrightBrace, currentScope);

		/*
		 *	Here again, we don't set the physics since this construct is used for
		 *	a set of possibly-differently-dimensioned values (in the `<->` construct).
		 *
		 *	Created GitHub issue to track moving all setting of physics from
		 *	parse-time to a dedicated to physics tree-annotation pass, #402.
		 */
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PquantityFactor, kNewtonIrNodeType_PquantityFactor, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PquantityFactor);
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



/*
 *	Grammar production:
 *
 *		quantity			::=	numericConst | (identifier ["[" numericFactor "]"]) .
 */
IrNode *
newtonParseQuantity(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	intermediate = genIrNode(N, kNewtonIrNodeType_Pquantity,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

	if (inFirst(N, kNewtonIrNodeType_PnumericConst, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, intermediate, newtonParseNumericConst(N, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tidentifier))
	{
		IrNode *	identifierNode = newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);

		addLeafWithChainingSeq(N, intermediate, identifierNode);

		/*
		 *	TODO: This is odd. This is carried over from Jonathan's implementation. A deep copy from self to self makes no sense, and copying the physics->value makes no sense either. Check and remove in future --- PSM.
		 */
		identifierNode->physics = deepCopyPhysicsNode(N, identifierNode->physics);
		identifierNode->value = identifierNode->physics->value;

		/*
		 *	TODO: This is carried over from Jonathan's implementation. Replace with a non-fatal graceful exit.
		 */
		assert(identifierNode->tokenString != NULL);

		if (peekCheck(N, 1, kNewtonIrNodeType_TleftBracket))
		{
			// newtonParseTerminal(N, kNewtonIrNodeType_TatSign, currentScope);
			newtonParseTerminal(N, kNewtonIrNodeType_TleftBracket, currentScope);
			newtonParseTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
			newtonParseResetPhysicsWithCorrectSubindex(
				N,
				identifierNode,
				currentScope,
				identifierNode->token->identifier,
				currentScope->currentSubindex);

			newtonParseTerminal(N, kNewtonIrNodeType_TrightBracket, currentScope);
		}

		/*
		 *	TODO: This is carried over from Jonathan's implementation:
		 *		Check if there's a matchable parameter corresponding the invariant parameter.
		 */
		if (!newtonIsDimensionless(N, identifierNode->physics) &&
			!identifierNode->physics->isConstant &&
			newtonPhysicsTablePhysicsForDimensionAliasAbbreviation(N, N->newtonIrTopScope, identifierNode->tokenString) == NULL &&
			newtonPhysicsTablePhysicsForDimensionAlias(N, N->newtonIrTopScope, identifierNode->tokenString) == NULL &&
			currentScope->scopeParameterList)
		{
			IrNode *	matchingParameter = newtonParseFindParameterByTokenString(N,
													currentScope->scopeParameterList,
													identifierNode->token->identifier
												);
			identifierNode->parameterNumber = matchingParameter->parameterNumber;
		}

		intermediate->physics = identifierNode->physics;
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pquantity, kNewtonIrNodeType_Pquantity, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pquantity);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Pquantity, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pquantity, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pquantity);
	}
	*/

	return intermediate;
}



/*
 *	Grammar production:
 *
 *		functionalOperator		::=	"derivative" | "integral" .
 */
IrNode *
newtonParseFunctionalOperator(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PfunctionalOperator,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (peekCheck(N, 1, kNewtonIrNodeType_Tderivative))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tderivative, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tintegral))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tintegral, currentScope));
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PfunctionalOperator, kNewtonIrNodeType_PfunctionalOperator, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PfunctionalOperator);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PfunctionalOperator, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PfunctionalOperator, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PfunctionalOperator);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		vectorOp			::=	"dot" | "cross" .
 */
IrNode *
newtonParseVectorOp(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PvectorOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (peekCheck(N, 1, kNewtonIrNodeType_Tdot))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tdot, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tcross))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tcross, currentScope));
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PvectorOp, kNewtonIrNodeType_PvectorOp, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PvectorOp);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PvectorOp, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PvectorOp, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PvectorOp);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		exponentiationOperator		::=	"**" .
 */
IrNode *
newtonParseExponentiationOperator(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PexponentiationOperator,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (peekCheck(N, 1, kNewtonIrNodeType_Texponentiation))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Texponentiation, currentScope));
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PexponentiationOperator, kNewtonIrNodeType_PexponentiationOperator, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PexponentiationOperator);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PexponentiationOperator, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PexponentiationOperator, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PexponentiationOperator);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		lowPrecedenceBinaryOp		::=	"+" | "-" | ">>" | "<<" | "|" .
 */
IrNode *
newtonParseLowPrecedenceBinaryOp(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PlowPrecedenceBinaryOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (peekCheck(N, 1, kNewtonIrNodeType_Tplus))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tplus, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tminus))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tminus, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TrightShift))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TrightShift, currentScope));
	}

	else if (peekCheck(N, 1, kNewtonIrNodeType_TleftShift))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TleftShift, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TbitwiseOr))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TbitwiseOr, currentScope));
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

	return node;
}



/*
 *	Grammar production:
 *
 *		unaryOp				::=	"-"  | "+" .
 */
IrNode *
newtonParseUnaryOp(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PunaryOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (peekCheck(N, 1, kNewtonIrNodeType_Tminus))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tminus, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tplus))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tplus, currentScope));
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

	return node;
}



/*
 *	Grammar production:
 *
 *		comparisonOperator		::=	"o<" | "~" | "<" | "<=" | ">" | ">=" | "==" | "<->" .
 */
IrNode *
newtonParseCompareOp(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PcomparisonOperator,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	/*
	 *	NOTE: The '<->' syntax is intended to be used in combination with the
	 *	"set of quantities syntax", i.e.,
	 *
	 *		pendulumPeriod <-> {acceleration, length}
	 */
	IrNodeType	type = lexPeek(N, 1)->type;
	if (
		type == kNewtonIrNodeType_TdimensionallyAgnosticProportional	||
		type == kNewtonIrNodeType_TdimensionallyMatchingProportional	||
		type == kNewtonIrNodeType_Tlt					||
		type == kNewtonIrNodeType_Tle					||
		type == kNewtonIrNodeType_Tge					||
		type == kNewtonIrNodeType_Tgt					||
		type == kNewtonIrNodeType_Tequals				||
		type == kNewtonIrNodeType_Trelated
		)
	{
		addLeaf(N, node, newtonParseTerminal(N, type, currentScope));
		return node;
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PcomparisonOperator, kNewtonIrNodeType_PcomparisonOperator, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PcomparisonOperator);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PcomparisonOperator, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PcomparisonOperator, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PcomparisonOperator);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		highPrecedenceBinaryOp		::=	"*" | "/" | "%" | "**" .
 */
IrNode *
newtonParseHighPrecedenceBinaryOp(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (peekCheck(N, 1, kNewtonIrNodeType_Tmul))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tmul, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tdiv))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tdiv, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tpercent))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tpercent, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Texponentiation))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Texponentiation, currentScope));
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



/*
 *	Grammar production:
 *
 *		lowPrecedenceOperator		::=	"+" | "-" .
 */
IrNode *
newtonParseLowPrecedenceOperator(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PlowPrecedenceOperator,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (peekCheck(N, 1, kNewtonIrNodeType_Tplus))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tplus, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tminus))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tminus, currentScope));
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PlowPrecedenceOperator, kNewtonIrNodeType_PlowPrecedenceOperator, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PlowPrecedenceOperator);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PlowPrecedenceOperator, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PlowPrecedenceOperator, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PlowPrecedenceOperator);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		highPrecedenceOperator		::=	"*" | "/" .
 */
IrNode *
newtonParseHighPrecedenceOperator(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PhighPrecedenceOperator,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (peekCheck(N, 1, kNewtonIrNodeType_Tmul))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tmul, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tdiv))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tdiv, currentScope));
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PhighPrecedenceOperator, kNewtonIrNodeType_PhighPrecedenceOperator, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PhighPrecedenceOperator);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PhighPrecedenceOperator, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PhighPrecedenceOperator, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PhighPrecedenceOperator);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		highPrecedenceQuantityOperator	::=	highPrecedenceOperator | vectorOp | "><" .
 */
IrNode *
newtonParseHighPrecedenceQuantityOperator(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PhighPrecedenceQuantityOperator,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);


	if (inFirst(N, kNewtonIrNodeType_PhighPrecedenceOperator, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseHighPrecedenceOperator(N, currentScope));
	}
	else if (inFirst(N, kNewtonIrNodeType_PvectorOp, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseVectorOp(N, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tmutualinf))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tmutualinf, currentScope));
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PhighPrecedenceQuantityOperator, kNewtonIrNodeType_PhighPrecedenceQuantityOperator, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PhighPrecedenceQuantityOperator);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PhighPrecedenceQuantityOperator, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PhighPrecedenceQuantityOperator, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PhighPrecedenceQuantityOperator);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		constraint			::=	quantityExpression comparisonOperator quantityExpression | identifier callParameterTuple .
 */
IrNode *
newtonParseConstraint(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pconstraint,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (inFirst(N, kNewtonIrNodeType_Pconstraint, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		if (lexPeek(N,2)->type == kNewtonIrNodeType_TleftParen && lexPeek(N,1)->type == kNewtonIrNodeType_Tidentifier)
		{
			// newtonParseIdentifier(N,currentScope);
			// TODO;
			/*  
			*	Parse identifier. Look at the correct scopes.
			*	We may also have to look at the symbol table update
			*	when defining a new invariant.
			*/
			addLeaf(N,node,newtonParseInvariantIdentifierUsageTerminal(N,currentScope));
			addLeafWithChainingSeq(N,node,newtonParseCallParameterTuple(N,currentScope));
		}
		else 
		{
			addLeaf(N, node, newtonParseQuantityExpression(N, currentScope));
			addLeafWithChainingSeq(N, node, newtonParseCompareOp(N, currentScope));
			addLeafWithChainingSeq(N, node, newtonParseQuantityExpression(N, currentScope));
		}

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
*	Takes state and the current scope, creates an identifier irNode
*	similar to the newtonParseIdentifierUsageTerminal, checks if the called
*	invariant exists and then returns the created node.
*/
IrNode*
newtonParseInvariantIdentifierUsageTerminal(State * N,Scope * currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);
	if (!peekCheck(N, 1, kNewtonIrNodeType_Tidentifier))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Tidentifier, kNewtonIrNodeType_Tidentifier, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Tidentifier);

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

	Invariant * invariantSearchResult = newtonGetInvariant(N,n->tokenString);
	if (invariantSearchResult == NULL)
	{
		char *	details;

		asprintf(&details, "%s: \"%s\"\n", Iundeclared, t->identifier);
		newtonParserSemanticError(N, kNewtonIrNodeType_Tidentifier, details);
		free(details);

		newtonParserErrorRecovery(N, kNewtonIrNodeType_Tidentifier);
	}
	
	if (currentScope->parent != NULL)
	{
		Symbol * symbolSearchResult = commonSymbolTableSymbolForIdentifier(N, currentScope->parent, t->identifier);
		
		n->symbol = symbolSearchResult;
	}
	/*
	*	TODO; check what happens if ever this check fails
	*/
	return n;
}


/*
 *	Grammar production:
 *
 *		callParameterTuple		::=	"(" identifier {"," identifier} ")" .
 */
IrNode*
newtonParseCallParameterTuple(State * N, Scope * currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);
	
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PcallParameterTuple,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);
	
	newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);

	// int	parameterNumber = 0;
	addLeaf(N, node, newtonParseIdentifier(N,currentScope));

	while (peekCheck(N, 1, kNewtonIrNodeType_Tcomma))
	{
		newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
		addLeafWithChainingSeq(N, node, newtonParseIdentifier(N, currentScope));
	}
	newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PcallParameterTuple, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PcallParameterTuple, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PcallParameterTuple);
	}
	*/
	return node;
}	


/*
 *	Grammar production:
 *
 *		constraintList			::=	[constraint] {"," constraint} .
 */
IrNode *
newtonParseConstraintList(State *  N, Scope *  currentScope)

{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PconstraintList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (inFirst(N, kNewtonIrNodeType_PconstraintList, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		IrNode *	firstConstraint = newtonParseConstraint(N, currentScope);
		addLeafWithChainingSeq(N, node, firstConstraint);

		while (peekCheck(N, 1, kNewtonIrNodeType_Tcomma))
		{
			newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
			addLeafWithChainingSeq(N, node, newtonParseConstraint(N, currentScope));
		}
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
	if (!inFollow(N, kNewtonIrNodeType_PconstraintList, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PconstraintList, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PconstraintList);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		sensorDefinition		::=	identifier ":" "sensor" parameterTuple "=" "{" sensorPropertyList "}" .
 */
IrNode *
newtonParseSensorDefinition(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PsensorDefinition,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	addLeaf(N, node, newtonParseIdentifierDefinitionTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);

	/*
	 *	The new scope begins at the `sensor` (and includes the parameter list)
	 */
	IrNode *	scopeBegin	= newtonParseTerminal(N, kNewtonIrNodeType_Tsensor, currentScope);
	Scope *		newScope	= commonSymbolTableOpenScope(N, currentScope, scopeBegin);


	IrNode *	parameterList = newtonParseParameterTuple(N, newScope);
	addLeafWithChainingSeq(N, node, parameterList);
	newScope->scopeParameterList = parameterList;

	newtonParseTerminal(N, kNewtonIrNodeType_Tassign, newScope);
	newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, newScope);
	addLeafWithChainingSeq(N, node, newtonParseSensorPropertyList(N, newScope));

	IrNode *	scopeEnd	= newtonParseTerminal(N, kNewtonIrNodeType_TrightBrace, newScope);
	commonSymbolTableCloseScope(N, newScope, scopeEnd);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PsensorDefinition, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PsensorDefinition, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsensorDefinition);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		sensorPropertyList		::=	sensorProperty {"," sensorProperty} .
 */
IrNode *
newtonParseSensorPropertyList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PsensorPropertyList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	addLeaf(N, node, newtonParseSensorProperty(N, currentScope));

	while (peekCheck(N, 1, kNewtonIrNodeType_Tcomma))
	{
		newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
		addLeafWithChainingSeq(N, node, newtonParseSensorProperty(N, currentScope));
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PsensorPropertyList, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PsensorPropertyList, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsensorPropertyList);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		sensorProperty			::=	rangeStatement | uncertaintyStatement | erasureValueStatement | accuracyStatement | precisionStatement | sensorInterfaceStatement .
 */
IrNode *
newtonParseSensorProperty(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PsensorProperty,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (inFirst(N, kNewtonIrNodeType_PrangeStatement, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseRangeStatement(N, currentScope));
	}
	else if (inFirst(N, kNewtonIrNodeType_PuncertaintyStatement, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseUncertaintyStatement(N, currentScope));
	}
	else if (inFirst(N, kNewtonIrNodeType_PerasureValueStatement, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseErasureValueStatement(N, currentScope));
	}
	else if (inFirst(N, kNewtonIrNodeType_PaccuracyStatement, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseAccuracyStatement(N, currentScope));
	}
	else if (inFirst(N, kNewtonIrNodeType_PprecisionStatement, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParsePrecisionStatement(N, currentScope));
	}
	else if (inFirst(N, kNewtonIrNodeType_PsensorInterfaceStatement, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseSensorInterfaceStatement(N, currentScope));
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PsensorProperty, kNewtonIrNodeType_PsensorProperty, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsensorProperty);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PsensorProperty, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PsensorProperty, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsensorProperty);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		sensorInterfaceStatement	::=	"interface" identifier ["@" numericFactor "bits"] "==" sensorInterfaceType [parameterTuple ["{" sensorInterfaceCommandList "}"]] .
 */
IrNode *
newtonParseSensorInterfaceStatement(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PsensorInterfaceStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_Tinterface, currentScope);

	addLeaf(N, node, newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));

	if (peekCheck(N, 1, kNewtonIrNodeType_TatSign))
	{
		newtonParseTerminal(N, kNewtonIrNodeType_TatSign, currentScope);
		addLeafWithChainingSeq(N, node, newtonParseNumericFactor(N, currentScope));
		newtonParseTerminal(N, kNewtonIrNodeType_Tmul, currentScope);
		newtonParseTerminal(N, kNewtonIrNodeType_Tbits, currentScope);
	}

	/*
	 *	The new scope begins at the `==` (and includes the parameter list)
	 */
	IrNode *	scopeBegin	= newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
	Scope *		newScope	= commonSymbolTableOpenScope(N, currentScope, scopeBegin);

	addLeafWithChainingSeq(N, node, newtonParseSensorInterfaceType(N, newScope));

	/*
	 *	NOTE/TODO: We currently allow the parameterList to be absent. This means the
	 *	scopeParameterList for such scopes will be NULL. This is in principle OK,
	 *	but I'm not yet happy with the syntax for these sensorInterfaceStatements
	 *	anyway and we'll likely need to evolve them.
	 *
	 *	One possible synthax is:
	 *
	 *		interface interfaceName(bits: 32, io: i2c, address: 16rFF) = {...}
	 */
	IrNode *	parameterList = NULL;

	if (inFirst(N, kNewtonIrNodeType_PparameterTuple, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		parameterList = newtonParseParameterTuple(N, newScope);
		addLeafWithChainingSeq(N, node, parameterList);
	}

	if (peekCheck(N, 1, kNewtonIrNodeType_TleftBrace))
	{
		newScope->scopeParameterList = parameterList;

		newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, newScope);
		addLeafWithChainingSeq(N, node, newtonParseSensorInterfaceCommandList(N, newScope));

		IrNode *	scopeEnd	= newtonParseTerminal(N, kNewtonIrNodeType_TrightBrace, newScope);
		commonSymbolTableCloseScope(N, newScope, scopeEnd);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PsensorInterfaceStatement, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PsensorInterfaceStatement, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsensorInterfaceStatement);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		sensorInterfaceType		::=	"i2c" | "spi" | "analog" .
 */
IrNode *
newtonParseSensorInterfaceType(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PsensorInterfaceType,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (peekCheck(N, 1, kNewtonIrNodeType_Ti2c))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Ti2c, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tspi))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tspi, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tanalog))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tanalog, currentScope));
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PsensorInterfaceType, kNewtonIrNodeType_PsensorInterfaceType, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsensorInterfaceType);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PsensorInterfaceType, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PsensorInterfaceType, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsensorInterfaceType);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		sensorInterfaceCommandList	::=	sensorInterfaceCommand ";" {sensorInterfaceCommand ";"} .
 */
IrNode *
newtonParseSensorInterfaceCommandList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PsensorInterfaceCommandList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	addLeaf(N, node, newtonParseSensorInterfaceCommand(N, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

	while (inFirst(N, kNewtonIrNodeType_PsensorInterfaceCommand, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseSensorInterfaceCommand(N, currentScope));
		newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PsensorInterfaceCommandList, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PsensorInterfaceCommandList, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsensorInterfaceCommandList);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		sensorInterfaceCommand		::=	readRegisterCommand | writeRegisterCommand | delayCommand | arithmeticCommand .
 */
IrNode *
newtonParseSensorInterfaceCommand(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PsensorInterfaceCommand,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (peekCheck(N, 3, kNewtonIrNodeType_Tread))
	{
		addLeaf(N, node, newtonParseReadRegisterCommand(N, currentScope));
	}
	else if (inFirst(N, kNewtonIrNodeType_PwriteRegisterCommand, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseWriteRegisterCommand(N, currentScope));
	}
	else if (inFirst(N, kNewtonIrNodeType_PdelayCommand, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseDelayCommand(N, currentScope));
	}
	else if (inFirst(N, kNewtonIrNodeType_ParithmeticCommand, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseArithmeticCommand(N, currentScope));
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PsensorInterfaceCommand, kNewtonIrNodeType_PsensorInterfaceCommand, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsensorInterfaceCommand);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PsensorInterfaceCommand, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PsensorInterfaceCommand, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PsensorInterfaceCommand);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		readRegisterCommand		::=	identifier "=" "read" ["[" numericExpression "]" ","] numericExpression .
 */
IrNode *
newtonParseReadRegisterCommand(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PreadRegisterCommand,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	addLeaf(N, node, newtonParseIdentifierDefinitionTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tassign, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tread, currentScope);

	if (peekCheck(N, 1, kNewtonIrNodeType_TleftBracket))
	{
		newtonParseTerminal(N, kNewtonIrNodeType_TleftBracket, currentScope);
		addLeafWithChainingSeq(N, node, newtonParseNumericExpression(N, currentScope));
		newtonParseTerminal(N, kNewtonIrNodeType_TrightBracket, currentScope);
		newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
	}

	addLeafWithChainingSeq(N, node, newtonParseNumericExpression(N, currentScope));

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PreadRegisterCommand, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PreadRegisterCommand, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PreadRegisterCommand);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		writeRegisterCommand		::=	"write" numericExpression "," numericExpression .
 */
IrNode *
newtonParseWriteRegisterCommand(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PwriteRegisterCommand,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_Twrite, currentScope);
	addLeaf(N, node, newtonParseNumericExpression(N, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
	addLeaf(N, node, newtonParseNumericExpression(N, currentScope));

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PwriteRegisterCommand, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PwriteRegisterCommand, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PwriteRegisterCommand);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		delayCommand			::=	"delay" numericExpression .
 */
IrNode *
newtonParseDelayCommand(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PdelayCommand,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_Tdelay, currentScope);
	addLeaf(N, node, newtonParseNumericExpression(N, currentScope));

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PdelayCommand, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PdelayCommand, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PdelayCommand);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		arithmeticCommand		::=	identifier "=" expression .
 */
IrNode *
newtonParseArithmeticCommand(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_ParithmeticCommand,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	addLeaf(N, node, newtonParseIdentifierDefinitionTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tassign, currentScope);
	addLeaf(N, node, newtonParseExpression(N, currentScope));

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_ParithmeticCommand, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_ParithmeticCommand, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_ParithmeticCommand);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		rangeStatement			::=	"range" identifier "==" "[" numericFactor [unitFactor] "," numericFactor [unitFactor] "]" .
 */
IrNode *
newtonParseRangeStatement(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PrangeStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_Trange, currentScope);
	addLeaf(N, node, newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_TleftBracket, currentScope);
	addLeafWithChainingSeq(N, node, newtonParseNumericFactor(N, currentScope));

	if (inFirst(N, kNewtonIrNodeType_PunitFactor, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseUnitFactor(N, currentScope));
	}

	newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
	addLeaf(N, node, newtonParseNumericFactor(N, currentScope));

	if (inFirst(N, kNewtonIrNodeType_PunitFactor, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseUnitFactor(N, currentScope));
	}

	newtonParseTerminal(N, kNewtonIrNodeType_TrightBracket, currentScope);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PrangeStatement, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PrangeStatement, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PrangeStatement);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		uncertaintyStatement		::=	"uncertainty" identifier "==" factor [unitFactor] .
 */
IrNode *
newtonParseUncertaintyStatement(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PuncertaintyStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_Tuncertainty, currentScope);
	addLeaf(N, node, newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
	addLeafWithChainingSeq(N, node, newtonParseFactor(N, currentScope));

	if (inFirst(N, kNewtonIrNodeType_PunitExpression, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseUnitFactor(N, currentScope));
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PuncertaintyStatement, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PuncertaintyStatement, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PuncertaintyStatement);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		erasureValueStatement		::=	"erasuretoken" identifier "==" numericFactor [unitFactor] .
 */
IrNode *
newtonParseErasureValueStatement(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PerasureValueStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_TerasureToken, currentScope);
	addLeaf(N, node, newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
	addLeafWithChainingSeq(N, node, newtonParseNumericFactor(N, currentScope));

	if (inFirst(N, kNewtonIrNodeType_PunitExpression, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseUnitFactor(N, currentScope));
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PerasureValueStatement, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PerasureValueStatement, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PerasureValueStatement);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		accuracyStatement		::=	"accuracy" identifier "==" numericConstTupleList .
 */
IrNode *
newtonParseAccuracyStatement(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PaccuracyStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_Taccuracy, currentScope);
	addLeaf(N, node, newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
	addLeaf(N, node, newtonParseNumericConstTupleList(N, currentScope));

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PaccuracyStatement, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PaccuracyStatement, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PaccuracyStatement);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		precisionStatement		::=	"precision" identifier "==" numericConstTupleList .
 */
IrNode *
newtonParsePrecisionStatement(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PprecisionStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_Tprecision, currentScope);
	addLeaf(N, node, newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
	addLeaf(N, node, newtonParseNumericConstTupleList(N, currentScope));

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PprecisionStatement, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PprecisionStatement, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PprecisionStatement);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		numericConstTupleList		::=	"{" numericConstTuple {"," numericConstTuple} "}" .
 */
IrNode *
newtonParseNumericConstTupleList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PnumericConstTupleList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, currentScope);
	addLeaf(N, node, newtonParseNumericConstTuple(N, currentScope));

	while (peekCheck(N, 1, kNewtonIrNodeType_Tcomma))
	{
		newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
		addLeaf(N, node, newtonParseNumericConstTuple(N, currentScope));
	}
	newtonParseTerminal(N, kNewtonIrNodeType_TrightBrace, currentScope);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PnumericConstTupleList, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PnumericConstTupleList, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnumericConstTupleList);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		numericConst			::=	integerConst | realConst .
 */
IrNode *
newtonParseNumericConst(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PnumericConst,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (peekCheck(N, 1, kNewtonIrNodeType_TintegerConst))
	{
		IrNode *	valueNode = newtonParseTerminal(N, kNewtonIrNodeType_TintegerConst, currentScope);
		addLeaf(N, node, valueNode);
		node->integerValue = valueNode->integerValue;
		node->value = (double) valueNode->integerValue;
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TrealConst))
	{
		IrNode *	valueNode = newtonParseTerminal(N, kNewtonIrNodeType_TrealConst, currentScope);
		addLeaf(N, node, valueNode);
		node->value = valueNode->value;
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PnumericConst, kNewtonIrNodeType_PnumericConst, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnumericConst);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PnumericConst, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PnumericConst, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnumericConst);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		numericConstTuple		::=	"(" numericFactor [unitFactor] "," numericFactor ")" .
 */
IrNode *
newtonParseNumericConstTuple(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PnumericConstTuple,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
	addLeaf(N, node, newtonParseNumericFactor(N, currentScope));

	if (inFirst(N, kNewtonIrNodeType_PunitExpression, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseUnitFactor(N, currentScope));
	}

	newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
	addLeaf(N, node, newtonParseNumericFactor(N, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PnumericConstTuple, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PnumericConstTuple, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnumericConstTuple);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		expression			::=	term {lowPrecedenceBinaryOp term} .
 */
IrNode *
newtonParseExpression(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pexpression,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	addLeaf(N, node, newtonParseTerm(N, currentScope));

	while (inFirst(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseLowPrecedenceBinaryOp(N, currentScope));
		addLeafWithChainingSeq(N, node, newtonParseTerm(N, currentScope));
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Pexpression, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pexpression, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pexpression);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		term				::=	[unaryOp] factor ["++" | "--"] {highPrecedenceBinaryOp factor} .
 */
IrNode *
newtonParseTerm(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pterm,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (inFirst(N, kNewtonIrNodeType_PunaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseUnaryOp(N, currentScope));
	}

	if (peekCheck(N, 1, kNewtonIrNodeType_TplusPlus))
	{
		addLeafWithChainingSeq(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TplusPlus, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TminusMinus))
	{
		addLeafWithChainingSeq(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TminusMinus, currentScope));
	}

	while (inFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, node, newtonParseHighPrecedenceBinaryOp(N, currentScope));
		addLeafWithChainingSeq(N, node, newtonParseFactor(N, currentScope));
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Pterm, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pterm, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pterm);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		factor				::=	numericConst | "(" expression ")" | distributionFactor | identifier ["[" numericExpression "]"] .
 */
IrNode *
newtonParseFactor(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pfactor,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (inFirst(N, kNewtonIrNodeType_PnumericConst, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseNumericConst(N, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TleftParen))
	{
		newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
		addLeaf(N, node, newtonParseExpression(N, currentScope));
		newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);
	}
	else if (inFirst(N, kNewtonIrNodeType_PdistributionFactor, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseDistributionFactor(N, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_Tidentifier))
	{
		addLeaf(N, node, newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));

		if (peekCheck(N, 1, kNewtonIrNodeType_TleftBracket))
		{
			newtonParseTerminal(N, kNewtonIrNodeType_TleftBracket, currentScope);
			addLeaf(N, node, newtonParseNumericExpression(N, currentScope));
			newtonParseTerminal(N, kNewtonIrNodeType_TrightBracket, currentScope);
		}
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pfactor, kNewtonIrNodeType_Pfactor, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pfactor);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Pfactor, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pfactor, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pfactor);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		distributionFactor		::=	distribution parameterValueList .
 */
IrNode *
newtonParseDistributionFactor(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PdistributionFactor,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	addLeaf(N, node, newtonParseDistribution(N, currentScope));
	addLeaf(N, node, newtonParseParameterValueList(N, currentScope));

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PdistributionFactor, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PdistributionFactor, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PdistributionFactor);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		parameterValueList		::=	"(" identifier ":" expression {"," identifier ":" expression} ")" .
 */
IrNode *
newtonParseParameterValueList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PparameterValueList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
	addLeaf(N, node, newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
	addLeafWithChainingSeq(N, node, newtonParseExpression(N, currentScope));

	while (peekCheck(N, 1, kNewtonIrNodeType_Tcomma))
	{
		newtonParseTerminal(N, kNewtonIrNodeType_Tcomma, currentScope);
		addLeafWithChainingSeq(N, node, newtonParseIdentifierUsageTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope));
		newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
		addLeafWithChainingSeq(N, node, newtonParseExpression(N, currentScope));
	}

	newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_PparameterValueList, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PparameterValueList, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PparameterValueList);
	}
	*/

	return node;
}



/*
 *	Grammar production:
 *
 *		distribution			::=	  "Gaussian" | "Laplacian" | "StudentT" | "Bernoulli" | "Binomial"
 *							| "Poisson" | "NegativeBinomial" | "BetaBinomial" | "Exponential"
 *							| "Gamma" | "Multinomial" | "Beta" | "LogitNormal" | "Dirichlet"
 *							| "Cauchy" | "LogNormal" | "Pareto" | "BetaPrime" | "StudentZ"
 *							| "Weibull" | "Erlang" | "Maxwell" | "FermiDirac" | "FisherZ"
 *							| "LogSeries" | "Gumbel" | "Rayleigh" | "Gibrat" | "PearsonIII"
 *							| "ExtremeValue" | "F" | "Xi" | "XiSquared" | "Unconstrained" .
 */
IrNode *
newtonParseDistribution(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pdistribution,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);


	if (peekCheck(N, 1, kNewtonIrNodeType_TGaussian))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TGaussian, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TLaplacian))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TLaplacian, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TStudentT))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TStudentT, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TBernoulli))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TBernoulli, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TBinomial))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TBinomial, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TPoisson))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TPoisson, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TNegativeBinomial))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TNegativeBinomial, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TBetaBinomial))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TBetaBinomial, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TExponential))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TExponential, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TGamma))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TGamma, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TMultinomial))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TMultinomial, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TBeta))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TBeta, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TLogitNormal))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TLogitNormal, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TDirichlet))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TDirichlet, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TCauchy))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TCauchy, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TLogNormal))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TLogNormal, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TPareto))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TPareto, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TBetaPrime))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TBetaPrime, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TStudentZ))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TStudentZ, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TWeibull))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TErlang, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TMaxwell))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TMaxwell, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TFermiDirac))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TFermiDirac, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TFisherZ))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TFisherZ, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TLogSeries))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TLogSeries, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TGumbel))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TGumbel, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TRayleigh))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TRayleigh, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TGibrat))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TGibrat, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TPearsonIII))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TPearsonIII, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TExtremeValue))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TExtremeValue, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TF))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TF, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TXi))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TXi, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TXiSquared))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TXiSquared, currentScope));
	}
	else if (peekCheck(N, 1, kNewtonIrNodeType_TUnconstrained))
	{
		addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_TUnconstrained, currentScope));
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pdistribution, kNewtonIrNodeType_Pdistribution, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pdistribution);
	}

	/*
	 *	Activate this when Newton's FFI sets have been corrected. See issue #317.
	 */
	/*
	if (!inFollow(N, kNewtonIrNodeType_Pdistribution, gNewtonFollows, kNewtonIrNodeTypeMax))
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Pdistribution, kNewtonIrNodeTypeMax, gNewtonFollows);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Pdistribution);
	}
	*/

	return node;
}

/*
 *	Grammar production:
 *
 *		transcendental	::= "sin" | "cos" | "tan" | "cotan" | "sec"  | "cosec"
 *						| "arcsin" | "arccos" | "arctan" | "arccotan" | "arcsec"  | "arccosec"
 *						| "sinh" | "cosh" | "tanh" | "cotanh" | "sech"  | "cosech"
 *						| "arcsinh" | "arccosh" | "arctanh" | "arccotanh" | "arcsech"  | "arccosech"
 *						| "exp" | "sqrt"
 *						| "ln" | "log10" | "log2" .
 */
IrNode *
newtonParseTranscendental(State * N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Ptranscendental,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */
					);

	if (inFirst(N, kNewtonIrNodeType_Ptranscendental, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		addLeaf(N, node, newtonParseTerminal(N, lexPeek(N, 1)->type, currentScope));
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Ptranscendental, kNewtonIrNodeType_Ptranscendental, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Ptranscendental);
	}

	return node;
}













/*
 *		Helper routines (identifier definition, identifier use, etc.)
 */


/*
 *	Remove an identifier _definition_ terminal, performing symtab insertion
 */
IrNode *
newtonParseIdentifierDefinitionTerminal(State *  N, IrNodeType  expectedType, Scope *  scope)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

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
	TimeStampTraceMacro(kNewtonTimeStampKey);

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

	Physics *   physicsSearchResult;
	Symbol * symbolSearchResult;


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
		if (scope->scopeParameterList == NULL)
		{
			char *	details;

			asprintf(&details, "%s: \"%s\"\n", Eundeclared, t->identifier);
			newtonParserSemanticError(N, kNewtonIrNodeType_Tidentifier, details);
			free(details);

			newtonParserErrorRecovery(N, kNewtonIrNodeType_Tidentifier);
		}

		symbolSearchResult = commonSymbolTableSymbolForIdentifier(N, scope, t->identifier);
		physicsSearchResult = newtonParseGetPhysicsByBoundIdentifier(N, scope->scopeParameterList, t->identifier);
	}

	if (physicsSearchResult == NULL && symbolSearchResult == NULL)
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

	n->symbol = symbolSearchResult;
	n->physics = deepCopyPhysicsNode(N, physicsSearchResult);
	assert(n->physics->dimensions != NULL);

/*
fprintf(stderr, "In identifier use of [%s], physics dimensions exponents are:\n", t->identifier);
Dimension *p = n->physics->dimensions;
while (p != NULL)
{
	fprintf(stderr, "\tdim name [%s], dim abbrv [%s], dim exponent [%f]\n", p->name, p->abbreviation, p->exponent);
	p = p->next;
}
*/
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
 *	Routines related to Newton API. TODO: Should be moved out of newton-parser.c into a separate (API-related) source file. I have in any case started getting rid of these from the implementation since Jonathan is no longer maintaining them---PSM.
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
	TimeStampTraceMacro(kNewtonTimeStampKey);

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
	TimeStampTraceMacro(kNewtonTimeStampKey);

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
	TimeStampTraceMacro(kNewtonTimeStampKey);

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
newtonParseGetPhysicsByBoundIdentifier(State *  N, IrNode * root, char* boundVariableIdentifier)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

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
newtonGetInvariantIdByParameters(State *  N, IrNode * parameterTreeRoot, unsigned long long int invariantId)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

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
	State *  N,
	IrNode * node,
	Scope * scope,
	char * identifier,
	int subindex)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

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
	node->physics = deepCopyPhysicsNode(N, physicsSearchResult);
	if (node->physics->dimensions == NULL)
	{
		fatal(N, Esanity);
	}
}

bool
newtonIsDimensionless(State *  N, Physics *  physics)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	if (physics == NULL)
	{
		return true;
	}

	bool		isDimensionless = true;
	Dimension *	current = physics->dimensions;
	while (current != NULL)
	{
		/*
		 *	If the base is a Physics quantity, we used to check that the
		 *	exponent must be an integer. We no longer restrict this.
		 *	One use case is noise for many sensors (e.g., accelerometers)
		 *	which is derivation = 1E-6 * (acceleration / (frequency ** 0.5));
		 *
		 *	TODO: Get rid of this comment and block in a future cleanup
		 */
		/*
		assert(current->exponent == (int) current->exponent);
		*/

		isDimensionless = isDimensionless && (current->exponent == 0);

		current = current->next;
	}

	return isDimensionless;
}

int
newtonGetPhysicsId(State *  N, Physics * physics)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	return primeNumbers[N->primeNumbersIndex++];
}







/*
 *	Exported non-parse routines
 */







void
newtonParserSyntaxAndSemanticPre(State *  N, IrNodeType currentlyParsingTokenOrProduction,
	const char *  string1, const char *  string2, const char *  string3, const char *  string4)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	flexprint(N->Fe, N->Fm, N->Fperr, "\n-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --\n");
	if (N->mode & kCommonModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "<b>");
	}

	if (N->mode & kCommonModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\t%s, line %d position %d, %s %s\"",
						string1,
						lexPeek(N, 1)->sourceInfo->lineNumber,
						lexPeek(N, 1)->sourceInfo->columnNumber,
						string4,
						kNewtonErrorTokenHtmlTagOpen);
		lexPrintToken(N, lexPeek(N, 1), gNewtonTokenDescriptions);
		flexprint(N->Fe, N->Fm, N->Fperr, "\"%s %s `%s`.<br><br>%s%s",
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
		flexprint(N->Fe, N->Fm, N->Fperr, "\" %s `%s`.\n\n\t%s",
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
	TimeStampTraceMacro(kNewtonTimeStampKey);

	if (N->mode & kCommonModeCGI)
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
		flexprint(N->Fe, N->Fm, N->Fperr, "`%s`", gNewtonTokenDescriptions[expectedProductionOrToken]);
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
	TimeStampTraceMacro(kNewtonTimeStampKey);

	TimeStampTraceMacro(kNewtonTimeStampKeyParserSemanticError);
	newtonParserSyntaxAndSemanticPre(N, currentlyParsingTokenOrProduction, EsemanticsA, EsemanticsB, details, EsemanticsD);
	newtonParserSyntaxAndSemanticPost(N);
}



void
newtonParserErrorRecovery(State *  N, IrNodeType expectedProductionOrToken)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyParserErrorRecovery);

	if (N->verbosityLevel & kCommonVerbosityDebugParser)
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
		flexprint(N->Fe, N->Fm, N->Fperr, "doing longjmp...");

		/*
		 *	Return info on which line number of the input we have reached, and let, e.g.,
		 *	the CGI version highlight the point at which processing stopped.
		 */
		longjmp(N->jmpbuf, lexPeek(N, 1)->sourceInfo->lineNumber);
	}

	/*
	 *	Not reached if N->jmpbufIsValid
	 */
	consolePrintBuffers(N);

	exit(EXIT_SUCCESS);
}
