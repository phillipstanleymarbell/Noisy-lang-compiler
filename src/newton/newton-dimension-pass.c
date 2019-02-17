/*
	Authored 2017. Jonathan Lim. Modified 2018, Phillip Stanley-Marbell.

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
#include "newton-symbolTable.h"
#include "common-firstAndFollow.h"
#include "newton-parser.h"
#include "newton-dimension-pass.h"


extern const char *		gNewtonTokenDescriptions[];
extern char *			gNewtonAstNodeStrings[];
extern int			gNewtonFirsts[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax];


void
newtonDimensionPassParse(State *  N, Scope *  currentScope)
{
	newtonDimensionPassParseFile(N, currentScope);
}

/*
 *	kNoisyIrNodeType_PruleList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PdimensionTypeNameScope
 *		node.right	= Xseq of more scopes
 */
void
newtonDimensionPassParseFile(State *  N, Scope *  currentScope)
{
	newtonDimensionPassParseStatementList(N, currentScope);

	if (lexPeek(N, 1)->type != kNewtonIrNodeType_Zeof)
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Zeof, kNewtonIrNodeTypeMax, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Zeof);
	}

	/*
	 *	Skip eof token without using lexGet
	 */
	N->tokenList = N->tokenList->next;
}

void
newtonDimensionPassParseStatementList(State *  N, Scope *  currentScope)
{
	while (inFirst(N, kNewtonIrNodeType_Prule, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		newtonDimensionPassParseStatement(N, currentScope);
	}
}

void
newtonDimensionPassParseStatement(State * N, Scope * currentScope)
{
	/*
	 *	Rules can be one of sensor, signal, invariant, or constant.
	 *	Signal and invariant blocks end with a '}'. A constant
	 *	definition on the other hand ends with a ';'. Because we
	 *	can have nested braces in sensor descriptions, need to discard
	 *	tokens while tracking number of braces seen.
	 */
	if (lexPeek(N, 3)->type == kNewtonIrNodeType_Tinvariant)
	{
		while (lexPeek(N, 1)->type != kNewtonIrNodeType_TrightBrace)
		{
			lexGet(N, gNewtonTokenDescriptions);
		}
		lexGet(N, gNewtonTokenDescriptions);
	}
	else if (lexPeek(N, 3)->type == kNewtonIrNodeType_Tconstant)
	{
		while (lexPeek(N, 1)->type != kNewtonIrNodeType_Tsemicolon)
		{
			lexGet(N, gNewtonTokenDescriptions);
		}
		lexGet(N, gNewtonTokenDescriptions);
	}
	else if (lexPeek(N, 3)->type == kNewtonIrNodeType_Tsensor)
	{
		/*
		 *	First, find the opening left brace and junk it:
		 */
		while (lexPeek(N, 1)->type != kNewtonIrNodeType_TleftBrace)
		{
			lexGet(N, gNewtonTokenDescriptions);
		}
		lexGet(N, gNewtonTokenDescriptions);

		int	unclosedBraces = 1;

		/*
		 *	Next, while we have one or more unclosed left braces,
		 *	junk all tokens:
		 */
		while (unclosedBraces > 0)
		{
			if (lexPeek(N, 1)->type == kNewtonIrNodeType_TrightBrace)
			{
				unclosedBraces--;
			}
			else if (lexPeek(N, 1)->type == kNewtonIrNodeType_TleftBrace)
			{
				unclosedBraces++;
			}
			lexGet(N, gNewtonTokenDescriptions);
		}
	}
	else if (lexPeek(N, 3)->type == kNewtonIrNodeType_Tsignal)
	{
		newtonDimensionPassParseBaseSignal(N, currentScope);
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Prule, kNewtonIrNodeType_Prule, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Prule);
	}
}

void
newtonDimensionPassParseSubindex(State * N, Scope * currentScope)
{
	newtonParseTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
	newtonParseNumericConst(N, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tto, currentScope);
	newtonParseNumericConst(N, currentScope);
}

void
newtonDimensionPassParseSubindexTuple(State * N, Scope * currentScope)
{
	newtonParseTerminal(N, kNewtonIrNodeType_TleftParen, currentScope);
	newtonDimensionPassParseSubindex(N, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_TrightParen, currentScope);
}


void
newtonDimensionPassParseBaseSignal(State * N, Scope * currentScope)
{
	newtonParseTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tsignal, currentScope);

	if (lexPeek(N, 5)->type == kNewtonIrNodeType_Tto)
	{
		newtonDimensionPassParseSubindexTuple(N, currentScope);
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
	}

	/*
	 *	Abbreviation syntax is also optional
	 */
	IrNode *    unitAbbreviation = NULL;
	if (inFirst(N, kNewtonIrNodeType_PsymbolStatement, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		unitAbbreviation = newtonParseSymbol(N, currentScope);
	}

	newtonParseTerminal(N, kNewtonIrNodeType_Tderivation, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tassign, currentScope);

	/*
	 *	These are the derived signals
	 */
	if ((lexPeek(N, 1)->type != kNewtonIrNodeType_Tnone) && (lexPeek(N, 1)->type != kNewtonIrNodeType_Tdimensionless))
	{
		/*
		 *	just gobble tokens for expressions 
		 */
		while(inFirst(N, kNewtonIrNodeType_PunaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  inFirst(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  inFirst(N, kNewtonIrNodeType_PhighPrecedenceOperator, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  inFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  inFirst(N, kNewtonIrNodeType_PquantityExpression, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  inFirst(N, kNewtonIrNodeType_PquantityTerm, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  inFirst(N, kNewtonIrNodeType_PquantityFactor, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  lexPeek(N, 1)->type == kNewtonIrNodeType_TatSign ||
			  lexPeek(N, 1)->type == kNewtonIrNodeType_TrightParen
			)
		{
			lexGet(N, gNewtonTokenDescriptions);
		}
	}
	else
	{
		/*
		 *	These are the base signals
		 */
		if (unitName == NULL || unitAbbreviation == NULL)
		{
			char *	details;

			asprintf(&details, "%s\n", EbaseDimensionNameOrAbbreviation);
			newtonParserSemanticError(N, kNewtonIrNodeType_Tidentifier, details);
			free(details);

			newtonParserErrorRecovery(N, kNewtonIrNodeType_Tidentifier);
		}

		if (lexPeek(N, 1)->type == kNewtonIrNodeType_Tnone)
		{
			newtonParseTerminal(N, kNewtonIrNodeType_Tnone, currentScope);
		}
		else if (lexPeek(N, 1)->type == kNewtonIrNodeType_Tdimensionless)
		{
			newtonParseTerminal(N, kNewtonIrNodeType_Tdimensionless, currentScope);
		}

		newtonDimensionTableAddDimensionForToken(
			N,
			currentScope,
			unitName->token,
			unitAbbreviation->token
			);
	}
	newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_TrightBrace, currentScope);
}

IrNode *
newtonDimensionPassParseName(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_PnameStatement,
								NULL /* left child */,
								NULL /* right child */,
								lexPeek(N, 1)->sourceInfo /* source info */);

	newtonParseTerminal(N, kNewtonIrNodeType_Tname, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tassign, currentScope);

	IrNode *	baseSignalName = newtonParseTerminal(N, kNewtonIrNodeType_TstringConst, currentScope);
	node->token = baseSignalName->token;

	if (lexPeek(N, 1)->type == kNewtonIrNodeType_TEnglish)
	{
		newtonParseTerminal(N, lexPeek(N, 1)->type, currentScope);
	}
	else
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_PnameStatement, kNewtonIrNodeTypeMax, gNewtonFirsts);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnameStatement);
	}

	return node;
}
