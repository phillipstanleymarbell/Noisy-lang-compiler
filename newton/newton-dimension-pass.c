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

    IrNode * unitName = newtonDimensionPassParseName(N, currentScope);
    IrNode * unitAbbreviation = newtonDimensionPassParseSymbol(N, currentScope);
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
