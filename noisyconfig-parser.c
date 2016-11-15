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
		addLeafWithChainingSeq(N, n, noisyConfigParseVectorScalarPairScope(N, currentScope), currentScope);
	}
    
    if (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PconfigFile))
	{
		addLeafWithChainingSeq(N, n, noisyConfigParseLawScope(N, currentScope), currentScope);
	}
	
    if (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PconfigFile))
	{
		addLeafWithChainingSeq(N, n, noisyConfigParseDimensionAliasScope(N, currentScope), currentScope);
	}
    
    // TODO combine vector and scalar integral scopes into one
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
        NoisyConfigIrNode * basicPhysicsIdentifier = noisyConfigParseIdentifier(N, currentScope);
        Physics * newPhysics = noisyConfigPhysicsTableAddPhysicsForToken(N, currentScope, basicPhysicsIdentifier->token);
		addLeaf(N, n, basicPhysicsIdentifier, currentScope);

		if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PassignOp))
		{
            noisyConfigParseAssignOp(N, currentScope);
            
            NoisyConfigIrNode * dimensionNode = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TstringConst, currentScope);
            Dimension * newDimension = noisyConfigDimensionTableAddDimensionForToken(N, currentScope, dimensionNode->token);
            noisyConfigPhysicsAddNumeratorDimension(N, newPhysics, newDimension);

			addLeaf(N, n, dimensionNode, currentScope);

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
 *	kNoisyConfigIrNodeType_PvectorScalarPairScope
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_TvectorScalarPairs
 *		node.right	= kNoisyConfigIrNodeType_PvectorScalarPairStatementList
 */
NoisyConfigIrNode *
noisyConfigParseVectorScalarPairScope(NoisyConfigState *  N, NoisyConfigScope *  scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	kNoisyConfigIrNodeType_PvectorScalarPairScope,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = scope;


	NoisyConfigIrNode *	vectorScalarPairsToken = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TvectorScalarPairs, scope);
	addLeaf(N, n, vectorScalarPairsToken, scope);
	
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	
    addLeaf(N, n, noisyConfigParseVectorScalarPairStatementList(N, scope), scope);
	
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightBrace, scope);

    return n;
}


/*	
 *  kNoisyConfigIrNodeType_PvectorScalarPairStatementList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_PvectorScalarPairStatement or NULL
 *		node.right	= Xseq of kNoisyConfigIrNodeType_PvectorScalarPairStatement 
 */
NoisyConfigIrNode *
noisyConfigParseVectorScalarPairStatementList(NoisyConfigState * N, NoisyConfigScope * scope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
		N,
        kNoisyConfigIrNodeType_PvectorScalarPairStatement,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);
    n->currentScope = scope;

    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PvectorScalarPairStatement)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseVectorScalarPairStatement(N, scope), scope);
    }

    return n;
}

/*
 *	kNoisyConfigIrNodeType_PvectorScalarPairStatement
 *
 *	Generated AST subtree:
 *
 *		node.left	= NULL | kNoisyConfigIrNodeType_PidentifierOrNilList
 *		node.right	= NULL | kNoisyConfigIrNodeType_PconstantDeclaration | .. | kNoisyConfigIrNodeType_Pexpression
 */
NoisyConfigIrNode *
noisyConfigParseVectorScalarPairStatement(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
						N,
                        kNoisyConfigIrNodeType_PvectorScalarPairStatement,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = currentScope;


	if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PvectorScalarPairStatement))
	{
        NoisyConfigIrNode * vectorPhysicsIdentifier = noisyConfigParseIdentifier(N, currentScope);
        Physics * vectorPhysics = noisyConfigPhysicsTablePhysicsForIdentifier(N, currentScope, vectorPhysicsIdentifier->token->identifier);

        if (vectorPhysics == NULL)
        {
            vectorPhysics = noisyConfigPhysicsTableAddPhysicsForToken(N, currentScope, vectorPhysicsIdentifier->token);
        }

        vectorPhysics->isVector = true;
		
		addLeaf(N, n, vectorPhysicsIdentifier, currentScope);

		if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PassignOp))
		{
            noisyConfigParseAssignOp(N, currentScope);
            
            NoisyConfigIrNode * scalarPhysicsIdentifier = noisyConfigParseIdentifier(N, currentScope);
            Physics * scalarPhysics = noisyConfigPhysicsTablePhysicsForIdentifier(N, currentScope, scalarPhysicsIdentifier->token->identifier);

            if (scalarPhysics == NULL)
            {
                scalarPhysics = noisyConfigPhysicsTableAddPhysicsForToken(N, currentScope, scalarPhysicsIdentifier->token);
            }

            scalarPhysics->vectorCounterpart = vectorPhysics;
            vectorPhysics->vectorCounterpart = scalarPhysics;
			
            addLeaf(N, n, scalarPhysicsIdentifier, currentScope);
		}
		else
		{
			noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PvectorScalarPairStatement, kNoisyConfigIrNodeTypeMax);
		}
	}
	else
	{

		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PvectorScalarPairStatement, kNoisyConfigIrNodeTypeMax);
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
        NoisyConfigIrNode * irNode = noisyConfigParseVectorIntegralList(N, scope);

        if (N->vectorIntegralLists == NULL)
        {
            N->vectorIntegralLists = irNode->vectorIntegralList;
        }
        else
        {
            IntegralList* current = getTailIntegralList(N->vectorIntegralLists);
            current->next = irNode->vectorIntegralList;
        }

        addLeafWithChainingSeq(N, n, irNode, scope);
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

    n->vectorIntegralList = (IntegralList *) calloc(1, sizeof(IntegralList));
    n->vectorIntegralList->head = NULL;

    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrac, scope);

    
    while (peekCheck(N, 1, kNoisyConfigIrNodeType_Tidentifier)) 
    {
        NoisyConfigIrNode * physicsIdentifier = noisyConfigParseIdentifierUsageTerminal(N, kNoisyConfigIrNodeType_Tidentifier, scope);
        Physics * physics = noisyConfigPhysicsTablePhysicsForIdentifier(N, scope, physicsIdentifier->token->identifier);

        if (physics == NULL)
        {
            noisyConfigFatal(N, Esanity);
        }

        if (n->vectorIntegralList->head == NULL)
        {
            n->vectorIntegralList->head = copyPhysicsNode(physics);
        }
        else
        {
            Physics * current = getTailPhysics(n->vectorIntegralList->head);
            current->next = copyPhysicsNode(physics);
        }

        addLeafWithChainingSeq(N, n, physicsIdentifier, scope);
       
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
        NoisyConfigIrNode * irNode = noisyConfigParseScalarIntegralList(N, scope);

        if (N->scalarIntegralLists == NULL)
        {
            N->scalarIntegralLists = irNode->scalarIntegralList;
        }
        else
        {
            IntegralList* current = getTailIntegralList(N->scalarIntegralLists);
            current->next = irNode->scalarIntegralList;
        }
        
        addLeafWithChainingSeq(N, n, irNode, scope);
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

    n->scalarIntegralList = (IntegralList *) calloc(1, sizeof(IntegralList));
    n->scalarIntegralList->head = NULL;

    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrac, scope);

    while (peekCheck(N, 1, kNoisyConfigIrNodeType_Tidentifier)) 
    {
        NoisyConfigIrNode * physicsIdentifier = noisyConfigParseIdentifierUsageTerminal(N, kNoisyConfigIrNodeType_Tidentifier, scope);
        Physics * physics = noisyConfigPhysicsTablePhysicsForIdentifier(N, scope, physicsIdentifier->token->identifier);

        if (physics == NULL)
        {
            noisyConfigFatal(N, Esanity);
        }

        if (n->scalarIntegralList->head == NULL)
        {
            n->scalarIntegralList->head = copyPhysicsNode(physics);
        }
        else
        {
            Physics * current = getTailPhysics(n->scalarIntegralList->head);
            current->next = copyPhysicsNode(physics);
        }

        addLeafWithChainingSeq(N, n, physicsIdentifier, scope);
       
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
        NoisyConfigIrNode * derivedPhysicsIdentifier = noisyConfigParseIdentifier(N, currentScope);
        Physics * derivedPhysics = noisyConfigPhysicsTablePhysicsForIdentifier(N, currentScope, derivedPhysicsIdentifier->token->identifier);

        if (derivedPhysics == NULL)
        {
            noisyConfigFatal(N, Esanity);
        }
		
        addLeaf(N, n, derivedPhysicsIdentifier, currentScope);

		if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PassignOp))
		{
            noisyConfigParseAssignOp(N, currentScope);
            
            NoisyConfigIrNode * dimensionAlias = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TstringConst, currentScope);
            derivedPhysics->dimensionAlias = dimensionAlias->token->stringConst;

			addLeaf(N, n, dimensionAlias, currentScope);
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
        NoisyConfigIrNode * derivedPhysicsIdentifier = noisyConfigParseIdentifier(N, currentScope);
        Physics * derivedPhysics = noisyConfigPhysicsTablePhysicsForIdentifier(N, currentScope, derivedPhysicsIdentifier->token->identifier);

        if (derivedPhysics == NULL)
        {
            derivedPhysics = noisyConfigPhysicsTableAddPhysicsForToken(N, currentScope, derivedPhysicsIdentifier->token);
        }
        
		addLeaf(N, n, derivedPhysicsIdentifier, currentScope);

		if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PassignOp))
		{
            noisyConfigParseAssignOp(N, currentScope);

            NoisyConfigIrNode * expression = noisyConfigParseExpression(N, currentScope);
			addLeaf(N, n, expression, currentScope);
            
            noisyConfigPhysicsCopyNumeratorDimensions(N, derivedPhysics, expression->physics);
            noisyConfigPhysicsCopyDenominatorDimensions(N, derivedPhysics, expression->physics);
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

// TODO the lower expression methods should take in Physics * and depending on 
// the operation, add previously existing Physics dimension dimension to 
// numerator or denominator list at Expression, Term, Factor level
// 1. Expression: Add or Subtract. 
//    * Make a new Physics struct inside the new NoisyConfigIrNode
//    * Parse LHS which adds to numerator or denominator.
//    * Parse Add or Subtract
//    * Parse RHS but don't add anything to numerator or denominator
// 2. Term: Mul or Division.
//    * Make a new Physics struct inside the new NoisyConfigIrNode
//    * Parse LHS which adds to numerator or denominator
//    * Parse Mul or Division operator
//    * Parse RHS. 
//    If Mul: append the resulting numerator list from 
//            Physics struct from ParseFactor to numerator, and same for
//            denominator. update the prime number.
//    If Div: append the resulting numerator list from 
//            Physics struct from ParseFactor to denominator and
//            denominator list to numerator.
//            update the prime number
// 3. Factor: dot product or cross product
//    Make a new Physics struct inside the new NoisyConfigIrNode
//    Parse LHS. Make sure such identifier exists in the global physics list
//    Recursively parse Expression
//    If identifier: search Physics list and add the resulting Physics numerator 
//      to passed down Physics numerator, and same for denominator
//    If dot product: add the resulting numerator list from
//        Physics struct from ParseExpression to numerator, and same for denominator
//        update the prime number
//    If cross product: same as above, but add Dimension angle to the 
//        resulting denominator list from Physics struct from Parse 
NoisyConfigIrNode*
noisyConfigParseExpression(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
    NoisyConfigIrNode * left;
    NoisyConfigIrNode * right;

    if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_Pterm))
    {
        left = noisyConfigParseTerm(N, currentScope);

        while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp))
        {
            addLeafWithChainingSeq(N, left, noisyConfigParseLowPrecedenceBinaryOp(N, currentScope), currentScope);
            
            right = noisyConfigParseTerm(N, currentScope);
            addLeafWithChainingSeq(N, left, right, currentScope);
            
            // compare LHS and RHS prime numbers and make sure they're equal
            assert(left->physics->numeratorPrimeProduct == right->physics->numeratorPrimeProduct);
            assert(left->physics->denominatorPrimeProduct == right->physics->denominatorPrimeProduct);
        }

    }
    else
    {
        noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_Pexpression, kNoisyConfigIrNodeTypeMax);
        // noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_Pexpression);
        noisyConfigFatal(N, Esanity);
    }
    left->currentScope = currentScope;

    return left;
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
    NoisyConfigIrNode *   intermediate = genNoisyConfigIrNode(N,   kNoisyConfigIrNodeType_Pterm,
                        NULL /* left child */,
                        NULL /* right child */,
                        noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

    intermediate->currentScope = currentScope;
    
    intermediate->physics = (Physics *) calloc(1, sizeof(Physics));
    intermediate->physics->numeratorPrimeProduct = 1;
    intermediate->physics->denominatorPrimeProduct = 1;

    if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PunaryOp))
    {
        addLeaf(N, intermediate, noisyConfigParseUnaryOp(N, currentScope), currentScope);
    }

    NoisyConfigIrNode * left = noisyConfigParseFactor(N, currentScope);
    addLeaf(N, intermediate, left, currentScope);
    
    noisyConfigPhysicsCopyNumeratorDimensions(N, intermediate->physics, left->physics);
    noisyConfigPhysicsCopyDenominatorDimensions(N, intermediate->physics, left->physics);
    
    NoisyConfigIrNode * right;

    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp))
    {
        NoisyConfigIrNode * binOp = noisyConfigParseHighPrecedenceBinaryOp(N, currentScope);
        addLeafWithChainingSeq(N, intermediate, binOp, currentScope);
        
        right = noisyConfigParseFactor(N, currentScope);
        addLeafWithChainingSeq(N, intermediate, right, currentScope);

        if (binOp->type == kNoisyConfigIrNodeType_Tmul) 
        {
            noisyConfigPhysicsCopyNumeratorDimensions(N, intermediate->physics, right->physics);
            noisyConfigPhysicsCopyDenominatorDimensions(N, intermediate->physics, right->physics);
        }
        else if (binOp->type == kNoisyConfigIrNodeType_Tdiv)
        {
            noisyConfigPhysicsCopyNumeratorToDenominatorDimensions(N, intermediate->physics, right->physics);
            noisyConfigPhysicsCopyDenominatorToNumeratorDimensions(N, intermediate->physics, right->physics);
        }
    }

    return intermediate;
}

NoisyConfigIrNode *
noisyConfigParseFactor(NoisyConfigState * N, NoisyConfigScope * currentScope)
{
    NoisyConfigIrNode *   n;

    if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tidentifier))
    {
        n = noisyConfigParseIdentifierUsageTerminal(N, kNoisyConfigIrNodeType_Tidentifier, currentScope);
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
    NoisyConfigIrNode *   intermediate = genNoisyConfigIrNode(N,   kNoisyConfigIrNodeType_PvectorOp,
                        NULL /* left child */,
                        NULL /* right child */,
                        noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

    intermediate->currentScope = currentScope;
    intermediate->physics = (Physics *) calloc(1, sizeof(Physics));
    intermediate->physics->numeratorPrimeProduct = 1;
    intermediate->physics->denominatorPrimeProduct = 1;
    
    bool addAngleToDenominator = false;
    
    if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tdot))
    {
        addLeaf(N, intermediate, noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tdot, currentScope), currentScope);
    } 
    else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tcross))
    {
        addLeaf(N, intermediate, noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcross, currentScope), currentScope);
        addAngleToDenominator = true;
    } 
    else 
    {
        noisyConfigFatal(N, "noisyConfigParseVectorOp: op is not dot or cross\n");
    }
    
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftParen, currentScope);
    
    NoisyConfigIrNode * left;
    left = noisyConfigParseExpression(N, currentScope);
    addLeafWithChainingSeq(N, intermediate, left, currentScope);
    noisyConfigPhysicsCopyNumeratorDimensions(N, intermediate->physics, left->physics);
    noisyConfigPhysicsCopyDenominatorDimensions(N, intermediate->physics, left->physics);
    
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcomma, currentScope);
    
    NoisyConfigIrNode * right;
    right = noisyConfigParseExpression(N, currentScope);
    addLeafWithChainingSeq(N, intermediate, right, currentScope);

    // TODO BUG: after this statement, left->physics->numeratorDimensions changes
    noisyConfigPhysicsCopyNumeratorDimensions(N, intermediate->physics, right->physics);
    noisyConfigPhysicsCopyDenominatorDimensions(N, intermediate->physics, right->physics);

    if (addAngleToDenominator) 
    {
        Dimension* angle = noisyConfigDimensionTableDimensionForIdentifier(N, currentScope, "rad");
        noisyConfigPhysicsAddDenominatorDimension(N, intermediate->physics, angle);
    } 
    
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightParen, currentScope);

    return intermediate;
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
        // TODO add noisyConfigParseExpression here
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
    
    n->token = t;
    n->tokenString = t->identifier;
   
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

    n->token = t;
    n->tokenString = t->identifier;
    n->currentScope = scope;
    
    Physics * physicsSearchResult = noisyConfigPhysicsTablePhysicsForIdentifier(N, scope, t->identifier);
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

    n->token = t;
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
