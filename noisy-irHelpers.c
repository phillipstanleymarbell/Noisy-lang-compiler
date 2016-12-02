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
#include "noisy-errors.h"
#include "version.h"
#include "noisy-timeStamps.h"
#include "noisy.h"
#include "noisy-irHelpers.h"
#include "noisy-lexer.h"



NoisyIrNode *
genNoisyIrNode(NoisyState *  N, NoisyIrNodeType type, NoisyIrNode *  irLeftChild, NoisyIrNode *  irRightChild, NoisySourceInfo *  sourceInfo)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyGenNoisyIrNode);

	NoisyIrNode *		node;

	node = (NoisyIrNode *) calloc(1, sizeof(NoisyIrNode));
	if (node == NULL)
	{
		noisyFatal(N, Emalloc);

		/*	Not reached	*/
	}

	node->type		= type;
	node->sourceInfo	= sourceInfo;
	node->irLeftChild	= irLeftChild;
	node->irRightChild	= irRightChild;

	if (irLeftChild != NULL)
	{
		irLeftChild->irParent = node;
	}

	if (irRightChild != NULL)
	{
		irRightChild->irParent = node;
	}

	/*
	 *	Not madatory, but provides higher-fidelity attribution, by making 
	 *	sure that any time between here and next stamping is not attributed
	 *	to genNoisyIrNode().
	 */
	//NoisyTimeStampTraceMacro(kNoisyTimeStampKeyUnknown);


	return node;
}

NoisyScope *
progtypeName2scope(NoisyState *  N, const char *  identifier)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserProgtypeName2scope);

//	FlexListItem *	tmp = N->progtypeScopes->hd;

	/*
	 *	Each item is a tuple of (identifier, scope).
	 */
//	while (tmp != NULL)
//	{
//		if (	noisyValidFlexTupleCheckMacro(tmp)			&&
//			(tmp->siblings->hd->type == kNoisyFlexListTypeString)	&&
//			!strcmp(tmp->siblings->hd->value, identifier)		&&
//			(tmp->siblings->hd->next->type == kNoisyFlexListTypeNoisyScopePointer))
//		{
//			return tmp->siblings->hd->next->value;
//		}
//
//		tmp = tmp->next;
//	}

	return NULL;
}


void
errorUseBeforeDefinition(NoisyState *  N, const char *  identifier)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserErrorUseBeforeDefinition);

	flexprint(N->Fe, N->Fm, N->Fperr, "Saw identifier \"%s\" in use before definition\n", identifier);
}

void
errorMultiDefinition(NoisyState *  N, NoisySymbol *  symbol)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserErrorMultiDefinition);
}


bool
peekCheck(NoisyState *  N, int lookAhead, NoisyIrNodeType expectedType)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserPeekCheck);

	if (noisyLexPeek(N, lookAhead) == NULL)
	{
		return false;
	}

	return (noisyLexPeek(N, lookAhead)->type == expectedType);
}


NoisyIrNode *
depthFirstWalk(NoisyState *  N, NoisyIrNode *  node)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserDepthFirstWalk);

	if (node->irLeftChild == NULL || node->irRightChild == NULL)
	{
		return node;
	}

	return depthFirstWalk(N, node->irRightChild);
}

void
addLeaf(NoisyState *  N, NoisyIrNode *  parent, NoisyIrNode *  newNode)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserAddLeaf);

	NoisyIrNode *	node = depthFirstWalk(N, parent);

	if (node == NULL)
	{
		noisyFatal(N, Esanity);
	}
	
	if (node->irLeftChild == NULL)
	{
		node->irLeftChild = newNode;
		
		return;
	}

	node->irRightChild = newNode;
}

void
addLeafWithChainingSeq(NoisyState *  N, NoisyIrNode *  parent, NoisyIrNode *  newNode)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserAddLeafWithChainingSeq);

	NoisyIrNode *	node = depthFirstWalk(N, parent);

	if (node->irLeftChild == NULL)
	{
		node->irLeftChild = newNode;

		return;
	}
	
	node->irRightChild = genNoisyIrNode(N,	kNoisyIrNodeType_Xseq,
						newNode /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);
}

void
addToProgtypeScopes(NoisyState *  N, char *  identifier, NoisyScope *  progtypeScope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserAddToProgtypeScopes);

	progtypeScope->identifier = identifier;

	if (N->progtypeScopes == NULL)
	{
		N->progtypeScopes = progtypeScope;

		return;
	}

	NoisyScope *	p = N->progtypeScopes;
	while (p->next != NULL)
	{
		p = p->next;
	}
	p->next = progtypeScope;;

	return;
}



/*
 *	kNoisyIrNodeType_PidentifierList
 *
 *	AST subtree:
 *
 *		node		= kNoisyIrNodeType_Tidentifier
 *		node.left	= kNoisyIrNodeType_Tidentifier
 *		node.right	= Xseq of kNoisyIrNodeType_Tidentifier
 */
void
assignTypes(NoisyState *  N, NoisyIrNode *  node, NoisyIrNode *  typeExpression)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserAssignTypes);

	/*
	 *	TODO: The typeExpr might be, say, an identifier that is an
	 *	alias for a type. We should check for this case and get the
	 *	identifier's sym->typeTree. Also, do sanity checks to validate
	 *	the typeTree, e.g., make sure it is always made up of basic
	 *	types and also that it's not NULL.
	 */

	if (node->type != kNoisyIrNodeType_Tidentifier)
	{
		noisyFatal(N, EassignTypeSanity);
	}

	/*
	 *	Walk subtree identifierList, set each node->symbol.typeExpr = typeExpr
	 */
	node->symbol->typeTree = typeExpression;

	/*
	 *	Might be only one ident, or only two, or a whole Xseq of them
	 */
	if (node->irLeftChild != NULL)
	{
		node->irLeftChild->symbol->typeTree = typeExpression;
	}

	node = node->irRightChild;

	while (node != NULL)
	{
		/*
		 *	In here, node->type is always Xseq, with node->irLeftChild a node,
		 *	and node->irRightChild either another Xseq or NULL.
		 */
		if (node->irLeftChild != NULL)
		{
			node->irLeftChild->symbol->typeTree = typeExpression;
		}

		node = node->irRightChild;
	}
}
