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
#include "newton-dimension-pass.h"


extern unsigned long int bigNumberOffset;
extern int primeNumbers[168];
extern const char * gNewtonTokenDescriptions[kNoisyIrNodeTypeMax];
extern char *		gNewtonAstNodeStrings[];
extern int		gNewtonFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];

extern void		fatal(State *  N, const char *  msg);
extern void		error(State *  N, const char *  msg);



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

    assert(lexPeek(N, 1)->type == kNewtonIrNodeType_Zeof);
	N->tokenList = N->tokenList->next; /* skip eof token without using lexGet*/
}

void
newtonDimensionPassParseRuleList(State *  N, Scope *  currentScope)
{
  if (inFirst(N, kNewtonIrNodeType_Prule, gNewtonFirsts))
	{
		newtonDimensionPassParseRule(N, currentScope);
	}

  while (inFirst(N, kNewtonIrNodeType_Prule, gNewtonFirsts))
	{
		newtonDimensionPassParseRule(N, currentScope);
	}
}

void
newtonDimensionPassParseRule(State * N, Scope * currentScope)
{
  if (lexPeek(N, 3)->type != kNewtonIrNodeType_Tsignal)
    {
      while (lexPeek(N, 1)->type != kNewtonIrNodeType_TrightBrace)
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
newtonDimensionPassParseBaseSignal(State * N, Scope * currentScope)
{
    newtonParseTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tsignal, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
	  newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, currentScope);

	/*
	 * name syntax is optional
	 */
	IrNode * unitName;
	if (inFirst(N, kNewtonIrNodeType_Pname, gNewtonFirsts))
	{
	  unitName = newtonParseName(N, currentScope);
	}

	/*
	 * abbreviation syntax is also optional
	 */
	IrNode * unitAbbreviation;
	if (inFirst(N, kNewtonIrNodeType_Psymbol, gNewtonFirsts))
	{
	  unitAbbreviation = newtonParseSymbol(N, currentScope);
	}
    newtonParseTerminal(N, kNewtonIrNodeType_Tderivation, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);

    /*
     * These are the derived signals
     */
    if (lexPeek(N, 1)->type != kNewtonIrNodeType_Tnone)
    {
		/* just gobble tokens for expressions */
		while(inFirst(N, kNewtonIrNodeType_PunaryOp, gNewtonFirsts) ||
			  inFirst(N, kNewtonIrNodeType_PlowPrecedenceBinaryOp, gNewtonFirsts) ||
			  inFirst(N, kNewtonIrNodeType_PmidPrecedenceBinaryOp, gNewtonFirsts) ||
			  inFirst(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp, gNewtonFirsts) ||
			  inFirst(N, kNewtonIrNodeType_PquantityExpression, gNewtonFirsts) ||
			  inFirst(N, kNewtonIrNodeType_PquantityTerm, gNewtonFirsts) ||
			  inFirst(N, kNewtonIrNodeType_PquantityFactor, gNewtonFirsts)
            )
        {
			lexGet(N, gNewtonTokenDescriptions);
        }
    }

    /*
     * These are the base signals
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

    IrNode * baseSignalName = newtonParseTerminal(N, kNewtonIrNodeType_TstringConst, currentScope);
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

    IrNode * baseSignalAbbreviation = newtonParseTerminal(N, kNewtonIrNodeType_TstringConst, currentScope);
    node->token = baseSignalAbbreviation->token;

    newtonParseTerminal(N, kNewtonIrNodeType_Tsemicolon, currentScope);

    return node;
}
