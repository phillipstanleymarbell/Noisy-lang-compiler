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
#include "newton-parser.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton-firstAndFollow.h"


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
		newtonLexPeek(N, 1)->sourceInfo /* source info */
    );

	addLeaf(N, node, newtonParseRuleList(N, currentScope));

    assert(newtonLexPeek(N, 1)->type == kNewtonIrNodeType_Zeof);
	N->tokenList = N->tokenList->next; /* skip eof token without using newtonLexGet*/

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
		newtonLexPeek(N, 1)->sourceInfo /* source info */
    );
    
    if (newtonInFirst(N, kNewtonIrNodeType_Prule))
	{
		addLeaf(N, node, newtonParseRule(N, currentScope));
	}

    while (newtonInFirst(N, kNewtonIrNodeType_Prule))
	{
		addLeafWithChainingSeqNewton(N, node, newtonParseRule(N, currentScope));
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
		newtonLexPeek(N, 1)->sourceInfo /* source info */
    );
	
    currentScope->begin = newtonLexPeek(N, 1)->sourceInfo;
    
    switch(newtonLexPeek(N, 3)->type)
    {
        case kNewtonIrNodeType_Tsignal:
            newtonParseBaseSignal(N, currentScope);
            break;
        case kNewtonIrNodeType_Tconstant:
            // newtonParseConstant(N, currentScope);
            break;
        case kNewtonIrNodeType_Tinvariant:
            // newtonParseInvariant(N, currentScope);
            break;
        default:
            noisyFatal(N, "newton-parser.c:newtonParseRule neither signal, constant, nor invariant\n");
    }

    currentScope->end = newtonLexPeek(N, 1)->sourceInfo;

    return node;
}

NoisyIrNode *
newtonParseBaseSignal(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(N,	kNewtonIrNodeType_PbaseSignal,
						NULL /* left child */,
						NULL /* right child */,
						newtonLexPeek(N, 1)->sourceInfo /* source info */);

    NoisyIrNode * basicPhysicsIdentifier = newtonParseIdentifier(N, currentScope);
    addLeaf(N, node, basicPhysicsIdentifier);
    Physics * newPhysics = newtonPhysicsTableAddPhysicsForToken(N, currentScope, basicPhysicsIdentifier->token);
    
    newtonParseTerminal(N, kNewtonIrNodeType_Tcolon, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tsignal, currentScope);
    newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope);
	newtonParseTerminal(N, kNewtonIrNodeType_TleftBrace, currentScope);
	
    NoisyIrNode * unitName = newtonParseName(N, currentScope);
    addLeafWithChainingSeqNewton(N, node, unitName);
    NoisyIrNode * unitAbbreviation = newtonParseSymbol(N, currentScope);
    addLeafWithChainingSeqNewton(N, node, unitAbbreviation);
    
    Dimension * newDimension = newtonDimensionTableAddDimensionForToken(N, currentScope, unitName->token, unitAbbreviation->token);
   
    newtonPhysicsAddNumeratorDimension(N, newPhysics, newDimension);

    addLeafWithChainingSeqNewton(N, node, newtonParseDerivation(N, currentScope));
	
    newtonParseTerminal(N, kNewtonIrNodeType_TrightBrace, currentScope);

    return node;
}

NoisyIrNode *
newtonParseName(NoisyState * N, NoisyScope * currentScope)
{
	NoisyIrNode *	node = genNoisyIrNode(N,	kNewtonIrNodeType_Pname,
						NULL /* left child */,
						NULL /* right child */,
						newtonLexPeek(N, 1)->sourceInfo /* source info */);

    addLeaf(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tname, currentScope));
    addLeafWithChainingSeqNewton(N, node, newtonParseTerminal(N, kNewtonIrNodeType_Tequals, currentScope));
    
    NoisyIrNode * baseSignalName = newtonParseTerminal(N, kNewtonIrNodeType_TstringConst, currentScope);
    node->token = baseSignalName->token;
    
    addLeafWithChainingSeqNewton(N, node, baseSignalName);
    
    if (newtonLexPeek(N, 1)->type == kNewtonIrNodeType_TEnglish || 
                newtonLexPeek(N, 1)->type == kNewtonIrNodeType_TSpanish)
	{
		addLeafWithChainingSeqNewton(N, node, newtonParseTerminal(N, newtonLexPeek(N, 1)->type, currentScope));
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
						newtonLexPeek(N, 1)->sourceInfo /* source info */);
    
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
						newtonLexPeek(N, 1)->sourceInfo /* source info */);
    return node;
}



/*
 *  Simply remove a terminal
 */
NoisyIrNode *
newtonParseTerminal(NoisyState *  N, NoisyIrNodeType expectedType, NoisyScope * currentScope)
{
    if (!peekCheckNewton(N, 1, expectedType))
    {
        noisyFatal(N, "newton-parser.c: newtonParseTerminal");
        // noisyParserSyntaxError(N, kNoisyIrNodeTypeMax, expectedType);

        /*
         *  In this case, we know the specific expected type/token.
         */
        // noisyParserErrorRecovery(N, expectedType);
    }

    NoisyToken *  t = newtonLexGet(N);
    NoisyIrNode * n = genNoisyIrNode(N, t->type,
                        NULL /* left child */,
                        NULL /* right child */,
                        t->sourceInfo /* source info */);
    
    n->token = t;
    n->tokenString = t->identifier;
   
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

	if (peekCheckNewton(N, 1, kNewtonIrNodeType_Tidentifier))
	{
		n = newtonParseIdentifierDefinitionTerminal(N, kNewtonIrNodeType_Tidentifier, currentScope);
	    return n;
	}
	else
	{
		noisyParserSyntaxError(N, kNewtonIrNodeType_Tidentifier, kNoisyIrNodeTypeMax);
		// noisyParserErrorRecovery(N, kNewtonIrNodeType_TidentifierOrNil);
	}
    return NULL;
}

/*
 *  Remove an identifier _usage_ terminal, performing symtab lookup
 */
NoisyIrNode *
newtonParseIdentifierUsageTerminal(NoisyState *  N, NoisyIrNodeType expectedType, NoisyScope *  scope)
{
    if (!peekCheckNewton(N, 1, expectedType))
    {
        noisyParserSyntaxError(N, kNoisyIrNodeTypeMax, expectedType);

        /*
         *  In this case, we know the specific expected type/token.
         */
        // noisyParserErrorRecovery(N, expectedType);
        return NULL;
    }

    NoisyToken *    t = newtonLexGet(N);
    NoisyIrNode *   n = genNoisyIrNode(N,   t->type,
                        NULL /* left child */,
                        NULL /* right child */,
                        t->sourceInfo /* source info */);

    n->token = t;
    n->tokenString = t->identifier;
    
    Physics * physicsSearchResult = newtonPhysicsTablePhysicsForIdentifier(N, scope, t->identifier);
    if (physicsSearchResult == NULL)
    {
        errorUseBeforeDefinition(N, t->identifier);
        // TODO: do noisyParserErrorRecovery() here ?
    } else {
        n->physics = physicsSearchResult;
    }

    return n;
}

/*
 *	Remove an identifier _definition_ terminal, performing symtab insertion
 */
NoisyIrNode *
newtonParseIdentifierDefinitionTerminal(NoisyState *  N, NoisyIrNodeType  expectedType, NoisyScope *  scope)
{
	if (!peekCheckNewton(N, 1, expectedType))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeTypeMax, expectedType);

		/*
		 *	In this case, we know the specific expected type/token.
		 */
		// noisyParserErrorRecovery(N, expectedType);
	}

	NoisyToken *	t = newtonLexGet(N);
	NoisyIrNode *	n = genNoisyIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

    n->token = t;
	n->tokenString = t->identifier;

	//	NOTE: newtonSymbolTableAddOrLookupSymbolForToken(N, ) adds token 't' to scope 'scope'
    // TODO fill in symbol table stuff after finishing the IR scope
	// NoisySymbol *	sym = newtonSymbolTableAddOrLookupSymbolForToken(N, scope, t);
	// if (sym->definition != NULL)
	// {
	// 	errorMultiDefinition(N, sym);
	// 	// TODO: do noisyParserErrorRecovery() here ?
	// }
	// n->symbol = sym;

	return n;
}
