/*
	Authored 2015. Jonathan Lim.

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
#include "noisy-timeStamps.h"
#include "noisy-errors.h"
#include "noisy.h"
#include "noisy-irHelpers.h"
#include "noisy-parser.h"
#include "noisyconfig-parser.h"
#include "noisyconfig-lexer.h"
#include "noisyconfig-symbolTable.h"
#include "noisyconfig-firstAndFollow.h"


extern char *		gNoisyConfigAstNodeStrings[];
extern int		gNoisyConfigFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];

extern void		noisyFatal(NoisyState *  N, const char *  msg);
extern void		noisyError(NoisyState *  N, const char *  msg);



// static char		kNoisyErrorTokenHtmlTagOpen[]	= "<span style=\"background-color:#FFCC00; color:#FF0000;\">";
// static char		kNoisyErrorTokenHtmlTagClose[]	= "</span>";
// static char		kNoisyErrorDetailHtmlTagOpen[]	= "<span style=\"background-color:#A9E9FF; color:#000000;\">";
// static char		kNoisyErrorDetailHtmlTagClose[]	= "</span>";


NoisyIrNode *
noisyConfigParse(NoisyState *  N, NoisyScope *  currentScope)
{
	return noisyConfigParseConfigFile(N, currentScope);
}


/*
 *	kNoisyIrNodeType_PconfigFile
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PdimensionTypeNameScope
 *		node.right	= Xseq of more scopes
 */
NoisyIrNode *
noisyConfigParseConfigFile(NoisyState *  N, NoisyScope *  currentScope)
{

	NoisyIrNode *	n = genNoisyIrNode(
        N,
        kNoisyConfigIrNodeType_PconfigFile,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
    );

	/*
	 *	Before we start parsing, set begin source line of toplevel scope.
	 */
	currentScope->begin = noisyConfigLexPeek(N, 1)->sourceInfo;

	addLeaf(N, n, noisyConfigParseDimensionTypeNameScope(N, currentScope));
	
    if (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PconfigFile))
	{
		addLeafWithChainingSeq(N, n, noisyConfigParseVectorScalarPairScope(N, currentScope));
	}
    
    if (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PconfigFile))
	{
		addLeafWithChainingSeq(N, n, noisyConfigParseLawScope(N, currentScope));
	}
	
    if (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PconfigFile))
	{
		addLeafWithChainingSeq(N, n, noisyConfigParseDimensionAliasScope(N, currentScope));
	}
    
    // TODO combine vector and scalar integral scopes into one
    if (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PconfigFile))
	{
		addLeafWithChainingSeq(N, n, noisyConfigParseVectorIntegralScope(N, currentScope));
	}

    if (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PconfigFile))
	{
		addLeafWithChainingSeq(N, n, noisyConfigParseScalarIntegralScope(N, currentScope));
	}
	
    /*
	 *	We can now fill in end src info for toplevel scope.
	 */
	currentScope->end = noisyConfigLexPeek(N, 1)->sourceInfo;

    assert(noisyConfigLexPeek(N, 1)->type == kNoisyConfigIrNodeType_Zeof);
	N->tokenList = N->tokenList->next; /* skip eof token without using noisyConfigLexGet*/

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
NoisyIrNode *
noisyConfigParseDimensionTypeNameScope(NoisyState *  N, NoisyScope *  scope)
{
	NoisyIrNode *	n = genNoisyIrNode(N,	kNoisyConfigIrNodeType_PdimensionTypeNameScope,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

	NoisyIrNode *	dimensionTypeNames = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TdimensionTypeNames, scope);
	addLeaf(N, n, dimensionTypeNames);
	
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	
    addLeaf(N, n, noisyConfigParseDimensionTypeNameStatementList(N, scope));
	
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
NoisyIrNode *
noisyConfigParseDimensionTypeNameStatementList(NoisyState * N, NoisyScope * scope)
{
	NoisyIrNode *	n = genNoisyIrNode(
		N,
		kNoisyConfigIrNodeType_PdimensionTypeNameStatementList,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);

    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PdimensionTypeNameStatement)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseDimensionTypeNameStatement(N, scope));
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
NoisyIrNode *
noisyConfigParseDimensionTypeNameStatement(NoisyState *  N, NoisyScope *  currentScope)
{
	NoisyIrNode *	n = genNoisyIrNode(
						N,
						kNoisyConfigIrNodeType_PdimensionTypeNameStatement,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);


	if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PdimensionTypeNameStatement))
	{
        NoisyIrNode * basicPhysicsIdentifier = noisyConfigParseIdentifier(N, currentScope);
        Physics * newPhysics = noisyConfigPhysicsTableAddPhysicsForToken(N, currentScope, basicPhysicsIdentifier->token);
		addLeaf(N, n, basicPhysicsIdentifier);

		if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PassignOp))
		{
            noisyConfigParseAssignOp(N, currentScope);
            
            NoisyIrNode * dimensionNode = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TstringConst, currentScope);
            Dimension * newDimension = noisyConfigDimensionTableAddDimensionForToken(N, currentScope, dimensionNode->token);
            noisyConfigPhysicsAddNumeratorDimension(N, newPhysics, newDimension);

			addLeaf(N, n, dimensionNode);

		}
		else
		{
			noisyParserSyntaxError(N, kNoisyConfigIrNodeType_PdimensionTypeNameStatement, kNoisyIrNodeTypeMax);
		}
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyConfigIrNodeType_PdimensionTypeNameStatement, kNoisyIrNodeTypeMax);
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
NoisyIrNode *
noisyConfigParseVectorScalarPairScope(NoisyState *  N, NoisyScope *  scope)
{
	NoisyIrNode *	n = genNoisyIrNode(N,	kNoisyConfigIrNodeType_PvectorScalarPairScope,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

	NoisyIrNode *	vectorScalarPairsToken = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TvectorScalarPairs, scope);
	addLeaf(N, n, vectorScalarPairsToken);
	
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	
    addLeaf(N, n, noisyConfigParseVectorScalarPairStatementList(N, scope));
	
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
NoisyIrNode *
noisyConfigParseVectorScalarPairStatementList(NoisyState * N, NoisyScope * scope)
{
	NoisyIrNode *	n = genNoisyIrNode(
		N,
        kNoisyConfigIrNodeType_PvectorScalarPairStatement,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);

    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PvectorScalarPairStatement)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseVectorScalarPairStatement(N, scope));
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
NoisyIrNode *
noisyConfigParseVectorScalarPairStatement(NoisyState *  N, NoisyScope *  currentScope)
{
	NoisyIrNode *	n = genNoisyIrNode(
						N,
                        kNoisyConfigIrNodeType_PvectorScalarPairStatement,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

	if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PvectorScalarPairStatement))
	{
        NoisyIrNode * vectorPhysicsIdentifier = noisyConfigParseIdentifier(N, currentScope);
        Physics * vectorPhysics = noisyConfigPhysicsTablePhysicsForIdentifier(N, currentScope, vectorPhysicsIdentifier->token->identifier);

        if (vectorPhysics == NULL)
        {
            vectorPhysics = noisyConfigPhysicsTableAddPhysicsForToken(N, currentScope, vectorPhysicsIdentifier->token);
        }

        vectorPhysics->isVector = true;
		
		addLeaf(N, n, vectorPhysicsIdentifier);

		if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PassignOp))
		{
            noisyConfigParseAssignOp(N, currentScope);
            
            NoisyIrNode * scalarPhysicsIdentifier = noisyConfigParseIdentifier(N, currentScope);
            Physics * scalarPhysics = noisyConfigPhysicsTablePhysicsForIdentifier(N, currentScope, scalarPhysicsIdentifier->token->identifier);

            if (scalarPhysics == NULL)
            {
                scalarPhysics = noisyConfigPhysicsTableAddPhysicsForToken(N, currentScope, scalarPhysicsIdentifier->token);
            }

            scalarPhysics->vectorCounterpart = vectorPhysics;
            vectorPhysics->vectorCounterpart = scalarPhysics;
			
            addLeaf(N, n, scalarPhysicsIdentifier);
		}
		else
		{
			noisyParserSyntaxError(N, kNoisyConfigIrNodeType_PvectorScalarPairStatement, kNoisyIrNodeTypeMax);
		}
	}
	else
	{

		noisyParserSyntaxError(N, kNoisyConfigIrNodeType_PvectorScalarPairStatement, kNoisyIrNodeTypeMax);
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
NoisyIrNode *
noisyConfigParseVectorIntegralScope(NoisyState *  N, NoisyScope *  scope)
{
	NoisyIrNode *	n = genNoisyIrNode(N,	kNoisyConfigIrNodeType_PvectorIntegralScope,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

	NoisyIrNode *	vectorIntegralsToken = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TvectorIntegrals, scope);
	addLeaf(N, n, vectorIntegralsToken);
	
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	
    addLeaf(N, n, noisyConfigParseVectorIntegralLists(N, scope));
	
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
NoisyIrNode *
noisyConfigParseVectorIntegralLists(NoisyState * N, NoisyScope * scope)
{
	NoisyIrNode *	n = genNoisyIrNode(
		N,
        kNoisyConfigIrNodeType_PvectorIntegralLists,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);
    
    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PvectorIntegralList)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseVectorIntegralList(N, scope));
    }

    /*
     * check the numbers of "time" structs inside the integral list are correct
     * e.g.) in a list of [displacement, velocity, acceleration]
     * displacement should have one more "time" than velocity.
     */
    Physics * current = N->vectorIntegralLists->head;
    int pastNumberTime = countNumberTime(current->numeratorDimensions) - countNumberTime(current->denominatorDimensions);

    while (current->next != NULL)
    {
        current = current->next;
        int curNumberTime = countNumberTime(current->numeratorDimensions) - countNumberTime(current->denominatorDimensions);
        assert(curNumberTime == pastNumberTime - 1);
        pastNumberTime = curNumberTime;
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
NoisyIrNode *
noisyConfigParseVectorIntegralList(NoisyState * N, NoisyScope * scope)
{
	NoisyIrNode *	n = genNoisyIrNode(
		N,
        kNoisyConfigIrNodeType_PvectorIntegralList,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);

    IntegralList *vectorIntegralList = (IntegralList *) calloc(1, sizeof(IntegralList));
    vectorIntegralList->head = NULL;

    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrac, scope);

    
    while (peekCheck(N, 1, kNoisyConfigIrNodeType_Tidentifier)) 
    {
        NoisyIrNode * physicsIdentifier = noisyConfigParseIdentifierUsageTerminal(N, kNoisyConfigIrNodeType_Tidentifier, scope);
        Physics * physics = noisyConfigPhysicsTablePhysicsForIdentifier(N, scope, physicsIdentifier->token->identifier);

        assert(physics->isVector);

        if (physics == NULL)
        {
            noisyFatal(N, Esanity);
        }

        if (vectorIntegralList->head == NULL)
        {
            vectorIntegralList->head = copyPhysicsNode(physics);
        }
        else
        {
            getTailPhysics(vectorIntegralList->head)->next = copyPhysicsNode(physics);
        }

        addLeafWithChainingSeq(N, n, physicsIdentifier);
       
        if (peekCheck(N, 1 ,kNoisyConfigIrNodeType_TrightBrac)) {
            noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightBrac, scope);
            break;
        } else {
            noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcomma, scope);
        }
    }

    if (N->vectorIntegralLists == NULL)
        N->vectorIntegralLists = vectorIntegralList;
    else
        appendIntegralList(N->vectorIntegralLists, vectorIntegralList);

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
NoisyIrNode *
noisyConfigParseScalarIntegralScope(NoisyState *  N, NoisyScope *  scope)
{
	NoisyIrNode *	n = genNoisyIrNode(N,	kNoisyConfigIrNodeType_PscalarIntegralScope,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

	NoisyIrNode *	scalarIntegralsToken = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TscalarIntegrals, scope);
	addLeaf(N, n, scalarIntegralsToken);
	
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	
    addLeaf(N, n, noisyConfigParseScalarIntegralLists(N, scope));
	
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
NoisyIrNode *
noisyConfigParseScalarIntegralLists(NoisyState * N, NoisyScope * scope)
{
	NoisyIrNode *	n = genNoisyIrNode(
		N,
        kNoisyConfigIrNodeType_PscalarIntegralLists,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);

    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PscalarIntegralList)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseScalarIntegralList(N, scope));
    }
    
    /*
     * See the comment for noisyConfigParseVectorIntegralLists
     */
    Physics * current = N->scalarIntegralLists->head;
    int pastNumberTime = countNumberTime(current->numeratorDimensions) - countNumberTime(current->denominatorDimensions);

    while (current->next != NULL)
    {
        current = current->next;
        int curNumberTime = countNumberTime(current->numeratorDimensions) - countNumberTime(current->denominatorDimensions);
        assert(curNumberTime == pastNumberTime - 1);
        pastNumberTime = curNumberTime;
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
NoisyIrNode *
noisyConfigParseScalarIntegralList(NoisyState * N, NoisyScope * scope)
{
	NoisyIrNode *	n = genNoisyIrNode(
		N,
        kNoisyConfigIrNodeType_PscalarIntegralList,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);

    IntegralList *scalarIntegralList = (IntegralList *) calloc(1, sizeof(IntegralList));
    scalarIntegralList->head = NULL;

    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrac, scope);

    while (peekCheck(N, 1, kNoisyConfigIrNodeType_Tidentifier)) 
    {
        NoisyIrNode * physicsIdentifier = noisyConfigParseIdentifierUsageTerminal(N, kNoisyConfigIrNodeType_Tidentifier, scope);
        Physics * physics = noisyConfigPhysicsTablePhysicsForIdentifier(N, scope, physicsIdentifier->token->identifier);

        assert(!physics->isVector);

        if (physics == NULL)
        {
            noisyFatal(N, Esanity);
        }

        if (scalarIntegralList->head == NULL)
        {
            scalarIntegralList->head = copyPhysicsNode(physics);
        }
        else
        {
            getTailPhysics(scalarIntegralList->head)->next = copyPhysicsNode(physics);
        }

        addLeafWithChainingSeq(N, n, physicsIdentifier);
       
        if (peekCheck(N, 1 ,kNoisyConfigIrNodeType_TrightBrac)) {
            noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightBrac, scope);
            break;
        } else {
            noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcomma, scope);
        }
    }
    
    if (N->scalarIntegralLists == NULL)
        N->scalarIntegralLists = scalarIntegralList;
    else
        appendIntegralList(N->scalarIntegralLists, scalarIntegralList);

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
NoisyIrNode *
noisyConfigParseDimensionAliasScope(NoisyState *  N, NoisyScope *  scope)
{
	NoisyIrNode *	n = genNoisyIrNode(N,	kNoisyConfigIrNodeType_PdimensionAliasScope,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);


	NoisyIrNode *	dimensionAliasesToken = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TdimensionAliases, scope);
	addLeaf(N, n, dimensionAliasesToken);
	
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	
    addLeaf(N, n, noisyConfigParseDimensionAliasStatementList(N, scope));
	
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
NoisyIrNode *
noisyConfigParseDimensionAliasStatementList(NoisyState * N, NoisyScope * scope)
{
	NoisyIrNode *	n = genNoisyIrNode(
		N,
		kNoisyConfigIrNodeType_PdimensionAliasStatementList,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);

    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PdimensionAliasStatement)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseDimensionAliasStatement(N, scope));
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
NoisyIrNode *
noisyConfigParseDimensionAliasStatement(NoisyState *  N, NoisyScope *  currentScope)
{
	NoisyIrNode *	n = genNoisyIrNode(
						N,
						kNoisyConfigIrNodeType_PdimensionAliasStatement,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);


	if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PdimensionAliasStatement))
	{
        NoisyIrNode * derivedPhysicsIdentifier = noisyConfigParseIdentifier(N, currentScope);
        Physics * derivedPhysics = noisyConfigPhysicsTablePhysicsForIdentifier(N, currentScope, derivedPhysicsIdentifier->token->identifier);

        if (derivedPhysics == NULL)
        {
            noisyFatal(N, Esanity);
        }
		
        addLeaf(N, n, derivedPhysicsIdentifier);

		if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PassignOp))
		{
            noisyConfigParseAssignOp(N, currentScope);
            
            NoisyIrNode * dimensionAlias = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TstringConst, currentScope);
            derivedPhysics->dimensionAlias = dimensionAlias->token->stringConst;

			addLeaf(N, n, dimensionAlias);
		}
		else
		{
			noisyParserSyntaxError(N, kNoisyConfigIrNodeType_PdimensionAliasStatement, kNoisyIrNodeTypeMax);
		}
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyConfigIrNodeType_PdimensionAliasStatement, kNoisyIrNodeTypeMax);
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
NoisyIrNode *
noisyConfigParseLawScope(NoisyState *  N, NoisyScope *  scope)
{
	NoisyIrNode *	n = genNoisyIrNode(N,
                        kNoisyConfigIrNodeType_PlawScope,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

	NoisyIrNode *	lawToken = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tlaw, scope);
	addLeaf(N, n, lawToken);
	
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	
    addLeaf(N, n, noisyConfigParseLawStatementList(N, scope));
	
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
NoisyIrNode *
noisyConfigParseLawStatementList(NoisyState * N, NoisyScope * scope)
{
	NoisyIrNode *	n = genNoisyIrNode(
		N,
		kNoisyConfigIrNodeType_PlawStatementList,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);

    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PlawStatement)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseLawStatement(N, scope));
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
NoisyIrNode *
noisyConfigParseLawStatement(NoisyState *  N, NoisyScope *  currentScope)
{
	NoisyIrNode *	n = genNoisyIrNode(
						N,
						kNoisyConfigIrNodeType_PlawStatement,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);


	if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PlawStatement))
	{
        NoisyIrNode * derivedPhysicsIdentifier = noisyConfigParseIdentifier(N, currentScope);
        Physics * derivedPhysics = noisyConfigPhysicsTablePhysicsForIdentifier(N, currentScope, derivedPhysicsIdentifier->token->identifier);

        if (derivedPhysics == NULL)
        {
            derivedPhysics = noisyConfigPhysicsTableAddPhysicsForToken(N, currentScope, derivedPhysicsIdentifier->token);
        }
        
		addLeaf(N, n, derivedPhysicsIdentifier);

		if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PassignOp))
		{
            noisyConfigParseAssignOp(N, currentScope);

            NoisyIrNode * expression = noisyConfigParseExpression(N, currentScope);
			addLeaf(N, n, expression);
            
            noisyConfigPhysicsCopyNumeratorDimensions(N, derivedPhysics, expression->physics);
            noisyConfigPhysicsCopyDenominatorDimensions(N, derivedPhysics, expression->physics);


            /*
             * If LHS is declared a vector in vectorScalarPairScope, then 
             * the expression must evaluate to a vector.
             */
            if (derivedPhysics->isVector)
            {
                assert(expression->physics->isVector);
            }
		}
		else
		{
			noisyParserSyntaxError(N, kNoisyConfigIrNodeType_PdimensionTypeNameStatement, kNoisyIrNodeTypeMax);
		}
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyConfigIrNodeType_PlawStatement, kNoisyIrNodeTypeMax);
	}
    
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tsemicolon, currentScope);

	return n;
}

// The lower expression methods should take in Physics * and depending on 
// the operation, add previously existing Physics dimension dimension to 
// numerator or denominator list at Expression, Term, Factor level
// 1. Expression: Add or Subtract. 
//    * Make a new Physics struct inside the new NoisyIrNode
//    * Parse LHS which adds to numerator or denominator.
//    * Parse Add or Subtract
//    * Parse RHS but don't add anything to numerator or denominator
// 2. Term: Mul or Division.
//    * Make a new Physics struct inside the new NoisyIrNode
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
//    Make a new Physics struct inside the new NoisyIrNode
//    Parse LHS. Make sure such identifier exists in the global physics list
//    Recursively parse Expression
//    If identifier: search Physics list and add the resulting Physics numerator 
//      to passed down Physics numerator, and same for denominator
//    If dot product: add the resulting numerator list from
//        Physics struct from ParseExpression to numerator, and same for denominator
//        update the prime number
//    If cross product: same as above, but add Dimension angle to the 
//        resulting denominator list from Physics struct from Parse 
NoisyIrNode*
noisyConfigParseExpression(NoisyState *  N, NoisyScope *  currentScope)
{
    NoisyIrNode * left;
    NoisyIrNode * right;

    if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_Pterm))
    {
        left = noisyConfigParseTerm(N, currentScope);

        while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp))
        {
            addLeafWithChainingSeq(N, left, noisyConfigParseLowPrecedenceBinaryOp(N, currentScope));
            
            right = noisyConfigParseTerm(N, currentScope);
            addLeafWithChainingSeq(N, left, right);
            
            // compare LHS and RHS prime numbers and make sure they're equal
            assert(left->physics->numeratorPrimeProduct == right->physics->numeratorPrimeProduct);
            assert(left->physics->denominatorPrimeProduct == right->physics->denominatorPrimeProduct);
        }

    }
    else
    {
        noisyParserSyntaxError(N, kNoisyConfigIrNodeType_Pexpression, kNoisyIrNodeTypeMax);
        // noisyParserErrorRecovery(N, kNoisyConfigIrNodeType_Pexpression);
        noisyFatal(N, Esanity);
    }

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
NoisyIrNode *
noisyConfigParseTerm(NoisyState *  N, NoisyScope *  currentScope)
{
    NoisyIrNode *   intermediate = genNoisyIrNode(N,   kNoisyConfigIrNodeType_Pterm,
                        NULL /* left child */,
                        NULL /* right child */,
                        noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

    intermediate->physics = (Physics *) calloc(1, sizeof(Physics));
    intermediate->physics->numeratorPrimeProduct = 1;
    intermediate->physics->denominatorPrimeProduct = 1;

    if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PunaryOp))
    {
        addLeaf(N, intermediate, noisyConfigParseUnaryOp(N, currentScope));
    }

    NoisyIrNode * left = noisyConfigParseFactor(N, currentScope);
    addLeaf(N, intermediate, left);

    noisyConfigPhysicsCopyNumeratorDimensions(N, intermediate->physics, left->physics);
    noisyConfigPhysicsCopyDenominatorDimensions(N, intermediate->physics, left->physics);
    
    int numVectorsInTerm = 0;
    /*
     * If either LHS or RHS is a vector (not both), then the resultant is a vector
     */
    if (left->physics->isVector)
    {
        intermediate->physics->isVector = true;
        numVectorsInTerm++;
    }
    
    NoisyIrNode * right;

    while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp))
    {
        NoisyIrNode * binOp = noisyConfigParseHighPrecedenceBinaryOp(N, currentScope);
        addLeafWithChainingSeq(N, intermediate, binOp);
        
        right = noisyConfigParseFactor(N, currentScope);
        addLeafWithChainingSeq(N, intermediate, right);
        
        if (right->physics->isVector)
        {
            intermediate->physics->isVector = true;
            numVectorsInTerm++;
        }

        /*
         * Cannot perform multiply or divide operations on two vectors
         * e.g.) vector * scalar * scalar / vector is illegal because
         * it boils down to vector / vector which is illegal
         */
        assert(numVectorsInTerm < 2);

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

NoisyIrNode *
noisyConfigParseFactor(NoisyState * N, NoisyScope * currentScope)
{
    NoisyIrNode *   n;

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
        noisyParserSyntaxError(N, kNoisyConfigIrNodeType_Pfactor, kNoisyIrNodeTypeMax);
        noisyFatal(N, "noisyConfigParseFactor: missed a case in factor\n");
    }

    return n;
}

NoisyIrNode *
noisyConfigParseVectorOp(NoisyState *  N, NoisyScope * currentScope)
{
    NoisyIrNode *   intermediate = genNoisyIrNode(N,   kNoisyConfigIrNodeType_PvectorOp,
                        NULL /* left child */,
                        NULL /* right child */,
                        noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

    intermediate->physics = (Physics *) calloc(1, sizeof(Physics));
    intermediate->physics->numeratorPrimeProduct = 1;
    intermediate->physics->denominatorPrimeProduct = 1;
    
    bool addAngleToDenominator = false;
    
    if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tdot))
    {
        addLeaf(N, intermediate, noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tdot, currentScope));
    } 
    else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tcross))
    {
        addLeaf(N, intermediate, noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcross, currentScope));
        addAngleToDenominator = true;
    } 
    else 
    {
        noisyFatal(N, "noisyConfigParseVectorOp: op is not dot or cross\n");
    }
    
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftParen, currentScope);
    
    NoisyIrNode * left;
    left = noisyConfigParseExpression(N, currentScope);
    addLeafWithChainingSeq(N, intermediate, left);
    noisyConfigPhysicsCopyNumeratorDimensions(N, intermediate->physics, left->physics);
    noisyConfigPhysicsCopyDenominatorDimensions(N, intermediate->physics, left->physics);
    
    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcomma, currentScope);
    
    NoisyIrNode * right;
    right = noisyConfigParseExpression(N, currentScope);
    addLeafWithChainingSeq(N, intermediate, right);

    assert(left->physics->isVector && right->physics->isVector);

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

NoisyIrNode *
noisyConfigParseLowPrecedenceBinaryOp(NoisyState *  N, NoisyScope * currentScope)
{
    NoisyIrNode *   n;


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
        noisyParserSyntaxError(N, kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp, kNoisyIrNodeTypeMax);
        // noisyParserErrorRecovery(N, kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp);
        return NULL;
    }

    return n;
}

NoisyIrNode *
noisyConfigParseHighPrecedenceBinaryOp(NoisyState *  N, NoisyScope * currentScope)
{
    NoisyIrNode *   n;

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
        noisyParserSyntaxError(N, kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp, kNoisyIrNodeTypeMax);
        // noisyParserErrorRecovery(N, kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp);
        return NULL;
    }
    

    return n;
}

NoisyIrNode *
noisyConfigParseUnaryOp(NoisyState *  N, NoisyScope * currentScope)
{
    NoisyIrNode *   n = NULL;

    if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tminus))
    {
        n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tminus, currentScope);
        // TODO add noisyConfigParseExpression here
    }
    else
    {
        noisyParserSyntaxError(N, kNoisyConfigIrNodeType_PunaryOp, kNoisyIrNodeTypeMax);
        // noisyParserErrorRecovery(N, kNoisyConfigIrNodeType_PunaryOp);
        return NULL;
    }

    return n;
}


NoisyIrNode *
noisyConfigParseAssignOp(NoisyState *  N, NoisyScope * currentScope)
{
    if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tequals))
    {
        NoisyIrNode * n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tequals, currentScope);
        return n;
    }
    else
    {
        noisyParserSyntaxError(N, kNoisyConfigIrNodeType_PassignOp, kNoisyIrNodeTypeMax);
        // noisyParserErrorRecovery(N, kNoisyIrNodeType_PassignOp);
    }
    return NULL;

}

/*
 *  Simply remove a terminal
 */
NoisyIrNode *
noisyConfigParseTerminal(NoisyState *  N, NoisyIrNodeType expectedType, NoisyScope * currentScope)
{
    if (!peekCheck(N, 1, expectedType))
    {
        noisyParserSyntaxError(N, kNoisyIrNodeTypeMax, expectedType);

        /*
         *  In this case, we know the specific expected type/token.
         */
        // noisyParserErrorRecovery(N, expectedType);
    }

    NoisyToken *  t = noisyConfigLexGet(N);
    NoisyIrNode * n = genNoisyIrNode(N, t->type,
                        NULL /* left child */,
                        NULL /* right child */,
                        t->sourceInfo /* source info */);
    
    n->token = t;
    n->tokenString = t->identifier;
   
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
NoisyIrNode *
noisyConfigParseIdentifier(NoisyState *  N, NoisyScope *  currentScope)
{
	NoisyIrNode *	n;

	if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tidentifier))
	{
		n = noisyConfigParseIdentifierDefinitionTerminal(N, kNoisyConfigIrNodeType_Tidentifier, currentScope);
	    return n;
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyConfigIrNodeType_Tidentifier, kNoisyIrNodeTypeMax);
		// noisyParserErrorRecovery(N, kNoisyConfigIrNodeType_TidentifierOrNil);
	}
    return NULL;
}

/*
 *  Remove an identifier _usage_ terminal, performing symtab lookup
 */
NoisyIrNode *
noisyConfigParseIdentifierUsageTerminal(NoisyState *  N, NoisyIrNodeType expectedType, NoisyScope *  scope)
{
    if (!peekCheck(N, 1, expectedType))
    {
        noisyParserSyntaxError(N, kNoisyIrNodeTypeMax, expectedType);

        /*
         *  In this case, we know the specific expected type/token.
         */
        // noisyParserErrorRecovery(N, expectedType);
        return NULL;
    }

    NoisyToken *    t = noisyConfigLexGet(N);
    NoisyIrNode *   n = genNoisyIrNode(N,   t->type,
                        NULL /* left child */,
                        NULL /* right child */,
                        t->sourceInfo /* source info */);

    n->token = t;
    n->tokenString = t->identifier;
    
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
NoisyIrNode *
noisyConfigParseIdentifierDefinitionTerminal(NoisyState *  N, NoisyIrNodeType  expectedType, NoisyScope *  scope)
{
	if (!peekCheck(N, 1, expectedType))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeTypeMax, expectedType);

		/*
		 *	In this case, we know the specific expected type/token.
		 */
		// noisyParserErrorRecovery(N, expectedType);
	}

	NoisyToken *	t = noisyConfigLexGet(N);
	NoisyIrNode *	n = genNoisyIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

    n->token = t;
	n->tokenString = t->identifier;

	//	NOTE: noisyConfigSymbolTableAddOrLookupSymbolForToken(N, ) adds token 't' to scope 'scope'
    // TODO fill in symbol table stuff after finishing the IR scope
	// NoisySymbol *	sym = noisyConfigSymbolTableAddOrLookupSymbolForToken(N, scope, t);
	// if (sym->definition != NULL)
	// {
	// 	errorMultiDefinition(N, sym);
	// 	// TODO: do noisyParserErrorRecovery() here ?
	// }
	// n->symbol = sym;

	return n;
}
