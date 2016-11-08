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
	
    if (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PconfigFile))
	{
		addLeafWithChainingSeq(N, n, noisyConfigParseLawScope(N, currentScope), currentScope);
	}
	
    if (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PconfigFile))
	{
		addLeafWithChainingSeq(N, n, noisyConfigParseDimensionAliasScope(N, currentScope), currentScope);
	}
    
    if (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PconfigFile))
	{
		addLeafWithChainingSeq(N, n, noisyConfigParseVectorIntegralScope(N, currentScope), currentScope);
	}

    if (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PconfigFile))
	{
		addLeafWithChainingSeq(N, n, noisyConfigParseScalarIntegralScope(N, currentScope), currentScope);
	}
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
 *		node.right	= kNoisyConfigIrNodeType_TdimensionTypeNameStatementList
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
		addLeaf(N, n, noisyConfigParseIdentifier(N, currentScope), currentScope);

		if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PassignOp))
		{
            noisyConfigParseAssignOp(N, currentScope);
			addLeaf(N, n, noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TstringConst, currentScope), currentScope);
		}
		else
		{
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

/*
 *	kNoisyConfigIrNodeType_PvectorIntegralScope
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_TvectorIntegrals
 *		node.right	= kNoisyConfigIrNodeType_PvetorIntegralList
 */
NoisyConfigIrNode *
noisyConfigParseVectorIntegralScope(NoisyConfigState *  N, NoisyConfigScope *  scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	kNoisyConfigIrNodeType_PvectorIntegralScope,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = scope;

	NoisyConfigIrNode *	vectorIntegralsToken = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TvectorIntegrals, scope);
	addLeaf(N, n, vectorIntegralsToken, scope);
	
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	
    addLeaf(N, n, noisyConfigParseVectorIntegralLists(N, scope), scope);
	
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightBrace, scope);

    return n;
}

/*	
 *  kNoisyConfigIrNodeType_PvetorIntegralLists
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_PvetorIntegralList
 *		node.right	= Xseq of kNoisyConfigIrNodeType_PvetorIntegralList
 */
NoisyConfigIrNode *
noisyConfigParseVectorIntegralLists(NoisyConfigState * N, NoisyConfigScope * scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
		N,
        kNoisyConfigIrNodeType_PvectorIntegralLists,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);
    n->currentScope = scope;

    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PvectorIntegralList)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseVectorIntegralList(N, scope), scope);
    }

    return n;
}

/*	
 *  kNoisyConfigIrNodeType_PvectorIntegralList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_Tidentifier
 *		node.right	= Xseq of kNoisyConfigIrNodeType_Tidentifier
 */
NoisyConfigIrNode *
noisyConfigParseVectorIntegralList(NoisyConfigState * N, NoisyConfigScope * scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
		N,
        kNoisyConfigIrNodeType_PvectorIntegralList,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);
    n->currentScope = scope;

    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrac, scope);

    while (peekCheck(N, 1, kNoisyConfigIrNodeType_Tidentifier)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseIdentifierUsageTerminal(N, kNoisyConfigIrNodeType_Tidentifier, scope), scope);
       
        if (peekCheck(N, 1 ,kNoisyConfigIrNodeType_TrightBrac)) {
            noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightBrac, scope);
            break;
        } else {
            noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcomma, scope);
        }
    }

    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tsemicolon, scope);

    return n;
}

/*
 *	kNoisyConfigIrNodeType_PscalarIntegralScope
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_TscalarIntegrals
 *		node.right	= kNoisyConfigIrNodeType_PscalarIntegralList
 */
NoisyConfigIrNode *
noisyConfigParseScalarIntegralScope(NoisyConfigState *  N, NoisyConfigScope *  scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	kNoisyConfigIrNodeType_PscalarIntegralScope,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = scope;

	NoisyConfigIrNode *	scalarIntegralsToken = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TscalarIntegrals, scope);
	addLeaf(N, n, scalarIntegralsToken, scope);
	
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	
    addLeaf(N, n, noisyConfigParseScalarIntegralLists(N, scope), scope);
	
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightBrace, scope);

    return n;
}

/*	
 *  kNoisyConfigIrNodeType_PscalarIntegralLists
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_PscalarIntegralList
 *		node.right	= Xseq of kNoisyConfigIrNodeType_PscalarIntegralList
 */
NoisyConfigIrNode *
noisyConfigParseScalarIntegralLists(NoisyConfigState * N, NoisyConfigScope * scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
		N,
        kNoisyConfigIrNodeType_PscalarIntegralLists,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);
    n->currentScope = scope;

    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PscalarIntegralList)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseScalarIntegralList(N, scope), scope);
    }

    return n;
}

/*	
 *  kNoisyConfigIrNodeType_PscalarIntegralList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_Tidentifier
 *		node.right	= Xseq of kNoisyConfigIrNodeType_Tidentifier
 */
NoisyConfigIrNode *
noisyConfigParseScalarIntegralList(NoisyConfigState * N, NoisyConfigScope * scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
		N,
        kNoisyConfigIrNodeType_PscalarIntegralList,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);
    n->currentScope = scope;

    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrac, scope);

    while (peekCheck(N, 1, kNoisyConfigIrNodeType_Tidentifier)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseIdentifierUsageTerminal(N, kNoisyConfigIrNodeType_Tidentifier, scope), scope);
       
        if (peekCheck(N, 1 ,kNoisyConfigIrNodeType_TrightBrac)) {
            noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightBrac, scope);
            break;
        } else {
            noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcomma, scope);
        }
    }

    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tsemicolon, scope);

    return n;
}

/*
 *	kNoisyConfigIrNodeType_PdimensionAliasScope
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_TdimensionAliases
 *		node.right	= kNoisyConfigIrNodeType_TdimensionAliasStatementList
 */
NoisyConfigIrNode *
noisyConfigParseDimensionAliasScope(NoisyConfigState *  N, NoisyConfigScope *  scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	kNoisyConfigIrNodeType_PdimensionAliasScope,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = scope;


	NoisyConfigIrNode *	dimensionAliasesToken = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TdimensionAliases, scope);
	addLeaf(N, n, dimensionAliasesToken, scope);
	
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	
    addLeaf(N, n, noisyConfigParseDimensionAliasStatementList(N, scope), scope);
	
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightBrace, scope);

    return n;
}

/*	
 *	kNoisyConfigIrNodeType_PdimensionAliasStatementList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_PdimensionTypeNameStatement or NULL
 *		node.right	= Xseq of kNoisyConfigIrNodeType_PdimensionTypeNameStatement
 */
NoisyConfigIrNode *
noisyConfigParseDimensionAliasStatementList(NoisyConfigState * N, NoisyConfigScope * scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
		N,
		kNoisyConfigIrNodeType_PdimensionAliasStatementList,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);
    n->currentScope = scope;

    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PdimensionAliasStatement)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseDimensionAliasStatement(N, scope), scope);
    }

    return n;
}

/*
 *	kNoisyConfigIrNodeType_PdimensionAliasStatement
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_Tidentifier
 *		node.right	= kNoisyConfigIrNodeType_TstringConst
 */
NoisyConfigIrNode *
noisyConfigParseDimensionAliasStatement(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
						N,
						kNoisyConfigIrNodeType_PdimensionAliasStatement,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = currentScope;


	if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PdimensionAliasStatement))
	{
		addLeaf(N, n, noisyConfigParseIdentifier(N, currentScope), currentScope);

		if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PassignOp))
		{
            noisyConfigParseAssignOp(N, currentScope);
			addLeaf(N, n, noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TstringConst, currentScope), currentScope);
		}
		else
		{
			noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PdimensionAliasStatement, kNoisyConfigIrNodeTypeMax);
		}
	}
	else
	{

		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PdimensionAliasStatement, kNoisyConfigIrNodeTypeMax);
	}
    
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tsemicolon, currentScope);

	return n;
}



/*
 *	kNoisyConfigIrNodeType_PlawScope
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_Tlaw
 *		node.right	= kNoisyConfigIrNodeType_PlawStatementList
 */
NoisyConfigIrNode *
noisyConfigParseLawScope(NoisyConfigState *  N, NoisyConfigScope *  scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,
                        kNoisyConfigIrNodeType_PlawScope,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = scope;


	NoisyConfigIrNode *	lawToken = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tlaw, scope);
	addLeaf(N, n, lawToken, scope);
	
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	
    addLeaf(N, n, noisyConfigParseLawStatementList(N, scope), scope);
	
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightBrace, scope);

    return n;
}

/*	
 *	kNoisyConfigIrNodeType_PlawStatementList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_PlawStatement
 *		node.right	= Xseq of kNoisyConfigIrNodeType_PlawStatement
 */
NoisyConfigIrNode *
noisyConfigParseLawStatementList(NoisyConfigState * N, NoisyConfigScope * scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
		N,
		kNoisyConfigIrNodeType_PlawStatementList,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);
    n->currentScope = scope;

    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PlawStatement)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseLawStatement(N, scope), scope);
    }

    return n;
}

/*
 *	kNoisyConfigIrNodeType_PlawStatement
 *
 *	Generated AST subtree:
 *
 *		node.left	= NULL | kNoisyConfigIrNodeType_PidentifierOrNilList
 *		node.right	= kNoisyConfigIrNodeType_Pexpression
 */
NoisyConfigIrNode *
noisyConfigParseLawStatement(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
						N,
						kNoisyConfigIrNodeType_PlawStatement,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = currentScope;


	if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PlawStatement))
	{
		addLeaf(N, n, noisyConfigParseIdentifier(N, currentScope), currentScope);

		if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PassignOp))
		{
            noisyConfigParseAssignOp(N, currentScope);
			addLeaf(N, n, noisyConfigParseExpression(N, currentScope), currentScope);
		}
		else
		{
			noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PdimensionTypeNameStatement, kNoisyConfigIrNodeTypeMax);
		}
	}
	else
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PlawStatement, kNoisyConfigIrNodeTypeMax);
	}
    
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tsemicolon, currentScope);

	return n;
}

NoisyConfigIrNode*
noisyConfigParseExpression(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
    NoisyConfigIrNode *   n;

    if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_Pterm))
    {
        n = noisyConfigParseTerm(N, currentScope);

        while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp))
        {
            addLeafWithChainingSeq(N, n, noisyConfigParseLowPrecedenceBinaryOp(N, currentScope), currentScope);
            addLeafWithChainingSeq(N, n, noisyConfigParseTerm(N, currentScope), currentScope);
        }
    }
    else
    {
        noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_Pexpression, kNoisyConfigIrNodeTypeMax);
        // noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_Pexpression);
        return NULL;
    }
    n->currentScope = currentScope;

    return n;	
}

/*
 *  kNoisyIrNodeType_Pterm
 *
 *  Generated AST subtree:
 *
 *      node.left   = kNoisyIrNodeType_Pfactor
 *      node.right  = Xseq of kNoisyIrNodeType_PhighPrecedenceBinaryOp  and kNoisyIrNodeType_Pfactor
 */
NoisyConfigIrNode *
noisyConfigParseTerm(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
    NoisyConfigIrNode *   n = genNoisyConfigIrNode(N,   kNoisyConfigIrNodeType_Pterm,
                        NULL /* left child */,
                        NULL /* right child */,
                        noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

    n->currentScope = currentScope;

    if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PunaryOp))
    {
        addLeaf(N, n, noisyConfigParseUnaryOp(N, currentScope), currentScope);
    }

    addLeaf(N, n, noisyConfigParseFactor(N, currentScope), currentScope);
    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp))
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseHighPrecedenceBinaryOp(N, currentScope), currentScope);
        addLeafWithChainingSeq(N, n, noisyConfigParseFactor(N, currentScope), currentScope);
    }

    return n;
}

NoisyConfigIrNode *
noisyConfigParseLowPrecedenceBinaryOp(NoisyConfigState *  N, NoisyConfigScope * currentScope)
{
    NoisyConfigIrNode *   n;


    if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tplus))
    {
        n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tplus, currentScope);
    }
    else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tminus))
    {
        n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tminus, currentScope);
    }
    else
    {
        noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp, kNoisyConfigIrNodeTypeMax);
        // noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp);
        return NULL;
    }
    n->currentScope = currentScope;

    return n;
}

NoisyConfigIrNode *
noisyConfigParseHighPrecedenceBinaryOp(NoisyConfigState *  N, NoisyConfigScope * currentScope)
{
    NoisyConfigIrNode *   n;

    if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tmul))
    {
        n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tmul, currentScope);
    }
    else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tdiv))
    {
        n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tdiv, currentScope);
    }
    else
    {
        noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp, kNoisyConfigIrNodeTypeMax);
        // noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp);
        return NULL;
    }
    
	n->currentScope = currentScope;

    return n;
}

NoisyConfigIrNode *
noisyConfigParseUnaryOp(NoisyConfigState *  N, NoisyConfigScope * currentScope)
{
    NoisyConfigIrNode *   n = NULL;

    if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tminus))
    {
        n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tminus, currentScope);
    }
    else
    {
        noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PunaryOp, kNoisyConfigIrNodeTypeMax);
        // noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_PunaryOp);
        return NULL;
    }
    n->currentScope = currentScope;

    return n;
}

NoisyConfigIrNode *
noisyConfigParseFactor(NoisyConfigState * N, NoisyConfigScope * currentScope)
{
    NoisyConfigIrNode *   n;

    if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tidentifier))
    {
        n = noisyConfigParseIdentifierUsageTerminal(N, kNoisyConfigIrNodeType_Tidentifier, currentScope);
    }
    else if (peekCheck(N, 1, kNoisyConfigIrNodeType_TstringConst))
    {
        n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TstringConst, currentScope);
    }
    else if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PvectorOp) && peekCheck(N, 2, kNoisyConfigIrNodeType_TleftParen) && peekCheck(N, 4, kNoisyConfigIrNodeType_Tcomma))
    {
		n = noisyConfigParseVectorOp(N, currentScope);
    }
    else if (peekCheck(N, 1, kNoisyConfigIrNodeType_TleftParen))
    {
        noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftParen, currentScope);
        n = noisyConfigParseExpression(N, currentScope);
        noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightParen, currentScope);
    }
    else
    {
        noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_Pfactor, kNoisyConfigIrNodeTypeMax);
        noisyConfigFatal(N, "noisyConfigParseFactor: missed a case in factor\n");
    }
    n->currentScope = currentScope;

    return n;
}

NoisyConfigIrNode *
noisyConfigParseVectorOp(NoisyConfigState *  N, NoisyConfigScope * currentScope)
{
    NoisyConfigIrNode *   n = genNoisyConfigIrNode(N,   kNoisyConfigIrNodeType_PvectorOp,
                        NULL /* left child */,
                        NULL /* right child */,
                        noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

    n->currentScope = currentScope;
    
    if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tdot))
    {
        addLeaf(N, n, noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tdot, currentScope), currentScope);
    } 
    else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tcross))
    {
        addLeaf(N, n, noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcross, currentScope), currentScope);
    } else {
        noisyConfigFatal(N, "noisyConfigParseVectorOp: op is not dot or cross\n");
    }
    
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftParen, currentScope);
    
    addLeafWithChainingSeq(N, n, noisyConfigParseExpression(N, currentScope), currentScope);
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcomma, currentScope);
    addLeafWithChainingSeq(N, n, noisyConfigParseExpression(N, currentScope), currentScope);
    
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightParen, currentScope);

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
 *  Remove an identifier _usage_ terminal, performing symtab lookup
 */
NoisyConfigIrNode *
noisyConfigParseIdentifierUsageTerminal(NoisyConfigState *  N, NoisyConfigIrNodeType expectedType, NoisyConfigScope *  scope)
{
    if (!peekCheck(N, 1, expectedType))
    {
        noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeTypeMax, expectedType);

        /*
         *  In this case, we know the specific expected type/token.
         */
        // noisyConfigParserErrorRecovery(N, expectedType);
        return NULL;
    }

    NoisyConfigToken *    t = noisyConfigLexGet(N);
    NoisyConfigIrNode *   n = genNoisyConfigIrNode(N,   t->type,
                        NULL /* left child */,
                        NULL /* right child */,
                        t->sourceInfo /* source info */);

    n->tokenString = t->identifier;

    n->symbol = noisyConfigSymbolTableSymbolForIdentifier(N, scope, t->identifier);
    n->currentScope = scope;
    if (n->symbol == NULL)
    {
        errorUseBeforeDefinition(N, t->identifier);
        // TODO: do noisyParserErrorRecovery() here ?
    }

    return n;
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
