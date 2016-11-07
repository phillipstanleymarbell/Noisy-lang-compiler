#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "version.h"
#include "noisyconfig.h"
#include "noisyconfig-errors.h"
#include "noisyconfig-parser.h"
#include "noisyconfig-irHelpers.h"
#include "noisyconfig-parser-helper.h"
#include "noisyconfig-parser-errors.h"
#include "noisyconfig-lexer.h"
#include "noisyconfig-symbolTable.h"
#include "noisyconfig-firstAndFollow.h"


extern char *		gNoisyConfigAstNodeStrings[];
extern int		gNoisyConfigFirsts[kNoisyConfigIrNodeTypeMax][kNoisyConfigIrNodeTypeMax];

extern void		noisyConfigFatal(NoisyConfigState *  N, const char *  msg);
extern void		noisyConfigError(NoisyConfigState *  N, const char *  msg);



// static char		kNoisyConfigErrorTokenHtmlTagOpen[]	= "<span style=\"background-color:#FFCC00; color:#FF0000;\">";
// static char		kNoisyConfigErrorTokenHtmlTagClose[]	= "</span>";
// static char		kNoisyConfigErrorDetailHtmlTagOpen[]	= "<span style=\"background-color:#A9E9FF; color:#000000;\">";
// static char		kNoisyConfigErrorDetailHtmlTagClose[]	= "</span>";


NoisyConfigIrNode *
noisyConfigParse(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	return noisyConfigParseConfigFile(N, currentScope);
}


/*
 *	kNoisyConfigIrNodeType_PconfigFile
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_PdimensionTypeNameScope
 *		node.right	= Xseq of more scopes
 */
NoisyConfigIrNode *
noisyConfigParseConfigFile(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{

	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
        N,
        kNoisyConfigIrNodeType_PconfigFile,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
    );
    n->currentScope = currentScope;


	/*
	 *	Before we start parsing, set begin source line of toplevel scope.
	 */
	currentScope->begin = noisyConfigLexPeek(N, 1)->sourceInfo;

	addLeaf(N, n, noisyConfigParseDimensionTypeNameScope(N, currentScope), currentScope);
	
    // TODO after getting dimension scope working, uncomment the below
    // if (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PconfigFile))
	// {
	// 	addLeafWithChainingSeq(N, n, noisyConfigParseEquationScope(N, currentScope), currentScope);
	// }
	// 
    // if (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PconfigFile))
	// {
	// 	addLeafWithChainingSeq(N, n, noisyConfigParseIntegralsScope(N, currentScope), currentScope);
	// }

	/*
	 *	We can now fill in end src info for toplevel scope.
	 */
	currentScope->end = noisyConfigLexPeek(N, 1)->sourceInfo;


	return n;
}


/*
 *	kNoisyConfigIrNodeType_PdimensionTypeNameScope
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_TdimensionTypeName
 *		node.right	= kNoisyConfigIrNodeType_PscopedDimensionStatementList
 */
NoisyConfigIrNode *
noisyConfigParseDimensionTypeNameScope(NoisyConfigState *  N, NoisyConfigScope *  scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	kNoisyConfigIrNodeType_PdimensionTypeNameScope,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = scope;


	NoisyConfigIrNode *	dimensionTypeNames = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TdimensionTypeNames, scope);
	addLeaf(N, n, dimensionTypeNames, scope);
	
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	
    addLeaf(N, n, noisyConfigParseDimensionTypeNameStatementList(N, scope), scope);
	
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightBrace, scope);

    return n;
}


/*	
 *	kNoisyConfigIrNodeType_PdimensionStatementList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_PdimensionTypeNameStatement or NULL
 *		node.right	= Xseq of kNoisyConfigIrNodeType_PdimensionTypeNameStatement
 */
NoisyConfigIrNode *
noisyConfigParseDimensionTypeNameStatementList(NoisyConfigState * N, NoisyConfigScope * scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
		N,
		kNoisyConfigIrNodeType_PdimensionTypeNameStatementList,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);
    n->currentScope = scope;

    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PdimensionTypeNameStatement)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseDimensionTypeNameStatement(N, scope), scope);
    }

    return n;
}

/*
 *	kNoisyConfigIrNodeType_PdimensionStatement
 *
 *	Generated AST subtree:
 *
 *		node.left	= NULL | kNoisyConfigIrNodeType_PidentifierOrNilList
 *		node.right	= NULL | kNoisyConfigIrNodeType_PconstantDeclaration | .. | kNoisyConfigIrNodeType_Pexpression
 */
NoisyConfigIrNode *
noisyConfigParseDimensionTypeNameStatement(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
						N,
						kNoisyConfigIrNodeType_PdimensionTypeNameStatement,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = currentScope;


	if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PdimensionTypeNameStatement))
	{
	    // flexprint(N->Fe, N->Fm, N->Fperr, "what is this%s\n", N->currentToken);
		addLeaf(N, n, noisyConfigParseIdentifier(N, currentScope), currentScope);

		if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PassignOp))
		{
	        flexprint(N->Fe, N->Fm, N->Fperr, "assign %s\n", N->tokenList);
            noisyConfigParseAssignOp(N, currentScope);
	        flexprint(N->Fe, N->Fm, N->Fperr, "after assign %s\n", N->tokenList);
			addLeafWithChainingSeq(N, n, noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TstringConst, currentScope), currentScope);
		}
		else
		{
	        flexprint(N->Fe, N->Fm, N->Fperr, "hello%s\n", N->currentToken);
			noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PdimensionTypeNameStatement, kNoisyConfigIrNodeTypeMax);
		}
	}
	else
	{

		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PdimensionTypeNameStatement, kNoisyConfigIrNodeTypeMax);
	}
    
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tsemicolon, currentScope);

	return n;
}

NoisyConfigIrNode *
noisyConfigParseAssignOp(NoisyConfigState *  N, NoisyConfigScope * currentScope)
{
    if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tequals))
    {
        NoisyConfigIrNode * n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tequals, currentScope);
        n->currentScope = currentScope;
        return n;
    }
    else
    {
        noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PassignOp, kNoisyConfigIrNodeTypeMax);
        // noisyParserErrorRecovery(N, kNoisyIrNodeType_PassignOp);
    }
    return NULL;

}

/*
 *  Simply remove a terminal
 */
NoisyConfigIrNode *
noisyConfigParseTerminal(NoisyConfigState *  N, NoisyConfigIrNodeType expectedType, NoisyConfigScope * currentScope)
{
    if (!peekCheck(N, 1, expectedType))
    {
        noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeTypeMax, expectedType);

        /*
         *  In this case, we know the specific expected type/token.
         */
        // noisyConfigParserErrorRecovery(N, expectedType);
    }

    NoisyConfigToken *  t = noisyConfigLexGet(N);
    NoisyConfigIrNode * n = genNoisyConfigIrNode(N, t->type,
                        NULL /* left child */,
                        NULL /* right child */,
                        t->sourceInfo /* source info */);
    n->currentScope = currentScope;

    return n;
}


/*
 *	kNoisyConfigIrNodeType_PidentifierOrNil
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_Tidentifier
 *		node.right	= Xseq of kNoisyConfigIrNodeType_PfieldSelect
 */
NoisyConfigIrNode *
noisyConfigParseIdentifier(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigIrNode *	n;

	if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tidentifier))
	{
		n = noisyConfigParseIdentifierDefinitionTerminal(N, kNoisyConfigIrNodeType_Tidentifier, currentScope);
        n->currentScope = currentScope;
	    return n;
	}
	else
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_Tidentifier, kNoisyConfigIrNodeTypeMax);
		// noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_TidentifierOrNil);
	}
    return NULL;

}

/*
 *	Remove an identifier _definition_ terminal, performing symtab insertion
 */
NoisyConfigIrNode *
noisyConfigParseIdentifierDefinitionTerminal(NoisyConfigState *  N, NoisyConfigIrNodeType  expectedType, NoisyConfigScope *  scope)
{
	if (!peekCheck(N, 1, expectedType))
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeTypeMax, expectedType);

		/*
		 *	In this case, we know the specific expected type/token.
		 */
		// noisyConfigParserErrorRecovery(N, expectedType);
	}

	NoisyConfigToken *	t = noisyConfigLexGet(N);
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

	n->tokenString = t->identifier;
    n->currentScope = scope;

	//	NOTE: noisyConfigSymbolTableAddOrLookupSymbolForToken(N, ) adds token 't' to scope 'scope'
    // TODO fill in symbol table stuff after finishing the IR scope
	// NoisyConfigSymbol *	sym = noisyConfigSymbolTableAddOrLookupSymbolForToken(N, scope, t);
	// if (sym->definition != NULL)
	// {
	// 	errorMultiDefinition(N, sym);
	// 	// TODO: do noisyConfigParserErrorRecovery() here ?
	// }
	// n->symbol = sym;

	return n;
}
