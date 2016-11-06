#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisyconfig-errors.h"
#include "version.h"
#include "noisyconfig-timeStamps.h"
#include "noisyconfig.h"
#include "noisyconfig-parser.h"
#include "noisyconfig-parser-helper.h"
#include "noisyconfig-parser-errors.h"
#include "noisyconfig-lexer.h"
#include "noisyconfig-symbolTable.h"
#include "noisyconfig-irHelpers.h"
#include "noisyconfig-firstAndFollow.h"
#include "noisyconfig-types.h"


extern char *		gProductionStrings[];
extern char *		gProductionDescriptions[];
extern char *		gReservedTokenDescriptions[];
extern char *		gTerminalStrings[];
extern char *		gNoisyConfigAstNodeStrings[];
extern int		gNoisyConfigFirsts[kNoisyConfigIrNodeTypeMax][kNoisyConfigIrNodeTypeMax];

extern void		noisyConfigFatal(NoisyConfigState *  N, const char *  msg);
extern void		noisyConfigError(NoisyConfigState *  N, const char *  msg);



static char		kNoisyConfigErrorTokenHtmlTagOpen[]	= "<span style=\"background-color:#FFCC00; color:#FF0000;\">";
static char		kNoisyConfigErrorTokenHtmlTagClose[]	= "</span>";
static char		kNoisyConfigErrorDetailHtmlTagOpen[]	= "<span style=\"background-color:#A9E9FF; color:#000000;\">";
static char		kNoisyConfigErrorDetailHtmlTagClose[]	= "</span>";


NoisyConfigIrNode *
noisyConfigParse(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParse);

	return noisyConfigParseConfigFile(N, currentScope);
}


/*
 *	kNoisyConfigIrNodeType_PconfigFile
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_PprogtypeDeclaration
 *		node.right	= Xseq of kNoisyConfigIrNodeType_PnamegenDefinition
 */
NoisyConfigIrNode *
noisyConfigParseConfigFile(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseProgram);


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

	addLeaf(N, n, noisyConfigParseDimensionScope(N, currentScope), currentScope);
	
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
 *	kNoisyConfigIrNodeType_PdimensionScope
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_Tdimension 
 *		node.right	= kNoisyConfigIrNodeType_PscopedDimensionStatementList
 */
NoisyConfigIrNode *
noisyConfigParseDimensionScope(NoisyConfigState *  N, NoisyConfigScope *  scope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigIrNodeType_PdimensionScope);


	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	kNoisyConfigIrNodeType_PdimensionScope,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = scope;


	NoisyConfigIrNode *	dimensionToken = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tdimension, scope);
	addLeaf(N, n, dimensionToken, scope);
	
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);

	NoisyConfigIrNode *	scopeBegin	= noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	NoisyConfigScope *	dimensionScope = noisyConfigSymbolTableOpenScope(N, scope, scopeBegin);
	
    NoisyConfigIrNode *	typeTree = noisyConfigParseDimensionStatementList(N, dimensionScope);
    addLeaf(N, n, noisyConfigParseDimensionStatementList(N, dimensionScope), dimensionScope);
	
    NoisyConfigIrNode *	scopeEnd = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightBrace, scope);
	noisyConfigSymbolTableCloseScope(N, dimensionScope, scopeEnd);

	identifier->symbol->typeTree = typeTree;

	addToConfigFileTypeScopes(N, dimensionToken->symbol->identifier, dimensionScope);

    return n;
}



/*	
 *	kNoisyConfigIrNodeType_PdimensionStatementList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_PdimensionStatement or NULL
 *		node.right	= Xseq of kNoisyConfigIrNodeType_Pstatement
 */
NoisyConfigIrNode *
noisyConfigParseDimensionStatementList(NoisyConfigState * N, NoisyConfigScope * scope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyTimeStampKeyParserDimensionStatementList);

	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
		N,
		kNoisyConfigIrNodeType_PdimensionStatementList,
		NULL /* left child */,
		NULL /* right child */,
		noisyConfigLexPeek(N, 1)->sourceInfo /* source info */
	);
    n->currentScope = scope;

    while (noisyConfigInFirst(kNoisyConfigIrNodeType_PdimensionStatement)) 
    {
        addLeafWithChainingSeq(N, n, noisyConfigParseDimensionStatement(N, scope), scope);
	    noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tsemicolon, scope);
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
noisyConfigParseDimensionStatement(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseDimensionStatement);


	NoisyConfigIrNode *	n = genNoisyConfigIrNode(
						N,
						kNoisyConfigIrNodeType_PdimensionStatement,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = currentScope;


	if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tsemicolon))
	{
		noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tsemicolon, currentScope);

		return n;
	}

	if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_Tidentifier))
	{
		addLeaf(
			N,
			n,
			noisyConfigParseIdentifier(N, currentScope),
			currentScope
		);

		if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PassignOp))
		{
			addLeafWithChainingSeq(N, n, noisyConfigParseAssignOp(N, currentScope), currentScope);
			addLeafWithChainingSeq(N, n, noisyConfigParseExpression(N, currentScope), currentScope);
		}
		else
		{
			noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_Pstatement, kNoisyConfigIrNodeTypeMax);
			noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_Pstatement);
		}
	}
	else if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PdimensionStatementList))
	{
		addLeaf(N, n, noisyConfigParseDimensionStatementList(N, currentScope), currentScope);
	}
	else
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_Pstatement, kNoisyConfigIrNodeTypeMax);
		noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_Pstatement);
	}

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
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseIdentifier);

	NoisyConfigIrNode *	n;

	if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tidentifier))
	{
		/*
		 *	The typeTree's get filled-in in our caller
		 */
		n = noisyConfigParseIdentifierDefinitionTerminal(N, kNoisyConfigIrNodeType_Tidentifier, currentScope);

		while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PfieldSelect))
		{
			addLeafWithChainingSeq(N, n, noisyConfigParseFieldSelect(N, currentScope), currentScope);
		}
	}
	else
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PidentifierOrNil, kNoisyConfigIrNodeTypeMax);
		noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_PidentifierOrNil);
	}
    
    n->currentScope = currentScope;

	return n;
}


/*
 *	kNoisyConfigIrNodeType_PidentifierList
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyConfigIrNodeType_Tidentifier
 *		node.left	= kNoisyConfigIrNodeType_Tidentifier
 *		node.right	= Xseq of kNoisyConfigIrNodeType_Tidentifier
 */
NoisyConfigIrNode *
noisyConfigParseIdentifierList(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseIdentifierList);


	NoisyConfigIrNode *	n;


	/*
	 *	The typeTree's get filled in in our caller
	 */
	n = noisyConfigParseIdentifierDefinitionTerminal(N, kNoisyConfigIrNodeType_Tidentifier, currentScope);
	
	/*
	 *	Could also have done
	 *		while (peekCheck(N, 1, kNoisyConfigIrNodeType_Tcomma))
	 */
	while (!noisyConfigInFollow(N, kNoisyConfigIrNodeType_PidentifierList))
	{
		noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcomma, currentScope);
		addLeafWithChainingSeq(N, n, noisyConfigParseIdentifierDefinitionTerminal(N, kNoisyConfigIrNodeType_Tidentifier, currentScope), currentScope);
	}
    n->currentScope = currentScope;


	return n;
}





/*
 *	kNoisyConfigIrNodeType_PanonAggregateType
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyConfigIrNodeType_ParrayType | ... | kNoisyConfigIrNodeType_PsetType
 *		node.left	= NULL
 *		node.right	= NULL
 */
NoisyConfigIrNode *
noisyConfigParseAnonAggregateType(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseAnonAggregateType);


	NoisyConfigIrNode *	n;


	if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_ParrayType))
	{
		n = noisyConfigParseArrayType(N, currentScope);
	}
	else if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PlistType))
	{
		n = noisyConfigParseListType(N, currentScope);
	}
	else if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PtupleType))
	{
		n = noisyConfigParseTupleType(N, currentScope);
	}
	else if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PsetType))
	{
		n = noisyConfigParseSetType(N, currentScope);
	}
	else
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PanonAggregateType, kNoisyConfigIrNodeTypeMax);
		noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_PanonAggregateType);
	}
    n->currentScope = currentScope;

	return n;
}



/*
 *	kNoisyConfigIrNodeType_PlistType
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_PtypeExpression
 *		node.right	= NULL
 */
NoisyConfigIrNode *
noisyConfigParseListType(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseListType);


	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	kNoisyConfigIrNodeType_PlistType,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = currentScope;


	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tlist, currentScope);
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tof, currentScope);
	addLeaf(N, n, noisyConfigParseTypeExpression(N, currentScope), currentScope);

	return n;
}




/*
 *	kNoisyConfigIrNodeType_PidxInitList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_Pelement
 *		node.right	= Xseq of kNoisyConfigIrNodeType_Pelement
 */
NoisyConfigIrNode *
noisyConfigParseIdxInitList(NoisyConfigState *  N, NoisyConfigScope *  scope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseIdxInitList);


	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	kNoisyConfigIrNodeType_PinitList,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = scope;


	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrace, scope);
	addLeaf(N, n, noisyConfigParseElement(N, scope), scope);
	while (peekCheck(N, 1, kNoisyConfigIrNodeType_Tcomma))
	{
		noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcomma, scope);
		addLeafWithChainingSeq(N, n, noisyConfigParseElement(N, scope), scope);
	}
	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightBrace, scope);

	return n;
}





/*
 *	kNoisyConfigIrNodeType_PassignOp
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyConfigIrNodeType_Tas | ... | kNoisyConfigIrNodeType_TdefineAs
 *		node.left	= NULL
 *		node.right	= NULL
 */
NoisyConfigIrNode *
noisyConfigParseAssignOp(NoisyConfigState *  N, NoisyConfigScope * scope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseAssignOp);


	NoisyConfigIrNode *	n;

	if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tequals))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tequals, scope);
	}
	else
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PassignOp, kNoisyConfigIrNodeTypeMax);
		noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_PassignOp);
	}
    n->currentScope = scope;

	return n;
}



/*
 *	kNoisyConfigIrNodeType_Pexpression
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyConfigIrNodeType_Pterm | kNoisyConfigIrNodeType_PanonAggregateCastExpression | ... | kNoisyConfigIrNodeType_Pname2chanExpression
 *		node.left	= kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp
 *		node.right	= Xseq of kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp, kNoisyConfigIrNodeType_Pterm
 */
NoisyConfigIrNode *
noisyConfigParseExpression(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseExpression);


	NoisyConfigIrNode *	n;


	if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_Pterm))
	{
		n = noisyConfigParseTerm(N, currentScope);

		while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp))
		{
			addLeafWithChainingSeq(N, n, noisyConfigParseLowPrecedenceBinaryOp(N, currentScope), currentScope);
			addLeafWithChainingSeq(N, n, noisyConfigParseTerm(N, currentScope), currentScope);
		}
	}
	else if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PanonAggregateCastExpression))
	{
		n = noisyConfigParseAnonAggregateCastExpression(N, currentScope);
	}
	else
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_Pexpression, kNoisyConfigIrNodeTypeMax);
		noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_Pexpression);
	}
    n->currentScope = currentScope;

	return n;
}




/*
 *	kNoisyConfigIrNodeType_ParrayCastExpression
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_PidxInitList | kNoisyConfigIrNodeType_TintConst
 *		node.right	= NULL | kNoisyConfigIrNodeType_PstarInitList
 */
NoisyConfigIrNode *
noisyConfigParseArrayCastExpression(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseArrayCastExpression);


	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	kNoisyConfigIrNodeType_ParrayCastExpression,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);
    n->currentScope = currentScope;


	noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tarray, currentScope);
	
	if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tof))
	{
		noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tof, currentScope);
		addLeaf(N, n, noisyConfigParseIdxInitList(N, currentScope), currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_TleftBrac))
	{
		noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftBrac, currentScope);
		addLeaf(N, n, noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TintConst, currentScope), currentScope);
		noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightBrac, currentScope);
		noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tof, currentScope);
		addLeaf(N, n, noisyConfigParseStarInitList(N, currentScope), currentScope);
	}
	else
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_ParrayCastExpression, kNoisyConfigIrNodeTypeMax);
		noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_ParrayCastExpression);
	}

	return n;
}

/*
 *	kNoisyConfigIrNodeType_PanonAggregateCastExpression
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyConfigIrNodeType_PlistCast | kNoisyConfigIrNodeType_PsetCast | kNoisyConfigIrNodeType_ParrayCast
 *		node.left	= NULL
 *		node.right	= NULL
 */
NoisyConfigIrNode *
noisyConfigParseAnonAggregateCastExpression(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseAnonAggregateCastExpression);


	NoisyConfigIrNode *	n;


	if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_ParrayCastExpression))
	{
		n = noisyConfigParseArrayCastExpression(N, currentScope);
	}
	else
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PanonAggregateCastExpression, kNoisyConfigIrNodeTypeMax);
		noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_PanonAggregateCastExpression);
	}
    n->currentScope = currentScope;

	return n;
}



/*
 *	kNoisyConfigIrNodeType_Pterm
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyConfigIrNodeType_Pfactor
 *		node.right	= Xseq of kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp  and kNoisyConfigIrNodeType_Pfactor
 */
NoisyConfigIrNode *
noisyConfigParseTerm(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseTerm);


	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	kNoisyConfigIrNodeType_Pterm,
						NULL /* left child */,
						NULL /* right child */,
						noisyConfigLexPeek(N, 1)->sourceInfo /* source info */);

    n->currentScope = currentScope;

	/*
	 *	TODO/BUG: Double-check our handling of [basictype] and [unop] here
	 */
	if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PbasicType))
	{
		addLeaf(N, n, noisyConfigParseBasicType(N, currentScope), currentScope);
	}

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

/*
 *	kNoisyConfigIrNodeType_Pfactor
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyConfigIrNodeType_Tidentifier | ... | kNoisyConfigIrNodeType_Pexpression
 *		node.left	= NULL or kNoisyConfigIrNodeType_PfieldSelect
 *		node.right	= NULL or Xseq of kNoisyConfigIrNodeType_PfieldSelect
 */
NoisyConfigIrNode *
noisyConfigParseFactor(NoisyConfigState *  N, NoisyConfigScope *  currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseFactor);


	NoisyConfigIrNode *	n;

	if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tidentifier))
	{
		n = noisyConfigParseIdentifierUsageTerminal(N, kNoisyConfigIrNodeType_Tidentifier, currentScope);

		while (noisyConfigInFirst(N, kNoisyConfigIrNodeType_PfieldSelect))
		{
			/*
			 *	TODO/BUG: This looks suspicious.
			 */
			addLeafWithChainingSeq(N, n, noisyConfigParseFieldSelect(N, currentScope), currentScope);
		}
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_TintConst))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TintConst, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_TrealConst))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrealConst, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_TstringConst))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TstringConst, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_TboolConst))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TboolConst, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_TleftParen) && peekCheck(N, 3, kNoisyConfigIrNodeType_Tcomma))
	{
		/*
		 *	If we see
		 *
		 *			'(' identifier ','
		 *
		 *	then try to parse a tupleValue. Otherwise, parse an expression
		 */
		n = noisyConfigParseTupleValue(N, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_TleftParen))
	{
		noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TleftParen, currentScope);
		n = noisyConfigParseExpression(N, currentScope);
		noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_TrightParen, currentScope);
	}
/*
 *	TODO/BUG: the following two should not be here. See grammar
 *
 *	else if (noisyConfigInFirst(N, kNoisyConfigIrNodeType_Punop))
 *	{
 *		n = noisyConfigParseunop(N, currentScope);
 *		addLeaf(N, n, noisyConfigParseFactor(N, currentScope));
 *	}
 *	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tchan2name))
 *	{
 *		noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tchan2name);
 *		n = noisyConfigParseFactor(N, currentScope);
 *
 *		if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tstring))
 *		{
 *			addLeaf(N, n, noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tstring));
 *		}
 *	}
 */
	else
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_Pfactor, kNoisyConfigIrNodeTypeMax);
		noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_Pfactor);
	}
    n->currentScope = currentScope;

	return n;
}




/*
 *	kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyConfigIrNodeType_Tasterisk | ... | kNoisyConfigIrNodeType_TCONS
 *		node.left	= NULL
 *		node.right	= NULL
 */
NoisyConfigIrNode *
noisyConfigParseHighPrecedenceBinaryOp(NoisyConfigState *  N, NoisyConfigScope * currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseHighPrecedenceBinaryO);


	NoisyConfigIrNode *	n;


	if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tmul))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tmul, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tdiv))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tdiv, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tpercent))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tpercent, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tcaret))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcaret, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tcons))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tcons, currentScope);
	}
	else
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp, kNoisyConfigIrNodeTypeMax);
		noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp);
	}
    n->currentScope = currentScope;

	return n;
}

/*
 *	kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyConfigIrNodeType_Tplus | ... | kNoisyConfigIrNodeType_Tstroke
 *		node.left	= NULL
 *		node.right	= NULL
 */
NoisyConfigIrNode *
noisyConfigParseLowPrecedenceBinaryOp(NoisyConfigState *  N, NoisyConfigScope * currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseLowPrecedenceBinaryOp);


	NoisyConfigIrNode *	n;


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
		noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp);
	}
    n->currentScope = currentScope;

	return n;
}



/*
 *	kNoisyConfigIrNodeType_Punop
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyConfigIrNodeType_Tname2chan | ... | kNoisyConfigIrNodeType_Tgets
 *		node.left	= NULL
 *		node.right	= NULL
 */
NoisyConfigIrNode *
noisyConfigParseUnaryOp(NoisyConfigState *  N, NoisyConfigScope * currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseUnaryOp);


	NoisyConfigIrNode *	n = NULL;


	//name2chan is not a unary op. Why did we have it here?
	//if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tname2chan))
	//{
	//	n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tname2chan);
	//}
	//else
	if (peekCheck(N, 1, kNoisyConfigIrNodeType_Ttilde))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Ttilde, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tbang))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tbang, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tminus))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tminus, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tplus))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tplus, currentScope);
	}
	//T_inc is not a unary op. Why did we have it here?
	//else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tinc))
	//{
	//	n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tinc);
	//}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tgets))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tgets, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Thd))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Thd, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Ttl))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Ttl, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyConfigIrNodeType_Tlen))
	{
		n = noisyConfigParseTerminal(N, kNoisyConfigIrNodeType_Tlen, currentScope);
	}
	else
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeType_PunaryOp, kNoisyConfigIrNodeTypeMax);
		noisyConfigParserErrorRecovery(N, kNoisyConfigIrNodeType_PunaryOp);
	}
    n->currentScope = currentScope;

	return n;
}

/*
 *	Simply remove a terminal
 */
NoisyConfigIrNode *
noisyConfigParseTerminal(NoisyConfigState *  N, NoisyConfigIrNodeType expectedType, NoisyConfigScope * currentScope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseTerminal);

	if (!peekCheck(N, 1, expectedType))
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeTypeMax, expectedType);

		/*
		 *	In this case, we know the specific expected type/token.
		 */
		noisyConfigParserErrorRecovery(N, expectedType);
	}

	NoisyConfigToken *	t = noisyConfigLexGet(N);
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);
    n->currentScope = currentScope;

	return n;
}

/*
 *	Remove an identifier _usage_ terminal, performing symtab lookup
 */
NoisyConfigIrNode *
noisyConfigParseIdentifierUsageTerminal(NoisyConfigState *  N, NoisyConfigIrNodeType expectedType, NoisyConfigScope *  scope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseIdentifierUsageTerminal);

	if (!peekCheck(N, 1, expectedType))
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeTypeMax, expectedType);

		/*
		 *	In this case, we know the specific expected type/token.
		 */
		noisyConfigParserErrorRecovery(N, expectedType);
	}

	NoisyConfigToken *	t = noisyConfigLexGet(N);
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

	n->tokenString = t->identifier;

	n->symbol = noisyConfigSymbolTableSymbolForIdentifier(N, scope, t->identifier);
    n->currentScope = scope;
	if (n->symbol == NULL)
	{
		errorUseBeforeDefinition(N, t->identifier);
		// TODO: do noisyConfigParserErrorRecovery() here ?
	}

	return n;
}

/*
 *	Remove an identifier _definition_ terminal, performing symtab insertion
 */
NoisyConfigIrNode *
noisyConfigParseIdentifierDefinitionTerminal(NoisyConfigState *  N, NoisyConfigIrNodeType  expectedType, NoisyConfigScope *  scope)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyParseIdentifierDefinitionTerminal);

	if (!peekCheck(N, 1, expectedType))
	{
		noisyConfigParserSyntaxError(N, kNoisyConfigIrNodeTypeMax, expectedType);

		/*
		 *	In this case, we know the specific expected type/token.
		 */
		noisyConfigParserErrorRecovery(N, expectedType);
	}

	NoisyConfigToken *	t = noisyConfigLexGet(N);
	NoisyConfigIrNode *	n = genNoisyConfigIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

	n->tokenString = t->identifier;
    n->currentScope = scope;

	//	NOTE: noisyConfigSymbolTableAddOrLookupSymbolForToken(N, ) adds token 't' to scope 'scope'
	NoisyConfigSymbol *	sym = noisyConfigSymbolTableAddOrLookupSymbolForToken(N, scope, t);
	if (sym->definition != NULL)
	{
		errorMultiDefinition(N, sym);
		// TODO: do noisyConfigParserErrorRecovery() here ?
	}
	n->symbol = sym;

	return n;
}






