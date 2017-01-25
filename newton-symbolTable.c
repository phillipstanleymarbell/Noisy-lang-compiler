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
#include <assert.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "version.h"
#include "noisy-timeStamps.h"
#include "noisy.h"
#include "newton-symbolTable.h"


extern int primeNumbers[168];

/*
 *	TODO: need to tag scopes corresponding to progtypes with a name, so
 *	that when we want to lookup a progtype-qualified-name in symtab, we
 *	can use the "a" of "a->b" to lookup, and get the type structure of
 *	"b".  See, e.g., comments at P_TYPENAME in newton-irPass-cBackend.
 */

static Dimension* copyDimensionNode(Dimension* list);
static Dimension* copyDimensionList(Dimension* list);
static Dimension* getTailDimension(Dimension* list);


/*
 * this function counts the number of dimensions in a physics struct
 * that has time
 */
int countNumberTime(Dimension* dimensionHead)
{
    int numTimes = 0;
    Dimension * current = dimensionHead;
    while (current != NULL)
    {
		if (!strcmp(current->identifier, "s")) // TODO this is hard coded... fix it. use a method that finds physics named time
        {
            numTimes++;
        }
        current = current->next;
    }

    return numTimes;
}

static Invariant*
getTailInvariant(Invariant * head)
{
    if (head == NULL)
        return NULL;
    else
    {
        Invariant* current = head;
        while (current->next != NULL)
        {
            current = current->next;
        }
        return current;
    }
}

static Dimension*
getTailDimension(Dimension* list)
{
    if (list == NULL)
        return NULL;
    else
    {
        Dimension* current = list;
        while (current->next != NULL)
        {
            current = current->next;
        }
        return current;
    }
}


static Dimension*
copyDimensionNode(Dimension* list)
{
    Dimension * temp = (Dimension*) calloc(1, sizeof(Dimension));
    temp->identifier = list->identifier;
    temp->scope = list->scope;
    temp->sourceInfo = list->sourceInfo;
    temp->primeNumber = list->primeNumber;

    if (list->next != NULL)
    {
        Dimension * nextCopy = (Dimension*) calloc(1, sizeof(Dimension));
        memcpy(nextCopy, list->next, sizeof(Dimension));
        temp->next = nextCopy;
    }
    return temp;
}

static Dimension*
copyDimensionList(Dimension* source)
{
    Dimension * returnNode = NULL;
    Dimension * destHead = NULL;
    Dimension * sourceHead = source;
    while (sourceHead != NULL) 
    {
        if (destHead == NULL)
        {
            destHead = copyDimensionNode(sourceHead);
            returnNode = destHead;
        }
        else
        {
            destHead->next = copyDimensionNode(sourceHead);
            destHead = destHead->next;
        }

        sourceHead = sourceHead->next;
    }
    return returnNode;
}

Physics*
copyPhysicsNode(Physics* list)
{
    Physics * copy = (Physics*) calloc(1, sizeof(Physics));
    memcpy(copy, list, sizeof(Physics));
    copy->next = NULL;

    return copy;
}

IntegralList*
getTailIntegralList(IntegralList* list)
{
    if (list == NULL)
        return NULL;
    else
    {
        IntegralList* current = list;
        while (current->next != NULL)
        {
            current = current->next;
        }
        return current;
    }
}

Physics*
getTailPhysics(Physics* list)
{
    if (list == NULL)
        return NULL;
    else
    {
        Physics* current = list;
        while (current->next != NULL)
        {
            current = current->next;
        }
        return current;
    }
}

Physics * 
newtonPhysicsTableCopyAndAddPhysics(NoisyState * N, NoisyScope * scope, Physics * source)
{
    Physics * dest = (Physics *) calloc(1, sizeof(Physics));
    dest->numeratorPrimeProduct = 1;
    dest->denominatorPrimeProduct = 1;
    
    newtonPhysicsCopyNumeratorDimensions(N, dest, source);
    newtonPhysicsCopyDenominatorDimensions(N, dest, source);

    Physics * tail;
    if ((tail = getTailPhysics(source)) == NULL)
        scope->firstPhysics = dest;
    else
        tail->next = dest;

    return dest;
}

void
newtonAddInvariant(NoisyState * N, Invariant * invariant)
{
    Invariant * tail;
    if ((tail = getTailInvariant(invariant)) == NULL)
        N->invariantList = invariant;
    else
        tail->next = invariant;
}

// TODO clean up the code. make helper methods
void 
newtonPhysicsCopyDenominatorToNumeratorDimensions(NoisyState * N, Physics * dest, Physics * source)
{
    if (dest->numeratorDimensions == NULL)
    {
        dest->numeratorDimensions = copyDimensionList(source->denominatorDimensions);
    }
    else
    {
        getTailDimension(dest->numeratorDimensions)->next = copyDimensionList(source->denominatorDimensions);
    }
    
    dest->numberOfNumerators += source->numberOfDenominators;
    dest->numeratorPrimeProduct *= source->denominatorPrimeProduct;
}

void 
newtonPhysicsCopyNumeratorToDenominatorDimensions(NoisyState * N, Physics * dest, Physics * source)
{
    if (dest->denominatorDimensions == NULL)
    {
        dest->denominatorDimensions = copyDimensionList(source->numeratorDimensions);
    }
    else
    {
        getTailDimension(dest->denominatorDimensions)->next = copyDimensionList(source->numeratorDimensions);
    }
    
    dest->numberOfDenominators += source->numberOfNumerators;
    dest->denominatorPrimeProduct *= source->numeratorPrimeProduct;

}

// TODO change method name from copy to transfer
void newtonPhysicsCopyNumeratorDimensions(NoisyState * N, Physics * dest, Physics * source) 
{
    if (dest->numeratorDimensions == NULL)
    {
        dest->numeratorDimensions = copyDimensionList(source->numeratorDimensions);
    }
    else
    {
        getTailDimension(dest->numeratorDimensions)->next = copyDimensionList(source->numeratorDimensions);
    }
    
    dest->numberOfNumerators += source->numberOfNumerators;
    dest->numeratorPrimeProduct *= source->numeratorPrimeProduct;
}

void newtonPhysicsCopyDenominatorDimensions(NoisyState * N, Physics * dest, Physics * source)
{
    if (dest->denominatorDimensions == NULL)
    {
        dest->denominatorDimensions = copyDimensionList(source->denominatorDimensions);
    }
    else
    {
        getTailDimension(dest->denominatorDimensions)->next = copyDimensionList(source->denominatorDimensions);
    }
    
    dest->numberOfDenominators += source->numberOfDenominators;
    dest->denominatorPrimeProduct *= source->denominatorPrimeProduct;
}

void 
newtonPhysicsAddNumeratorDimension(NoisyState * N, Physics * dest, Dimension * numerator)
{
    if (dest->numeratorDimensions == NULL)
    {
        dest->numeratorDimensions = copyDimensionList(numerator);
    }
    else
    {
        getTailDimension(dest->numeratorDimensions)->next = copyDimensionList(numerator);
    }

    dest->numeratorPrimeProduct *= numerator->primeNumber;
    dest->numberOfNumerators++;
}

void 
newtonPhysicsAddDenominatorDimension(NoisyState * N, Physics * dest, Dimension * denominator)
{
    if (dest->denominatorDimensions == NULL)
    {
        dest->denominatorDimensions = copyDimensionList(denominator);
    }
    else
    {
        getTailDimension(dest->denominatorDimensions)->next = copyDimensionList(denominator);
    }
    
    dest->denominatorPrimeProduct *= denominator->primeNumber;
    dest->numberOfDenominators++;
}

Dimension *
newtonDimensionTableAddDimensionForToken(NoisyState *  N, NoisyScope *  scope, NoisyToken *  nameToken, NoisyToken * abbrevToken)
{
	Dimension *	newDimension;

	newDimension = (Dimension *)calloc(1, sizeof(Dimension));
	if (newDimension == NULL)
	{
		noisyFatal(N, Emalloc);
	}

    newDimension->abbreviation  = abbrevToken->stringConst;
	newDimension->identifier	= nameToken->stringConst;
	newDimension->sourceInfo	= nameToken->sourceInfo;
	newDimension->scope	= scope;
    newDimension->primeNumber = primeNumbers[N->primeNumbersIndex++];

    if (scope->firstDimension == NULL) {
        scope->firstDimension = newDimension;
    } else {
        Dimension * curDimension = scope->firstDimension;
        while (curDimension->next != NULL) {
            curDimension = curDimension->next;
        }
        curDimension->next = newDimension;
    }

	return newDimension;
}

Physics *
newtonPhysicsTableAddPhysicsForToken(NoisyState *  N, NoisyScope *  scope, NoisyToken *  token)
{
	Physics *	newPhysics;

	newPhysics = (Physics *)calloc(1, sizeof(Physics));
	if (newPhysics == NULL)
	{
		noisyFatal(N, Emalloc);
	}

	newPhysics->identifier	= token->identifier;
	newPhysics->sourceInfo	= token->sourceInfo;
	newPhysics->scope	= scope;
    
    newPhysics->numeratorPrimeProduct = 1;
    newPhysics->denominatorPrimeProduct = 1;

	newPhysics->definition	= newtonPhysicsTablePhysicsForIdentifier(N, scope, token->identifier);

    if (scope->firstPhysics == NULL) {
        scope->firstPhysics = newPhysics;        
    } else {
        Physics * curPhysics = scope->firstPhysics;
        while (curPhysics->next != NULL) {
            curPhysics = curPhysics->next;
        }
        curPhysics->next = newPhysics;
    }

	return newPhysics;
}

Dimension *
newtonDimensionTableDimensionForIdentifier(NoisyState *  N, NoisyScope *  scope, const char *  identifier)
{
	if (scope == NULL)
	{
		return NULL;
	}

	Dimension * curDimension = scope->firstDimension;
	while (curDimension != NULL)
	{
		if (!strcmp(curDimension->identifier, identifier))
		{
			return curDimension;
		}
		curDimension = curDimension->next;
	}

	return newtonDimensionTableDimensionForIdentifier(N, scope->parent, identifier);
}

Physics *
newtonPhysicsTablePhysicsForIdentifier(NoisyState *  N, NoisyScope *  scope, const char *  identifier)
{
	if (scope == NULL)
	{
		return NULL;
	}

	Physics * curPhysics = scope->firstPhysics;
	while (curPhysics != NULL)
	{
		if (!strcmp(curPhysics->identifier, identifier))
		{
			return curPhysics;
		}
		curPhysics = curPhysics->next;
	}

	return newtonPhysicsTablePhysicsForIdentifier(N, scope->parent, identifier);
}

// NoisySymbol *
// newtonSymbolTableSymbolForIdentifier(NoisyState *  N, NoisyScope *  scope, const char *  identifier)
// {
// 	/*
// 	 *	Recursion falls out when we reach root which has nil parent
// 	 */
// 	if (scope == NULL)
// 	{
// 		return NULL;
// 	}
// 
// 	/*
// 	 *	Search current and parent (not siblings or children)
// 	 */
// 	NoisySymbol *	p = scope->firstSymbol;
// 	while (p != NULL)
// 	{
// 		if (!strcmp(p->identifier, identifier))
// 		{
// 			return p;
// 		}
// 		p = p->next;
// 	}
// 
// 	return newtonSymbolTableSymbolForIdentifier(N, scope->parent, identifier);
// }
// 
// 
// NoisySymbol *
// newtonSymbolTableAddOrLookupSymbolForToken(NoisyState *  N, NoisyScope *  scope, NoisyToken *  token)
// {
// 	NoisySymbol *	newSymbol;
// 
// 	newSymbol = (NoisySymbol *)calloc(1, sizeof(NoisySymbol));
// 	if (newSymbol == NULL)
// 	{
// 		noisyFatal(N, Emalloc);
// 	}
// 
// 	newSymbol->identifier	= token->identifier;
// 	newSymbol->sourceInfo	= token->sourceInfo;
// 	newSymbol->scope	= scope;
// 
// 	/*
// 	 *	NOTE:	An extant definition might not exist.
// 	 */
// 	newSymbol->definition	= newtonSymbolTableSymbolForIdentifier(N, scope, token->identifier);
// 
// 	/*
// 	 *	NOTE:	Caller sets (1) intconst/etc. fields, (2) type, based on context.
// 	 *		Caller sets the typesig based on the parsed typeexpr for defns.
// 	 */
// 	if (scope->firstSymbol == NULL)
// 	{
// 		scope->firstSymbol = newSymbol;
// 	}
// 	else
// 	{
// 		NoisySymbol *	p = scope->firstSymbol;
// 		while (p->next != NULL)
// 		{
// 			p = p->next;
// 		}
// 		p->next = newSymbol;
// 	}
// 
// 	return newSymbol;
// }

NoisyScope *
newtonSymbolTableAllocScope(NoisyState *  N)
{
	NoisyScope *	newScope;

	newScope = (NoisyScope *)calloc(1, sizeof(NoisyScope));
	if (newScope == NULL)
	{
		noisyFatal(N, Emalloc);
	}

	return newScope;
}


NoisyScope *
newtonSymbolTableOpenScope(NoisyState *  N, NoisyScope *  scope, NoisyIrNode *  subTree)
{
	NoisyScope *	newScope = newtonSymbolTableAllocScope(N);

	newScope->parent = scope;
	newScope->begin = subTree->sourceInfo;
	scope->firstChild = newScope;

	return newScope;
}


void
newtonSymbolTableCloseScope(NoisyState *  N, NoisyScope *  scope, NoisyIrNode *  subTree)
{
	scope->end = subTree->sourceInfo;
}
