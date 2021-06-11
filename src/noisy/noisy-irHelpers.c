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
#include "noisy-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "noisy-irHelpers.h"
#include "common-lexers-helpers.h"
#include "noisy-lexer.h"




Scope *
moduleName2scope(State *  N, const char *  identifier)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParserModuleName2scope);

//	FlexListItem *	tmp = N->moduleScopes->hd;

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
addToModuleScopes(State *  N, char *  identifier, Scope *  moduleScope)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParserAddToModuleScopes);

	moduleScope->identifier = identifier;

	if (N->moduleScopes == NULL)
	{
		N->moduleScopes = moduleScope;

		return;
	}

	Scope *	p = N->moduleScopes;
	while (p->next != NULL)
	{
		p = p->next;
	}
	p->next = moduleScope;;

	return;
}



void
assignTypes(State *  N, IrNode *  node, IrNode *  typeExpression)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParserAssignTypes);

	/*
	 *	See issue #297: The typeExpr might be, say, an identifier that is
	 *	an alias for a type. We should check for this case and get the
	 *	identifier's sym->typeTree. Also, do sanity checks to validate
	 *	the typeTree, e.g., make sure it is always made up of basic
	 *	types and also that it's not NULL.
	 */

	if (	node->type != kNoisyIrNodeType_Tidentifier &&
		node->type != kNoisyIrNodeType_PidentifierList &&
		node->type != kNoisyIrNodeType_PidentifierOrNilList &&
		node->type != kNoisyIrNodeType_PqualifiedIdentifier
		)
	{
		fatal(N, EassignTypeSanity);
	}

	/*
	 *	Walk subtree identifierList, set each node->symbol.typeExpr = typeExpr
	 */
	if (node->type == kNoisyIrNodeType_Tidentifier)
	{
		node->symbol->typeTree = typeExpression;
	}
	else if (node->type == kNoisyIrNodeType_PidentifierList)
	{
		for (IrNode * currentNode = node; currentNode != NULL; currentNode = currentNode->irRightChild)
		{
			currentNode->irLeftChild->symbol->typeTree = typeExpression;
		}
	}
	else if (node->type == kNoisyIrNodeType_PidentifierOrNilList)
	{
		for (IrNode * currentNode = node; currentNode != NULL; currentNode = currentNode->irRightChild)
		{
			if (node->irLeftChild->type != kNewtonIrNodeType_Tnil)
			{
				assignTypes(N,currentNode->irLeftChild->irLeftChild,typeExpression);
			}
		}
	}
	else if (node->type == kNoisyIrNodeType_PqualifiedIdentifier)
	{
		/*
		*	Assign types is called only when we declare variables. Therefore the field select does
		*	not need to be handled when we assign types. However type checker might need to check if
		*	we declare with a field select.
		*/
		node->irLeftChild->symbol->typeTree = typeExpression;
	}
	/*
	*	TODO: It needs fixing for PidentifierOrNilList and PqualifiedIdentifier. It works for
	*	identifier and identifierList.
	*/

	return ;
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
