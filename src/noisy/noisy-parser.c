/*
	Authored 2015-2018. Phillip Stanley-Marbell.

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
// #define _GNU_SOURCE

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "version.h"
#include "noisy-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "noisy-parser.h"
#include "common-lexers-helpers.h"
#include "noisy-lexer.h"
#include "common-symbolTable.h"
#include "common-irHelpers.h"
#include "noisy-irHelpers.h"
#include "common-firstAndFollow.h"
#include "noisy-typeCheck.h"



extern char *		gNoisyProductionDescriptions[];
extern const char *	gNoisyTokenDescriptions[];
extern char *		gAstNodeStrings[];
extern int		gNoisyFirsts[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax];
extern int		gNoisyFollows[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax];

extern void		fatal(State *  N, const char *  msg);
extern void		error(State *  N, const char *  msg);




static char		kNoisyErrorTokenHtmlTagOpen[]	= "<span style=\"background-color:#FFCC00; color:#FF0000;\">";
static char		kNoisyErrorTokenHtmlTagClose[]	= "</span>";
static char		kNoisyErrorDetailHtmlTagOpen[]	= "<span style=\"background-color:#A9E9FF; color:#000000;\">";
static char		kNoisyErrorDetailHtmlTagClose[]	= "</span>";


IrNode *
noisyParse(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParse);

	return noisyParseProgram(N, currentScope);
}


/*
 *	kNoisyIrNodeType_Pprogram
 *
 *	Grammar production:
 *		program		::=	moduleDecl {(moduleDecl | functionDefn | problemDefn | predicateFnDefn)} .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PmoduleDecl
 *		node.right	= Xseq of one or more of kNoisyIrNodeType_PmoduleDecl, kNoisyIrNodeType_PfunctionDefn, kNoisyIrNodeType_PproblemDefn, or kNoisyIrNodeType_PpredicateFnDefn
 */
IrNode *
noisyParseProgram(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseProgram);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_Pprogram,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	/*
	 *	Before we start parsing, set begin source line of toplevel scope.
	 */
	currentScope->begin = lexPeek(N, 1)->sourceInfo;

	addLeaf(N, n, noisyParseModuleDecl(N, currentScope));
	while (!inFollow(N, kNoisyIrNodeType_Pprogram, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		if (peekCheck(N, 3, kNoisyIrNodeType_Tmodule))
		{
			addLeafWithChainingSeq(N, n, noisyParseModuleDecl(N, currentScope));
		}
		else if (peekCheck(N, 3, kNoisyIrNodeType_Tfunction))
		{
			addLeafWithChainingSeq(N, n, noisyParseFunctionDefn(N, currentScope));
		}
		else if (peekCheck(N, 3, kNoisyIrNodeType_Tprobdef))
		{
			addLeafWithChainingSeq(N, n, noisyParseProblemDefn(N, currentScope));
		}
		else if (peekCheck(N, 3, kNoisyIrNodeType_Tpredicate))
		{
			addLeafWithChainingSeq(N, n, noisyParsePredicateFnDefn(N, currentScope));
		}
		else
		{
			noisyParserSyntaxError(N, kNoisyIrNodeType_Pprogram, kNoisyIrNodeTypeMax, gNoisyFirsts);
			noisyParserErrorRecovery(N, kNoisyIrNodeType_Pprogram);
		}
	}

	/*
	 *	We can now fill in end src info for toplevel scope.
	 */
	currentScope->end = lexPeek(N, 1)->sourceInfo;

	if (!inFollow(N, kNoisyIrNodeType_Pprogram, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Pprogram, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Pprogram);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PmoduleDecl
 *
 *	Grammar production:
 *		moduleDecl	::=	identifier ":" "module" "(" typeParameterList ")" "{" moduleDeclBody "}" .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= Xseq of kNoisyIrNodeType_PtypeParameterList and kNoisyIrNodeType_PmoduleDeclBody
 */
IrNode *
noisyParseModuleDecl(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseModuleDecl);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PmoduleDecl,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	IrNode *	identifier = noisyParseIdentifierDefinitionTerminal(N, scope);

	identifier->symbol->symbolType = kNoisySymbolTypeModule;

	addLeaf(N, n, identifier);
	noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
	noisyParseTerminal(N, kNoisyIrNodeType_Tmodule);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeafWithChainingSeq(N, n, noisyParseTypeParameterList(N, scope));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	/*
	 *	We keep a global handle on the module scope
	 */
	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	Scope *		moduleScope	= commonSymbolTableOpenScope(N, scope, scopeBegin);
	IrNode *	typeTree	= noisyParseModuleDeclBody(N, moduleScope);

	addLeaf(N, n, typeTree);

	IrNode *	scopeEnd	= noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);

	commonSymbolTableCloseScope(N, moduleScope, scopeEnd);
	identifier->symbol->typeTree = typeTree;

	addToModuleScopes(N, identifier->symbol->identifier, moduleScope);

	if (!inFollow(N, kNoisyIrNodeType_PmoduleDecl, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PmoduleDecl, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PmoduleDecl);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PmoduleDeclBody
 *
 *	Grammar production:
 *		moduleDeclBody		::=	{moduleTypeNameDecl ";"} .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PmoduleTypeNameDecl
 *		node.right	= Xseq of one or more kNoisyIrNodeType_PmoduleTypeNameDecl
 */
IrNode *
noisyParseModuleDeclBody(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseModuleDeclBody);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PmoduleDeclBody,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	while (!inFollow(N, kNoisyIrNodeType_PmoduleDeclBody, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParseModuleTypeNameDecl(N, scope));
		noisyParseTerminal(N, kNoisyIrNodeType_Tsemicolon);
	}

	if (!inFollow(N, kNoisyIrNodeType_PmoduleDeclBody, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PmoduleDeclBody, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PmoduleDeclBody);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PmoduleTypeNameDecl
 *
 *	Grammar production:
 *		moduleTypeNameDecl	::=	identifier ":" (constantDecl | typeDecl | typeAnnoteDecl | functionDecl | probdefDecl | predicateFnDecl) .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pidentifier
 *		node.right	= kNoisyIrNodeType_PconstantDecl | kNoisyIrNodeType_PtypeDecl | kNoisyIrNodeType_PtypeAnnoteDecl
 *				  kNoisyIrNodeType_PfunctionDecl | kNoisyIrNodeType_PprobdefDecl | kNoisyIrNodeType_PpredicateFnDecl
 */
IrNode *
noisyParseModuleTypeNameDecl(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseModuleTypenameDecl);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PmoduleTypeNameDecl,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	IrNode *	identifier = noisyParseIdentifierDefinitionTerminal(N, scope);
	addLeaf(N, n, identifier);
	noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);


	IrNode *	typeExpr;
	if (inFirst(N, kNoisyIrNodeType_PconstantDecl, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		typeExpr = noisyParseConstantDecl(N, scope);
		identifier->symbol->symbolType = kNoisySymbolTypeConstantDeclaration;
	}
	else if (inFirst(N, kNoisyIrNodeType_PtypeDecl, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		typeExpr = noisyParseTypeDecl(N, scope);
		identifier->symbol->symbolType = kNoisySymbolTypeTypeDeclaration;
	}
	else if (inFirst(N, kNoisyIrNodeType_PtypeAnnoteDecl, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		typeExpr = noisyParseTypeAnnoteDecl(N, scope);
	}
	else if (inFirst(N, kNoisyIrNodeType_PfunctionDecl, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		typeExpr = noisyParseFunctionDecl(N, scope);
		identifier->symbol->symbolType = kNoisySymbolTypeNamegenDeclaration;
	}
	else if (inFirst(N, kNoisyIrNodeType_PprobdefDecl, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		typeExpr = noisyParseProbdefDecl(N, scope);
	}
	else if (inFirst(N, kNoisyIrNodeType_PpredicateFnDecl, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		typeExpr = noisyParsePredicateFnDecl(N, scope);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PmoduleTypeNameDecl, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PmoduleTypeNameDecl);
	}
	addLeaf(N, n, typeExpr);

	/*
	 *	We've now figured out type ascribed to members of identifierList
	 */
	assignTypes(N, identifier, typeExpr);

	if (!inFollow(N, kNoisyIrNodeType_PmoduleTypeNameDecl, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PmoduleTypeNameDecl, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PmoduleTypeNameDecl);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PconstantDecl
 *
 *	Grammar production:
 *		constantDecl	=	"const" (integerConst | realConst | boolConst) .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_TintegerConst | kNoisyIrNodeType_TrealConst | kNoisyIrNodeType_TboolConst
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseConstantDecl(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseConstantDecl);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PconstantDecl,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

	noisyParseTerminal(N, kNoisyIrNodeType_Tconst);

	if (peekCheck(N, 1, kNoisyIrNodeType_TintegerConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintegerConst));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TrealConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TboolConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TboolConst));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PconstantDecl, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PconstantDecl);
	}

	if (!inFollow(N, kNoisyIrNodeType_PconstantDecl, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PconstantDecl, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PconstantDecl);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PtypeDecl
 *
 *	Grammar production:
 *		typeDecl		=	("type" typeExpr) | adtTypeDecl | vectorTypeDecl .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Ttype | kNoisyIrNodeType_PadtTypeDecl | kNoisyIrNodeType_PvectorTypeDecl
 *		node.right	= kNoisyIrNodeType_PtypeExpr | NULL
 */
IrNode *
noisyParseTypeDecl(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTypeDecl);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PtypeDecl,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Ttype))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Ttype));
		addLeaf(N, n, noisyParseTypeExpr(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tadt))
	{
		addLeaf(N, n, noisyParseAdtTypeDecl(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tvector))
	{
		addLeaf(N, n, noisyParseVectorTypeDecl(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeDecl, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeDecl);
	}

	if (!inFollow(N, kNoisyIrNodeType_PtypeDecl, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeDecl, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeDecl);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PtypeAnnoteDecl
 *
 *	Grammar production:
 *		typeAnnoteDecl		::=	"typeannote" typeAnnoteList .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PtypeAnnoteList
 *		node.right	= NULL
 */
IrNode *
noisyParseTypeAnnoteDecl(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTypeAnnoteDecl);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PtypeAnnoteDecl,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

	noisyParseTerminal(N, kNoisyIrNodeType_Ttypeannote);
	addLeaf(N, n, noisyParseTypeAnnoteList(N, currentScope));

	return n;
}



/*
 *	kNoisyIrNodeType_PadtTypeDecl
 *
 *	Grammar production:
 *		adtTypeDecl	::=	"adt" "{" identifierList ":" typeExpr ";" {identifierList ":" typeExpr ";"} [valfnSignature ";"] "}" .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PidentifierList
 *		node.right	= Xseq of kNoisyIrNodeType_PtypeExpr + zero or more kNoisyIrNodeType_PidentifierList+kNoisyIrNodeType_PtypeExpr
 */
IrNode *
noisyParseAdtTypeDecl(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseAdtTypeDecl);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PadtTypeDecl,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tadt);

	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	Scope *		currentScope	= commonSymbolTableOpenScope(N, scope, scopeBegin);
	IrNode *	identifierList	= noisyParseIdentifierList(N, currentScope);

	addLeaf(N, n, identifierList);
	noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);

	IrNode *	typeExpr	= noisyParseTypeExpr(N, currentScope);

	addLeafWithChainingSeq(N, n, typeExpr);
	assignTypes(N, identifierList, typeExpr);
	noisyParseTerminal(N, kNoisyIrNodeType_Tsemicolon);

	while (!peekCheck(N, 1, kNoisyIrNodeType_TrightBrace))
	{
		IrNode *	identifierList2	= noisyParseIdentifierList(N, currentScope);

		addLeafWithChainingSeq(N, n, identifierList2);
		noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
		IrNode *	typeExpr2 = noisyParseTypeExpr(N, currentScope);
		addLeafWithChainingSeq(N, n, typeExpr2);
		assignTypes(N, identifierList2, typeExpr2);
		noisyParseTerminal(N, kNoisyIrNodeType_Tsemicolon);
	}

	if (inFirst(N, kNoisyIrNodeType_PvalfnSignature, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParseValfnSignature(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_Tsemicolon);
	}

	IrNode *	scopeEnd  = noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);
	commonSymbolTableCloseScope(N, currentScope, scopeEnd);

	if (!inFollow(N, kNoisyIrNodeType_PadtTypeDecl, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PadtTypeDecl, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PadtTypeDecl);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PvalfnSignature
 *
 *	Grammar production:
 *		valfnSignature		=	identifier ":" "valfn" .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pidentifier
 *		node.right	= NULL
 */
IrNode *
noisyParseValfnSignature(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseValfnSignature);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PvalfnSignature,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, scope));

	if (!inFollow(N, kNoisyIrNodeType_PvalfnSignature, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PvalfnSignature, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PvalfnSignature);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PvectorTypeDecl
 *
 *	Grammar production:
 *		vectorTypeDecl		::=	"vector" "[" integerConst "]" "of" typeExpr .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_TintegerConst
 *		node.right	= kNoisyIrNodeType_PtypeExpr
 */
IrNode *
noisyParseVectorTypeDecl(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseVectorType);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PvectorTypeDecl,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tvector);

	noisyParseTerminal(N, kNoisyIrNodeType_TleftBracket);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintegerConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBracket);

	noisyParseTerminal(N, kNoisyIrNodeType_TleftBracket);
	addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintegerConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBracket);
	
	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeafWithChainingSeq(N, n, noisyParseTypeExpr(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PvectorTypeDecl, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PvectorTypeDecl, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PvectorTypeDecl);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PfunctionDecl
 *
 *	Grammar production:
 *		functionDecl		::=	"function" writeTypeSignature "->" readTypeSignature .
 *
 *	Generated AST subtree:
 *
 *		node.left	= writeTypeSignature
 *		node.right	= readTypeSignature
 */
IrNode *
noisyParseFunctionDecl(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseFunctionDecl);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PfunctionDecl,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_Tfunction);
	Scope *		currentScope	= commonSymbolTableOpenScope(N, scope, scopeBegin);

	addLeaf(N, n, noisyParseWriteTypeSignature(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_Tarrow);

	IrNode *	scopeEnd = noisyParseReadTypeSignature(N, currentScope);

	addLeaf(N, n, scopeEnd);
	commonSymbolTableCloseScope(N, currentScope, scopeEnd);

	if (!inFollow(N, kNoisyIrNodeType_PfunctionDecl, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PfunctionDecl, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PfunctionDecl);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PprobdefDecl
 *
 *	Grammar production:
 *		probdefDecl		::=	"probdef" writeTypeSignature "->" readTypeSignature .
 *
 *	Generated AST subtree:
 *
 *		node.left	= writeTypeSignature
 *		node.right	= readTypeSignature
 */
IrNode *
noisyParseProbdefDecl(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseProbdefDecl);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PprobdefDecl,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_Tprobdef);
	Scope *		currentScope	= commonSymbolTableOpenScope(N, scope, scopeBegin);

	addLeaf(N, n, noisyParseWriteTypeSignature(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_Tarrow);

	IrNode *	scopeEnd = noisyParseWriteTypeSignature(N, currentScope);

	addLeaf(N, n, scopeEnd);
	commonSymbolTableCloseScope(N, currentScope, scopeEnd);

	if (!inFollow(N, kNoisyIrNodeType_PprobdefDecl, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PprobdefDecl, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PprobdefDecl);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PreadTypeSignature
 *
 *	Grammar production:
 *		readTypeSignature	=	signature .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Psignature
 *		node.right	= NULL
 */
IrNode *
noisyParseReadTypeSignature(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseReadTypeSignature);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PreadTypeSignature,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseSignature(N, scope,true));

	if (!inFollow(N, kNoisyIrNodeType_PreadTypeSignature, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PreadTypeSignature, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PreadTypeSignature);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PwriteTypeSignature
 *
 *	Grammar production:
 *		writeTypeSignature	=	signature .
 *
 *	Generated AST subtree:
 *
 *		node.left	= writeTypeSignature
 *		node.right	= NULL
 */
IrNode *
noisyParseWriteTypeSignature(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseWriteTypeSignature);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PwriteTypeSignature,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseSignature(N, scope,false));

	if (!inFollow(N, kNoisyIrNodeType_PwriteTypeSignature, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PwriteTypeSignature, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PwriteTypeSignature);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PpredicateFnDecl
 *
 *	Grammar production:
 *		predicateFnDecl		::=	"predicate" signature .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Psignature
 *		node.right	= NULL
 */
IrNode *
noisyParsePredicateFnDecl(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParsePredicateFnDecl);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PpredicateFnDecl,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_Tpredicate);
	Scope *		currentScope	= commonSymbolTableOpenScope(N, scope, scopeBegin);
	/*
	*	TODO; This parse signature may be wrong!
	*/
	IrNode *	scopeEnd = noisyParseSignature(N, currentScope,false);

	addLeaf(N, n, scopeEnd);
	commonSymbolTableCloseScope(N, currentScope, scopeEnd);

	if (!inFollow(N, kNoisyIrNodeType_PpredicateFnDecl, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredicateFnDecl, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredicateFnDecl);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PidentifierOrNil
 *
 *	Grammar production:
 *		identifierOrNil		=	qualifiedIdentifier | "nil" .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= Xseq of kNoisyIrNodeType_PfieldSelect
 */
IrNode *
noisyParseIdentifierOrNil(State *  N, Scope *  currentScope, bool isDefinition)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseIdentifierOrNil);

	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PidentifierOrNil,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

	if (peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		/*
		 *	The typeTree's get filled-in in our caller
		 */
		addLeaf(N, n, noisyParseQualifiedIdentifier(N, currentScope, isDefinition));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tnil))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tnil));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PidentifierOrNil, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PidentifierOrNil);
	}

	if (!inFollow(N, kNoisyIrNodeType_PidentifierOrNil, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PidentifierOrNil, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PidentifierOrNil);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PidentifierOrNilList
 *
 *	Grammar production:
 *		identifierOrNilList	=	identifierOrNil {"," identifierOrNil} .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PidentifierOrNil
 *		node.right	= Xseq of kNoisyIrNodeType_PidentifierOrNil
 */
IrNode *
noisyParseIdentifierOrNilList(State *  N, Scope *  currentScope, bool isDefinition)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseIdentifierOrNilList);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PidentifierOrNilList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

	/*
	 *	The typeTree's get filled in in our caller
	 */
	addLeaf(N, n, noisyParseIdentifierOrNil(N, currentScope, isDefinition));
	
	/*
	 *	Could also have done
	 *		while (!inFollow(N, kNoisyIrNodeType_PidentifierOrNilList, gNoisyFollows, kNoisyIrNodeTypeMax))
	 */
	while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
		addLeafWithChainingSeq(N, n, noisyParseIdentifierOrNil(N, currentScope, isDefinition));
	}

	if (!inFollow(N, kNoisyIrNodeType_PidentifierOrNilList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PidentifierOrNilList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PidentifierOrNilList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PidentifierList
 *
 *	Grammar production:
 *		identifierList		=	identifier {"," identifier} .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PidentifierList
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= Xseq of kNoisyIrNodeType_Tidentifier
 */
IrNode *
noisyParseIdentifierList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseIdentifierList);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PidentifierList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	/*
	 *	The typeTree's get filled in in our caller
	 */
	addLeaf(N, n, noisyParseIdentifierDefinitionTerminal(N, currentScope));
	
	/*
	 *	Could also have done
	 *		while (!inFollow(N, kNoisyIrNodeType_PidentifierList, gNoisyFollows, kNoisyIrNodeTypeMax))
	 */
	while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
		addLeafWithChainingSeq(N, n, noisyParseIdentifierDefinitionTerminal(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PidentifierList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PidentifierList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PidentifierList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PtypeExpr
 *
 *	Grammar production:
 *		typeExpr		::=	(basicType ["and" typeAnnoteList]) | anonAggregateType | typeName .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PtypeExpr
 *		node.left	= kNoisyIrNodeType_PbasicType | kNoisyIrNodeType_PanonAggregateType | kNoisyIrNodeType_PtypeName
 *		node.right	=  Xseq of kNoisyIrNodeType_PtypeAnnoteList | NULL
 */
IrNode *
noisyParseTypeExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTypeExpr);

	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PtypeExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PbasicType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseBasicType(N, currentScope));

		if (inFollow(N, kNoisyIrNodeType_PtypeExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
		{
			return n;
		}

		if (peekCheck(N,1,kNoisyIrNodeType_Tand))
		{
			noisyParseTerminal(N, kNoisyIrNodeType_Tand);
			addLeafWithChainingSeq(N, n, noisyParseTypeAnnoteList(N, currentScope));
		}
		
	}
	else if (inFirst(N, kNoisyIrNodeType_PanonAggregateType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseAnonAggregateType(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PtypeName, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseTypeName(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeExpr, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeExpr);
	}

	if (!inFollow(N, kNoisyIrNodeType_PtypeExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PtypeAnnoteItem
 *
 *	Grammar production:
 *		typeAnnoteItem	::= 	dimensionsDesignation | unitsDesignation | signalDesignation
 *					| timeseriesDesignation | sigfigDesignation | tolerance .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PtypeAnnoteItem
 *		node.left	= kNoisyIrNodeType_PdimensionsDesignation | kNoisyIrNodeType_PunitsDesignation | kNoisyIrNodeType_PsignalDesignation
 *					| kNoisyIrNodeType_PtimeseriesDesignation | kNoisyIrNodeType_PsigfigDesignation | kNoisyIrNodeType_Ptolerance
 *		node.right	= NULL
 */
IrNode *
noisyParseTypeAnnoteItem(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTypeAnnoteItem);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PtypeAnnoteItem,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PdimensionsDesignation, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseDimensionsDesignation(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PunitsDesignation, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseUnitsDesignation(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PsignalDesignation, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseSignalDesignation(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PtimeseriesDesignation, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseTimeseriesDesignation(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PsigfigDesignation, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseSigfigDesignation(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_Ptolerance, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseTolerance(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeAnnoteItem, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeAnnoteItem);
	}

	if (!inFollow(N, kNoisyIrNodeType_PtypeAnnoteItem, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeAnnoteItem, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeAnnoteItem);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PtypeAnnoteList
 *
 *	Grammar production:
 *		typeAnnoteList		=	typeAnnoteItem {"and" typeAnnoteItem} .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PtypeAnnoteList
 *		node.left	= kNoisyIrNodeType_PtypeAnnoteItem
 *		node.right	= Xseq of kNoisyIrNodeType_PtypeAnnoteItem
 */
IrNode *
noisyParseTypeAnnoteList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTypeAnnoteList);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PtypeAnnoteList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseTypeAnnoteItem(N, currentScope));
	while (peekCheck(N, 1, kNoisyIrNodeType_Tand))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tand);
		addLeafWithChainingSeq(N, n, noisyParseTypeAnnoteItem(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PtypeAnnoteList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeAnnoteList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeAnnoteList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PtypeName
 *
 *	Grammar production:
 *		typeName		::=	identifier ["->" identifier] .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pidentifier
 *		node.right	= NULL | Xseq of kNoisyIrNodeType_Tarrow, kNoisyIrNodeType_Tidentifier
 */
IrNode *
noisyParseTypeName(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTypeName);


	IrNode *	id1;
	IrNode *	id2;
	Symbol *	idsym;
	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PtypeName,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

	/*
	 *	TODO: The second identifier should be looked up in the scope of the first, 
	 *	which is a module. Needed for statement such as:
	 *
	 *		load System->print (path System->print);
	 */

	/*
	 *	This is a use, not a def, so doesn't go into symtab (noisyParseIdentifierDefinitionTerminal);
	 *	instead, noisyParseIdentifierUsageTerminal looks up its symbol in symtab.
	 */
	id1 = noisyParseIdentifierUsageTerminal(N, scope);
	if (id1->symbol == NULL)
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Tidentifier, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Tidentifier);
	}

	addLeaf(N, n, id1);

	/*
	 *	Since it's a use, it should already be in symtab. If it is a module
	 *	qualified name, we look in the module's scope, otherwise we look
	 *	in the current scope.
	 */
	if (peekCheck(N, 1, kNoisyIrNodeType_Tarrow))
	{
		/*
		 *	Note: kNoisyIrNodeType_Tarrow is the "->". We do not need to
		 *	put it in AST, since, from parent node type = kNoisyIrNodeType_PtypeName,
		 *	we know that if we have > 1 child, then it is a ptype-qualified
		 *	name. It is not a chain of Xseqs, because we cannnot have 
		 *	module embedded in module, so we at most have "a->b"
		 */
		noisyParseTerminal(N, kNoisyIrNodeType_Tarrow);
		id2 = noisyParseIdentifierUsageTerminal(N, moduleName2scope(N, id1->symbol->identifier));
		if (id2->symbol == NULL)
		{
			char *	details;
	
			asprintf(&details, "%s: '%s%s%s'\n", Eundeclared, id1->symbol->identifier, "->", "noisyParseTypeName: semantic Error" /*id2->symbol->identifier*/);
			noisyParserSemanticError(N, kNoisyIrNodeType_PtypeName, details);
		}
		idsym = id2->symbol;

		addLeaf(N, n, id2);
	}
	else
	{
		idsym = commonSymbolTableSymbolForIdentifier(N, scope, id1->symbol->identifier);
		if (idsym == NULL)
		{
			char *	details;
	
			asprintf(&details, "%s: %s\n", Eundeclared, id1->symbol->identifier);
			noisyParserSemanticError(N, kNoisyIrNodeType_PtypeName, details);
		}
	}

	/*
	 *	Set the symbol table entry for this to be that of the type or
	 *	prog. qualified type, so that, among other things, it gets the
	 *	type of the kNoisyIrNodeType_PtypeName treenode set to the discerned type.
	 */
	n->symbol = idsym;

	if (!inFollow(N, kNoisyIrNodeType_PtypeName, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeName, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeName);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PdimensionsDesignation
 *
 *	Grammar production:
 *		dimensionsDesignation	::=	"dimensions" dimensionArithExpr .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PdimensionsDesignation
 *		node.left	= kNoisyIrNodeType_PdimensionArithExpr
 *		node.right	= NULL
 */
IrNode *
noisyParseDimensionsDesignation(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseDimensionsDesignation);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PdimensionsDesignation,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tdimensions);
	addLeaf(N, n, noisyParseDimensionArithExpr(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PdimensionsDesignation, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PdimensionsDesignation, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PdimensionsDesignation);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PsigfigDesignation
 *
 *	Grammar production:
 *		sigfigDesignation	::=	"sigfigs" integerConst
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsigfigDesignation
 *		node.left	= kNoisyIrNodeType_TintegerConst
 *		node.right	= NULL
 */
IrNode *
noisyParseSigfigDesignation(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSigfigDesignation);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PsigfigDesignation,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tsigfigs);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintegerConst));

	if (!inFollow(N, kNoisyIrNodeType_PsigfigDesignation, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsigfigDesignation, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsigfigDesignation);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PsignalDesignation
 *
 *	Grammar production:
 *		signalDesignation	::=	"signal" (basicSignal | identifier) .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsignalDesignation
 *		node.left	= kNoisyIrNodeType_PbasicSignal | kNoisyIrNodeType_Tidentifier
 *		node.right	= NULL
 */
IrNode *
noisyParseSignalDesignation(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSignalDesignation);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PsignalDesignation,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tsignal);
	if (inFirst(N, kNoisyIrNodeType_PbasicSignal, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseBasicSignal(N, currentScope));
	}
	else
	{
		addLeaf(N, n, noisyParseIdentifierDefinitionTerminal(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PsignalDesignation, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsignalDesignation, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsignalDesignation);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PtimeseriesDesignation
 *
 *	Grammar production:
 *		timeseriesDesignation	::=	"timeseries" | "measurement" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PtimeseriesDesignation
 *		node.left	= kNoisyIrNodeType_Ttimeseries | kNoisyIrNodeType_Tmeasurement
 *		node.right	= NULL
 */
IrNode *
noisyParseTimeseriesDesignation(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTimeseriesDesignation);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PtimeseriesDesignation,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Ttimeseries))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Ttimeseries));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tmeasurement))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tmeasurement));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtimeseriesDesignation, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtimeseriesDesignation);
	}

	if (!inFollow(N, kNoisyIrNodeType_PtimeseriesDesignation, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtimeseriesDesignation, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtimeseriesDesignation);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PunitsDesignation
 *
 *	Grammar production:
 *		unitsDesignation	::=	"units" unitsArithExpr .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PunitsDesignation
 *		node.left	= kNoisyIrNodeType_PunitsArithExpr
 *		node.right	= NULL
 */
IrNode *
noisyParseUnitsDesignation(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseUnitsDesignation);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PunitsDesignation,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tunits);
	addLeaf(N, n, noisyParseUnitsArithExpr(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PunitsDesignation, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PunitsDesignation, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PunitsDesignation);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PdimensionArithFactor
 *
 *	Grammar production:
 *		dimensionArithFactor	::=	basicSignalDimension | "(" dimensionArithExpr ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PdimensionArithFactor
 *		node.left	= kNoisyIrNodeType_PbasicSignalDimension | kNoisyIrNodeType_PdimensionArithExpr
 *		node.right	= NULL
 */
IrNode *
noisyParseDimensionArithFactor(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseDimensionArithFactor);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PdimensionArithFactor,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PbasicSignalDimension, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseBasicSignalDimension(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftParens))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
		addLeaf(N, n, noisyParseDimensionArithExpr(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PdimensionArithFactor, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PdimensionArithFactor);
	}

	if (!inFollow(N, kNoisyIrNodeType_PdimensionArithFactor, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PdimensionArithFactor, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PdimensionArithFactor);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PdimensionArithTerm
 *
 *	Grammar production:
 *		dimensionArithTerm	::=	dimensionArithFactor {highPrecedenceArith2ArithOp dimensionArithFactor} .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PdimensionArithTerm
 *		node.left	= dimensionArithFactor
 *		node.right	= Xseq of repeating highPrecedenceArith2ArithOp dimensionArithFactor
 */
IrNode *
noisyParseDimensionArithTerm(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseDimensionArithTerm);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PdimensionArithTerm,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseDimensionArithFactor(N, currentScope));
	while (inFirst(N, kNoisyIrNodeType_PhighPrecedenceArith2ArithOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParseHighPrecedenceArith2ArithOp(N, currentScope));
		addLeafWithChainingSeq(N, n, noisyParseDimensionArithFactor(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PdimensionArithTerm, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PdimensionArithTerm, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PdimensionArithTerm);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PdimensionArithExpr
 *
 *	Grammar production:
 *		dimensionArithExpr	::=	dimensionArithTerm {lowPrecedenceArith2ArithOp dimensionArithTerm} .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PdimensionArithExpr
 *		node.left	= kNoisyIrNodeType_PdimensionArithTerm
 *		node.right	= Xseq of repeating kNoisyIrNodeType_PlowPrecedenceArith2ArithOp kNoisyIrNodeType_PdimensionArithTerm
 */
IrNode *
noisyParseDimensionArithExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseDimensionArithExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PdimensionArithExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseDimensionArithTerm(N, currentScope));
	while (inFirst(N, kNoisyIrNodeType_PlowPrecedenceArith2ArithOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParseLowPrecedenceArith2ArithOp(N, currentScope));
		addLeafWithChainingSeq(N, n, noisyParseDimensionArithTerm(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PdimensionArithExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PdimensionArithExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PdimensionArithExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PunitsArithFactor
 *
 *	Grammar production:
 *		unitsArithFactor	::=	(basicSignalUnits | identifier | numericConst) | "(" unitsArithExpr ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PunitsArithFactor
 *		node.left	= kNoisyIrNodeType_PbasicSignalUnits | kNoisyIrNodeType_Tidentifier | kNoisyIrNodeType_TnumericConst | kNoisyIrNodeType_PunitsArithExpr
 *		node.right	= NULL
 */
IrNode *
noisyParseUnitsArithFactor(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseUnitsArithFactor);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PunitsArithFactor,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_TleftParens))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
		addLeaf(N, n, noisyParseUnitsArithExpr(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);
	}
	else if (inFirst(N, kNoisyIrNodeType_PbasicSignalUnits, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseBasicSignalUnits(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		addLeaf(N, n, noisyParseIdentifierDefinitionTerminal(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PnumericConst, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseNumericConst(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PunitsArithFactor, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PunitsArithFactor);
	}

	if (!inFollow(N, kNoisyIrNodeType_PunitsArithFactor, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PunitsArithFactor, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PunitsArithFactor);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PunitsArithTerm
 *
 *	Grammar production:
 *		unitsArithTerm	::=	unitsArithFactor {highPrecedenceArith2ArithOp unitsArithFactor} .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PunitsArithTerm
 *		node.left	= unitsArithFactor
 *		node.right	= Xseq of repeating highPrecedenceArith2ArithOp unitsArithFactor
 */
IrNode *
noisyParseUnitsArithTerm(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseUnitsArithTerm);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PunitsArithTerm,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseUnitsArithFactor(N, currentScope));
	while (inFirst(N, kNoisyIrNodeType_PhighPrecedenceArith2ArithOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParseHighPrecedenceArith2ArithOp(N, currentScope));
		addLeafWithChainingSeq(N, n, noisyParseUnitsArithFactor(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PunitsArithTerm, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PunitsArithTerm, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PunitsArithTerm);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PunitsArithExpr
 *
 *	Grammar production:
 *		unitsArithExpr	::=	unitsArithTerm {lowPrecedenceArith2ArithOp unitsArithTerm} .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PunitsArithExpr
 *		node.left	= kNoisyIrNodeType_PunitsArithTerm
 *		node.right	= Xseq of repeating kNoisyIrNodeType_PlowPrecedenceArith2ArithOp kNoisyIrNodeType_PunitsArithTerm
 */
IrNode *
noisyParseUnitsArithExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseUnitsArithExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PunitsArithExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseUnitsArithTerm(N, currentScope));
	while (inFirst(N, kNoisyIrNodeType_PlowPrecedenceArith2ArithOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_PlowPrecedenceArith2ArithOp));
		addLeafWithChainingSeq(N, n, noisyParseUnitsArithTerm(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PunitsArithExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PunitsArithExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PunitsArithExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PbasicSignalDimension
 *
 *	Grammar production:
 *		basicSignalDimension	::=	"distance" | "mass" | "time" | "material" | "current" | "luminosity" | "temperature" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PbasicSignalDimension
 *		node.left	= kNoisyIrNodeType_Tdistance | kNoisyIrNodeType_Tmass | kNoisyIrNodeType_Ttime 
 *				| kNoisyIrNodeType_Tmaterial | kNoisyIrNodeType_Tcurrent | kNoisyIrNodeType_Tluminosity
 *				| kNoisyIrNodeType_Ttemperature
 *		node.right	= NULL
 */
IrNode *
noisyParseBasicSignalDimension(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseBasicSignalDimension);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PbasicSignalDimension,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tdistance))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tdistance));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tmass))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tmass));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Ttime))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Ttime));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tmaterial))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tmaterial));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tcurrent))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tcurrent));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tluminosity))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tluminosity));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Ttemperature))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Ttemperature));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PbasicSignalDimension, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PbasicSignalDimension);
	}

	if (!inFollow(N, kNoisyIrNodeType_PbasicSignalDimension, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PbasicSignalDimension, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PbasicSignalDimension);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PbasicSignalUnits
 *
 *	Grammar production:
 *		basicSignalUnits	::=	"m" | "kg" | "s" | "mole" | "A" | "cd" | "K" | "meter" | "kilogram" | "second" | "mole" | "Ampere" | "candela" | "Kelvin" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PbasicSignalUnits
 *		node.left	= kNoisyIrNodeType_Tm | kNoisyIrNodeType_Tkg | kNoisyIrNodeType_Ts | kNoisyIrNodeType_Tmole | kNoisyIrNodeType_TA | kNoisyIrNodeType_Tcd
 *				| kNoisyIrNodeType_TK | kNoisyIrNodeType_Tmeter | kNoisyIrNodeType_Tkilogram | kNoisyIrNodeType_Tsecond
 *				| kNoisyIrNodeType_Tmole | kNoisyIrNodeType_TAmpere | kNoisyIrNodeType_Tcandela | kNoisyIrNodeType_TKelvin
 *		node.right	= NULL
 */
IrNode *
noisyParseBasicSignalUnits(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseBasicSignalUnits);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PbasicSignalUnits,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tm))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tm));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tkg))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tkg));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Ts))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Ts));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tmole))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tmole));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TA))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TA));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tcd))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tcd));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TK))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TK));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tmeter))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tmeter));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tkilogram))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tkilogram));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tsecond))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tsecond));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tmole))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tmole));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TAmpere))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TAmpere));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tcandela))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tcandela));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TKelvin))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TKelvin));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PbasicSignalUnits, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PbasicSignalUnits);
	}

	if (!inFollow(N, kNoisyIrNodeType_PbasicSignalUnits, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PbasicSignalUnits, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PbasicSignalUnits);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PbasicSignal
 *
 *	Grammar production:
 *		basicSignal	::=	basicSignalDimension | "pressure" | "acceleration" | "magneticfluxdensity"
 *					| "relativehumidity" | "anglerate" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PbasicSignal
 *		node.left	= kNoisyIrNodeType_PbasicSignalDimension | kNoisyIrNodeType_Tpressure | kNoisyIrNodeType_Tacceleration | kNoisyIrNodeType_Tmagneticfluxdensity
 *						| kNoisyIrNodeType_Trelativehumidity | kNoisyIrNodeType_Tanglerate
 *		node.right	= NULL
 */
IrNode *
noisyParseBasicSignal(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseBasicSignal);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PbasicSignal,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PbasicSignalDimension, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseBasicSignalDimension(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tpressure))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tpressure));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tacceleration))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tacceleration));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tmagneticfluxdensity))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tmagneticfluxdensity));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Trelativehumidity))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Trelativehumidity));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tanglerate))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tanglerate));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PbasicSignal, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PbasicSignal);
	}

	if (!inFollow(N, kNoisyIrNodeType_PbasicSignal, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PbasicSignal, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PbasicSignal);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_Ptolerance
 *
 *	Grammar production:
 *		tolerance		::=	accuracyTolerance | lossTolerance | latencyTolerance .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PaccuracyTolerance | kNoisyIrNodeType_PlossTolerance | kNoisyIrNodeType_PlatencyTolerance
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseTolerance(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTolerance);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_Ptolerance,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PaccuracyTolerance, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseAccuracyTolerance(N));
	}
	else if (inFirst(N, kNoisyIrNodeType_PlossTolerance, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseLossTolerance(N));
	}
	else if (inFirst(N, kNoisyIrNodeType_PlatencyTolerance, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseLatencyTolerance(N));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Ptolerance, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Ptolerance);
	}

	if (!inFollow(N, kNoisyIrNodeType_Ptolerance, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Ptolerance, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Ptolerance);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PaccuracyTolerance
 *
 *	Grammar production:
 *		accuracyTolerance	::=	"epsilon" "(" realConst "," realConst ")" .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_TrealConst
 *		node.right	= kNoisyIrNodeType_TrealConst
 */
IrNode *
noisyParseAccuracyTolerance(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseAccuracyTolerance);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PaccuracyTolerance,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tepsilon);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PaccuracyTolerance, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PaccuracyTolerance, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PaccuracyTolerance);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_PlossTolerance
 *
 *	Grammar production:
 *		lossTolerance		::=	"alpha" "(" realConst "," realConst ")" .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_TrealConst
 *		node.right	= kNoisyIrNodeType_TrealConst
 */
IrNode *
noisyParseLossTolerance(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseLossTolerance);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PaccuracyTolerance,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Talpha);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PlossTolerance, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PlossTolerance, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PlossTolerance);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PlatencyTolerance
 *
 *	Grammar production:
 *		latencyTolerance	::=	"tau" "(" realConst "," realConst ")" .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_TrealConst
 *		node.right	= kNoisyIrNodeType_TrealConst
 */
IrNode *
noisyParseLatencyTolerance(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseLatencyTolerance);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PlatencyTolerance,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Ttau);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PlatencyTolerance, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PlatencyTolerance, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PlatencyTolerance);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PbasicType
 *
 *	Grammar production:
 *		basicType		::=	"bool" | integerType | realType | "string" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PbasicType
 *		node.left	= kNoisyIrNodeType_Tbool | ... | kNoisyIrNodeType_Tstring
 *		node.right	= NULL
 */
IrNode *
noisyParseBasicType(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseBasicType);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PbasicType,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tbool))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tbool));
	}
	else if (inFirst(N, kNoisyIrNodeType_PintegerType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseIntegerType(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PrealType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseRealType(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tstring))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tstring));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PbasicType, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PbasicType);
	}

	if (!inFollow(N, kNoisyIrNodeType_PbasicType, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PbasicType, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PbasicType);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PintegerType
 *
 *	Grammar production:
 *		integerType	::=	"nat4" | "nat8" | "nat16" | "nat32" | "nat64" | "nat128"
 *				| "int4" | "int8" | "int16" | "int32" | "int64" | "int128" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PintegerType
 *		node.left	= kNoisyIrNodeType_Tnat4 | kNoisyIrNodeType_Tnat8 | kNoisyIrNodeType_Tnat16 | kNoisyIrNodeType_Tnat32 | kNoisyIrNodeType_Tnat64 | kNoisyIrNodeType_Tnat128
 *				| kNoisyIrNodeType_Tint4 | kNoisyIrNodeType_Tint8 | kNoisyIrNodeType_Tint16 | kNoisyIrNodeType_Tint32 | kNoisyIrNodeType_Tint64 | kNoisyIrNodeType_Tint128 .
 *
 *		node.right	= NULL
 */
IrNode *
noisyParseIntegerType(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseIntegerType);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PintegerType,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tnat4))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tnat4));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tnat8))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tnat8));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tnat16))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tnat16));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tnat32))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tnat32));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tnat64))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tnat64));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tnat128))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tnat128));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tint4))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tint4));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tint8))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tint8));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tint16))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tint16));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tint32))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tint32));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tint64))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tint64));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tint128))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tint128));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PintegerType, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PintegerType);
	}

	if (!inFollow(N, kNoisyIrNodeType_PintegerType, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PintegerType, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PintegerType);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PrealType
 *
 *	Grammar produdtion:
 *		realType		::=	"float4" | "float8" | "float16" | "float32" | "float64" | "float128" | fixedType .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PrealType
 *		node.left	= kNoisyIrNodeType_Tfloat4 | kNoisyIrNodeType_Tfloat8 | kNoisyIrNodeType_Tfloat16 
 *				| kNoisyIrNodeType_Tfloat32 | kNoisyIrNodeType_Tfloat64 | kNoisyIrNodeType_Tfloat128 | kNoisyIrNodeType_PfixedType
 *		node.right	= NULL
 */
IrNode *
noisyParseRealType(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseRealType);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PrealType,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tfloat4))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tfloat4));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tfloat8))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tfloat8));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tfloat16))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tfloat16));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tfloat32))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tfloat32));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tfloat64))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tfloat64));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tfloat128))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tfloat128));
	}
	else if (inFirst(N, kNoisyIrNodeType_PfixedType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseFixedType(N));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PrealType, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PrealType);
	}

	if (!inFollow(N, kNoisyIrNodeType_PrealType, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PrealType, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PrealType);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PfixedType
 *
 *	Grammar production:
 *		fixedType	::=	"fixed" integerConst "." integerConst .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PfixedType
 *		node.left	= kNoisyIrNodeType_TintegerConst
 *		node.right	= kNoisyIrNodeType_TintegerConst
 */
IrNode *
noisyParseFixedType(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseFixedType);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PfixedType,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tfixed);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintegerConst));
	noisyParseTerminal(N, kNoisyIrNodeType_Tdot);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintegerConst));

	if (!inFollow(N, kNoisyIrNodeType_PfixedType, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PfixedType, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PfixedType);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PnumericType
 *
 *	Grammar production:
 *		numericType		::=	integerType | realType | fixedType | rationalType .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PnumericType
 *		node.left	= kNoisyIrNodeType_PintegerType | kNoisyIrNodeType_PrealType | kNoisyIrNodeType_PfixedType | kNoisyIrNodeType_PrationalType
 *		node.right	= NULL
 */
IrNode *
noisyParseNumericType(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseNumericType);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PnumericType,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PintegerType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseIntegerType(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PrealType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseRealType(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PfixedType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseFixedType(N));
	}
	else if (inFirst(N, kNoisyIrNodeType_PrationalType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseRationalType(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PnumericType, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PnumericType);
	}

	if (!inFollow(N, kNoisyIrNodeType_PnumericType, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PnumericType, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PnumericType);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PcomplexType
 *
 *	Grammar production:
 *		complexType		::=	"complex" numericType .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PcomplexType
 *		node.left	= kNoisyIrNodeType_PnumericType
 *		node.right	= NULL
 */
IrNode *
noisyParseComplexType(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseComplexType);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PcomplexType,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tcomplex);
	addLeaf(N, n, noisyParseNumericType(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PcomplexType, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PcomplexType, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PcomplexType);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PanonAggregateType
 *
 *	Grammar production:
 *		anonAggregateType	::=	arrayType | listType | tupleType | setType | rationalType | complexType .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PanonAggregateType
 *		node.left	= kNoisyIrNodeType_ParrayType | kNoisyIrNodeType_PlistType | kNoisyIrNodeType_PtupleType 
 *				| kNoisyIrNodeType_PsetType | kNoisyIrNodeType_PrationalType | kNoisyIrNodeType_PcomplexType
 *		node.right	= NULL
 */
IrNode *
noisyParseAnonAggregateType(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseAnonAggregateType);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PanonAggregateType,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_ParrayType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseArrayType(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PlistType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseListType(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PtupleType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseTupleType(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PsetType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseSetType(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PrationalType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseRationalType(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PcomplexType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseComplexType(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PanonAggregateType, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PanonAggregateType);
	}

	if (!inFollow(N, kNoisyIrNodeType_PanonAggregateType, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PanonAggregateType, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PanonAggregateType);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_ParrayType
 *
 *	Grammar production:
 *		arrayType		::=	"array" "[" integerConst "]" {"[" integerConst "]"} "of" typeExpr ["rawmem" "@" intParamOrConst] .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_ParrayType
 *		node.left	= kNoisyIrNodeType_TintegerConst
 *		node.right	= Xseq of zero or more kNoisyIrNodeType_TintegerConst then a kNoisyIrNodeType_PtypeExpr
 */
IrNode *
noisyParseArrayType(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseArrayType);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_ParrayType,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tarray);

	noisyParseTerminal(N, kNoisyIrNodeType_TleftBracket);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintegerConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBracket);

	while (peekCheck(N, 1, kNoisyIrNodeType_TleftBracket))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_TleftBracket);
		addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintegerConst));
		noisyParseTerminal(N, kNoisyIrNodeType_TrightBracket);
	}
	
	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeafWithChainingSeq(N, n, noisyParseTypeExpr(N, currentScope));

	if (peekCheck(N, 1, kNoisyIrNodeType_Trawmem))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Trawmem);
		noisyParseTerminal(N, kNoisyIrNodeType_Tat);
		addLeaf(N, n, noisyParseIntParamOrConst(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_ParrayType, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_ParrayType, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_ParrayType);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PlistType
 *
 *	Grammar production:
 *		listType		::=	"list" "of" typeExpr .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PlistType
 *		node.left	= kNoisyIrNodeType_PtypeExpr
 *		node.right	= NULL
 */
IrNode *
noisyParseListType(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseListType);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PlistType,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tlist);
	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeaf(N, n, noisyParseTypeExpr(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PlistType, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PlistType, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PlistType);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PtupleType
 *
 *	Grammar production:
 *		tupleType		::=	"(" typeExpr {"," typeExpr} ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PtupleType
 *		node.left	= kNoisyIrNodeType_PtypeExpr
 *		node.right	= Xseq of kNoisyIrNodeType_PtypeExpr
 */
IrNode *
noisyParseTupleType(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTupleType);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PtupleType,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParseTypeExpr(N, currentScope));
	while(peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
		addLeafWithChainingSeq(N, n, noisyParseTypeExpr(N, currentScope));
	}
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PtupleType, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtupleType, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtupleType);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PsetType
 *
 *	Grammar production:
 *		setType			::=	"set" "[" integerConst "]" "of" typeExpr .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsetType
 *		node.left	= kNoisyIrNodeType_TintegerConst
 *		node.right	= kNoisyIrNodeType_PtypeExpr
 */
IrNode *
noisyParseSetType(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSetType);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PsetType,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tset);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftBracket);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintegerConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBracket);
	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeaf(N, n, noisyParseTypeExpr(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PsetType, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsetType, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsetType);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PrationalType
 *
 *	Grammar production:
 *		rationalType		::=	"rational" numericType .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PrationalType
 *		node.left	= kNoisyIrNodeType_PnumericType
 *		node.right	= NULL
 */
IrNode *
noisyParseRationalType(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseRationalType);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PrationalType,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Trational);
	addLeaf(N, n, noisyParseNumericType(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PrationalType, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PrationalType, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PrationalType);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PnumericConst
 *
 *	Grammar production:
 *		numericConst		=	integerConst | realConst .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PnumericConst
 *		node.left	= kNoisyIrNodeType_TintegerConst | kNoisyIrNodeType_TrealConst
 *		node.right	= NULL
 */
IrNode *
noisyParseNumericConst(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseNumericConst);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PnumericConst,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_TintegerConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintegerConst));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TrealConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PnumericConst, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PnumericConst);
	}

	if (!inFollow(N, kNoisyIrNodeType_PnumericConst, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PnumericConst, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PnumericConst);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PinitList
 *
 *	Grammar production:
 *		initList		=	"{" expression {"," expression} "}" .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pexpression
 *		node.right	= Xseq of kNoisyIrNodeType_Pexpression
 */
IrNode *
noisyParseInitList(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseInitList);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PinitList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	addLeaf(N, n, noisyParseExpression(N, scope));
	while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
		addLeafWithChainingSeq(N, n, noisyParseExpression(N, scope));
	}
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);

	if (!inFollow(N, kNoisyIrNodeType_PinitList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PinitList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PinitList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PidxInitList
 *
 *	Grammar production:
 *		idxInitList		=	"{" element {"," element} "}" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PidxInitList
 *		node.left	= kNoisyIrNodeType_Pelement
 *		node.right	= Xseq of kNoisyIrNodeType_Pelement
 */
IrNode *
noisyParseIdxInitList(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseIdxInitList);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PinitList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	addLeaf(N, n, noisyParseElement(N, scope));
	while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
		addLeafWithChainingSeq(N, n, noisyParseElement(N, scope));
	}
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);

	if (!inFollow(N, kNoisyIrNodeType_PidxInitList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PidxInitList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PidxInitList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PstarInitList
 *
 *	Grammar production:
 *		starInitList		=	"{" element {"," element} ["," "*" "=>" expression] "}" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PstarInitList
 *		node.left	= kNoisyIrNodeType_Pelement
 *		node.right	= Xseq of kNoisyIrNodeType_Pelement, zero or more "*"+kNoisyIrNodeType_Pelement
 */
IrNode *
noisyParseStarInitList(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseStarInitList);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PstarInitList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	addLeaf(N, n, noisyParseElement(N, scope));
	while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);

		if (inFirst(N, kNoisyIrNodeType_Pelement, gNoisyFirsts, kNoisyIrNodeTypeMax))
		{
			addLeafWithChainingSeq(N, n, noisyParseElement(N, scope));
		}
		else if (peekCheck(N, 1, kNoisyIrNodeType_Tasterisk))
		{
			/*
			 *	Need to put this in AST to differentiate from default form
			 */
			addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tasterisk));
			noisyParseTerminal(N, kNoisyIrNodeType_Timplies);
			addLeafWithChainingSeq(N, n, noisyParseElement(N, scope));
		}
		else
		{
			fatal(N, EelementOrStar);
		}
	}

	noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);

	if (!inFollow(N, kNoisyIrNodeType_PstarInitList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PstarInitList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PstarInitList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_Pelement
 *
 *	Grammar production:
 *		element			=	expression [ "=>" expression ] .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Pelement
 *		node.left	= kNoisyIrNodeType_Pexpression
 *		node.right	= NULL | kNoisyIrNodeType_Pexpression
 */
IrNode *
noisyParseElement(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseElement);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_Pelement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseExpression(N, scope));
	if (peekCheck(N, 1, kNoisyIrNodeType_Timplies))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Timplies);
		addLeaf(N, n, noisyParseExpression(N, scope));
	}

	if (!inFollow(N, kNoisyIrNodeType_Pelement, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Pelement, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Pelement);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PtypeParameterList
 *
 *	Grammar production:
 *		typeParameterList	=	[identifier ":" "type" {"," identifier ":"  "type"}] .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PtypeParameterList
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= Xseq of repeating kNoisyIrNodeType_Tidentifier
 */
IrNode *
noisyParseTypeParameterList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTypeParameterList);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PtypeParameterList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		IrNode * identifier = noisyParseIdentifierDefinitionTerminal(N, currentScope);
		identifier->symbol->symbolType = kNoisySymbolTypeModuleParameter;
		addLeaf(N, n, identifier);

		noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
		noisyParseTerminal(N, kNoisyIrNodeType_Ttype);

		while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
		{
			noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
			IrNode * identifier = noisyParseIdentifierDefinitionTerminal(N, currentScope);
			identifier->symbol->symbolType = kNoisySymbolTypeModuleParameter;
			addLeafWithChainingSeq(N, n, identifier);
			noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
			noisyParseTerminal(N, kNoisyIrNodeType_Ttype);
		}
	}

	if (!inFollow(N, kNoisyIrNodeType_PtypeParameterList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeParameterList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeParameterList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PfunctionDefn
 *
 *	Grammar production:
 *		functionDefn	::=	identifier ":" "function" signature "->" signature "="  scopedStatementList .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PfunctionDefn
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= [Xseq of 2 tupletypes] kNoisyIrNodeType_PscopeStatementList
 */
IrNode *
noisyParseFunctionDefn(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseNamegenDefinition);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PfunctionDefn,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	IrNode *	identifier;
	/*
	 *	Depending on whether this is a local function definition or
	 *	one defined in module interface, pick between using existing
	 *	noisyParseIdentifierUsageTerminal vs noisyParseIdentifierDefinitionTerminal machinery.
	 *	 
	 */
	if (commonSymbolTableSymbolForIdentifier(N, scope, lexPeek(N, 1)->identifier) != NULL)
	{
		identifier = noisyParseIdentifierUsageTerminal(N, scope);
	}
	else
	{
		identifier = noisyParseIdentifierDefinitionTerminal(N, scope);
	}

	identifier->symbol->symbolType = kNoisySymbolTypeNamegenDefinition;
	identifier->symbol->isTypeComplete = true;
	addLeaf(N, n, identifier);

	noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);

	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_Tfunction);
	Scope *		currentScope	= commonSymbolTableOpenScope(N, scope, scopeBegin);

	IrNode *	t1 = noisyParseSignature(N, currentScope,false);
	addLeafWithChainingSeq(N, n, t1);
	noisyParseTerminal(N, kNoisyIrNodeType_Tarrow);
	IrNode *	t2 = noisyParseSignature(N, currentScope,true);
	addLeafWithChainingSeq(N, n, t2);

	/*
	 *	We need a valid subtree representing the type.
	 *	Need to be careful about not clobbering the
	 *	main AST with new links, so we make copies.
	 */
	
	IrNode * typeTree = calloc(1,sizeof(IrNode));
	addLeaf(N, typeTree, shallowCopyIrNode(N, t1));
	addLeaf(N, typeTree, shallowCopyIrNode(N, t2));

	/*
	*	If the function is previously declared check if type of declaration and definition match.
	*/
	if (identifier->symbol->typeTree != NULL)
	{
		if (!noisySignatureIsMatching(N,typeTree->irLeftChild,LL(identifier->symbol->typeTree))
		|| !noisySignatureIsMatching(N,typeTree->irRightChild,RL(identifier->symbol->typeTree)))
		{
			noisyParserSemanticError(N,kNoisyIrNodeType_PfunctionDefn,"Declaration and definition types don't match");
			noisyParserErrorRecovery(N, kNoisyIrNodeType_PfunctionDefn);
		}
	}

	identifier->symbol->typeTree = typeTree;
	/*
	*	Function scopes have names so we can search them.
	*/
	currentScope->identifier = identifier->symbol->identifier;

	noisyParseTerminal(N, kNoisyIrNodeType_Tassign);
	IrNode *	scopeEnd = noisyParseScopedStatementList(N, currentScope);
	addLeaf(N, n, scopeEnd);
	commonSymbolTableCloseScope(N, currentScope, scopeEnd);

	if (!inFollow(N, kNoisyIrNodeType_PfunctionDefn, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PfunctionDefn, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PfunctionDefn);
	}

	identifier->symbol->functionDefinition = n;
	return n;
}



/*
 *	kNoisyIrNodeType_PproblemDefn
 *
 *	Grammar production:
 *		problemDefn		::=	identifier ":" "probdef" signature "->" signature "=>" scopedPredStmtList .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PproblemDefn
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= Xseq of kNoisyIrNodeType_Psignature kNoisyIrNodeType_Psignature kNoisyIrNodeType_PscopedPredStmtList
 */
IrNode *
noisyParseProblemDefn(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseProblemDefn);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PproblemDefn,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	IrNode *	identifier;
	/*
	 *	Depending on whether this is a local problem definition or
	 *	one defined in module interface, pick between using existing
	 *	noisyParseIdentifierUsageTerminal vs noisyParseIdentifierDefinitionTerminal machinery.
	 *	 
	 */
	if (commonSymbolTableSymbolForIdentifier(N, scope, lexPeek(N, 1)->identifier) != NULL)
	{
		identifier = noisyParseIdentifierUsageTerminal(N, scope);
	}
	else
	{
		identifier = noisyParseIdentifierDefinitionTerminal(N, scope);
	}

	addLeaf(N, n, identifier);

	noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);

	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_Tprobdef);
	Scope *		currentScope	= commonSymbolTableOpenScope(N, scope, scopeBegin);

	IrNode *	t1 = noisyParseSignature(N, currentScope,false);
	addLeafWithChainingSeq(N, n, t1);
	noisyParseTerminal(N, kNoisyIrNodeType_Tarrow);
	IrNode *	t2 = noisyParseSignature(N, currentScope,true);
	addLeafWithChainingSeq(N, n, t2);

	/*
	 *	We need a valid subtree representing the type.
	 *	Need to be careful about not clobbering the
	 *	main AST with new links, so we make copies.
	 */
	IrNode *	typeTree = shallowCopyIrNode(N, t1);
	addLeaf(N, typeTree, shallowCopyIrNode(N, t2));
	identifier->symbol->typeTree = typeTree;

	noisyParseTerminal(N, kNoisyIrNodeType_Timplies);
	IrNode *	scopeEnd = noisyParseScopedPredStmtList(N, currentScope);
	addLeaf(N, n, scopeEnd);
	commonSymbolTableCloseScope(N, currentScope, scopeEnd);

	if (!inFollow(N, kNoisyIrNodeType_PproblemDefn, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PproblemDefn, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PproblemDefn);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PpredicateFnDefn
 *
 *	Grammar production:
 *		predicateFnDefn		::=	identifier ":" "predicate" signature "=>" scopedPredStmtList .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PpredicateFnDefn
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= Xseq of kNoisyIrNodeType_Psignature kNoisyIrNodeType_Psignature kNoisyIrNodeType_PscopedPredStmtList
 */
IrNode *
noisyParsePredicateFnDefn(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParsePredicateFnDefn);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PpredicateFnDefn,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);

	IrNode *	identifier;
	/*
	 *	Depending on whether this is a local predicate definition or
	 *	one defined in module interface, pick between using existing
	 *	noisyParseIdentifierUsageTerminal vs noisyParseIdentifierDefinitionTerminal machinery.
	 *	 
	 */
	if (commonSymbolTableSymbolForIdentifier(N, scope, lexPeek(N, 1)->identifier) != NULL)
	{
		identifier = noisyParseIdentifierUsageTerminal(N, scope);
	}
	else
	{
		identifier = noisyParseIdentifierDefinitionTerminal(N, scope);
	}

	addLeaf(N, n, identifier);

	noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);

	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_Tpredicate);
	Scope *		currentScope	= commonSymbolTableOpenScope(N, scope, scopeBegin);

	IrNode *	t1 = noisyParseSignature(N, currentScope,false);
	addLeafWithChainingSeq(N, n, t1);
	noisyParseTerminal(N, kNoisyIrNodeType_Tarrow);
	IrNode *	t2 = noisyParseSignature(N, currentScope,true);
	addLeafWithChainingSeq(N, n, t2);

	/*
	 *	We need a valid subtree representing the type.
	 *	Need to be careful about not clobbering the
	 *	main AST with new links, so we make copies.
	 */
	IrNode *	typeTree = shallowCopyIrNode(N, t1);
	addLeaf(N, typeTree, shallowCopyIrNode(N, t2));
	identifier->symbol->typeTree = typeTree;

	noisyParseTerminal(N, kNoisyIrNodeType_Timplies);
	IrNode *	scopeEnd = noisyParseScopedPredStmtList(N, currentScope);
	addLeaf(N, n, scopeEnd);
	commonSymbolTableCloseScope(N, currentScope, scopeEnd);

	if (!inFollow(N, kNoisyIrNodeType_PpredicateFnDefn, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredicateFnDefn, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredicateFnDefn);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_Psignature
 *
 *	Grammar production:
 *		signature		::=	"(" ((identifier ":" typeExpr {"," identifier ":" typeExpr}) | "nil") ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Psignature
 *		node.left	= kNoisyIrNodeType_Tnil | kNoisyIrNodeType_Tidentifier
 *		node.right	= kNoisyIrNodeType_PtypeExpr with Xseq of kNoisyIrNodeType_T_identifier and kNoisyIrNodeType_PtypeExpr
 */
IrNode *
noisyParseSignature(State *  N, Scope *  currentScope,bool isReturn)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSignature);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_Psignature,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	int paramPos = 0;
	if (peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		IrNode * identifierNode = noisyParseIdentifierDefinitionTerminal(N, currentScope);
		addLeaf(N, n, identifierNode);
		noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
		IrNode * typeTree = noisyParseTypeExpr(N, currentScope);
		addLeafWithChainingSeq(N, n, typeTree);
		identifierNode->symbol->typeTree = typeTree;
		identifierNode->symbol->paramPosition = paramPos;
		if (isReturn)
		{
			identifierNode->symbol->symbolType = kNoisySymbolTypeReturnParameter;
		}
		else
		{
			identifierNode->symbol->symbolType = kNoisySymbolTypeParameter;
		}

		while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
		{
			paramPos++;
			noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
			IrNode * identifierNode = noisyParseIdentifierDefinitionTerminal(N, currentScope);
			addLeafWithChainingSeq(N, n, identifierNode);
			noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
			IrNode * typeTree = noisyParseTypeExpr(N, currentScope);
			addLeafWithChainingSeq(N, n, typeTree);
			identifierNode->symbol->typeTree = typeTree;
			identifierNode->symbol->paramPosition = paramPos;
			if (isReturn)
			{
				identifierNode->symbol->symbolType = kNoisySymbolTypeReturnParameter;
			}
			else
			{
				identifierNode->symbol->symbolType = kNoisySymbolTypeParameter;
			}
		}
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tnil))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tnil));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Psignature, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Psignature);
	}
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_Psignature, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Psignature, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Psignature);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PscopedStatementList
 *
 *	Grammar production:
 *		scopedStatementList	::=	"{" statementList "}" .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PstatementList
 *		node.right	= NULL
 */
IrNode *
noisyParseScopedStatementList(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseScopedStatementList);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PscopedStatementList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	Scope *		currentScope	= commonSymbolTableOpenScope(N, scope, scopeBegin);

	addLeaf(N, n, noisyParseStatementList(N, currentScope));

	IrNode *	scopeEnd	= noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);

	commonSymbolTableCloseScope(N, currentScope, scopeEnd);

	if (!inFollow(N, kNoisyIrNodeType_PscopedStatementList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PscopedStatementList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PscopedStatementList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PstatementList
 *
 *	Grammar production:
 *		statementList		=	{statement} .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pstatement or NULL
 *		node.right	= Xseq of kNoisyIrNodeType_Pstatement
 */
IrNode *
noisyParseStatementList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseStatementList);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PstatementList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	/*
	 *	Could also have done
	 *		while (!inFollow(N, kNoisyIrNodeType_PstatementList, gNoisyFollows, kNoisyIrNodeTypeMax))
	 */
	while (inFirst(N, kNoisyIrNodeType_Pstatement, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParseStatement(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PstatementList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PstatementList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PstatementList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_Pstatement
 *
 *	statement		::=	[ assignmentStatement | matchStatement | iterateStatement | sequenceStatement
 *				| parallelStatement | scopedStatementList | operatorToleranceDecl | returnStatement] ";" .
 *
 *	Generated AST subtree:
 *
 *		node.left	= NULL | kNoisyIrNodeType_PidentifierOrNilList
 *		node.right	= NULL | kNoisyIrNodeType_PconstantDecl | .. | kNoisyIrNodeType_Pexpression
 */
IrNode *
noisyParseStatement(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseStatement);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_Pstatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tsemicolon))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tsemicolon);

		return n;
	}

	if (inFirst(N, kNoisyIrNodeType_PassignmentStatement, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseAssignmentStatement(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PmatchStatement, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseMatchStatement(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PiterateStatement, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseIterStatement(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PsequenceStatement, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseSequenceStatement(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PparallelStatement, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseParallelStatement(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PscopedStatementList, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseScopedStatementList(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PoperatorToleranceDecl, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseOperatorToleranceDecl(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PreturnStatement, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseReturnStatement(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Pstatement, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Pstatement);
	}

	noisyParseTerminal(N, kNoisyIrNodeType_Tsemicolon);

	if (!inFollow(N, kNoisyIrNodeType_Pstatement, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Pstatement, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Pstatement);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PassignmentStatement
 *
 *	Grammar production:
 *		assignmentStatement	::=	identifierOrNilList ((":" (constantDecl | typeDecl | typeExpr)) | (assignOp expression))
 *				| "(" identifierOrNilList ")" assignOp expression .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PassignmentStatement
 *		node.left	= See above
 *		node.right	= See above
 */
IrNode *
noisyParseAssignmentStatement(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseAssignmentStatement);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PassignmentStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);
	bool		isDefinition = false;

	/*
	 *	Scan ahead until the next assign operator to see if there is a ':', in which case this is a definition.
	 */
	for (int lookAhead = 1; true; lookAhead++)
	{
		if (peekCheck(N, lookAhead, kNoisyIrNodeType_Tcolon) || peekCheck(N, lookAhead, kNoisyIrNodeType_TcolonAssign))
		{
			isDefinition = true;
			break;
		}
		if (peekCheck(N, lookAhead-1,kNoisyIrNodeType_Tassign)
		|| peekCheck(N, lookAhead-1,kNoisyIrNodeType_TorAssign)
		|| peekCheck(N, lookAhead-1,kNoisyIrNodeType_TandAssign)
		|| peekCheck(N, lookAhead-1,kNoisyIrNodeType_TxorAssign)
		|| peekCheck(N, lookAhead-1,kNoisyIrNodeType_TplusAssign)
		|| peekCheck(N, lookAhead-1,kNoisyIrNodeType_TminusAssign)
		|| peekCheck(N, lookAhead-1,kNoisyIrNodeType_TasteriskAssign)
		|| peekCheck(N, lookAhead-1,kNoisyIrNodeType_TdivideAssign)
		|| peekCheck(N, lookAhead-1,kNoisyIrNodeType_TpercentAssign)
		|| peekCheck(N, lookAhead-1,kNoisyIrNodeType_TplusAssign)
		|| peekCheck(N, lookAhead-1,kNoisyIrNodeType_TchannelOperatorAssign))
		/*
		*	TODO; It needs more here (all the assigns).
		*/
		{
			break;
		}
	}


	if (inFirst(N, kNoisyIrNodeType_PidentifierOrNilList, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		IrNode *	identifierList = noisyParseIdentifierOrNilList(N, currentScope, isDefinition);
		addLeaf(N, n, identifierList);

		if (peekCheck(N, 1, kNoisyIrNodeType_Tcolon))
		{
			IrNode *	typeExpr;

			noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
			if (inFirst(N, kNoisyIrNodeType_PconstantDecl, gNoisyFirsts, kNoisyIrNodeTypeMax))
			{
				typeExpr = noisyParseConstantDecl(N, currentScope);
				addLeaf(N, n, typeExpr);
			}
			else if (inFirst(N, kNoisyIrNodeType_PtypeDecl, gNoisyFirsts, kNoisyIrNodeTypeMax))
			{
				typeExpr = noisyParseTypeDecl(N, currentScope);
				addLeaf(N, n, typeExpr);
			}
			else if (inFirst(N, kNoisyIrNodeType_PtypeExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
			{
				typeExpr = noisyParseTypeExpr(N, currentScope);
				addLeaf(N, n, typeExpr);
			}
			else
			{
				noisyParserSyntaxError(N, kNoisyIrNodeType_Pstatement, kNoisyIrNodeTypeMax, gNoisyFirsts);
				noisyParserErrorRecovery(N, kNoisyIrNodeType_Pstatement);
			}

			/*
			 *	assignTypes(N, ) handles expansion of typeTree, e.g., for kNoisyIrNodeType_PtypeName
			 */
			assignTypes(N, identifierList, typeExpr);
		}
		else if (inFirst(N, kNoisyIrNodeType_PassignOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
		{
			addLeafWithChainingSeq(N, n, noisyParseAssignOp(N));
			addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));
		}
		else
		{
			noisyParserSyntaxError(N, kNoisyIrNodeType_Pstatement, kNoisyIrNodeTypeMax, gNoisyFirsts);
			noisyParserErrorRecovery(N, kNoisyIrNodeType_Pstatement);
		}
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftParens))
	{
		/*
		 *	We add one L_PAREN to AST as a marker to differentiate
		 *	this case from that of regular assignment to identornil
		 *	Only need to add one, just as a marker.
		 */
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TleftParens));
		addLeafWithChainingSeq(N, n, noisyParseIdentifierOrNilList(N, currentScope, isDefinition));
		noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

		addLeafWithChainingSeq(N, n, noisyParseAssignOp(N));
		addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PassignmentStatement, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PassignmentStatement, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PassignmentStatement);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PreturnStatement
 *
 *	Grammar production:
 *		returnStatement		::=	"return" returnSignature .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PreturnStatement
 *		node.left	= kNoisyIrNodeType_PreturnSignature
 *		node.right	= NULL
 */
IrNode *
noisyParseReturnStatement(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseReturnStatement);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PreturnStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Treturn);
	addLeaf(N, n, noisyParseReturnSignature(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PreturnStatement, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PreturnStatement, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PreturnStatement);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PreturnSignature
 *
 *	Grammar production:
 *		returnSignature		=	"(" identifier ":" expression {"," identifier ":" expression} ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PreturnSignature
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= Xseq chain of repeating kNoisyIrNodeType_Pexpression and kNoisyIrNodeType_Tidentifiers
 */
IrNode *
noisyParseReturnSignature(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseReturnSignature);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PreturnSignature,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
	addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));

	while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
		addLeafWithChainingSeq(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
		addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));
	}

	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PreturnSignature, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PreturnSignature, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PreturnSignature);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PoperatorToleranceDecl
 *
 *	Grammar production:
 *		operatorToleranceDecl	=	(highPrecedenceBinaryOp | lowPrecedenceBinaryOp | unaryOp) ":" typeExpr .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PoperatorToleranceDecl
 *		node.left	= kNoisyIrNodeType_PhighPrecedenceBinaryOp | kNoisyIrNodeType_PlowPrecedenceBinaryOp | kNoisyIrNodeType_PunaryOp
 *		node.right	= kNoisyIrNodeType_PtypeExpr
 */
IrNode *
noisyParseOperatorToleranceDecl(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseOperatorToleranceDecl);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PoperatorToleranceDecl,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PhighPrecedenceBinaryOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseHighPrecedenceBinaryOp(N));
	}
	else if (inFirst(N, kNoisyIrNodeType_PlowPrecedenceBinaryOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseLowPrecedenceBinaryOp(N));
	}
	else if (inFirst(N, kNoisyIrNodeType_PunaryOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseUnaryOp(N));
	}

	noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
	addLeaf(N, n, noisyParseTypeExpr(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PoperatorToleranceDecl, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PoperatorToleranceDecl, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PoperatorToleranceDecl);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PassignOp
 *
 *	Grammar production:
 *		assignOp	::=	"=" | "^=" | "|=" | "&=" | "%=" | "/=" | "*=" | "-=" | "+=" | ">>="
 *				| "<<=" | "<-=" | ":=" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PassignOp
 *		node.left	= kNoisyIrNodeType_Tas | ... | kNoisyIrNodeType_TdefineAs
 *		node.right	= NULL
 */
IrNode *
noisyParseAssignOp(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseAssignOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PassignOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tassign))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tassign));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TxorAssign))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TxorAssign));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TorAssign))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TorAssign));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TandAssign))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TandAssign));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TpercentAssign))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TpercentAssign));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TdivideAssign))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TdivideAssign));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TasteriskAssign))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TasteriskAssign));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TminusAssign))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TminusAssign));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TplusAssign))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TplusAssign));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TrightShiftAssign))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrightShiftAssign));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftShiftAssign))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TleftShiftAssign));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TchannelOperatorAssign))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TchannelOperatorAssign));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TcolonAssign))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TcolonAssign));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PassignOp, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PassignOp);
	}

	if (!inFollow(N, kNoisyIrNodeType_PassignOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PassignOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PassignOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PmatchStatement
 *
 *	Grammar production:
 *		matchStatement		::=	("match" | "matchseq") "{" guardedStatementList "}" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PmatchStatement
 *		node.left	= kNoisyIrNodeType_Tmatch | kNoisyIrNodeType_TmatchSeq
 *		node.right	= kNoisyIrNodeType_PguardedStatementList
 */
IrNode *
noisyParseMatchStatement(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseMatchStatement);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PmatchStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tmatch))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tmatch));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tmatchseq))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tmatchseq));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PmatchStatement, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PmatchStatement);
	}

	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	Scope *		currentScope	= commonSymbolTableOpenScope(N, scope, scopeBegin);

	addLeaf(N, n, noisyParseGuardedStatementList(N, currentScope));

	IrNode *	scopeEnd	= noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);

	commonSymbolTableCloseScope(N, currentScope, scopeEnd);

	if (!inFollow(N, kNoisyIrNodeType_PmatchStatement, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PmatchStatement, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PmatchStatement);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PiterateStatement
 *
 *	Grammar production:
 *		iterateStatement	::=	"iterate" "{" guardedStatementList "}" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PiterateStatement
 *		node.left	= kNoisyIrNodeType_Titerate
 *		node.right	= kNoisyIrNodeType_PguardedStatementList
 */
IrNode *
noisyParseIterStatement(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseIterStatement);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PiterateStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Titerate));

	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	Scope *		currentScope	= commonSymbolTableOpenScope(N, scope, scopeBegin);

	addLeaf(N, n, noisyParseGuardedStatementList(N, currentScope));

	IrNode *	scopeEnd	= noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);

	commonSymbolTableCloseScope(N, currentScope, scopeEnd);

	if (!inFollow(N, kNoisyIrNodeType_PiterateStatement, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PiterateStatement, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PiterateStatement);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_Ps
 *
 *	Grammar production:
 *		sequenceStatement	::=	"sequence" (orderingHead | setHead) scopedStatementList .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsequenceStatement
 *		node.left	= kNoisyIrNodeType_PorderingHead | kNoisyIrNodeType_PsetHead
 *		node.right	= kNoisyIrNodeType_PscopedStatementList
 */
IrNode *
noisyParseSequenceStatement(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSequenceStatement);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PsequenceStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tsequence);
	if (inFirst(N, kNoisyIrNodeType_PorderingHead, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseOrderingHead(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PsetHead, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseSetHead(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsequenceStatement, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsequenceStatement);
	}

	addLeaf(N, n, noisyParseScopedStatementList(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PsequenceStatement, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsequenceStatement, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsequenceStatement);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PparallelStatement
 *
 *	Grammar production:
 *		parallelStatement	::=	"parallel" setHead scopedStatementList .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PparallelStatement
 *		node.left	= kNoisyIrNodeType_PsetHead
 *		node.right	= kNoisyIrNodeType_PscopedStatementList
 */
IrNode *
noisyParseParallelStatement(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseParallelStatement);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PparallelStatement,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tparallel);
	addLeaf(N, n, noisyParseSetHead(N, currentScope));
	addLeaf(N, n, noisyParseScopedStatementList(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PparallelStatement, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PparallelStatement, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PparallelStatement);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PsetHead
 *
 *	Grammar production:
 *		setHead		=	"(" identifier "in" expression ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsetHead
 *		node.left	= kNoisyIrNodeType_Pidentifier
 *		node.right	= kNoisyIrNodeType_Pexpression
 */
IrNode *
noisyParseSetHead(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSetHead);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PsetHead,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParseIdentifierDefinitionTerminal(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_Tin);
	addLeaf(N, n, noisyParseExpression(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PsetHead, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsetHead, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsetHead);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PorderingHead
 *
 *	Grammar production:
 *		orderingHead	=	"(" assignmentStatement ";" expression ";" assignmentStatement ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PorderingHead
 *		node.left	= kNoisyIrNodeType_PassignmentStatement
 *		node.right	= Xseq of kNoisyIrNodeType_Pexpression kNoisyIrNodeType_PassignmentStatement
 */
IrNode *
noisyParseOrderingHead(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseOrderingHead);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PorderingHead,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParseAssignmentStatement(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_Tsemicolon);
	addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_Tsemicolon);
	addLeafWithChainingSeq(N, n, noisyParseAssignmentStatement(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PorderingHead, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PorderingHead, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PorderingHead);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PguardedStatementList
 *
 *	Grammar production:
 *		guardedStatementList	=	{(expression | chanEventExpr) "=>" scopedStatementList} .
 *
 *	Generated AST subtree:
 *
 *		noe		= kNoisyIrNodeType_PguardedStatementList
 *		node.left	= kNoisyIrNodeType_Pexpression
 *		node.right	= Xseq of kNoisyIrNodeType_Pstatement and kNoisyIrNodeType_Pexpression alternating
 */
IrNode *
noisyParseGuardedStatementList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseGuardedStatementList);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PguardedStatementList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	while (inFirst(N, kNoisyIrNodeType_Pexpression, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_Timplies);
		addLeafWithChainingSeq(N, n, noisyParseScopedStatementList(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PguardedStatementList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PguardedStatementList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PguardedStatementList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PguardedExpressionList
 *
 *	Grammar production:
 *		guardedExpressionList	=	{expression "=>" expression} .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PguardedExpressionList
 *		node.left	= kNoisyIrNodeType_Pexpression
 *		node.right	= Xseq of an odd number of additional kNoisyIrNodeType_Pexpressions
 */
IrNode *
noisyParseGuardedExpressionList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseGuardedExpressionList);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PguardedExpressionList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	while (inFirst(N, kNoisyIrNodeType_Pexpression, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_Timplies);
		addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PguardedExpressionList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PguardedExpressionList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PguardedExpressionList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_Pexpression
 *
 *	Grammar production:
 *		expression	::=	(term {lowPrecedenceBinaryOp term}) | anonAggrCastExpr
 *					| loadExpr | quantizeExpression | sampleExpression .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Pterm | kNoisyIrNodeType_PanonAggrCastExpr | ... | kNoisyIrNodeType_Pname2chanExpression
 *		node.left	= kNoisyIrNodeType_PlowPrecedenceBinaryOp
 *		node.right	= Xseq of kNoisyIrNodeType_PlowPrecedenceBinaryOp, kNoisyIrNodeType_Pterm
 */
IrNode *
noisyParseExpression(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseExpression);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_Pexpression,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_Pterm, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseTerm(N, currentScope));

		while (inFirst(N, kNoisyIrNodeType_PlowPrecedenceBinaryOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
		{
			addLeafWithChainingSeq(N, n, noisyParseLowPrecedenceBinaryOp(N));
			addLeafWithChainingSeq(N, n, noisyParseTerm(N, currentScope));
		}
	}
	else if (inFirst(N, kNoisyIrNodeType_PanonAggrCastExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseAnonAggrCastExpr(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PloadExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseLoadExpr(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PquantizeExpression, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseQuantizeExpression(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PsampleExpression, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseSampleExpression(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Pexpression, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Pexpression);
	}

	if (!inFollow(N, kNoisyIrNodeType_Pexpression, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Pexpression, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Pexpression);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PquantizeExpression
 *
 *	Grammar production:
 *		quantizeExpression	::=	identifier "quantize" expression .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PquantizeExpression
 *		node.left	= kNoisyIrNodeType_Pidentifier
 *		node.right	= kNoisyIrNodeType_Pexpression
 */
IrNode *
noisyParseQuantizeExpression(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseQuantizeExpression);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PquantizeExpression,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_Tquantize);
	addLeaf(N, n, noisyParseExpression(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PquantizeExpression, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PquantizeExpression, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PquantizeExpression);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PsampleExpression
 *
 *	Grammar production:
 *		sampleExpression	::=	identifier "sample" expression .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsampleExpression
 *		node.left	= kNoisyIrNodeType_Pidentifier
 *		node.right	= kNoisyIrNodeType_Pexpression
 */
IrNode *
noisyParseSampleExpression(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSampleExpression);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PsampleExpression,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_Tsample);
	addLeaf(N, n, noisyParseExpression(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PsampleExpression, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsampleExpression, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsampleExpression);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PlistCastExpr
 *
 *	Grammar production;
 *		listCastExpr		::=	"list" "of" initList .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PlistCastExpr
 *		node.left	= kNoisyIrNodeType_PinitList
 *		node.right	= NULL
 */
IrNode *
noisyParseListCastExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseListCastExpr);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PlistCastExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tlist);
	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeaf(N, n, noisyParseInitList(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PlistCastExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PlistCastExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PlistCastExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PsetCastExpr
 *
 *	Grammar production:
 *		setCastExpr		::=	"set" "of" initList .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsetCastExpr
 *		node.left	= kNoisyIrNodeType_PinitList
 *		node.right	= NULL
 */
IrNode *
noisyParseSetCastExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSetCastExpr);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PsetCastExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tset);
	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeaf(N, n, noisyParseInitList(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PsetCastExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsetCastExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsetCastExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_ParrayCastExpr
 *
 *	Grammar production:
 *		arrayCastExpr		::=	"array" (("of" idxInitList) | ("[" integerConst "]" "of" starInitList)) .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_ParrayCastExpr
 *		node.left	= kNoisyIrNodeType_PidxInitList | kNoisyIrNodeType_TintegerConst
 *		node.right	= NULL | kNoisyIrNodeType_PstarInitList
 */
IrNode *
noisyParseArrayCastExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseArrayCastExpr);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_ParrayCastExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tarray);
	
	if (peekCheck(N, 1, kNoisyIrNodeType_Tof))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tof);
		addLeaf(N, n, noisyParseIdxInitList(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftBracket))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_TleftBracket);
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintegerConst));
		noisyParseTerminal(N, kNoisyIrNodeType_TrightBracket);
		noisyParseTerminal(N, kNoisyIrNodeType_Tof);
		addLeaf(N, n, noisyParseStarInitList(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_ParrayCastExpr, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_ParrayCastExpr);
	}

	if (!inFollow(N, kNoisyIrNodeType_ParrayCastExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_ParrayCastExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_ParrayCastExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PcomplexCastExpr
 *
 *	Grammar production:
 *		complexCastExpr		::=	"complex" "(" expression "," expression ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PcomplexCastExpr
 *		node.left	= kNoisyIrNodeType_Pexpression
 *		node.right	= kNoisyIrNodeType_Pexpression
 */
IrNode *
noisyParseComplexCastExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseComplexCastExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PcomplexCastExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tcomplex);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParseExpression(N, currentScope));
	addLeaf(N, n, noisyParseExpression(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PcomplexCastExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PcomplexCastExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PcomplexCastExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PrationalCastExpr
 *
 *	Grammar production:
 *		rationalCastExpr	::=	"rational" "(" expression "," expression ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PrationalCastExpr
 *		node.left	= kNoisyIrNodeType_Pexpression
 *		node.right	= kNoisyIrNodeType_Pexpression
 */
IrNode *
noisyParseRationalCastExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseRationalCastExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PrationalCastExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Trational);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParseExpression(N, currentScope));
	addLeaf(N, n, noisyParseExpression(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PrationalCastExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PrationalCastExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PrationalCastExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PanonAggrCastExpr
 *
 *	Grammar production:
 *		anonAggrCastExpr	::=	listCastExpr | setCastExpr | arrayCastExpr | complexCastExpr | rationalCastExpr .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PanonAggrCastExpr
 *		node.left	= kNoisyIrNodeType_PlistCast | kNoisyIrNodeType_PsetCast | kNoisyIrNodeType_ParrayCast | kNoisyIrNodeType_PcomplexCastExpr | kNoisyIrNodeType_PrationalCastExpr
 *		node.right	= NULL
 */
IrNode *
noisyParseAnonAggrCastExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseAnonAggrCastExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PanonAggrCastExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */); 


	if (inFirst(N, kNoisyIrNodeType_PlistCastExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseListCastExpr(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PsetCastExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n,noisyParseSetCastExpr(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_ParrayCastExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n,noisyParseArrayCastExpr(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PcomplexCastExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n,noisyParseComplexCastExpr(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PrationalCastExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n,noisyParseRationalCastExpr(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PanonAggrCastExpr, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PanonAggrCastExpr);
	}

	if (!inFollow(N, kNoisyIrNodeType_PanonAggrCastExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PanonAggrCastExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PanonAggrCastExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PchanEventExpr
 *
 *	Grammar production:
 *		chanEventExpr		::=	("erasures" | "distortions" | "latency") "of" identifier cmpOp expression .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Terasures | kNoisyIrNodeType_Tdistortions | kNoisyIrNodeType_Tlatency
 *		node.right	= Xseq of kNoisyIrNodeType_Tidentifier, cmpop, expr
 */
IrNode *
noisyParseChanEventExpression(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseChanEventExpr);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_PchanEventExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Terasures))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Terasures));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tdistortions))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tdistortions));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tlatency))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tlatency));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PchanEventExpr, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PchanEventExpr);
	}

	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeafWithChainingSeq(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	addLeafWithChainingSeq(N, n, noisyParseCmpOp(N));
	addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PchanEventExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PchanEventExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PchanEventExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PloadExpr
 *
 *	Grammar production:
 *		loadExpr	::=	"load" identifier ["->" identifier] [tupleType] (stringConst | "(" "path" typeName ")") .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PloadExpr
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= Xseq of [kNoisyIrNodeType_Ptupletype] kNoisyIrNodeType_TstringConst kNoisyIrNodeType_PtypeName
 */
IrNode *
noisyParseLoadExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseLoadExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PloadExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tload);

	addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	if (peekCheck(N, 1, kNoisyIrNodeType_Tarrow))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tarrow);
		addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	}

	if (inFirst(N, kNoisyIrNodeType_PtupleType, gNoisyFirsts, kNoisyIrNodeTypeMax) && !peekCheck(N, 2, kNoisyIrNodeType_Tpath))
	{
		addLeafWithChainingSeq(N, n, noisyParseTupleType(N, currentScope));
	}

	if (peekCheck(N, 1, kNoisyIrNodeType_TleftParens) && peekCheck(N, 2, kNoisyIrNodeType_Tpath))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
		noisyParseTerminal(N, kNoisyIrNodeType_Tpath);
		addLeafWithChainingSeq(N, n, noisyParseTypeName(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);
	}
	else
	{
		addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TstringConst));
	}

	if (!inFollow(N, kNoisyIrNodeType_PloadExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PloadExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PloadExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_Pterm
 *
 *	Grammar production:
 *		term			::=	[basicType] [unaryOp] factor ["++" | "--"] {highPrecedenceBinaryOp factor} .
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pfactor
 *		node.right	= Xseq of kNoisyIrNodeType_PhighPrecedenceBinaryOp  and kNoisyIrNodeType_Pfactor
 */
IrNode *
noisyParseTerm(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTerm);


	IrNode *	n = genIrNode(N,	kNoisyIrNodeType_Pterm,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PbasicType, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseBasicType(N, currentScope));

		/*
		 *	See issue number #203 for how this used to be before
		 *	(the second if was not enclosed here and we were using
		 *	just addLeaf, not addLeafWithChainingSeq)
		 */
		if (inFirst(N, kNoisyIrNodeType_PunaryOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
		{
			addLeafWithChainingSeq(N, n, noisyParseUnaryOp(N));
		}
	}
	else if (inFirst(N, kNoisyIrNodeType_PunaryOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParseUnaryOp(N));
	}

	addLeaf(N, n, noisyParseFactor(N, currentScope));

	if (peekCheck(N, 1, kNoisyIrNodeType_TplusPlus))
	{
		addLeafWithChainingSeq(N, n, noisyParseTerminal(N,kNoisyIrNodeType_TplusPlus));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TminusMinus))
	{
		addLeafWithChainingSeq(N, n, noisyParseTerminal(N,kNoisyIrNodeType_TminusMinus));
	}

	while (inFirst(N, kNoisyIrNodeType_PhighPrecedenceBinaryOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParseHighPrecedenceBinaryOp(N));
		addLeafWithChainingSeq(N, n, noisyParseFactor(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_Pterm, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Pterm, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Pterm);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PqualifiedIdentifier
 *
 *	Grammar production:
 *		qualifiedIdentifier	=	identifier {fieldSelect} .
 *
 *	Generated AST subtree:
 *
 *		node		= qualifiedIdentifier
 *		node.left	= kNoisyIrNodeType_Tidentifier 
 *		node.right	= NULL or Xseq of kNoisyIrNodeType_PfieldSelect kNoisyIrNodeType_Tidentifier
 */
IrNode *
noisyParseQualifiedIdentifier(State *  N, Scope *  currentScope, bool isDefinition)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParsenQualifiedIdentifier);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PqualifiedIdentifier,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (isDefinition)
	{
		addLeaf(N, n, noisyParseIdentifierDefinitionTerminal(N, currentScope));
	}
	else
	{
		addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	}

	while (inFirst(N, kNoisyIrNodeType_PfieldSelect, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParseFieldSelect(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PqualifiedIdentifier, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PqualifiedIdentifier, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PqualifiedIdentifier);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_Pfactor
 *
 *	Grammar production:
 *		factor		::=	qualifiedIdentifier | integerConst | realConst | stringConst | boolConst
 *				| "(" expression ")" | tupleValue | namegenInvokeShorthand | typeMinExpr | typeMaxExpr | nil.
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Pfactor
 *		node.left	= kNoisyIrNodeType_Tidentifier | kNoisyIrNodeType_TintegerConst | kNoisyIrNodeType_TrealConst | kNoisyIrNodeType_TstringConst | kNoisyIrNodeType_TboolConst 
 *				| kNoisyIrNodeType_Pexpression | kNoisyIrNodeType_PtupleValue | kNoisyIrNodeType_PnamegenInvokeShorthand | kNoisyIrNodeType_PtypeMinExpr | kNoisyIrNodeType_PtypeMaxExpr
 *		node.right	= NULL or Xseq of kNoisyIrNodeType_PfieldSelect kNoisyIrNodeType_Tidentifier
 */
IrNode *
noisyParseFactor(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseFactor);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_Pfactor,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		if (peekCheck(N, 2, kNoisyIrNodeType_TleftParens))
		{
			addLeafWithChainingSeq(N, n, noisyParseNamegenInvokeShorthand(N, currentScope));
		}
		else
		{
			addLeafWithChainingSeq(N, n, noisyParseQualifiedIdentifier(N, currentScope, false /* isDefinition */));
		}
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TintegerConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintegerConst));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TrealConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TstringConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TstringConst));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TboolConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TboolConst));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Ttypemin))
	{
		addLeaf(N, n, noisyParseTypeMinExpr(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Ttypemax))
	{
		addLeaf(N, n, noisyParseTypeMaxExpr(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tnil))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tnil));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftParens) && peekCheck(N, 3, kNoisyIrNodeType_Tcomma))
	{
		/*
		 *	If we see
		 *
		 *			'(' identifier ','
		 *
		 *	then try to parse a tupleValue. Otherwise, parse an expression
		 */
		addLeaf(N, n, noisyParseTupleValue(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftParens))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
		addLeaf(N, n, noisyParseExpression(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Pfactor, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Pfactor);
	}

	if (!inFollow(N, kNoisyIrNodeType_Pfactor, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Pfactor, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Pfactor);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PtypeMinExpr
 *
 *	Grammar production:
 *		typeMinExpr		::=	"typemin" "(" numericType ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PtypeMinExpr
 *		node.left	= kNoisyIrNodeType_PnumericType
 *		node.right	= NULL
 */
IrNode *
noisyParseTypeMinExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTypeMinExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PtypeMinExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Ttypemin);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParseNumericType(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PtypeMinExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeMinExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeMinExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PtypeMaxExpr
 *
 *	Grammar production:
 *		typeMaxExpr		::=	"typemax" "(" numericType ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PtypeMaxExpr
 *		node.left	= kNoisyIrNodeType_PnumericType
 *		node.right	= NULL
 */
IrNode *
noisyParseTypeMaxExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTypeMaxExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PtypeMaxExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Ttypemax);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParseNumericType(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PtypeMaxExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeMaxExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeMaxExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PnamegenInvokeShorthand
 *
 *	Grammar production:
 *		namegenInvokeShorthand	=	identifier "(" [identifier ":" expression {","  identifier ":" expression}] ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PnamegenInvokeShorthand
 *		node.left	= kNoisyIrNodeType_Pidentifier
 *		node.right	= Xseq of repeating kNoisyIrNodeType_Pidentifier and kNoisyIrNodeType_Pexpression
 */
IrNode *
noisyParseNamegenInvokeShorthand(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseNamegenInvokeShorthand);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PnamegenInvokeShorthand,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	/*
	 *	TODO: for now, we do not check the argument names below
	 *	(by calling noisyParseIdentifierTerminalIgnoreSymtab() <-- which we use only here).
	 *	This is OK since languages like C don't add that contraint.
	 *	In future, we will need to, e.g., hold on to the symbol
	 *	for the function name since we will use that scope to find
	 *	the function argument names.
	 */


	addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);

	if (peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		addLeafWithChainingSeq(N, n, noisyParseIdentifierTerminalIgnoreSymtab(N));
		noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
		addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));

		while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
		{
			noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
			addLeafWithChainingSeq(N, n, noisyParseIdentifierTerminalIgnoreSymtab(N));
			noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
			addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));
		}
	}

	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);


	if (!inFollow(N, kNoisyIrNodeType_PnamegenInvokeShorthand, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PnamegenInvokeShorthand, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PnamegenInvokeShorthand);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PtupleValue
 *
 *	Grammar production:
 *		tupleValue	::=	"(" identifierOrNilList ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PtupleValue
 *		node.left	= kNoisyIrNodeType_PidentifierOrNil
 *		node.right	= Xseq of kNoisyIrNodeType_PidentifierOrNil
 */
IrNode *
noisyParseTupleValue(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTupleValue);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PtupleValue,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);

	/*
	 *	Identical to the parse tree for an kNoisyIrNodeType_PidentifierOrNilList,
	 *	but labeled as a kNoisyIrNodeType_PtupleValue.
	 */
	IrNode *  tmp	= noisyParseIdentifierOrNilList(N, currentScope, false /* isDefinition */);
	tmp->type	= kNoisyIrNodeType_PtupleValue;
	addLeaf(N, n, tmp);
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);


	if (!inFollow(N, kNoisyIrNodeType_PtupleValue, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtupleValue, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtupleValue);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PfieldSelect
 *
 *	Grammar production:
 *		fieldSelect		::=	("." identifier) | ("[" expression [":" expression] "]") .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Tdot or kNoisyIrNodeType_TleftBrac (nothing else)
 *		node.left	= kNoisyIrNodeType_Tidentifier | kNoisyIrNodeType_Pexpression
 *		node.right	= NULL or kNoisyIrNodeType_Pexpression
 */
IrNode *
noisyParseFieldSelect(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseFieldSelect);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PfieldSelect,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	/*
	 *	We use a single AST node (kNoisyIrNodeType_Tdot or kNoisyIrNodeType_TleftBracket) to differentiate
	 *	the two types of subtrees.
	 */
	if (peekCheck(N, 1, kNoisyIrNodeType_Tdot))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tdot));
		addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftBracket))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TleftBracket));
		addLeaf(N, n, noisyParseExpression(N, currentScope));

		if (peekCheck(N, 1, kNoisyIrNodeType_Tcolon))
		{
			noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
			addLeaf(N, n, noisyParseExpression(N, currentScope));
		}

		noisyParseTerminal(N, kNoisyIrNodeType_TrightBracket);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PfieldSelect, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PfieldSelect);
	}

	if (!inFollow(N, kNoisyIrNodeType_PfieldSelect, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PfieldSelect, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PfieldSelect);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PhighPrecedenceBinaryOp
 *
 *	Grammar production:
 *		highPrecedenceBinaryOp	::=	"*" | "/" | "%" | "&" | "^" | "::" | "lowpass" | "highpass" | "dotproduct"
 *				| "crossproduct" | "centralmoment" | highPrecedenceBinaryBoolOp .
 *
 *	Generated AST subtree:
 *
 *		node		= 
 *		node.left	= kNoisyIrNodeType_Tasterisk | kNoisyIrNodeType_Tdivide | kNoisyIrNodeType_Tpercent 
 *				| kNoisyIrNodeType_Tampersand | kNoisyIrNodeType_Txor | kNoisyIrNodeType_Tcons | kNoisyIrNodeType_Tlowpass
 *				| kNoisyIrNodeType_Thighpass | kNoisyIrNodeType_Tdotproduct | kNoisyIrNodeType_Tcrossproduct | kNoisyIrNodeType_Tcentralmoment
 *				| kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp
 *		node.right	= NULL
 */
IrNode *
noisyParseHighPrecedenceBinaryOp(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseHighPrecedenceBinaryOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PhighPrecedenceBinaryOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tasterisk))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tasterisk));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tdivide))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tdivide));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tpercent))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tpercent));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TarithmeticAnd))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TarithmeticAnd));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Txor))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Txor));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TcolonColon))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TcolonColon));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tlowpass))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tlowpass));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Thighpass))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Thighpass));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tdotproduct))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tdotproduct));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tcrossproduct))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tcrossproduct));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tcentralmoment))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tcentralmoment));
	}
	else if (inFirst(N, kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseHighPrecedenceBinaryBoolOp(N));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PhighPrecedenceBinaryOp, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PhighPrecedenceBinaryOp);
	}

	if (!inFollow(N, kNoisyIrNodeType_PhighPrecedenceBinaryOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PhighPrecedenceBinaryOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PhighPrecedenceBinaryOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PlowPrecedenceBinaryOp
 *
 *	Grammar production:
 *		lowPrecedenceBinaryOp	::=	"+" | "-" | ">>" | "<<" | "|"  | cmpOp | lowPrecedenceBinaryBoolOp .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PlowPrecedenceBinaryOp
 *		node.left	= kNoisyIrNodeType_Tplus | ... | kNoisyIrNodeType_Tstroke
 *		node.right	= NULL
 */
IrNode *
noisyParseLowPrecedenceBinaryOp(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseLowPrecedenceBinaryOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PlowPrecedenceBinaryOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tplus))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tplus));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tminus))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tminus));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TrightShift))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrightShift));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftShift))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TleftShift));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TbitwiseOr))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TbitwiseOr));
	}
	else if (inFirst(N, kNoisyIrNodeType_PcmpOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseCmpOp(N));
	}
	else if (inFirst(N, kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseLowPrecedenceBinaryBoolOp(N));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PlowPrecedenceBinaryOp, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PlowPrecedenceBinaryOp);
	}

	if (!inFollow(N, kNoisyIrNodeType_PlowPrecedenceBinaryOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PlowPrecedenceBinaryOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PlowPrecedenceBinaryOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PcmpOp
 *
 *	Grammar production:
 *		cmpOp			::=	"==" | "!=" | ">" | "<" | "<=" | ">=" | ">=<" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PcmpOp
 *		node.left	= kNoisyIrNodeType_Teq | ... | kNoisyIrNodeType_Tor
 *		node.right	= NULL
 */
IrNode *
noisyParseCmpOp(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseCmpOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PcmpOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tequals))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tequals));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TnotEqual))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TnotEqual));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TgreaterThan))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TgreaterThan));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TlessThan))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TlessThan));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TlessThanEqual))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TlessThanEqual));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TgreaterThanEqual))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TgreaterThanEqual));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TisPermutationOf))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TisPermutationOf));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PcmpOp, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PcmpOp);
	}

	if (!inFollow(N, kNoisyIrNodeType_PcmpOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PcmpOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PcmpOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PunaryOp
 *
 *	unaryOp			::=	"~" | "-"  | "+" | "<-"  | "head" | "tail" | "tailtip"| "length" | "sort" | "uncertainty"
 *				| "tintegral" | "tderivative" | "timebase" | "sigfigs" | "samples" | "reverse"
 *				| "fourier" | "cardinality" | "frequencies" | "magnitudes" | unaryBoolOp .
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PunaryOp
 *		node.left	= kNoisyIrNodeType_Tname2chan | ... | kNoisyIrNodeType_Tgets
 *		node.right	= NULL
 */
IrNode *
noisyParseUnaryOp(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseUnaryOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PunaryOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Ttilde))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Ttilde));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tnot))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tnot));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tminus))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tminus));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tplus))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tplus));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TchannelOperator))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TchannelOperator));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Thead))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Thead));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Ttail))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Ttail));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Ttailtip))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Ttailtip));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tlength))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tlength));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tsort))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tsort));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tuncertainty))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tuncertainty));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Ttintegral))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Ttintegral));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Ttderivative))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Ttderivative));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Ttimebase))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Ttimebase));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tsigfigs))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tsigfigs));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tsamples))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tsamples));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Treverse))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Treverse));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tfourier))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tfourier));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tcardinality))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tcardinality));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tfrequencies))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tfrequencies));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tmagnitudes))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tmagnitudes));
	}
	else if (inFirst(N, kNoisyIrNodeType_PunaryBoolOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseUnaryBoolOp(N));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PunaryOp, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PunaryOp);
	}

	if (!inFollow(N, kNoisyIrNodeType_PunaryOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PunaryOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PunaryOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp
 *
 *	Grammar production:
 *		lowPrecedenceBinaryBoolOp	::= "||" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp
 *		node.left	= kNoisyIrNodeType_TlogicalOr
 *		node.right	= NULL
 */
IrNode *
noisyParseLowPrecedenceBinaryBoolOp(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseLowPrecedenceBinaryBoolOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TlogicalOr));

	if (!inFollow(N, kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp
 *
 *	Grammar production:
 *		highPrecedenceBinaryBoolOp	::=	"&&" | "^" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp
 *		node.left	= kNoisyIrNodeType_TlogicalAnd | kNoisyIrNodeType_Txor
 *		node.right	= NULL
 */
IrNode *
noisyParseHighPrecedenceBinaryBoolOp(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseHighPrecedenceBinaryBoolOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_TlogicalAnd))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TlogicalAnd));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Txor))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Txor));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp);
	}

	if (!inFollow(N, kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PunaryBoolOp
 *
 *	Grammar production:
 *		unaryBoolOp			::=	"!" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PunaryBoolOp
 *		node.left	= kNoisyIrNodeType_Tbang
 *		node.right	= NULL
 */
IrNode *
noisyParseUnaryBoolOp(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseUnaryBoolOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PunaryBoolOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tnot));

	if (!inFollow(N, kNoisyIrNodeType_PunaryBoolOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PunaryBoolOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PunaryBoolOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_Parith2BoolOp
 *
 *	Grammar production:
 *		arith2BoolOp	::=	"==" | "!=" | ">" | ">=" | "<" | "<=" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Parith2BoolOp
 *		node.left	= kNoisyIrNodeType_Tequals | kNoisyIrNodeType_TnotEquals | kNoisyIrNodeType_TgreaterThan
 *				| kNoisyIrNodeType_TgreaterThanEqual | kNoisyIrNodeType_TlessThan | kNoisyIrNodeType_TlessThanEqual
 *		node.right	= NULL
 */
IrNode *
noisyParseArith2BoolOp(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseArith2BoolOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_Parith2BoolOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tequals))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tequals));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TnotEqual))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TnotEqual));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TgreaterThan))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TgreaterThan));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TgreaterThanEqual))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TgreaterThanEqual));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TlessThan))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TlessThan));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TlessThanEqual))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TlessThanEqual));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Parith2BoolOp, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Parith2BoolOp);
	}

	if (!inFollow(N, kNoisyIrNodeType_Parith2BoolOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Parith2BoolOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Parith2BoolOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PhighPrecedenceArith2ArithOp
 *
 *	Grammar production:
 *		highPrecedenceArith2ArithOp	::=	"*" | "/" | "%" | "&" | "pow" | "nrt" | "log" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PhighPrecedenceArith2ArithOp
 *		node.left	= kNoisyIrNodeType_Tasterisk | kNoisyIrNodeType_Tdivide | kNoisyIrNodeType_Tpercent
 *				| kNoisyIrNodeType_TarithmeticAnd | kNoisyIrNodeType_Tpow | kNoisyIrNodeType_Tnrt | kNoisyIrNodeType_Tlog .
 *		node.right	= NULL
 */
IrNode *
noisyParseHighPrecedenceArith2ArithOp(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseHighPrecedenceArith2ArithOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PhighPrecedenceArith2ArithOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tasterisk))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tasterisk));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tdivide))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tdivide));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tpercent))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tpercent));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TarithmeticAnd))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TarithmeticAnd));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tpow))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tpow));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tnrt))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tnrt));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tlog))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tlog));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PhighPrecedenceArith2ArithOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PhighPrecedenceArith2ArithOp);
	}

	if (!inFollow(N, kNoisyIrNodeType_PhighPrecedenceArith2ArithOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PhighPrecedenceArith2ArithOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PhighPrecedenceArith2ArithOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PlowPrecedenceArith2ArithOp
 *
 *	Grammar production:
 *		lowPrecedenceArith2ArithOp	::=	"+" | "-" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PlowPrecedenceArith2ArithOp
 *		node.left	= kNoisyIrNodeType_Tplus | kNoisyIrNodeType_Tminus
 *		node.right	= NULL
 */
IrNode *
noisyParseLowPrecedenceArith2ArithOp(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseLowPrecedenceArith2ArithOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PlowPrecedenceArith2ArithOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tplus))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tplus));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tminus))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tminus));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PhighPrecedenceArith2ArithOp, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PhighPrecedenceArith2ArithOp);
	}

	if (!inFollow(N, kNoisyIrNodeType_PlowPrecedenceArith2ArithOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PlowPrecedenceArith2ArithOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PlowPrecedenceArith2ArithOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PscopedPredStmtList
 *
 *	Grammar production:
 *		scopedPredStmtList	::=	"{" predStmtList "}" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PscopedPredStmtList
 *		node.left	= kNoisyIrNodeType_PpredStmtList
 *		node.right	= NULL
 */
IrNode *
noisyParseScopedPredStmtList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseScopedPredStmtList);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PscopedPredStmtList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	addLeaf(N, n, noisyParsePredStmtList(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);

	if (!inFollow(N, kNoisyIrNodeType_PscopedPredStmtList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PscopedPredStmtList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PscopedPredStmtList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PpredStmtList
 *
 *	Grammar production:
 *		predStmtList		=	{predStmt} .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PpredStmtList
 *		node.left	= kNoisyIrNodeType_PpredStmt or NULL
 *		node.right	= Xseq of kNoisyIrNodeType_PpredStmt or NULL
 */
IrNode *
noisyParsePredStmtList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParsePredStmtList);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PpredStmtList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	while (inFirst(N, kNoisyIrNodeType_PpredStmt, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParsePredStmt(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PpredStmtList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredStmtList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredStmtList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PpredStmt
 *
 *	Grammar production:
 *		predStmt	::=	predExpr "," .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PpredStmt
 *		node.left	= kNoisyIrNodeType_PpredExpr
 *		node.right	= NULL
 */
IrNode *
noisyParsePredStmt(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParsePredStmt);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PpredStmt,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParsePredExpr(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PpredStmt, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredStmt, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredStmt);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PpredFactor
 *
 *	Grammar production:
 *		predFactor	::=	boolConst | identifier | "(" predExpr ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PpredFactor
 *		node.left	= kNoisyIrNodeType_TboolConst | kNoisyIrNodeType_Tidentifier | kNoisyIrNodeType_PpredExpr
 *		node.right	= NULL
 */
IrNode *
noisyParsePredFactor(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParsePredFactor);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PpredFactor,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_TboolConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TboolConst));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftParens))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
		addLeaf(N, n, noisyParsePredExpr(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredFactor, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredFactor);
	}
	
	if (!inFollow(N, kNoisyIrNodeType_PpredFactor, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredFactor, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredFactor);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PpredTerm
 *
 *	Grammar production:
 *		predTerm	::=	predFactor {highPrecedenceBinaryBoolOp predFactor}
 *				| predArithExpr arith2BoolOp ["@" (intParamOrConst | realParamOrConst)] predArithExpr
 *				| quantifiedBoolTerm
 *				| setCmpTerm
 *				| varTuple "in" ["@" (intParamOrConst | realParamOrConst)] setExpr
 *				| unaryBoolOp predFactor .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PpredTerm
 *		node.left	= See above
 *		node.right	= See above
 */
IrNode *
noisyParsePredTerm(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParsePredTerm);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PpredTerm,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PpredFactor, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParsePredFactor(N, currentScope));

		while (inFirst(N, kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
		{
			addLeafWithChainingSeq(N, n, noisyParseHighPrecedenceBinaryBoolOp(N));
			addLeafWithChainingSeq(N, n, noisyParsePredFactor(N, currentScope));
		}
	}
	else if (inFirst(N, kNoisyIrNodeType_PpredArithExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParsePredArithExpr(N, currentScope));
		addLeafWithChainingSeq(N, n, noisyParseArith2BoolOp(N, currentScope));
		if (peekCheck(N, 1, kNoisyIrNodeType_Tat))
		{
			/*
			 *	Not really needed, but add to tree as a signpost
			 */
			addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tat));

			if (inFirst(N, kNoisyIrNodeType_PintParamOrConst, gNoisyFirsts, kNoisyIrNodeTypeMax))
			{
				addLeafWithChainingSeq(N, n, noisyParseIntParamOrConst(N, currentScope));
			}
			else if (inFirst(N, kNoisyIrNodeType_PrealParamOrConst, gNoisyFirsts, kNoisyIrNodeTypeMax))
			{
				addLeafWithChainingSeq(N, n, noisyParseRealParamOrConst(N, currentScope));
			}
			else
			{
				noisyParserSyntaxError(N, kNoisyIrNodeType_PintParamOrConst, kNoisyIrNodeTypeMax, gNoisyFirsts);
				noisyParserSyntaxError(N, kNoisyIrNodeType_PrealParamOrConst, kNoisyIrNodeTypeMax, gNoisyFirsts);
				noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredTerm);
			}
		}
		addLeaf(N, n, noisyParsePredArithExpr(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PquantifiedBoolTerm, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseQuantifiedBoolTerm(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PsetCmpTerm, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseSetCmpTerm(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PvarTuple, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseVarTuple(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_Tin);

		if (peekCheck(N, 1, kNoisyIrNodeType_Tat))
		{
			/*
			 *	Not really needed, but add to tree as a signpost
			 */
			addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tat));

			if (inFirst(N, kNoisyIrNodeType_PintParamOrConst, gNoisyFirsts, kNoisyIrNodeTypeMax))
			{
				addLeafWithChainingSeq(N, n, noisyParseIntParamOrConst(N, currentScope));
			}
			else if (inFirst(N, kNoisyIrNodeType_PrealParamOrConst, gNoisyFirsts, kNoisyIrNodeTypeMax))
			{
				addLeafWithChainingSeq(N, n, noisyParseRealParamOrConst(N, currentScope));
			}
		}
		addLeaf(N, n, noisyParseSetExpr(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PunaryBoolOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseUnaryBoolOp(N));
		addLeaf(N, n, noisyParsePredFactor(N, currentScope));
	}
	else 
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredTerm, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredTerm);
	}

	if (!inFollow(N, kNoisyIrNodeType_PpredTerm, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredTerm, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredTerm);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PpredExpr
 *
 *	Grammar production:
 *		predExpr		::=	predTerm {(lowPrecedenceBinaryBoolOp | cmpOp) predTerm} .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PpredExpr
 *		node.left	= predTerm
 *		node.right	= Xseq of repeating kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp kNoisyIrNodeType_PpredTerm
 */
IrNode *
noisyParsePredExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParsePredExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PpredExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParsePredTerm(N, currentScope));
	while (	inFirst(N, kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp, gNoisyFirsts, kNoisyIrNodeTypeMax) ||
		inFirst(N, kNoisyIrNodeType_PcmpOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		if (inFirst(N, kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp, gNoisyFirsts, kNoisyIrNodeTypeMax) )
		{
			addLeafWithChainingSeq(N, n, noisyParseLowPrecedenceBinaryBoolOp(N));
		}
		else if (inFirst(N, kNoisyIrNodeType_PcmpOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
		{
			addLeafWithChainingSeq(N, n, noisyParseCmpOp(N));
		}

		addLeafWithChainingSeq(N, n, noisyParsePredTerm(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PpredExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PvarIntro
 *
 *	Grammar production:
 *		varIntro		::=	identifier "in" (setExpr | typeExpr) .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PvarIntro
 *		node.left	= kNoisyIrNodeType_PsetExpr or kNoisyIrNodeType_PtypeExpr
 *		node.right	= NULL
 */
IrNode *
noisyParseVarIntro(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseVarIntro);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PvarIntro,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseIdentifierDefinitionTerminal(N, currentScope));
	if (inFirst(N, kNoisyIrNodeType_PsetExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseSetExpr(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PtypeExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseTypeExpr(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PvarIntro, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PvarIntro, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PvarIntro);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PvarIntroList
 *
 *	Grammar production:
 *		varIntroList	::=	varIntro {"," varIntro} .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PvarIntroList
 *		node.left	= kNoisyIrNodeType_PvarIntro
 *		node.right	= Xseq of repeating kNoisyIrNodeType_PvarIntro
 */
IrNode *
noisyParseVarIntroList(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseVarIntroList);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PvarIntroList,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseVarIntro(N, currentScope));
	while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
		addLeafWithChainingSeq(N, n, noisyParseVarIntro(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PvarIntroList, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PvarIntroList, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PvarIntroList);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PvarTuple
 *
 *	Grammar production:
 *		varTuple	::=	"(" identifier {"," identifier} ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PvarTuple
 *		node.left	= kNoisyIrNodeType_Pidentifier
 *		node.right	= Xseq of repeating kNoisyIrNodeType_Pidentifier
 */
IrNode *
noisyParseVarTuple(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseVarTuple);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PvarTuple,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
		addLeafWithChainingSeq(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	}
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PvarTuple, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PvarTuple, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PvarTuple);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_ParithParamOrConst
 *
 *	Grammar production:
 *		arithParamOrConst	::=	intParamOrConst | realParamOrConst .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_ParithParamOrConst
 *		node.left	= kNoisyIrNodeType_PintParamOrConst | kNoisyIrNodeType_PrealParamOrConst
 *		node.right	= NULL
 */
IrNode *
noisyParseArithConst(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseArithConst);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_ParithParamOrConst,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PintParamOrConst, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseIntParamOrConst(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PrealParamOrConst, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseRealParamOrConst(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_ParithParamOrConst, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_ParithParamOrConst);
	}

	if (!inFollow(N, kNoisyIrNodeType_ParithParamOrConst, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_ParithParamOrConst, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_ParithParamOrConst);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PpredArithFactor
 *
 *	Grammar production:
 *		predArithFactor		::=	arithParamOrConst | varIntro | identifier | "(" predArithExpr ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PpredArithFactor
 *		node.left	= kNoisyIrNodeType_ParithParamOrConst | kNoisyIrNodeType_PvarIntro | kNoisyIrNodeType_Pidentifier | kNoisyIrNodeType_PpredArithExpr
 *		node.right	= NULL
 */
IrNode *
noisyParsePredArithFactor(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParsePredArithFactor);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PpredArithFactor,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_ParithParamOrConst, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseArithConst(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PvarIntro, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseVarIntro(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftParens))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
		addLeaf(N, n, noisyParsePredArithExpr(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);
	}
	else 
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredArithFactor, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredArithFactor);
	}

	if (!inFollow(N, kNoisyIrNodeType_PpredArithFactor, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredArithFactor, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredArithFactor);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PpredArithTerm
 *
 *	Grammar production:
 *		predArithTerm	::=	predArithFactor {highPrecedenceArith2ArithOp predArithFactor} .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PpredArithTerm
 *		node.left	= kNoisyIrNodeType_PpredArithFactor
 *		node.right	= Xseq of repeating kNoisyIrNodeType_PhighPrecedenceArith2ArithOp kNoisyIrNodeType_PpredArithFactor
 */
IrNode *
noisyParsePredArithTerm(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParsePredArithTerm);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PpredArithTerm,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParsePredArithFactor(N, currentScope));
	while (inFirst(N, kNoisyIrNodeType_PhighPrecedenceArith2ArithOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseHighPrecedenceArith2ArithOp(N, currentScope));
		addLeaf(N, n, noisyParsePredArithFactor(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PpredArithTerm, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredArithTerm, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredArithTerm);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PpredArithExpr
 *
 *	Grammar production:
 *		predArithExpr		::=	predArithTerm {lowPrecedenceArith2ArithOp predArithTerm}
 *				| sumForExpr | productForExpr | minForExpr | maxForExpr .
 *
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PpredArithExpr
 *		node.left	= kNoisyIrNodeType_PpredArithTerm | kNoisyIrNodeType_PsumForExpr | kNoisyIrNodeType_PproductForExpr | kNoisyIrNodeType_PminForExpr | kNoisyIrNodeType_PmaxForExpr
 *		node.right	= NULL or Xseq of repeating kNoisyIrNodeType_PlowPrecedenceArith2ArithOp kNoisyIrNodeType_PpredArithTerm
 *				
 */
IrNode *
noisyParsePredArithExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParsePredArithExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PpredArithExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PpredArithTerm, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParsePredArithTerm(N, currentScope));
		while (inFirst(N, kNoisyIrNodeType_PlowPrecedenceArith2ArithOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
		{
			addLeaf(N, n, noisyParseLowPrecedenceArith2ArithOp(N, currentScope));
			addLeaf(N, n, noisyParsePredArithTerm(N, currentScope));
		}
	}
	else if (inFirst(N, kNoisyIrNodeType_PsumForExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseSumForExpr(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PproductForExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseProductForExpr(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PminForExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseMinForExpr(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PmaxForExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseMaxForExpr(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredArithExpr, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredArithExpr);
	}

	if (!inFollow(N, kNoisyIrNodeType_PpredArithExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PpredArithExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PpredArithExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PsumForExpr
 *
 *	Grammar production:
 *		sumForExpr		::=	"sum" sumProdMinMaxBody .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsumForExpr
 *		node.left	= kNoisyIrNodeType_PsumProdMinMaxBody
 *		node.right	= NULL
 */
IrNode *
noisyParseSumForExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSumForExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PsumForExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tsum);
	addLeaf(N, n, noisyParseSumProdMinMaxBody(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PsumForExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsumForExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsumForExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PproductForExpr
 *
 *	Grammar production:
 *		productForExpr		::=	"product" sumProdMinMaxBody .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PproductForExpr
 *		node.left	= kNoisyIrNodeType_PsumProdMinMaxBody
 *		node.right	= NULL
 */
IrNode *
noisyParseProductForExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseProductForExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PproductForExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tproduct);
	addLeaf(N, n, noisyParseSumProdMinMaxBody(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PproductForExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PproductForExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PproductForExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PminForExpr
 *
 *	Grammar production:
 *		minForExpr		::=	"min" sumProdMinMaxBody .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PminForExpr
 *		node.left	= kNoisyIrNodeType_PsumProdMinMaxBody
 *		node.right	= NULL
 */
IrNode *
noisyParseMinForExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseMinForExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PminForExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tmin);
	addLeaf(N, n, noisyParseSumProdMinMaxBody(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PminForExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PminForExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PminForExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PmaxForExpr
 *
 *	Grammar production:
 *		maxForExpr		::=	"max" sumProdMinMaxBody .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PmaxForExpr
 *		node.left	= kNoisyIrNodeType_PsumProdMinMaxBody
 *		node.right	= NULL
 */
IrNode *
noisyParseMaxForExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseMaxForExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PmaxForExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tmax);
	addLeaf(N, n, noisyParseSumProdMinMaxBody(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PmaxForExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PmaxForExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PmaxForExpr);
	}

	return n;
}


/*
 *	kNoisyIrNodeType_PsumProdMinMaxBody
 *
 *	Grammar production:
 *		sumProdMinMaxBody	::=	["for" varIntro ["from" predArithExpr "to" predArithExpr]] ["with" predExpr] "of" "(" predArithExpr ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsumProdMinMaxBody
 *		node.left	= See above
 *		node.right	= See above
 */
IrNode *
noisyParseSumProdMinMaxBody(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSumProdMinMaxBody);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PsumProdMinMaxBody,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tfor))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tfor);
		addLeafWithChainingSeq(N, n, noisyParseVarIntro(N, currentScope));

		if (peekCheck(N, 1, kNoisyIrNodeType_Tfrom))
		{
			noisyParseTerminal(N, kNoisyIrNodeType_Tfrom);
			addLeafWithChainingSeq(N, n, noisyParsePredArithExpr(N, currentScope));

			noisyParseTerminal(N, kNoisyIrNodeType_Tto);
			addLeafWithChainingSeq(N, n, noisyParsePredArithExpr(N, currentScope));
		}
	}

	if (peekCheck(N, 1, kNoisyIrNodeType_Twith))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Twith);
		addLeafWithChainingSeq(N, n, noisyParsePredExpr(N, currentScope));
	}

	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeafWithChainingSeq(N, n, noisyParsePredArithExpr(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PsumProdMinMaxBody, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsumProdMinMaxBody, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsumProdMinMaxBody);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PquantifiedBoolTerm
 *
 *	Grammar production:
 *		quantifiedBoolTerm	::=	quantifierOp varIntroList "(" predExpr ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PquantifiedBoolTerm
 *		node.left	= kNoisyIrNodeType_PquantifierOp
 *		node.right	= Xseq of kNoisyIrNodeType_PvarIntroList kNoisyIrNodeType_PpredExpr
 */
IrNode *
noisyParseQuantifiedBoolTerm(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseQuantifiedBoolTerm);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PquantifiedBoolTerm,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseQuantifierOp(N, currentScope));
	addLeafWithChainingSeq(N, n, noisyParseVarIntroList(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParsePredExpr(N, currentScope));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_PquantifiedBoolTerm, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PquantifiedBoolTerm, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PquantifiedBoolTerm);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PsetCmpTerm
 *
 *	Grammar production:
 *		setCmpTerm	::=	setExpr setCmpOp setExpr .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsetCmpTerm
 *		node.left	= kNoisyIrNodeType_PsetExpr
 *		node.right	= Xseq of kNoisyIrNodeType_PsetCmpOp kNoisyIrNodeType_PsetExpr
 */
IrNode *
noisyParseSetCmpTerm(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSetCmpTerm);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PsetCmpTerm,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseSetExpr(N, currentScope));
	addLeafWithChainingSeq(N, n, noisyParseSetCmpOp(N, currentScope));
	addLeaf(N, n, noisyParseSetExpr(N, currentScope));

	if (!inFollow(N, kNoisyIrNodeType_PsetCmpTerm, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsetCmpTerm, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsetCmpTerm);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PsetFactor
 *
 *	Grammar production:
 *		setFactor	::=	constSetExpr ":" typeExpr | "{" "}" | "omega"
 *				| "(" setExpr ")" | "(" predExpr ":" typeExpr ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsetFactor
 *		node.left	= See above
 *		node.right	= See above
 */
IrNode *
noisyParseSetFactor(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSetFactor);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PsetFactor,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PconstSetExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseConstSetExpr(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
		addLeaf(N, n, noisyParseTypeExpr(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftBrace))
	{
		/*
		 *	We could get away with adding just one "{" as a signpost, but we add both
		 */
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace));
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tomega))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tomega));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftParens))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
		if (inFirst(N, kNoisyIrNodeType_PsetExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
		{
			addLeaf(N, n, noisyParseSetExpr(N, currentScope));
		}
		else if (inFirst(N, kNoisyIrNodeType_PpredExpr, gNoisyFirsts, kNoisyIrNodeTypeMax))
		{
			addLeaf(N, n, noisyParsePredExpr(N, currentScope));
			noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
			addLeaf(N, n, noisyParseTypeExpr(N, currentScope));
		}
		noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsetFactor, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsetFactor);
	}

	if (!inFollow(N, kNoisyIrNodeType_PsetFactor, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsetFactor, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsetFactor);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PsetTerm
 *
 *	Grammar production:
 *		setTerm		::=	setFactor {highPrecedenceBoolSetOp setFactor}
 *				| unarySetOp setFactor .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsetTerm
 *		node.left	= kNoisyIrNodeType_PsetFactor | kNoisyIrNodeType_PunarySetOp
 *		node.right	= Xseq of kNoisyIrNodeType_PhighPrecedenceBoolSetOp kNoisyIrNodeType_PsetFactor | kNoisyIrNodeType_PsetFactor
 */
IrNode *
noisyParseSetTerm(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSetTerm);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PsetTerm,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PsetFactor, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseSetFactor(N, currentScope));
		while (inFirst(N, kNoisyIrNodeType_PhighPrecedenceBoolSetOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
		{
			addLeafWithChainingSeq(N, n, noisyParseHighPrecedenceBoolSetOp(N, currentScope));
			addLeafWithChainingSeq(N, n, noisyParseSetFactor(N, currentScope));
		}
	}
	else if (inFirst(N, kNoisyIrNodeType_PunarySetOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseUnarySetOp(N, currentScope));
		addLeaf(N, n, noisyParseSetFactor(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsetTerm, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsetTerm);
	}

	if (!inFollow(N, kNoisyIrNodeType_PsetTerm, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsetTerm, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsetTerm);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PsetExpr
 *
 *	Grammar production:
 *		setExpr			::=	setTerm {lowPrecedenceBoolSetOp setTerm} .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsetExpr
 *		node.left	= kNoisyIrNodeType_PsetTerm
 *		node.right	= Xseq of repeating kNoisyIrNodeType_PlowPrecedenceBoolSetOp kNoisyIrNodeType_PsetTerm
 */
IrNode *
noisyParseSetExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSetExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PsetExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseSetTerm(N, currentScope));
	while (inFirst(N, kNoisyIrNodeType_PlowPrecedenceBoolSetOp, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeafWithChainingSeq(N, n, noisyParseLowPrecedenceBoolSetOp(N, currentScope));
		addLeafWithChainingSeq(N, n, noisyParseSetTerm(N, currentScope));
	}

	if (!inFollow(N, kNoisyIrNodeType_PsetExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsetExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsetExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PintParamOrConst
 *
 *	Grammar production:
 *		intParamOrConst		::=	integerConst | identifier .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PintParamOrConst
 *		node.left	= kNoisyIrNodeType_TintegerConst | kNoisyIrNodeType_Tidentifier
 *		node.right	= NULL
 */
IrNode *
noisyParseIntParamOrConst(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseIntParamOrConst);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PintParamOrConst,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_TintegerConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintegerConst));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PintParamOrConst, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PintParamOrConst);
	}

	if (!inFollow(N, kNoisyIrNodeType_PintParamOrConst, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PintParamOrConst, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PintParamOrConst);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PrealParamOrConst
 *
 *	Grammar production:
 *		realParamOrConst	::=	realConst | identifier .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PrealParamOrConst
 *		node.left	= kNoisyIrNodeType_TrealConst | kNoisyIrNodeType_Tidentifier
 *		node.right	= NULL
 */
IrNode *
noisyParseRealParamOrConst(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseRealParamOrConst);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PrealParamOrConst,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_TrealConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PrealParamOrConst, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PrealParamOrConst);
	}

	if (!inFollow(N, kNoisyIrNodeType_PrealParamOrConst, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PrealParamOrConst, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PrealParamOrConst);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PstringParamOrConst
 *
 *	Grammar production:
 *		stringParamOrConst	::=	stringConst | identifier .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PstringParamOrConst
 *		node.left	= kNoisyIrNodeType_TstringConst | kNoisyIrNodeType_Tidentifier
 *		node.right	= NULL
 */
IrNode *
noisyParseStringParamOrConst(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseStringParamOrConst);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PstringParamOrConst,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_TstringConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TstringConst));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PstringParamOrConst, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PstringParamOrConst);
	}

	if (!inFollow(N, kNoisyIrNodeType_PstringParamOrConst, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PstringParamOrConst, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PstringParamOrConst);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PbaseConst
 *
 *	Grammar production:
 *		baseConst	=	intParamOrConst | realParamOrConst | stringParamOrConst .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PbaseConst
 *		node.left	= kNoisyIrNodeType_PintParamOrConst | kNoisyIrNodeType_PrealParamOrConst | kNoisyIrNodeType_PstringParamOrConst
 *		node.right	= NULL
 */
IrNode *
noisyParseBaseConst(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseBaseConst);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PbaseConst,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (inFirst(N, kNoisyIrNodeType_PintParamOrConst, gNoisyFirsts, kNoisyIrNodeTypeMax)) // should also lookup type of the ident in the symtab to see if its int
	{
		addLeaf(N, n, noisyParseIntParamOrConst(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PrealParamOrConst, gNoisyFirsts, kNoisyIrNodeTypeMax)) // should also lookup type of the ident in the symtab to see if its real
	{
		addLeaf(N, n, noisyParseRealParamOrConst(N, currentScope));
	}
	else if (inFirst(N, kNoisyIrNodeType_PstringParamOrConst, gNoisyFirsts, kNoisyIrNodeTypeMax)) // should also lookup type of the ident in the symtab to see if its string
	{
		addLeaf(N, n, noisyParseStringParamOrConst(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PbaseConst, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PbaseConst);
	}

	if (!inFollow(N, kNoisyIrNodeType_PbaseConst, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PbaseConst, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PbaseConst);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_Ptuple
 *
 *	Grammar production:
 *		tuple		::=	"(" baseConst {"," baseConst} ")" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Ptuple
 *		node.left	= kNoisyIrNodeType_PbaseConst
 *		node.right	= Xseq of kNoisyIrNodeType_PbaseConst
 */
IrNode *
noisyParseTuple(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTuple);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_Ptuple,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftParens);
	addLeaf(N, n, noisyParseBaseConst(N, currentScope));
	while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tcomma));
		addLeafWithChainingSeq(N, n, noisyParseBaseConst(N, currentScope));
	}
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParens);

	if (!inFollow(N, kNoisyIrNodeType_Ptuple, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Ptuple, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Ptuple);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PconstSetExpr
 *
 *	Grammar production:
 *		constSetExpr	::=	"{" tuple {"," tuple} "}" | "{" baseConst {"," baseConst} "}" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PconstSetExpr
 *		node.left	= See above
 *		node.right	= See above
 */
IrNode *
noisyParseConstSetExpr(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseConstSetExpr);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PconstSetExpr,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);

	if (inFirst(N, kNoisyIrNodeType_Ptuple, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseTuple(N, currentScope));
		while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
		{
			addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tcomma));
			addLeafWithChainingSeq(N, n, noisyParseTuple(N, currentScope));
		}
	}
	else if (inFirst(N, kNoisyIrNodeType_PbaseConst, gNoisyFirsts, kNoisyIrNodeTypeMax))
	{
		addLeaf(N, n, noisyParseBaseConst(N, currentScope));
		while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
		{
			addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tcomma));
			addLeafWithChainingSeq(N, n, noisyParseBaseConst(N, currentScope));
		}
	}

	noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);

	if (!inFollow(N, kNoisyIrNodeType_PconstSetExpr, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PconstSetExpr, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PconstSetExpr);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PhighPrecedenceBoolSetOp
 *
 *	Grammar production:
 *		highPrecedenceBoolSetOp	::=	"#" | "><" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PhighPrecedenceBoolSetOp
 *		node.left	= kNoisyIrNodeType_TsetIntersect | kNoisyIrNodeType_TsetCross
 *		node.right	= NULL
 */
IrNode *
noisyParseHighPrecedenceBoolSetOp(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseHighPrecedenceBoolSetOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PhighPrecedenceBoolSetOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_TsetIntersect))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TsetIntersect));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TsetCross))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TsetCross));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PhighPrecedenceBoolSetOp, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PhighPrecedenceBoolSetOp);
	}

	if (!inFollow(N, kNoisyIrNodeType_PhighPrecedenceBoolSetOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PhighPrecedenceBoolSetOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PhighPrecedenceBoolSetOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PlowPrecedenceBoolSetOp
 *
 *	Grammar production:
 *		lowPrecedenceBoolSetOp	::=	"+" | "-" | "^" | "=>" | "<=>" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PlowPrecedenceBoolSetOp
 *		node.left	= kNoisyIrNodeType_Tplus | kNoisyIrNodeType_Tminus | kNoisyIrNodeType_Txor | kNoisyIrNodeType_Timplies | kNoisyIrNodeType_Tiff
 *		node.right	= NULL
 */
IrNode *
noisyParseLowPrecedenceBoolSetOp(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseLowPrecedenceBoolSetOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PlowPrecedenceBoolSetOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tplus))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tplus));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tminus))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tminus));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Txor))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Txor));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Timplies))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Timplies));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tiff))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tiff));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PlowPrecedenceBoolSetOp, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PlowPrecedenceBoolSetOp);
	}

	if (!inFollow(N, kNoisyIrNodeType_PlowPrecedenceBoolSetOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PlowPrecedenceBoolSetOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PlowPrecedenceBoolSetOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PunarySetOp
 *
 *	Grammar production:
 *		unarySetOp		::=	"powerset" | "complement" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PunarySetOp
 *		node.left	= kNoisyIrNodeType_Tpowerset | kNoisyIrNodeType_Tcomplement
 *		node.right	= NULL
 */
IrNode *
noisyParseUnarySetOp(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseUnarySetOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PunarySetOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tpowerset))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tpowerset));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tcomplement))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tcomplement));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PunarySetOp, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PunarySetOp);
	}

	if (!inFollow(N, kNoisyIrNodeType_PunarySetOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PunarySetOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PunarySetOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PquantifierOp
 *
 *	Grammar production:
 *		quantifierOp	::=	"forall" | "exists" | "given" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PquantifierOp
 *		node.left	= kNoisyIrNodeType_Tforall | kNoisyIrNodeType_Texists | kNoisyIrNodeType_Tgiven
 *		node.right	= NULL
 */
IrNode *
noisyParseQuantifierOp(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseQuantifierOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PquantifierOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tforall))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tforall));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Texists))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Texists));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tgiven))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tgiven));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PquantifierOp, kNoisyIrNodeTypeMax, gNoisyFirsts);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PquantifierOp);
	}

	if (!inFollow(N, kNoisyIrNodeType_PquantifierOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PquantifierOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PquantifierOp);
	}

	return n;
}



/*
 *	kNoisyIrNodeType_PsetCmpOp
 *
 *	Grammar production:
 *		setCmpOp	::=	"strongdominates" | "weakdominates" .
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PsetCmpOp
 *		node.left	= kNoisyIrNodeType_Tstrongdominates | kNoisyIrNodeType_Tweakdominates
 *		node.right	= NULL
 */
IrNode *
noisyParseSetCmpOp(State *  N, Scope *  currentScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseSetCmpOp);


	IrNode *	n = genIrNode(N, 	kNoisyIrNodeType_PsetCmpOp,
						NULL /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tstrongdominates))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tstrongdominates));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tweakdominates))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tweakdominates));
	}
	else
	{
		
	}

	if (!inFollow(N, kNoisyIrNodeType_PsetCmpOp, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PsetCmpOp, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PsetCmpOp);
	}

	return n;
}



/*
 *	Simply remove a terminal
 */
IrNode *
noisyParseTerminal(State *  N, IrNodeType expectedType)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseTerminal);

	if (!peekCheck(N, 1, expectedType))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeTypeMax, expectedType, gNoisyFirsts);

		/*
		 *	In this case, we know the specific expected type/token.
		 */
		noisyParserErrorRecovery(N, expectedType);
	}

	Token *	t = lexGet(N, gNoisyTokenDescriptions);
	IrNode *	n = genIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

	n->token = t;

	/*
	 *	Checking the FOLLOW() set of terminals will catch
	 *	errors such as "iterate" that is followed by anything
	 *	other than a "{" (the only token in its follow set).
	 */
	if (!inFollow(N, expectedType, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, expectedType, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, expectedType);
	}

	return n;
}

/*
 *	Remove an identifier terminal, ignoring symtab lookup/addition.
 *	We currently _only_ use this for the function argument names.
 *	There is an outstanding TODO at the usage point to change this.
 */
IrNode *
noisyParseIdentifierTerminalIgnoreSymtab(State *  N)
{
//	TimeStampTraceMacro(kNoisyTimeStampKeyParseIdentifierTerminalIgnoreSymtab);

	if (!peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeTypeMax, kNoisyIrNodeType_Tidentifier, gNoisyFirsts);

		/*
		 *	In this case, we know the specific expected type/token.
		 */
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Tidentifier);
	}

	Token *	t = lexGet(N, gNoisyTokenDescriptions);
	IrNode *	n = genIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

	n->tokenString = t->identifier;

	if (!inFollow(N, kNoisyIrNodeType_Tidentifier, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Tidentifier, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Tidentifier);
	}

	return n;
}



/*
 *	Remove an identifier _usage_ terminal, performing symtab lookup
 */
IrNode *
noisyParseIdentifierUsageTerminal(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseIdentifierUsageTerminal);

	if (!peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeTypeMax, kNoisyIrNodeType_Tidentifier, gNoisyFirsts);

		/*
		 *	In this case, we know the specific expected type/token.
		 */
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Tidentifier);
	}

	Token *	t = lexGet(N, gNoisyTokenDescriptions);
	IrNode *	n = genIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

	n->tokenString = t->identifier;

	/*
	 *	commonSymbolTableSymbolForIdentifier() will first, try current scope
	 *	(and its parents), and if not found, will try module scopes.
	 */
	n->symbol = commonSymbolTableSymbolForIdentifier(N, scope, t->identifier);
	if (n->symbol == NULL)
	{
		errorUseBeforeDefinition(N, t->identifier);
		noisyParserSyntaxError(N, kNoisyIrNodeType_Tidentifier, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Tidentifier);
	}

	if (!inFollow(N, kNoisyIrNodeType_Tidentifier, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Tidentifier, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Tidentifier);
	}

	return n;
}

/*
 *	Remove an identifier _definition_ terminal, performing symtab insertion
 */
IrNode *
noisyParseIdentifierDefinitionTerminal(State *  N, Scope *  scope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParseIdentifierDefinitionTerminal);

	if (!peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeTypeMax, kNoisyIrNodeType_Tidentifier, gNoisyFirsts);

		/*
		 *	In this case, we know the specific expected type/token.
		 */
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Tidentifier);
	}

	Token *	t = lexGet(N, gNoisyTokenDescriptions);
	IrNode *	n = genIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

	n->tokenString = t->identifier;

	/*
	 *	NOTE: commonSymbolTableAddOrLookupSymbolForToken(N, scope, t) adds token 't' to scope 'scope'
	 */
	Symbol * searchSym = commonSymbolTableSymbolForIdentifier(N,scope,n->tokenString);

	if (searchSym != NULL && searchSym->scope == scope)
	{
		noisyParserSemanticError(N,kNoisyIrNodeType_Tidentifier,"Duplicate definiton of variable");
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Tidentifier);
	}

	Symbol *	sym = commonSymbolTableAddOrLookupSymbolForToken(N, scope, t);

	/*
	 *	We allow for definition of a function and probdef with the same name.
	 *	TODO: there are more rigorous ways to enforce the above. The check below
	 *	is to bootstrap examples.
	 */
	if ((sym->definition != NULL) && (peekCheck(N, 2, kNoisyIrNodeType_Tprobdef)))
	{
		errorMultiDefinition(N, sym);
		noisyParserSyntaxError(N, kNoisyIrNodeType_Tidentifier, kNoisyIrNodeTypeMax, gNoisyFollows);
	}
	n->symbol = sym;

	if (!inFollow(N, kNoisyIrNodeType_Tidentifier, gNoisyFollows, kNoisyIrNodeTypeMax))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Tidentifier, kNoisyIrNodeTypeMax, gNoisyFollows);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Tidentifier);
	}

	return n;
}



/*
 *	Exported non-parse routines
 */



void
noisyParserSyntaxAndSemanticPre(State *  N, IrNodeType currentlyParsingTokenOrProduction,
	const char *  string1, const char *  string2, const char *  string3, const char *  string4)
{
	flexprint(N->Fe, N->Fm, N->Fperr, "\n-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --\n");
	if (N->mode & kCommonModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "<b>");
	}

	if (N->mode & kCommonModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\t%s, line %d position %d, %s %s\"",
						string1,
						lexPeek(N, 1)->sourceInfo->lineNumber,
						lexPeek(N, 1)->sourceInfo->columnNumber,
						string4,
						kNoisyErrorTokenHtmlTagOpen);
		lexPrintToken(N, lexPeek(N, 1), gNoisyTokenDescriptions);
		flexprint(N->Fe, N->Fm, N->Fperr, "\"%s %s %s.<br><br>%s%s",
			kNoisyErrorTokenHtmlTagClose,
			string2,
			(currentlyParsingTokenOrProduction > kNoisyIrNodeType_TMax ?
				gNoisyProductionDescriptions[currentlyParsingTokenOrProduction] :
				gNoisyTokenDescriptions[currentlyParsingTokenOrProduction]),
			kNoisyErrorDetailHtmlTagOpen,
			string3);
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\t%s, %s line %d position %d, %s \"",
						string1,
						lexPeek(N, 1)->sourceInfo->fileName,
						lexPeek(N, 1)->sourceInfo->lineNumber,
						lexPeek(N, 1)->sourceInfo->columnNumber,
						string4);
		lexPrintToken(N, lexPeek(N, 1), gNoisyTokenDescriptions);
		flexprint(N->Fe, N->Fm, N->Fperr, "\" %s %s.\n\n\t%s",
			string2,
			(currentlyParsingTokenOrProduction > kNoisyIrNodeType_TMax ?
				gNoisyProductionDescriptions[currentlyParsingTokenOrProduction] :
				gNoisyTokenDescriptions[currentlyParsingTokenOrProduction]),
			string3);
	}	
}

void
noisyParserSyntaxAndSemanticPost(State *  N)
{
	if (N->mode & kCommonModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s</b>", kNoisyErrorDetailHtmlTagClose);
	}

	flexprint(N->Fe, N->Fm, N->Fperr, "\n-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --\n\n");	
}

void
noisyParserSyntaxError(State *  N, IrNodeType currentlyParsingTokenOrProduction, IrNodeType expectedProductionOrToken, int firstOrFollowsArray[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax])
{
	int	seen = 0;


	TimeStampTraceMacro(kNoisyTimeStampKeyParserSyntaxError);

	noisyParserSyntaxAndSemanticPre(N, currentlyParsingTokenOrProduction, EsyntaxA, EsyntaxB, EsyntaxC, EsyntaxD);
	if (((expectedProductionOrToken > kNoisyIrNodeType_TMax) && (expectedProductionOrToken < kNoisyIrNodeType_PMax)) || (expectedProductionOrToken == kNoisyIrNodeTypeMax))
	{
		flexprint(N->Fe, N->Fm, N->Fperr, " one of:\n\n\t\t");
		for (int i = 0; i < kNoisyIrNodeTypeMax && firstOrFollowsArray[currentlyParsingTokenOrProduction][i] != kNoisyIrNodeTypeMax; i++)
		{
			if (seen > 0)
			{
				flexprint(N->Fe, N->Fm, N->Fperr, ",\n\t\t");
			}

			flexprint(N->Fe, N->Fm, N->Fperr, "'%s'", (currentlyParsingTokenOrProduction > kNoisyIrNodeType_TMax ?
				gNoisyProductionDescriptions[firstOrFollowsArray[currentlyParsingTokenOrProduction][i]] :
				gNoisyTokenDescriptions[firstOrFollowsArray[currentlyParsingTokenOrProduction][i]]));
			seen++;
		}
	}
	else if ((currentlyParsingTokenOrProduction == kNoisyIrNodeTypeMax) && (expectedProductionOrToken < kNoisyIrNodeType_TMax))
	{
		flexprint(N->Fe, N->Fm, N->Fperr, ":\n\n\t\t");
		flexprint(N->Fe, N->Fm, N->Fperr, "%s", (expectedProductionOrToken > kNoisyIrNodeType_TMax ?
							gNoisyProductionDescriptions[expectedProductionOrToken] :
							gNoisyTokenDescriptions[expectedProductionOrToken]));
	}
	else
	{
		fatal(N, Esanity);
	}

	flexprint(N->Fe, N->Fm, N->Fperr, ".\n\n\tInstead, saw:\n\n");
	lexPeekPrint(N, 5, 0, gNoisyTokenDescriptions);

	noisyParserSyntaxAndSemanticPost(N);
}


void
noisyParserSemanticError(State *  N, IrNodeType currentlyParsingTokenOrProduction, char *  details)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParserSemanticError);
	noisyParserSyntaxAndSemanticPre(N, currentlyParsingTokenOrProduction, EsemanticsA, EsemanticsB, details, EsemanticsD);
	noisyParserSyntaxAndSemanticPost(N);
}


void
noisyParserErrorRecovery(State *  N, IrNodeType expectedProductionOrToken)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParserErrorRecovery);

	if (N->verbosityLevel & kCommonVerbosityDebugParser)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "In noisyParserErrorRecovery(), about to discard tokens...\n");
	}

	/*
	while (!inFollow(N, expectedProductionOrToken, gNoisyFollows, kNoisyIrNodeTypeMax) && N->tokenList != NULL)
	{
		 *
		 *	Retrieve token and discard...
		 *
		Token *	token = lexGet(N);
		if (N->verbosityLevel & kCommonVerbosityDebugParser)
		{
			lexDebugPrintToken(N, token);
		}
	}
	*/

	if ((N != NULL) && (N->jmpbufIsValid))
	{
		//fprintf(stderr, "doing longjmp");

		/*
		 *	See issue #291.
		 */
		longjmp(N->jmpbuf, 0);
	}

	/*
	 *	Not reached if N->jmpbufIsValid
	 */
	consolePrintBuffers(N);

	exit(EXIT_SUCCESS);
}
