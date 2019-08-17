/*
	Authored 2016. Jonathan Lim. Modified 2018, Phillip Stanley-Marbell.

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
#include "newton-timeStamps.h"
#include "common-errors.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "newton-symbolTable.h"


extern int		primeNumbers[168];

/*
 *	TODO: need to tag scopes corresponding to progtypes with a name, so
 *	that when we want to lookup a progtype-qualified-name in symtab, we
 *	can use the "a" of "a->b" to lookup, and get the type structure of
 *	"b".  See, e.g., comments at P_TYPENAME in noisy-irPass-cBackend.
 */

static Dimension *	copyDimensionNode(State *  N, Dimension *  list);
static Dimension *	copyDimensionList(State *  N, Dimension *  list);


static Invariant *
getTailInvariant(State *  N, Invariant *  head)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	if (head == NULL)
	{
		return NULL;
	}
	else
	{
		Invariant *	current = head;
		while (current->next != NULL)
		{
			current = current->next;
		}
		return current;
	}
}


static Dimension *
copyDimensionNode(State *  N, Dimension *  list)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Dimension *	temp = (Dimension *) calloc(1, sizeof(Dimension));

	temp->name		= list->name;
	temp->abbreviation	= list->abbreviation;
	temp->scope		= list->scope;
	temp->sourceInfo	= list->sourceInfo;
	temp->primeNumber	= list->primeNumber;
	temp->exponent		= list->exponent;

	if (list->next != NULL)
	{
		Dimension *	nextCopy = (Dimension *) calloc(1, sizeof(Dimension));

		memcpy(nextCopy, list->next, sizeof(Dimension));
		temp->next = nextCopy;
	}

	return temp;
}

static Dimension *
copyDimensionList(State *  N, Dimension *  source)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Dimension *	returnNode = NULL;
	Dimension *	destHead = NULL;
	Dimension *	sourceHead = source;

	while (sourceHead != NULL) 
	{
		if (destHead == NULL)
		{
			destHead = copyDimensionNode(N, sourceHead);
			returnNode = destHead;
		}
		else
		{
			destHead->next = copyDimensionNode(N, sourceHead);
			destHead = destHead->next;
		}

		sourceHead = sourceHead->next;
	}

	return returnNode;
}

Physics *
shallowCopyPhysicsNode(State *  N, Physics *  node)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Physics *	copy = (Physics *) calloc(1, sizeof(Physics));

	copy->identifier	= node->identifier;
	copy->scope		= node->scope;
	copy->sourceInfo	= node->sourceInfo;

	copy->isVector		= node->isVector;
	copy->value		= node->value;

	copy->dimensionAlias	= node->dimensionAlias;
	copy->dimensionAliasAbbreviation = node->dimensionAliasAbbreviation;

	copy->definition	= node->definition;
	copy->next		= NULL;

	return copy;
}

Physics *
deepCopyPhysicsNode(State *  N, Physics *  node)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Physics *	copy = (Physics *) calloc(1, sizeof(Physics));

	copy->dimensions	= copyDimensionList(N, node->dimensions);

	copy->identifier	= node->identifier;
	copy->scope		= node->scope;
	copy->sourceInfo	= node->sourceInfo;

	copy->isVector		= node->isVector;
	copy->value		= node->value;
	copy->isConstant	= node->isConstant;
	copy->id		= node->id;
	copy->subindex		= node->subindex;

	copy->dimensionAlias	= node->dimensionAlias;
	copy->dimensionAliasAbbreviation = node->dimensionAliasAbbreviation;

	copy->definition	= node->definition;
	copy->next		= NULL;

	return copy;
}


IntegralList *
getTailIntegralList(State *  N, IntegralList *  list)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	if (list == NULL)
	{
		return NULL;
	}
	else
	{
		IntegralList *	current = list;
		while (current->next != NULL)
		{
			current = current->next;
		}
		return current;
	}
}

Physics *
getTailPhysics(State *  N, Physics *  list)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	if (list == NULL)
	{
		return NULL;
	}
	else
	{
		Physics *	current = list;
		while (current->next != NULL)
		{
			current = current->next;
		}
		return current;
	}
}

Physics * 
newtonPhysicsTableCopyAndAddPhysics(State *  N, Scope *  scope, Physics *  source)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Physics *	dest = deepCopyPhysicsNode(N, source);

	Physics *	tail;
	if ((tail = getTailPhysics(N, source)) == NULL)
	{
		scope->firstPhysics = dest;
	}
	else
	{
		tail->next = dest;
	}

	return dest;
}

void
newtonAddInvariant(State *  N, Invariant *  invariant)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Invariant *	tail;
	if ((tail = getTailInvariant(N, N->invariantList)) == NULL)
	{
		N->invariantList = invariant;
	}
	else
	{
		tail->next = invariant;
	}
}


void
newtonPhysicsIncrementExponent(State *  N, Physics *  source, Dimension *  added)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Dimension *	current = source->dimensions;

	bool	somethingWasAdded = false;
	while (current != NULL)
	{
		if (current->primeNumber == added->primeNumber)
		{
			current->exponent += 1;
			somethingWasAdded = true;
		}

		current = current->next;
	}

	assert(somethingWasAdded); /* TODO remove later */
}


void
newtonPhysicsAddExponents(State *  N, Physics *  left, Physics *  right)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	if (right == NULL)
	{
		return;
	}

	Dimension *	currentLeft = left->dimensions;
	Dimension *	currentRight = right->dimensions;

	assert(currentLeft != NULL && currentRight != NULL);

//fprintf(stderr, ">>>\n");
	while (currentLeft != NULL && currentRight != NULL)
	{
//fprintf(stderr, "(%f, %f) --> ", currentLeft->exponent, currentRight->exponent);
		currentLeft->exponent += currentRight->exponent;
//fprintf(stderr, "(%f, %f)\n", currentLeft->exponent, currentRight->exponent);

		currentLeft = currentLeft->next;
		currentRight = currentRight->next;
	}
//fprintf(stderr, "<<<\n");
}

void
newtonPhysicsAddExponentsRecursively(State *  N, Physics *  baseNode, IrNode * treeRoot)
{
	if (treeRoot == NULL)
	{
		return;
	} else {
		newtonPhysicsAddExponentsRecursively(N, baseNode, treeRoot->irLeftChild);
		newtonPhysicsAddExponentsRecursively(N, baseNode, treeRoot->irRightChild);
	}

	if ((treeRoot->physics != NULL) && (treeRoot->type == kNewtonIrNodeType_Punit)) {	
		newtonPhysicsAddExponents(N, baseNode, treeRoot->physics);
	} else {
		//flexprint(N->Fe, N->Fm, N->Fpinfo, "\t(This irLeftChild has no physics tokenString is \"%s\")\n", treeRoot->tokenString);
	}
}

void
newtonPhysicsSubtractExponents(State *  N, Physics *  left, Physics *  right)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Dimension *	currentLeft = left->dimensions;
	Dimension *	currentRight = right->dimensions;

	assert(currentLeft != NULL && currentRight != NULL);

	while (currentLeft != NULL && currentRight != NULL)
	{
		currentLeft->exponent -= currentRight->exponent;

		currentLeft = currentLeft->next;
		currentRight = currentRight->next;
	}
}


void
newtonPhysicsMultiplyExponents(State *  N, Physics *  source, double multiplier)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Dimension *	current = source->dimensions;
	assert(current != NULL);

	while (current != NULL)
	{
		current->exponent *= multiplier;

		current = current->next;
	}
}


Dimension *
newtonDimensionTableAddDimensionForToken(State *  N, Scope *  scope, Token *  nameToken, Token *  abbrevToken)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Dimension *	newDimension;

	newDimension = (Dimension *) calloc(1, sizeof(Dimension));

	if (newDimension == NULL)
	{
		fatal(N, Emalloc);
	}

	newDimension->abbreviation	= abbrevToken->identifier;
	newDimension->name		= nameToken->stringConst;
	newDimension->sourceInfo	= nameToken->sourceInfo;
	newDimension->scope		= scope;
	newDimension->primeNumber	= primeNumbers[N->primeNumbersIndex++];
	newDimension->exponent		= 0;

	if (scope->firstDimension == NULL)
	{
		scope->firstDimension = newDimension;
	}
	else
	{
		Dimension *	curDimension = scope->firstDimension;
		while (curDimension->next != NULL) 
		{
			curDimension = curDimension->next;
		}
		curDimension->next = newDimension;
	}

	return newDimension;
}

Physics * 
newtonInitPhysics(State * N, Scope * scope, Token * token)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Physics *	newPhysics = (Physics *) calloc(1, sizeof(Physics));
	if (newPhysics == NULL)
	{
		fatal(N, Emalloc);
	}

	assert(N->newtonIrTopScope->firstDimension != NULL);
	newPhysics->dimensions = copyDimensionList(N, N->newtonIrTopScope->firstDimension);

	newPhysics->scope = scope;

	if (token != NULL)
	{
		newPhysics->identifier = token->identifier;
		newPhysics->sourceInfo = token->sourceInfo;
	}
	newPhysics->next = NULL;

	return newPhysics;
}

Physics *
newtonPhysicsTableAddPhysicsForToken(State *  N, Scope *  scope, Token *  token)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Physics *	newPhysics;

	if (N->newtonIrTopScope->firstDimension == NULL)
	{
		newPhysics = (Physics *) calloc(1, sizeof(Physics));
		newPhysics->scope = scope;

		if (token != NULL)
		{
			newPhysics->identifier = token->identifier;
			newPhysics->sourceInfo = token->sourceInfo;
		}
	}
	else
	{
		newPhysics =  newtonInitPhysics(N, scope, token);
	}

	if (scope->firstPhysics == NULL)
	{
		scope->firstPhysics = newPhysics;
	}
	else
	{
		Physics *   curPhysics = scope->firstPhysics;
		while (curPhysics->next != NULL)
		{
			curPhysics = curPhysics->next;
		}
		curPhysics->next = newPhysics;
	}

	return newPhysics;
}

bool
areTwoPhysicsEquivalent(State * N, Physics * left, Physics * right)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Dimension *	leftCurrent = left->dimensions;
	Dimension *	rightCurrent = right->dimensions;

	assert(leftCurrent != NULL && rightCurrent != NULL);

	while (leftCurrent != NULL && rightCurrent != NULL)
	{
		if (leftCurrent->exponent != rightCurrent->exponent)
		{
//fprintf(stderr, "(%f, %f) ", leftCurrent->exponent, rightCurrent->exponent);
			return false;
		}
		leftCurrent = leftCurrent->next;
		rightCurrent = rightCurrent->next;
	}

	if (leftCurrent != NULL || rightCurrent != NULL)
	{
		return false;
	}

	return true;
}

Dimension *
newtonDimensionTableDimensionForName(State *  N, Scope *  scope, const char *  name)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	if (scope == NULL)
	{
		return NULL;
	}

	Dimension *	curDimension = scope->firstDimension;
	while (curDimension != NULL)
	{
		if (!strcmp(curDimension->name, name))
		{
			return curDimension;
		}
		curDimension = curDimension->next;
	}

	return newtonDimensionTableDimensionForName(N, scope->parent, name);
}

Physics *
newtonPhysicsTablePhysicsForDimensionAliasAbbreviation(State *  N, Scope *  scope, const char * dimensionAliasAbbreviation)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	if (scope == NULL)
	{
		return NULL;
	}

	Physics *	curPhysics = scope->firstPhysics;
	while (curPhysics != NULL)
	{
		/*
		 *	NOTE: not all physics structs have a dimensionAlias...
		 */
		if (curPhysics->dimensionAliasAbbreviation && !strcmp(curPhysics->dimensionAliasAbbreviation, dimensionAliasAbbreviation))
		{
			assert(curPhysics->dimensions != NULL);
			return curPhysics;
		}
		curPhysics = curPhysics->next;
	}

	return newtonPhysicsTablePhysicsForDimensionAliasAbbreviation(N, scope->parent, dimensionAliasAbbreviation);
}

Physics *
newtonPhysicsTablePhysicsForDimensionAlias(State *  N, Scope *  scope, const char * dimensionAliasIdentifier)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	if (scope == NULL)
	{
		return NULL;
	}

	Physics *	curPhysics = scope->firstPhysics;
	while (curPhysics != NULL)
	{
		/*
		 *	NOTE: not all physics structs have a dimensionAlias...
		 */
		if (curPhysics->dimensionAlias && !strcmp(curPhysics->dimensionAlias, dimensionAliasIdentifier))
		{
			assert(curPhysics->dimensions != NULL);
			return curPhysics;
		}
		curPhysics = curPhysics->next;
	}

	return newtonPhysicsTablePhysicsForDimensionAlias(N, scope->parent, dimensionAliasIdentifier);
}

Physics *
newtonPhysicsTablePhysicsForIdentifierAndSubindex(State *  N, Scope *  scope, const char *  identifier, int subindex)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	if (scope == NULL)
	{
		return NULL;
	}

	Physics *	curPhysics = scope->firstPhysics;
	while (curPhysics != NULL)
	{
		if (!strcmp(curPhysics->identifier, identifier) && curPhysics->subindex == subindex)
		{
			assert(curPhysics->dimensions != NULL);
			return curPhysics;
		}
		curPhysics = curPhysics->next;
	}

	return newtonPhysicsTablePhysicsForIdentifier(N, scope->parent, identifier);
}

Physics *
newtonPhysicsTablePhysicsForIdentifier(State *  N, Scope *  scope, const char *  identifier)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	if (scope == NULL)
	{
		return NULL;
	}

	Physics *	curPhysics = scope->firstPhysics;
	while (curPhysics != NULL)
	{
		if (!strcmp(curPhysics->identifier, identifier))
		{
			assert(curPhysics->dimensions != NULL);
			return curPhysics;
		}
		curPhysics = curPhysics->next;
	}

	return newtonPhysicsTablePhysicsForIdentifier(N, scope->parent, identifier);
}

Scope *
newtonSymbolTableAllocScope(State *  N)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Scope *		newScope;

	newScope = (Scope *) calloc(1, sizeof(Scope));
	if (newScope == NULL)
	{
		fatal(N, Emalloc);
	}

	return newScope;
}


Scope *
newtonSymbolTableOpenScope(State *  N, Scope *  scope, IrNode *  subTree)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	Scope *		newScope = newtonSymbolTableAllocScope(N);

	newScope->parent	= scope;
	newScope->begin		= subTree->sourceInfo;
	scope->firstChild	= newScope;

	return newScope;
}


void
newtonSymbolTableCloseScope(State *  N, Scope *  scope, IrNode *  subTree)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	scope->end = subTree->sourceInfo;
}
