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
#include "newton-parser-expression.h"
#include "common-lexers-helpers.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "common-firstAndFollow.h"
#include "newton-parser.h"
#include "newton-dimension-pass.h"


extern unsigned long int	bigNumberOffset;
extern int			primeNumbers[168];
extern const char *		gNewtonTokenDescriptions[];
extern char *			gNewtonAstNodeStrings[];
extern int			gNewtonFirsts[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax];

extern void			fatal(State *  N, const char *  msg);
extern void			error(State *  N, const char *  msg);



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
	newtonDimensionPassParseRuleList(N, currentScope);

	if (lexPeek(N, 1)->type != kNewtonIrNodeType_Zeof)
	{
		newtonParserSyntaxError(N, kNewtonIrNodeType_Zeof, kNewtonIrNodeTypeMax);
		newtonParserErrorRecovery(N, kNewtonIrNodeType_Zeof);
	}

	/*
	 *	Skip eof token without using lexGet
	 */
	N->tokenList = N->tokenList->next;
}

void
newtonDimensionPassParseRuleList(State *  N, Scope *  currentScope)
{
	while (inFirst(N, kNewtonIrNodeType_Prule, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		newtonDimensionPassParseRule(N, currentScope);
	}
}

void
newtonDimensionPassParseRule(State * N, Scope * currentScope)
{
	/*
	 *	Rules can be one of signal, invariant, or constant.
	 *	Signal and invariant blocks end with a '}'. A constant
	 *	definition on the other hand ends with a ';'.
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
	else
	{
		newtonDimensionPassParseBaseSignal(N, currentScope);
	}
}

void
newtonDimensionPassParseSubindex(State * N, Scope * currentScope)
{
	newtonParseTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tto, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tnumber, currentScope);
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

    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, currentScope);

	/*
	 *  name syntax is optional
	 */
	IrNode *    unitName = NULL;
	if (inFirst(N, kNewtonIrNodeType_Pname, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		unitName = newtonParseName(N, currentScope);
		newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);
	}

	/*
	 *  abbreviation syntax is also optional
	 */
	IrNode *    unitAbbreviation = NULL;
	if (inFirst(N, kNewtonIrNodeType_Psymbol, gNewtonFirsts, kNewtonIrNodeTypeMax))
	{
		unitAbbreviation = newtonParseSymbol(N, currentScope);
		newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);
	}

	newtonParseTerminal(N, kNewtonIrNodeType_Tderivation, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);

    /*
     *  These are the derived signals
     */
    if (lexPeek(N, 1)->type != kNewtonIrNodeType_Tnone)
    {
		/*
         *  just gobble tokens for expressions 
         */
		while(inFirst(N, kNewtonIrNodeType_PunaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  inFirst(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  inFirst(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  inFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  inFirst(N, kNewtonIrNodeType_PquantityExpression, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  inFirst(N, kNewtonIrNodeType_PquantityTerm, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  inFirst(N, kNewtonIrNodeType_PquantityFactor, gNewtonFirsts, kNewtonIrNodeTypeMax) ||
			  lexPeek(N, 1)->type == kNewtonIrNodeType_TatSign
            )
        {
			lexGet(N, gNewtonTokenDescriptions);
        }
    }

    /*
     *  These are the base signals
     */
    else
    {
		assert(unitName != NULL && unitAbbreviation != NULL);
        newtonParseTerminal(N, kNewtonIrNodeType_Tnone, currentScope);
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
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Pname,
								 NULL /* left child */,
								 NULL /* right child */,
								 lexPeek(N, 1)->sourceInfo /* source info */);

    newtonParseTerminal(N, kNewtonIrNodeType_Tname, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);

    IrNode *    baseSignalName = newtonParseTerminal(N, kNewtonIrNodeType_TstringConst, currentScope);
    node->token = baseSignalName->token;

    if (lexPeek(N, 1)->type == kNewtonIrNodeType_TEnglish || 
        lexPeek(N, 1)->type == kNewtonIrNodeType_TSpanish)
	{
        newtonParseTerminal(N, lexPeek(N, 1)->type, currentScope);
	}
    else
	{
        fatal(N, "newton-dimension-pass.c:newtonDimensionPassParseName no language setting\n");
	}

    return node;
}

IrNode *
newtonDimensionPassParseSymbol(State * N, Scope * currentScope)
{
	IrNode *	node = genIrNode(N,	kNewtonIrNodeType_Psymbol,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

    newtonParseTerminal(N, kNewtonIrNodeType_Tsymbol, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);

    IrNode *    baseSignalAbbreviation = newtonParseTerminal(N, kNewtonIrNodeType_TstringConst, currentScope);
    node->token = baseSignalAbbreviation->token;

    newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

    return node;
}
