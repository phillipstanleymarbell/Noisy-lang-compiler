/*
	Authored 2015. Phillip Stanley-Marbell.

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
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "version.h"
#include "common-timeStamps.h"
#include "data-structures.h"
#include "noisy-parser.h"
#include "common-lexers-helpers.h"
#include "noisy-lexer.h"
#include "noisy-symbolTable.h"
#include "common-irHelpers.h"
#include "noisy-irHelpers.h"
#include "common-firstAndFollow.h"


/*
 *
 *	TODO/BUG: We technically don't need the Xseq nodes at all, since we
 *		can figure out from context whether we are chaning a list
 *		of nodes at same level or not. Draw it out on paper, and
 *		you'll see. having the Xseq's however makes thinking about
 *		many things easier, as does visualizing, since we can easily
 *		pick out the cases where we are stringing together things
 *		at the same level, as opposed to having a hierarchy.
 *
 *	(1)	Need to flesh out the noisyParserSyntaxError/ErrorRecovery
 *		stuff. Added also a 'noisyParserSemanticError(N, )' for where
 *		we want to deliver information on a semantic error (e.g.,
 *		identifier use b/4 declaration). The 'noisyParserSemanticError'
 *		messages can't be implicit in the parse state (i.e., can't
 *		figure out error msg from current parse state and mlex), and
 *		they take an explicit message string.
 *
 *	(2)	assignTypes(N, IrNode *  n, IrNode *  typeExpression)
 *			the typeExpr might be, say, an identifier that is
 *			an alias for a type. We should check for this case
 *			and get the identifier's symbol->typeTree. Also, do
 *			sanity checks to validate the typeTree, e.g. make
 *			sure it is always made up of basic types and also
 *			that it's not NULL
 *
 *
 *	(3)	Urgent: we need to fogure out a strategy for recovering
 *			from syntax or semantic errors. We currently just
 *			exit. We can't simply ignore and continue either,
 *			since some errors lead to NULL structures (e.g.,
 *			ident not in symtab, so node->symbol field not
 *			set...
 *
 *
 */


extern char *		gProductionStrings[];
extern char *		gProductionDescriptions[];
extern const char *		gReservedTokenDescriptions[];
extern const char *		gTerminalStrings[];
extern char *		gAstNodeStrings[];
extern int		gNoisyFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];
extern int	    gNoisyFollows[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];

extern void		noisyFatal(State *  N, const char *  msg);
extern void		noisyError(State *  N, const char *  msg);




static char		kNoisyErrorTokenHtmlTagOpen[]	= "<span style=\"background-color:#FFCC00; color:#FF0000;\">";
static char		kNoisyErrorTokenHtmlTagClose[]	= "</span>";
static char		kNoisyErrorDetailHtmlTagOpen[]	= "<span style=\"background-color:#A9E9FF; color:#000000;\">";
static char		kNoisyErrorDetailHtmlTagClose[]	= "</span>";


IrNode *
noisyParse(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParse);

	return noisyParseProgram(N, currentScope);
}


/*
 *	kNoisyIrNodeType_Pprogram
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PprogtypeDeclaration
 *		node.right	= Xseq of kNoisyIrNodeType_PnamegenDefinition
 */
IrNode *
noisyParseProgram(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseProgram);


	IrNode *	n = genNoisyIrNode(N, 	kNoisyIrNodeType_Pprogram,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	/*
	 *	Before we start parsing, set begin source line of toplevel scope.
	 */
	currentScope->begin = noisyLexPeek(N, 1)->sourceInfo;

	addLeaf(N, n, noisyParseProgtypeDeclaration(N, currentScope));
	while (!noisyInFollow(N, kNoisyIrNodeType_Pprogram, gNoisyFollows))
	{
		addLeafWithChainingSeq(N, n, noisyParseNamegenDefinition(N, currentScope));
	}

	/*
	 *	We can now fill in end src info for toplevel scope.
	 */
	currentScope->end = noisyLexPeek(N, 1)->sourceInfo;


	return n;
}



/*
 *	kNoisyIrNodeType_PprogtypeDeclaration
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= kNoisyIrNodeType_PprogtypeBody
 */
IrNode *
noisyParseProgtypeDeclaration(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseProgtypeDeclaration);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PprogtypeDeclaration,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	IrNode *	identifier = noisyParseIdentifierDefinitionTerminal(N, kNoisyIrNodeType_Tidentifier, scope);
	addLeaf(N, n, identifier);
	noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
	noisyParseTerminal(N, kNoisyIrNodeType_Tprogtype);

	/*
	 *	We keep a global handle on the progtype scope
	 */
	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	Scope *	progtypeScope	= noisySymbolTableOpenScope(N, scope, scopeBegin);
	IrNode *	typeTree	= noisyParseProgtypeBody(N, progtypeScope);
	addLeaf(N, n, typeTree);
	IrNode *	scopeEnd	= noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);
	noisySymbolTableCloseScope(N, progtypeScope, scopeEnd);
	identifier->symbol->typeTree = typeTree;

	addToProgtypeScopes(N, identifier->symbol->identifier, progtypeScope);


	return n;
}



/*	
 *	kNoisyIrNodeType_PprogtypeBody
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PprogtypeTypenameDecl
 *		node.right	= Xseq of kNoisyIrNodeType_PprogtypeTypenameDecl
 */
IrNode *
noisyParseProgtypeBody(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseProgtypeBody);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PprogtypeBody,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	while (!noisyInFollow(N, kNoisyIrNodeType_PprogtypeBody, gNoisyFollows))
	{
		addLeafWithChainingSeq(N, n, noisyParseProgtypeTypenameDeclaration(N, scope));
		noisyParseTerminal(N, kNoisyIrNodeType_Tsemicolon);
	}


	return n;
}

/*
 *	kNoisyIrNodeType_PprogtypeTypenameDeclaration
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PidentifierList
 *		node.right	= kNoisyIrNodeType_PconstantDeclaration | kNoisyIrNodeType_PtypeDeclaration | kNoisyIrNodeType_PnamegenDeclaration
 */
IrNode *
noisyParseProgtypeTypenameDeclaration(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseProgtypeTypenameDeclaration);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PprogtypeTypenameDeclaration,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	IrNode *	identifierList = noisyParseIdentifierList(N, scope);
	addLeaf(N, n, identifierList);
	noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);

	IrNode *	typeExpression;
	if (noisyInFirst(N, kNoisyIrNodeType_PconstantDeclaration, gNoisyFirsts))
	{
		typeExpression = noisyParseConstantDeclaration(N, scope);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PtypeDeclaration, gNoisyFirsts))
	{
		typeExpression = noisyParseTypeDeclaration(N, scope);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PnamegenDeclaration, gNoisyFirsts))
	{
		typeExpression = noisyParseNamegenDeclaration(N, scope);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PprogtypeTypenameDeclaration, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PprogtypeTypenameDeclaration);
	}
	addLeaf(N, n, typeExpression);

	/*
	 *	We've now figured out type ascribed to members of identifierList
	 */
	assignTypes(N, identifierList, typeExpression);


	return n;
}


/*
 *	kNoisyIrNodeType_PconstantDeclaration
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_TintConst | kNoisyIrNodeType_TrealConst | kNoisyIrNodeType_TboolConst
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseConstantDeclaration(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseConstantDeclaration);


	IrNode *	n;

	noisyParseTerminal(N, kNoisyIrNodeType_Tconst);
	if (peekCheck(N, 1, kNoisyIrNodeType_TintConst))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TintConst);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TrealConst))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TrealConst);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TboolConst))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TboolConst);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PconstantDeclaration, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PconstantDeclaration);
	}


	return n;
}

/*
 *	kNoisyIrNodeType_PtypeDeclaration
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Ttype | kNoisyIrNodeType_PadtTypeDeclaration
 *		node.right	= kNoisyIrNodeType_PtypeExpression | NULL
 */
IrNode *
noisyParseTypeDeclaration(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseTypeDeclaration);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PtypeDeclaration,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Ttype))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Ttype));
		addLeaf(N, n, noisyParseTypeExpression(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tadt))
	{
		addLeaf(N, n, noisyParseAdtTypeDeclaration(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeDeclaration, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeDeclaration);
	}


	return n;
}

/*
 *	kNoisyIrNodeType_PadtTypeDeclaration
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PidentifierList
 *		node.right	= Xseq of texpr + zero or more kNoisyIrNodeType_PidentifierList+texpr
 */
IrNode *
noisyParseAdtTypeDeclaration(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseAdtTypeDeclaration);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PadtTypeDeclaration,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tadt);
	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	Scope *	currentScope	= noisySymbolTableOpenScope(N, scope, scopeBegin);
	IrNode *	identifierList	= noisyParseIdentifierList(N, currentScope);
	addLeaf(N, n, identifierList);
	noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
	IrNode *	typeExpression	= noisyParseTypeExpression(N, currentScope);
	addLeafWithChainingSeq(N, n, typeExpression);
	assignTypes(N, identifierList, typeExpression);
	noisyParseTerminal(N, kNoisyIrNodeType_Tsemicolon);
	while (!peekCheck(N, 1, kNoisyIrNodeType_TrightBrace))
	{
		IrNode *	identifierList2	= noisyParseIdentifierList(N, currentScope);

		addLeafWithChainingSeq(N, n, identifierList2);
		noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
		IrNode *	typeExpression2 = noisyParseTypeExpression(N, currentScope);
		addLeafWithChainingSeq(N, n, typeExpression2);
		assignTypes(N, identifierList2, typeExpression2);
		noisyParseTerminal(N, kNoisyIrNodeType_Tsemicolon);
	}

	IrNode *	scopeEnd  = noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);
	noisySymbolTableCloseScope(N, currentScope, scopeEnd);


	return n;
}

/*
 *	kNoisyIrNodeType_PvectorType
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_TintConst
 *		node.right	= kNoisyIrNodeType_PtypeExpression
 */
IrNode *
noisyParseVectorType(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseVectorType);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PvectorType,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tvector);

	noisyParseTerminal(N, kNoisyIrNodeType_TleftBrac);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBrac);

	noisyParseTerminal(N, kNoisyIrNodeType_TleftBrac);
	addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBrac);
	
	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeafWithChainingSeq(N, n, noisyParseTypeExpression(N, currentScope));

	return n;
}

/*
 *	kNoisyIrNodeType_PnamegenDeclaration
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PtupleType
 *		node.right	= kNoisyIrNodeType_PtupleType
 */
IrNode *
noisyParseNamegenDeclaration(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseNamegenDeclaration);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PnamegenDeclaration,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tnamegen);
	addLeaf(N, n, noisyParseTupleType(N, scope));
	noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
	addLeaf(N, n, noisyParseTupleType(N, scope));


	return n;
}

/*
 *	kNoisyIrNodeType_PidentifierOrNil
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= Xseq of kNoisyIrNodeType_PfieldSelect
 */
IrNode *
noisyParseIdentifierOrNil(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseIdentifierOrNil);

	IrNode *	n;

	if (peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		/*
		 *	The typeTree's get filled-in in our caller
		 */
		n = noisyParseIdentifierDefinitionTerminal(N, kNoisyIrNodeType_Tidentifier, currentScope);

		while (noisyInFirst(N, kNoisyIrNodeType_PfieldSelect, gNoisyFirsts))
		{
			addLeafWithChainingSeq(N, n, noisyParseFieldSelect(N, currentScope));
		}
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tnil))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tnil);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PidentifierOrNil, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PidentifierOrNil);
	}

	return n;
}


/*
 *	kNoisyIrNodeType_PidentifierOrNilList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PidentifierOrNil
 *		node.right	= Xseq of kNoisyIrNodeType_PidentifierOrNil
 */
IrNode *
noisyParseIdentifierOrNilList(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseIdentifierOrNilList);


	IrNode *	n;


	/*
	 *	The typeTree's get filled in in our caller
	 */
	n = noisyParseIdentifierOrNil(N, currentScope);
	
	/*
	 *	Could also have done
	 *		while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	 */
	while (!noisyInFollow(N, kNoisyIrNodeType_PidentifierOrNilList, gNoisyFollows))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
		addLeafWithChainingSeq(N, n, noisyParseIdentifierOrNil(N, currentScope));
	}


	return n;
}

/*
 *	kNoisyIrNodeType_PidentifierList
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Tidentifier
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= Xseq of kNoisyIrNodeType_Tidentifier
 */
IrNode *
noisyParseIdentifierList(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseIdentifierList);


	IrNode *	n;


	/*
	 *	The typeTree's get filled in in our caller
	 */
	n = noisyParseIdentifierDefinitionTerminal(N, kNoisyIrNodeType_Tidentifier, currentScope);
	
	/*
	 *	Could also have done
	 *		while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	 */
	while (!noisyInFollow(N, kNoisyIrNodeType_PidentifierList, gNoisyFollows))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
		addLeafWithChainingSeq(N, n, noisyParseIdentifierDefinitionTerminal(N, kNoisyIrNodeType_Tidentifier, currentScope));
	}


	return n;
}

/*
 *	kNoisyIrNodeType_PtypeExpression
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PbasicType | kNoisyIrNodeType_PanonAggregateType | kNoisyIrNodeType_PtypeName
 *		node.left	= kNoisyIrNodeType_Ptolerance | NULL
 *		node.right	= Xseq of kNoisyIrNodeType_Ptolerance | NULL
 */
IrNode *
noisyParseTypeExpression(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseTypeExpression);


	IrNode *	n;


	if (noisyInFirst(N, kNoisyIrNodeType_PbasicType, gNoisyFirsts))
	{
		n = noisyParseBasicType(N, currentScope);

		if (noisyInFollow(N, kNoisyIrNodeType_PtypeExpression, gNoisyFollows))
		{
			return n;
		}

		addLeaf(N, n, noisyParseTolerance(N, currentScope));

		/*
		 *	Could also have done
		 *		while (!noisyInFollow(N, kNoisyIrNodeType_PtypeExpression, gNoisyFollows))
		 */
		while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
		{
			noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
			addLeafWithChainingSeq(N, n, noisyParseTolerance(N, currentScope));
		}
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PanonAggregateType, gNoisyFirsts))
	{
		n = noisyParseAnonAggregateType(N, currentScope);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_Ptypename, gNoisyFirsts))
	{
		n = noisyParseTypeName(N, currentScope);
	}
	else
	{
        /*
         * Here, we compile the "Physics type"
         * TODO fix this. N needs to have NC stuff in it
         */
        // Token * nextToken = noisyLexPeek(N, 1);
        // Physics * physics = noisyConfigPhysicsTablePhysicsForIdentifier(N, N->noisyConfigIrTopScope, nextToken->identifier);
        // 
        // if (physics!= NULL)
        // {
        //     n->physics = physics;
        // }
        // else 
        // {
		//     noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeExpression, kNoisyIrNodeTypeMax);
		//     noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeExpression);
        // }
		noisyParserSyntaxError(N, kNoisyIrNodeType_PtypeExpression, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PtypeExpression);
	}


	return n;
}

/*
 *	kNoisyIrNodeType_PtypeName
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pidentifier
 *		node.right	= NULL | Xseq of kNoisyIrNodeType_TprogtypeQualifier, kNoisyIrNodeType_Tidentifier
 */
IrNode *
noisyParseTypeName(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseTypeName);


	IrNode *	id1;
	IrNode *	id2;
	Symbol *	idsym;
	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_Ptypename,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	/*
	 *	This is a use, not a def, so doesn't go into symtab (noisyParseIdentifierDefinitionTerminal);
	 *	instead, noisyParseIdentifierUsageTerminal looks up its symbol in symtab.
	 */
	id1 = noisyParseIdentifierUsageTerminal(N, kNoisyIrNodeType_Tidentifier, scope);
	if (id1->symbol == NULL)
	{
		/*
		 *	TODO: we need a comprehensive error-recovery strategy
		 */
		noisyFatal(N, Esanity);
	}

	addLeaf(N, n, id1);

	/*
	 *	Since it's a use, it should already be in symtab. If it is a progtype
	 *	qualified name, we look in the progtype's scope, otherwise we look
	 *	in the current scope.
	 */
	if (peekCheck(N, 1, kNoisyIrNodeType_TprogtypeQualifier))
	{
		/*
		 *	Note: kNoisyIrNodeType_TprogtypeQualifier is the "->". We do not need to
		 *	put it in AST, since, from parent node type = kNoisyIrNodeType_PtypeName,
		 *	we know that if we have > 1 child, then it is a ptype-qualified
		 *	name. It is not a chain of Xseqs, because we cannnot have 
		 *	progtype embedded in progtype, so we at most have "a->b"
		 */
		noisyParseTerminal(N, kNoisyIrNodeType_TprogtypeQualifier);
		id2 = noisyParseIdentifierUsageTerminal(N, kNoisyIrNodeType_Tidentifier, progtypeName2scope(N, id1->symbol->identifier));
		if (id2->symbol == NULL)
		{
			noisyParserSemanticError(N, "%s'%s%s%s'\n", Eundeclared, id1->symbol->identifier, "->", "noisyParseTypeName: semantic Error" /*id2->symbol->identifier*/);
		}
		idsym = id2->symbol;

		addLeaf(N, n, id2);
	}
	else
	{
		idsym = noisySymbolTableSymbolForIdentifier(N, scope, id1->symbol->identifier);
		if (idsym == NULL)
		{
			noisyParserSemanticError(N, "%s%s\n", Eundeclared, id1->symbol->identifier);
		}
	}

	/*
	 *	Set the symbol table entry for this to be that of the type or
	 *	prog. qualified type, so that, among other things, it gets the
	 *	type of the kNoisyIrNodeType_PtypeName treenode set to the discerned type.
	 */
	n->symbol = idsym;


	return n;
}

/*
 *	kNoisyIrNodeType_Ptolerance
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PERRORMAGTOL | kNoisyIrNodeType_PLOSSTOL | kNoisyIrNodeType_PLATENCYTOL
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseTolerance(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseTolerance);


	IrNode *	n;


	if (noisyInFirst(N, kNoisyIrNodeType_PerrorMagnitudeTolerance, gNoisyFirsts))
	{
		n = noisyParseErrorMagnitudeTolerance(N);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PlossTolerance, gNoisyFirsts))
	{
		n = noisyParseLossTolerance(N);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PlatencyTolerance, gNoisyFirsts))
	{
		n = noisyParseLatencyTolerance(N);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Ptolerance, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Ptolerance);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_PerrorMagnitudeTolerance
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_TrealConst
 *		node.right	= kNoisyIrNodeType_TrealConst
 */
IrNode *
noisyParseErrorMagnitudeTolerance(State *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseErrorMagnitudeTolerance);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PerrorMagnitudeTolerance,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tepsilon);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParen);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParen);

	return n;
}

/*
 *	kNoisyIrNodeType_PlossTolerance
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_TrealConst
 *		node.right	= kNoisyIrNodeType_TrealConst
 */
IrNode *
noisyParseLossTolerance(State *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseLossTolerance);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PerrorMagnitudeTolerance,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Talpha);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParen);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParen);

	return n;
}

/*
 *	kNoisyIrNodeType_PlatencyTolerance
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_TrealConst
 *		node.right	= kNoisyIrNodeType_TrealConst
 */
IrNode *
noisyParseLatencyTolerance(State *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseLatencyTolerance);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PlatencyTolerance,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Ttau);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParen);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParen);

	return n;
}

/*
 *	kNoisyIrNodeType_PbasicType
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Tbool | ... | kNoisyIrNodeType_Tstring
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseBasicType(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseBasicType);


	IrNode *	n;


	if (peekCheck(N, 1, kNoisyIrNodeType_Tbool))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tbool);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tnybble))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tnybble);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tbyte))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tbyte);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tint))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tint);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PrealType, gNoisyFirsts))
	{
		n = noisyParseRealType(N, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tstring))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tstring);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PbasicType, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PbasicType);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_PrealType
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Treal | kNoisyIrNodeType_PfixedType
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseRealType(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseRealType);


	IrNode *	n;


	if (peekCheck(N, 1, kNoisyIrNodeType_Treal))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Treal);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PfixedType, gNoisyFirsts))
	{
		n = noisyParseFixedType(N);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PrealType, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PrealType);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_PfixedType
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_TintConst
 *		node.right	= kNoisyIrNodeType_TintConst
 */
IrNode *
noisyParseFixedType(State *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseFixedType);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PfixedType,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tfixed);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintConst));
	noisyParseTerminal(N, kNoisyIrNodeType_Tdot);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintConst));
	
	return n;
}

/*
 *	kNoisyIrNodeType_PanonAggregateType
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_ParrayType | ... | kNoisyIrNodeType_PsetType
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseAnonAggregateType(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseAnonAggregateType);


	IrNode *	n;


	if (noisyInFirst(N, kNoisyIrNodeType_ParrayType, gNoisyFirsts))
	{
		n = noisyParseArrayType(N, currentScope);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PlistType, gNoisyFirsts))
	{
		n = noisyParseListType(N, currentScope);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PtupleType, gNoisyFirsts))
	{
		n = noisyParseTupleType(N, currentScope);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PsetType, gNoisyFirsts))
	{
		n = noisyParseSetType(N, currentScope);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PanonAggregateType, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PanonAggregateType);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_ParrayType
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_TintConst
 *		node.right	= Xseq of zero or more kNoisyIrNodeType_TintConst then a kNoisyIrNodeType_PtypeExpression
 */
IrNode *
noisyParseArrayType(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseArrayType);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_ParrayType,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tarray);

	noisyParseTerminal(N, kNoisyIrNodeType_TleftBrac);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBrac);

	while (peekCheck(N, 1, kNoisyIrNodeType_TleftBrac))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_TleftBrac);
		addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintConst));
		noisyParseTerminal(N, kNoisyIrNodeType_TrightBrac);
	}
	
	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeafWithChainingSeq(N, n, noisyParseTypeExpression(N, currentScope));

	return n;
}

/*
 *	kNoisyIrNodeType_PlistType
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PtypeExpression
 *		node.right	= NULL
 */
IrNode *
noisyParseListType(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseListType);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PlistType,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tlist);
	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeaf(N, n, noisyParseTypeExpression(N, currentScope));

	return n;
}

/*
 *	kNoisyIrNodeType_PtupleType
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PtypeExpression
 *		node.right	= Xseq of kNoisyIrNodeType_PtypeExpression
 */
IrNode *
noisyParseTupleType(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseTupleType);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PtupleType,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftParen);
	addLeaf(N, n, noisyParseTypeExpression(N, currentScope));
	while(peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
		addLeafWithChainingSeq(N, n, noisyParseTypeExpression(N, currentScope));
	}
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParen);

	return n;
}

/*
 *	kNoisyIrNodeType_PsetType
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_TintConst
 *		node.right	= kNoisyIrNodeType_PtypeExpression
 */
IrNode *
noisyParseSetType(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseSetType);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PsetType,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tset);
	noisyParseTerminal(N, kNoisyIrNodeType_TleftBrac);
	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintConst));
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBrac);
	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeaf(N, n, noisyParseTypeExpression(N, currentScope));

	return n;
}

/*
 *	kNoisyIrNodeType_PinitList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pexpression
 *		node.right	= Xseq of kNoisyIrNodeType_Pexpression
 */
IrNode *
noisyParseInitList(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseInitList);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PinitList,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	addLeaf(N, n, noisyParseExpression(N, scope));
	while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
		addLeafWithChainingSeq(N, n, noisyParseExpression(N, scope));
	}
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);

	return n;
}

/*
 *	kNoisyIrNodeType_PidxInitList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pelement
 *		node.right	= Xseq of kNoisyIrNodeType_Pelement
 */
IrNode *
noisyParseIdxInitList(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseIdxInitList);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PinitList,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	addLeaf(N, n, noisyParseElement(N, scope));
	while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);
		addLeafWithChainingSeq(N, n, noisyParseElement(N, scope));
	}
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);

	return n;
}

/*
 *	kNoisyIrNodeType_PstarInitList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pelement
 *		node.right	= Xseq of kNoisyIrNodeType_Pelement, zero or more "*"+kNoisyIrNodeType_Pelement
 */
IrNode *
noisyParseStarInitList(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseStarInitList);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PinitList,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	addLeaf(N, n, noisyParseElement(N, scope));
	while (peekCheck(N, 1, kNoisyIrNodeType_Tcomma))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcomma);

		/*
		 *	BUG?: verify w/ grammar (fixed from a yuck version)
		 */
		if (noisyInFirst(N, kNoisyIrNodeType_Pelement, gNoisyFirsts))
		{
			addLeafWithChainingSeq(N, n, noisyParseElement(N, scope));
		}
		else if (peekCheck(N, 1, kNoisyIrNodeType_Tasterisk))
		{
			/*
			 *	Need to put this in AST to differentiate from default form
			 */
			addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tasterisk));
			noisyParseTerminal(N, kNoisyIrNodeType_Tgoes);
			addLeafWithChainingSeq(N, n, noisyParseElement(N, scope));
		}
		else
		{
			noisyFatal(N, EelementOrStar);
		}
	}
	noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);

	return n;
}

/*
 *	kNoisyIrNodeType_Pelement
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pexpression
 *		node.right	= NULL | kNoisyIrNodeType_Pexpression
 */
IrNode *
noisyParseElement(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseElement);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_Pelement,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseExpression(N, scope));
	if (peekCheck(N, 1, kNoisyIrNodeType_Tgoes))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tgoes);
		addLeaf(N, n, noisyParseExpression(N, scope));
	}

	return n;
}

/*
 *	kNoisyIrNodeType_PnamegenDefinition
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= [Xseq of 2 tupletypes] kNoisyIrNodeType_PscopeStatementList
 */
IrNode *
noisyParseNamegenDefinition(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseNamegenDefinition);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PnamegenDefinition,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	IrNode *	identifier = noisyParseIdentifierDefinitionTerminal(N, kNoisyIrNodeType_Tidentifier, scope);
	addLeaf(N, n, identifier);

	if (peekCheck(N, 1, kNoisyIrNodeType_Tcolon))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);

		IrNode *	t1 = noisyParseTupleType(N, scope);
		addLeafWithChainingSeq(N, n, t1);
		noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
		IrNode *	t2 = noisyParseTupleType(N, scope);
		addLeafWithChainingSeq(N, n, t2);

		/*
		 *	We need a valid subtree representing the type.
		 *	Need to be careful about not clobbering the
		 *	main AST with new links, so we make copies.
		 *
		 *	TODO/BUG: is typeTree now really a copy or a reference ?
		 */
		IrNode *	typeTree = t1;
		addLeaf(N, typeTree, t2);
		identifier->symbol->typeTree = typeTree;
	}
	else
	{
		/*
		 *	Lookup the type of the ident from the progtype
		 *	decl which this file implements (N->progtypeOfFile).
		 *
		 *	TODO/BUG: We currently never set N->progtypeOfFile
		 */
		Symbol *	sym = noisySymbolTableSymbolForIdentifier(N, progtypeName2scope(N, N->progtypeOfFile), identifier->symbol->identifier);

		if (sym == NULL)
		{
			noisyParserSemanticError(N, "%s%s\n", Eundeclared, identifier->symbol->identifier);
		}
		else
		{
			identifier->symbol->typeTree = sym->typeTree;
		}
	}

	noisyParseTerminal(N, kNoisyIrNodeType_Tas);
	addLeaf(N, n, noisyParseScopedStatementList(N, scope));

	return n;
}

/*
 *	kNoisyIrNodeType_PscopedStatmentList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PstatementList
 *		node.right	= NULL
 */
IrNode *
noisyParseScopedStatementList(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseScopedStatementList);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PscopedStatementList,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	Scope *	currentScope	= noisySymbolTableOpenScope(N, scope, scopeBegin);
	addLeaf(N, n, noisyParseStatementList(N, currentScope));
	IrNode *	scopeEnd  	= noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);
	noisySymbolTableCloseScope(N, currentScope, scopeEnd);

	return n;
}

/*
 *	kNoisyIrNodeType_PstatementList
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pstatement or NULL
 *		node.right	= Xseq of kNoisyIrNodeType_Pstatement
 */
IrNode *
noisyParseStatementList(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseStatementList);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PstatementList,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	/*
	 *	Could also have done
	 *		while (!noisyInFollow(N, kNoisyIrNodeType_PstatementList, gNoisyFollows))
	 */
	while (noisyInFirst(N, kNoisyIrNodeType_Pstatement, gNoisyFirsts))
	{
		addLeafWithChainingSeq(N, n, noisyParseStatement(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_Tsemicolon);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_Pstatement
 *
 *	Generated AST subtree:
 *
 *		node.left	= NULL | kNoisyIrNodeType_PidentifierOrNilList
 *		node.right	= NULL | kNoisyIrNodeType_PconstantDeclaration | .. | kNoisyIrNodeType_Pexpression
 */
IrNode *
noisyParseStatement(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseStatement);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_Pstatement,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Tsemicolon))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tsemicolon);

		return n;
	}

	if (noisyInFirst(N, kNoisyIrNodeType_PidentifierOrNilList, gNoisyFirsts))
	{
		IrNode *	identifierList = noisyParseIdentifierOrNilList(N, currentScope);
		addLeaf(N, n, identifierList);

		if (peekCheck(N, 1, kNoisyIrNodeType_Tcolon))
		{
			IrNode *	typeExpr;

			noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
			if (noisyInFirst(N, kNoisyIrNodeType_PconstantDeclaration, gNoisyFirsts))
			{
				typeExpr = noisyParseConstantDeclaration(N, currentScope);
				addLeaf(N, n, typeExpr);
			}
			else if (noisyInFirst(N, kNoisyIrNodeType_PtypeDeclaration, gNoisyFirsts))
			{
				typeExpr = noisyParseTypeDeclaration(N, currentScope);
				addLeaf(N, n, typeExpr);
			}
			else if (noisyInFirst(N, kNoisyIrNodeType_PtypeExpression, gNoisyFirsts))
			{
				typeExpr = noisyParseTypeExpression(N, currentScope);
				addLeaf(N, n, typeExpr);
			}
			else
			{
				noisyParserSyntaxError(N, kNoisyIrNodeType_Pstatement, kNoisyIrNodeTypeMax);
				noisyParserErrorRecovery(N, kNoisyIrNodeType_Pstatement);
			}

			/*
			 *	assignTypes(N, ) handles expansion of typeTree, e.g., for kNoisyIrNodeType_PtypeName
			 */
			assignTypes(N, identifierList, typeExpr);
		}
		else if (noisyInFirst(N, kNoisyIrNodeType_PassignOp, gNoisyFirsts))
		{
			addLeafWithChainingSeq(N, n, noisyParseAssignOp(N));
			addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));
		}
		else
		{
			noisyParserSyntaxError(N, kNoisyIrNodeType_Pstatement, kNoisyIrNodeTypeMax);
			noisyParserErrorRecovery(N, kNoisyIrNodeType_Pstatement);
		}
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftParen))
	{
		/*
		 *	We add one L_PAREN to AST as a marker to differentiate
		 *	this case from that of regular assignment to identornil
		 *	Only need to add one, just as a marker.
		 */
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TleftParen));
		addLeafWithChainingSeq(N, n, noisyParseIdentifierOrNilList(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_TrightParen);

		addLeafWithChainingSeq(N, n, noisyParseAssignOp(N));
		addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PmatchStatement, gNoisyFirsts))
	{
		addLeaf(N, n, noisyParseMatchStatement(N, currentScope));
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PiterationStatement, gNoisyFirsts))
	{
		addLeaf(N, n, noisyParseIterStatement(N, currentScope));
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PscopedStatementList, gNoisyFirsts))
	{
		addLeaf(N, n, noisyParseScopedStatementList(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Pstatement, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Pstatement);
	}

	return n;
}


/*
 *	kNoisyIrNodeType_PassignOp
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Tas | ... | kNoisyIrNodeType_TdefineAs
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseAssignOp(State *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseAssignOp);


	IrNode *	n;


	if (peekCheck(N, 1, kNoisyIrNodeType_Tas))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tas);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TxorAs))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TxorAs);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TorAs))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TorAs);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TandAs))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TandAs);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TmodAs))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TmodAs);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TdivAs))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TdivAs);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TmulAs))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TmulAs);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TsubAs))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TsubAs);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TaddAs))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TaddAs);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TrightShiftAs))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TrightShiftAs);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftShiftAs))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TleftShiftAs);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TchanWrite))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TchanWrite);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TdefineAs))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TdefineAs);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PassignOp, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PassignOp);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_PmatchStatement
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Tmatch | kNoisyIrNodeType_TmatchSeq
 *		node.left	= kNoisyIrNodeType_PguardBody
 *		node.right	= NULL
 */
IrNode *
noisyParseMatchStatement(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseMatchStatement);


	IrNode *	n;


	if (peekCheck(N, 1, kNoisyIrNodeType_Tmatch))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tmatch);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TmatchSeq))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TmatchSeq);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PmatchStatement, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PmatchStatement);
	}

	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	Scope *	currentScope	= noisySymbolTableOpenScope(N, scope, scopeBegin);
	addLeaf(N, n, noisyParseGuardBody(N, currentScope));
	IrNode *	scopeEnd	= noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);
	noisySymbolTableCloseScope(N, currentScope, scopeEnd);

	return n;
}

/*
 *	kNoisyIrNodeType_PiterationStatement
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Titer
 *		node.right	= kNoisyIrNodeType_PguardBody
 */
IrNode *
noisyParseIterStatement(State *  N, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseIterStatement);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PiterationStatement,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Titer));
	IrNode *	scopeBegin	= noisyParseTerminal(N, kNoisyIrNodeType_TleftBrace);
	Scope *	currentScope	= noisySymbolTableOpenScope(N, scope, scopeBegin);
	addLeaf(N, n, noisyParseGuardBody(N, currentScope));
	IrNode *	scopeEnd	= noisyParseTerminal(N, kNoisyIrNodeType_TrightBrace);
	noisySymbolTableCloseScope(N, currentScope, scopeEnd);

	return n;
}

/*
 *	kNoisyIrNodeType_PguardBody
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pexpression
 *		node.right	= Xseq of kNoisyIrNodeType_Pstatement and kNoisyIrNodeType_Pexpression alternating
 */
IrNode *
noisyParseGuardBody(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseGuardBody);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PguardBody,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	if (noisyInFollow(N, kNoisyIrNodeType_PguardBody, gNoisyFollows))
	{
		if (N->verbosityLevel & kNoisyVerbosityDebugParser)
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "In noisyParseGuardBody(), known bug (noisyInFollow(N, kNoisyIrNodeType_PguardBody, gNoisyFollows))\n");
		}

		/*
		 *	BUG/TODO: we should not be returning NULL: return the childless Node n.
		 *
		 *	Leave this comment here for later cleanup
		 */
		return NULL;
	}
//fprintf(stderr, "In noisyParseGuardBody(), about to loop through parsing the expression + =>..., Source file line %llu\n", noisyLexPeek(N, 1)->sourceInfo->lineNumber);
	while (noisyInFirst(N, kNoisyIrNodeType_Pexpression, gNoisyFirsts))
	{
		addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));
		noisyParseTerminal(N, kNoisyIrNodeType_Tgoes);
		if (peekCheck(N, 1, kNoisyIrNodeType_TleftBrace))
		{
			addLeafWithChainingSeq(N, n, noisyParseScopedStatementList(N, currentScope));
		}
		else
		{
			addLeafWithChainingSeq(N, n, noisyParseStatementList(N, currentScope));
		}
	}

	return n;
}

/*
 *	kNoisyIrNodeType_Pexpression
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Pterm | kNoisyIrNodeType_PanonAggregateCastExpression | ... | kNoisyIrNodeType_Pname2chanExpression
 *		node.left	= kNoisyIrNodeType_PlowPrecedenceBinaryOp
 *		node.right	= Xseq of kNoisyIrNodeType_PlowPrecedenceBinaryOp, kNoisyIrNodeType_Pterm
 */
IrNode *
noisyParseExpression(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseExpression);


	IrNode *	n;

//fprintf(stderr, "In noisyParseExpression()... Source file line %llu\n", noisyLexPeek(N, 1)->sourceInfo->lineNumber);
//noisyLexPeekPrint(N, 5, 0);

	if (noisyInFirst(N, kNoisyIrNodeType_Pterm, gNoisyFirsts))
	{
		n = noisyParseTerm(N, currentScope);

		while (noisyInFirst(N, kNoisyIrNodeType_PlowPrecedenceBinaryOp, gNoisyFirsts))
		{
			addLeafWithChainingSeq(N, n, noisyParseLowPrecedenceBinaryOp(N));
			addLeafWithChainingSeq(N, n, noisyParseTerm(N, currentScope));
		}
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PanonAggregateCastExpression, gNoisyFirsts))
	{
		n = noisyParseAnonAggregateCastExpression(N, currentScope);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PchanEventExpression, gNoisyFirsts))
	{
		n = noisyParseChanEventExpression(N, currentScope);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_Pchan2nameExpression, gNoisyFirsts))
	{
		n = noisyParseChan2nameExpression(N, currentScope);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_Pvar2nameExpression, gNoisyFirsts))
	{
		n = noisyParseVar2nameExpression(N, currentScope);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_Pname2chanExpression, gNoisyFirsts))
	{
		n = noisyParseName2chanExpression(N, currentScope);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Pexpression, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Pexpression);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_PlistCastExpression
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PinitList
 *		node.right	= NULL
 */
IrNode *
noisyParseListCastExpression(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseListCastExpression);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PlistCastExpression,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tlist);
	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeaf(N, n, noisyParseInitList(N, currentScope));

	return n;
}

/*
 *	kNoisyIrNodeType_PsetCastExpression
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PinitList
 *		node.right	= NULL
 */
IrNode *
noisyParseSetCastExpression(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseSetCastExpression);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PsetCastExpression,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tset);
	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeaf(N, n, noisyParseInitList(N, currentScope));

	return n;
}

/*
 *	kNoisyIrNodeType_ParrayCastExpression
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PidxInitList | kNoisyIrNodeType_TintConst
 *		node.right	= NULL | kNoisyIrNodeType_PstarInitList
 */
IrNode *
noisyParseArrayCastExpression(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseArrayCastExpression);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_ParrayCastExpression,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tarray);
	
	if (peekCheck(N, 1, kNoisyIrNodeType_Tof))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_Tof);
		addLeaf(N, n, noisyParseIdxInitList(N, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftBrac))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_TleftBrac);
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TintConst));
		noisyParseTerminal(N, kNoisyIrNodeType_TrightBrac);
		noisyParseTerminal(N, kNoisyIrNodeType_Tof);
		addLeaf(N, n, noisyParseStarInitList(N, currentScope));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_ParrayCastExpression, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_ParrayCastExpression);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_PanonAggregateCastExpression
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_PlistCast | kNoisyIrNodeType_PsetCast | kNoisyIrNodeType_ParrayCast
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseAnonAggregateCastExpression(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseAnonAggregateCastExpression);


	IrNode *	n;


	if (noisyInFirst(N, kNoisyIrNodeType_PlistCastExpression, gNoisyFirsts))
	{
		n = noisyParseListCastExpression(N, currentScope);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_PsetCastExpression, gNoisyFirsts))
	{
		n = noisyParseSetCastExpression(N, currentScope);
	}
	else if (noisyInFirst(N, kNoisyIrNodeType_ParrayCastExpression, gNoisyFirsts))
	{
		n = noisyParseArrayCastExpression(N, currentScope);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PanonAggregateCastExpression, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PanonAggregateCastExpression);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_PchanEventExpression
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Terasures | kNoisyIrNodeType_Terrors | kNoisyIrNodeType_Tlatency
 *		node.right	= Xseq of kNoisyIrNodeType_Tidentifier, cmpop, expr
 */
IrNode *
noisyParseChanEventExpression(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseChanEventExpression);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_PchanEventExpression,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	if (peekCheck(N, 1, kNoisyIrNodeType_Terasures))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Terasures));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Terrors))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Terrors));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tlatency))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tlatency));
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PchanEventExpression, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PchanEventExpression);
	}

	noisyParseTerminal(N, kNoisyIrNodeType_Tof);
	addLeafWithChainingSeq(N, n, noisyParseIdentifierUsageTerminal(N, kNoisyIrNodeType_Tidentifier, currentScope));
	addLeafWithChainingSeq(N, n, noisyParseCmpOp(N));
	addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));

	return n;
}

/*
 *	kNoisyIrNodeType_Pchan2nameExpression
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pfactor
 *		node.right	= kNoisyIrNodeType_TstringConst or NULL
 */
IrNode *
noisyParseChan2nameExpression(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseChan2nameExpression);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_Pchan2nameExpression,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tchan2name);
	addLeaf(N, n, noisyParseFactor(N, currentScope));

	if (peekCheck(N, 1, kNoisyIrNodeType_TstringConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TstringConst));
	}

	return n;
}

/*
 *	kNoisyIrNodeType_Pvar2nameExpression
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pfactor
 *		node.right	= kNoisyIrNodeType_TstringConst | NULL
 */
IrNode *
noisyParseVar2nameExpression(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseVar2nameExpression);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_Pvar2nameExpression,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tvar2name);
	addLeaf(N, n, noisyParseFactor(N, currentScope));

	if (peekCheck(N, 1, kNoisyIrNodeType_TstringConst))
	{
		addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TstringConst));
	}

	return n;
}

/*
 *	kNoisyIrNodeType_Pname2chanExpression
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_PtypeExpression
 *		node.right	= Xseq of kNoisyIrNodeType_Pexpression, REALCONST
 */
IrNode *
noisyParseName2chanExpression(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseName2chanExpression);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_Pname2chanExpression,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	noisyParseTerminal(N, kNoisyIrNodeType_Tname2chan);
	addLeaf(N, n, noisyParseTypeExpression(N, currentScope));
	addLeafWithChainingSeq(N, n, noisyParseExpression(N, currentScope));
	addLeafWithChainingSeq(N, n, noisyParseTerminal(N, kNoisyIrNodeType_TrealConst));

	return n;
}

/*
 *	kNoisyIrNodeType_Pterm
 *
 *	Generated AST subtree:
 *
 *		node.left	= kNoisyIrNodeType_Pfactor
 *		node.right	= Xseq of kNoisyIrNodeType_PhighPrecedenceBinaryOp  and kNoisyIrNodeType_Pfactor
 */
IrNode *
noisyParseTerm(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseTerm);


	IrNode *	n = genNoisyIrNode(N,	kNoisyIrNodeType_Pterm,
						NULL /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);


	/*
	 *	TODO/BUG: Double-check our handling of [basictype] and [unop] here
	 */
	if (noisyInFirst(N, kNoisyIrNodeType_PbasicType, gNoisyFirsts))
	{
		addLeaf(N, n, noisyParseBasicType(N, currentScope));
	}

	if (noisyInFirst(N, kNoisyIrNodeType_PunaryOp, gNoisyFirsts))
	{
		addLeaf(N, n, noisyParseUnaryOp(N));
	}

	addLeaf(N, n, noisyParseFactor(N, currentScope));
	while (noisyInFirst(N, kNoisyIrNodeType_PhighPrecedenceBinaryOp, gNoisyFirsts))
	{
		addLeafWithChainingSeq(N, n, noisyParseHighPrecedenceBinaryOp(N));
		addLeafWithChainingSeq(N, n, noisyParseFactor(N, currentScope));
	}

	return n;
}

/*
 *	kNoisyIrNodeType_Pfactor
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Tidentifier | ... | kNoisyIrNodeType_Pexpression
 *		node.left	= NULL or kNoisyIrNodeType_PfieldSelect
 *		node.right	= NULL or Xseq of kNoisyIrNodeType_PfieldSelect
 */
IrNode *
noisyParseFactor(State *  N, Scope *  currentScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseFactor);


	IrNode *	n;

	if (peekCheck(N, 1, kNoisyIrNodeType_Tidentifier))
	{
		n = noisyParseIdentifierUsageTerminal(N, kNoisyIrNodeType_Tidentifier, currentScope);

		while (noisyInFirst(N, kNoisyIrNodeType_PfieldSelect, gNoisyFirsts))
		{
			/*
			 *	TODO/BUG: This looks suspicious.
			 */
			addLeafWithChainingSeq(N, n, noisyParseFieldSelect(N, currentScope));
		}
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TintConst))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TintConst);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TrealConst))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TrealConst);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TstringConst))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TstringConst);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TboolConst))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TboolConst);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftParen) && peekCheck(N, 3, kNoisyIrNodeType_Tcomma))
	{
		/*
		 *	If we see
		 *
		 *			'(' identifier ','
		 *
		 *	then try to parse a tupleValue. Otherwise, parse an expression
		 */
		n = noisyParseTupleValue(N, currentScope);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftParen))
	{
		noisyParseTerminal(N, kNoisyIrNodeType_TleftParen);
		n = noisyParseExpression(N, currentScope);
		noisyParseTerminal(N, kNoisyIrNodeType_TrightParen);
	}
/*
 *	TODO/BUG: the following two should not be here. See grammar
 *
 *	else if (noisyInFirst(N, kNoisyIrNodeType_Punop, gNoisyFirsts))
 *	{
 *		n = noisyParseunop(N, currentScope);
 *		addLeaf(N, n, noisyParseFactor(N, currentScope));
 *	}
 *	else if (peekCheck(N, 1, kNoisyIrNodeType_Tchan2name))
 *	{
 *		noisyParseTerminal(N, kNoisyIrNodeType_Tchan2name);
 *		n = noisyParseFactor(N, currentScope);
 *
 *		if (peekCheck(N, 1, kNoisyIrNodeType_Tstring))
 *		{
 *			addLeaf(N, n, noisyParseTerminal(N, kNoisyIrNodeType_Tstring));
 *		}
 *	}
 */
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_Pfactor, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_Pfactor);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_PtupleValue
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
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseTupleValue);


	IrNode *	n;
	noisyParseTerminal(N, kNoisyIrNodeType_TleftParen);
	n = noisyParseIdentifierOrNilList(N, currentScope);
	noisyParseTerminal(N, kNoisyIrNodeType_TrightParen);

	/*
	 *	Identical to the parse tree for an kNoisyIrNodeType_PidentifierOrNilList,
	 *	but labeled as a kNoisyIrNodeType_PtupleValue.
	 */
	n->type = kNoisyIrNodeType_PtupleValue;

	return n;
}

/*
 *	kNoisyIrNodeType_PfieldSelect
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
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseFieldSelect);


	IrNode *	n;


	/*
	 *	We use a single AST node (kNoisyIrNodeType_Tdot or kNoisyIrNodeType_TleftBrac) to differentiate
	 *	the two types of subtrees.
	 */
	if (peekCheck(N, 1, kNoisyIrNodeType_Tdot))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tdot);
		addLeaf(N, n, noisyParseIdentifierUsageTerminal(N, kNoisyIrNodeType_Tidentifier, currentScope));
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftBrac))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TleftBrac);
		addLeaf(N, n, noisyParseExpression(N, currentScope));
		
		if (peekCheck(N, 1, kNoisyIrNodeType_Tcolon))
		{
			noisyParseTerminal(N, kNoisyIrNodeType_Tcolon);
			addLeaf(N, n, noisyParseExpression(N, currentScope));
		}

		noisyParseTerminal(N, kNoisyIrNodeType_TrightBrac);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PfieldSelect, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PfieldSelect);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_PhighPrecedenceBinaryOp
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Tasterisk | ... | kNoisyIrNodeType_TCONS
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseHighPrecedenceBinaryOp(State *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseHighPrecedenceBinaryO);


	IrNode *	n;


	if (peekCheck(N, 1, kNoisyIrNodeType_Tasterisk))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tasterisk);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tdiv))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tdiv);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tpercent))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tpercent);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tcaret))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tcaret);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tcons))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tcons);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PhighPrecedenceBinaryOp, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PhighPrecedenceBinaryOp);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_PlowPrecedenceBinaryOp
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Tplus | ... | kNoisyIrNodeType_Tstroke
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseLowPrecedenceBinaryOp(State *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseLowPrecedenceBinaryOp);


	IrNode *	n;


	if (peekCheck(N, 1, kNoisyIrNodeType_Tplus))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tplus);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tminus))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tminus);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TrightShift))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TrightShift);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_TleftShift))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_TleftShift);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tstroke))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tstroke);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Teq))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Teq);
	}	
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tneq))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tneq);
	}	
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tgt))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tgt);
	}	
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tlt))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tlt);
	}	
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tge))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tge);
	}	
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tle))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tle);
	}	
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tand))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tand);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tor))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tor);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PlowPrecedenceBinaryOp, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PlowPrecedenceBinaryOp);
	}

	return n;
}

/*
 *	kNoisyIrNodeType_PcmpOp
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Teq | ... | kNoisyIrNodeType_Tor
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseCmpOp(State *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseCmpOp);


	IrNode *	n;


	if (peekCheck(N, 1, kNoisyIrNodeType_Teq))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Teq);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tneq))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tneq);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tgt))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tgt);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tlt))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tlt);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tle))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tle);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tge))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tge);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tand))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tand);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tor))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tor);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PcmpOp, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PcmpOp);
	}

	return n;
}


/*
 *	kNoisyIrNodeType_PBOOLEANOP
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Tand or kNoisyIrNodeType_Tor (nothing else)
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseBooleanOp(State *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseBooleanOp);


	IrNode *	n;


	if (peekCheck(N, 1, kNoisyIrNodeType_Tand))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tand);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tor))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tor);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PbooleanOp, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PbooleanOp);
	}

	return n;
}


/*
 *	kNoisyIrNodeType_Punop
 *
 *	Generated AST subtree:
 *
 *		node		= kNoisyIrNodeType_Tname2chan | ... | kNoisyIrNodeType_Tgets
 *		node.left	= NULL
 *		node.right	= NULL
 */
IrNode *
noisyParseUnaryOp(State *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseUnaryOp);


	IrNode *	n;


	//name2chan is not a unary op. Why did we have it here?
	//if (peekCheck(N, 1, kNoisyIrNodeType_Tname2chan))
	//{
	//	n = noisyParseTerminal(N, kNoisyIrNodeType_Tname2chan);
	//}
	//else
	if (peekCheck(N, 1, kNoisyIrNodeType_Ttilde))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Ttilde);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tbang))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tbang);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tminus))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tminus);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tplus))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tplus);
	}
	//T_inc is not a unary op. Why did we have it here?
	//else if (peekCheck(N, 1, kNoisyIrNodeType_Tinc))
	//{
	//	n = noisyParseTerminal(N, kNoisyIrNodeType_Tinc);
	//}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tgets))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tgets);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Thd))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Thd);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Ttl))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Ttl);
	}
	else if (peekCheck(N, 1, kNoisyIrNodeType_Tlen))
	{
		n = noisyParseTerminal(N, kNoisyIrNodeType_Tlen);
	}
	else
	{
		noisyParserSyntaxError(N, kNoisyIrNodeType_PunaryOp, kNoisyIrNodeTypeMax);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PunaryOp);
	}

	return n;
}

/*
 *	Simply remove a terminal
 */
IrNode *
noisyParseTerminal(State *  N, IrNodeType expectedType)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseTerminal);

	if (!peekCheck(N, 1, expectedType))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeTypeMax, expectedType);

		/*
		 *	In this case, we know the specific expected type/token.
		 */
		noisyParserErrorRecovery(N, expectedType);
	}

	Token *	t = noisyLexGet(N, gReservedTokenDescriptions);
	IrNode *	n = genNoisyIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

	return n;
}

/*
 *	Remove an identifier _usage_ terminal, performing symtab lookup
 */
IrNode *
noisyParseIdentifierUsageTerminal(State *  N, IrNodeType expectedType, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseIdentifierUsageTerminal);

	if (!peekCheck(N, 1, expectedType))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeTypeMax, expectedType);

		/*
		 *	In this case, we know the specific expected type/token.
		 */
		noisyParserErrorRecovery(N, expectedType);
	}

	Token *	t = noisyLexGet(N, gReservedTokenDescriptions);
	IrNode *	n = genNoisyIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

	n->tokenString = t->identifier;

	n->symbol = noisySymbolTableSymbolForIdentifier(N, scope, t->identifier);
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
IrNode *
noisyParseIdentifierDefinitionTerminal(State *  N, IrNodeType  expectedType, Scope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParseIdentifierDefinitionTerminal);

	if (!peekCheck(N, 1, expectedType))
	{
		noisyParserSyntaxError(N, kNoisyIrNodeTypeMax, expectedType);

		/*
		 *	In this case, we know the specific expected type/token.
		 */
		noisyParserErrorRecovery(N, expectedType);
	}

	Token *	t = noisyLexGet(N, gReservedTokenDescriptions);
	IrNode *	n = genNoisyIrNode(N,	t->type,
						NULL /* left child */,
						NULL /* right child */,
						t->sourceInfo /* source info */);

	n->tokenString = t->identifier;

	//	NOTE: noisySymbolTableAddOrLookupSymbolForToken(N, ) adds token 't' to scope 'scope'
	Symbol *	sym = noisySymbolTableAddOrLookupSymbolForToken(N, scope, t);
	if (sym->definition != NULL)
	{
		errorMultiDefinition(N, sym);
		// TODO: do noisyParserErrorRecovery() here ?
	}
	n->symbol = sym;

	return n;
}






/*
 *	Exported non-parse routines
 */






void
noisyParserSyntaxError(State *  N, IrNodeType currentlyParsingTokenOrProduction, IrNodeType expectedProductionOrToken)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserSyntaxError);

	int		seen = 0;


	//errors++;

	/*
	 *	TODO: Other places where we need the string form of a IrNodeType
	 *	should also use gAstNodeStrings[] like we do here, rather than 
	 *	gProductionStrings[] and gTerminalStrings[].
	 */
	flexprint(N->Fe, N->Fm, N->Fperr, "\n-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --\n");
	if (N->mode & kNoisyModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "<b>");
	}

	if (N->mode & kNoisyModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\t%s, line %d position %d, %s %s\"",
						EsyntaxA,
						noisyLexPeek(N, 1)->sourceInfo->lineNumber,
						noisyLexPeek(N, 1)->sourceInfo->columnNumber,
						EsyntaxD,
						kNoisyErrorTokenHtmlTagOpen);
		noisyLexPrintToken(N, noisyLexPeek(N, 1), gReservedTokenDescriptions);
		flexprint(N->Fe, N->Fm, N->Fperr, "\"%s %s %s.<br><br>%s%s", kNoisyErrorTokenHtmlTagClose, EsyntaxB, gProductionDescriptions[currentlyParsingTokenOrProduction], kNoisyErrorDetailHtmlTagOpen, EsyntaxC);
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\t%s, %s line %d position %d, %s \"",
						EsyntaxA,
						noisyLexPeek(N, 1)->sourceInfo->fileName,
						noisyLexPeek(N, 1)->sourceInfo->lineNumber,
						noisyLexPeek(N, 1)->sourceInfo->columnNumber,
						EsyntaxD);
		noisyLexPrintToken(N, noisyLexPeek(N, 1), gReservedTokenDescriptions);
		flexprint(N->Fe, N->Fm, N->Fperr, "\" %s %s.\n\n\t%s", EsyntaxB, gProductionDescriptions[currentlyParsingTokenOrProduction], EsyntaxC);
	}

	if (((expectedProductionOrToken > kNoisyIrNodeType_TMax) && (expectedProductionOrToken < kNoisyIrNodeType_PMax)) || (expectedProductionOrToken == kNoisyIrNodeTypeMax))
	{
		flexprint(N->Fe, N->Fm, N->Fperr, " one of:\n\n\t\t");
		for (int i = 0; i < kNoisyIrNodeTypeMax && gNoisyFirsts[currentlyParsingTokenOrProduction][i] != kNoisyIrNodeTypeMax; i++)
		{
			if (seen > 0)
			{
				flexprint(N->Fe, N->Fm, N->Fperr, ",\n\t\t");
			}

			flexprint(N->Fe, N->Fm, N->Fperr, "'%s'", gReservedTokenDescriptions[gNoisyFirsts[currentlyParsingTokenOrProduction][i]]);
			seen++;
		}
	}
	else if ((currentlyParsingTokenOrProduction == kNoisyIrNodeTypeMax) && (expectedProductionOrToken < kNoisyIrNodeType_TMax))
	{
		flexprint(N->Fe, N->Fm, N->Fperr, ":\n\n\t\t");
		flexprint(N->Fe, N->Fm, N->Fperr, "'%s'", gReservedTokenDescriptions[expectedProductionOrToken]);
	}
	else
	{
		noisyFatal(N, Esanity);
	}

	flexprint(N->Fe, N->Fm, N->Fperr, ".\n\n\tInstead, saw:\n\n");
	noisyLexPeekPrint(N, 5, 0, gReservedTokenDescriptions);
	
	if (N->mode & kNoisyModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s</b>", kNoisyErrorDetailHtmlTagClose);
	}

	flexprint(N->Fe, N->Fm, N->Fperr, "\n-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --\n\n");
}


void
noisyParserSemanticError(State *  N, const char * format, ...)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserSemanticError);

	va_list	arg;

	/*
	 *	We use varargs so that we can pass in a list of strings that should
	 *	get concatenated, akin to joining them with "+" in Limbo.
	 */
	if (N->verbosityLevel & kNoisyVerbosityDebugParser)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "In noisyParserSemanticError(), Ignoring semantic error...\n");
		flexprint(N->Fe, N->Fm, N->Fperr, "In noisyParserSemanticError(), Source file line %llu\n", noisyLexPeek(N, 1)->sourceInfo->lineNumber);
	}

	va_start(arg, format);
//	flexprint(N->Fe, N->Fm, N->Fperr, format, arg);
	va_end(arg);
}


void
noisyParserErrorRecovery(State *  N, IrNodeType expectedProductionOrToken)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserErrorRecovery);

	if (N->verbosityLevel & kNoisyVerbosityDebugParser)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "In noisyParserErrorRecovery(), about to discard tokens...\n");
	}

	/*
	while (!noisyInFollow(N, expectedProductionOrToken, gNoisyFollows) && N->tokenList != NULL)
	{
		 *
		 *	Retrieve token and discard...
		 *
		Token *	token = noisyLexGet(N);
		if (N->verbosityLevel & kNoisyVerbosityDebugParser)
		{
			noisyLexDebugPrintToken(N, token);
		}
	}
	*/

	if ((N != NULL) && (N->jmpbufIsValid))
	{
fprintf(stderr, "doing longjmp");

		/*
		 *	Could pass in case-specific info here, but just
		 *	pass 0.
		 *
		 *	TODO: We could, e.g., return info on which line
		 *	number of the input we have reached, and let, e.g.,
		 *	the CGI version highlight the point at which
		 *	processing stopped.
		 */
		longjmp(N->jmpbuf, 0);
	}

	/*
	 *	Not reached if N->jmpbufIsValid
	 */
	noisyConsolePrintBuffers(N);

	exit(EXIT_SUCCESS);
}





