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
#include "noisyconfig.h"
#include "noisyconfig-errors.h"
#include "version.h"
#include "noisyconfig-symbolTable.h"


extern int primeNumbers[168];

/*
 *	TODO: need to tag scopes corresponding to progtypes with a name, so
 *	that when we want to lookup a progtype-qualified-name in symtab, we
 *	can use the "a" of "a->b" to lookup, and get the type structure of
 *	"b".  See, e.g., comments at P_TYPENAME in noisyConfig-irPass-cBackend.
 */

void 
noisyConfigPhysicsCopyDenominatorToNumeratorDimensions(NoisyConfigState * N, Physics * dest, Physics * source)
{
    if (source->denominatorDimensions == NULL)
        return;
    
    if (dest->numeratorDimensions == NULL) 
    {
        Dimension * tmpDestDimension = (Dimension*) calloc(1, sizeof(Dimension));
        dest->numeratorDimensions = (Dimension *) memcpy(tmpDestDimension, source->denominatorDimensions, sizeof(Dimension) * source->numberOfDenominators);
        dest->numeratorDimensions[source->numberOfDenominators-1].next = NULL;
        dest->numberOfNumerators = source->numberOfDenominators;
    }
    else
    {
        Dimension * curDestDimension = dest->numeratorDimensions;
        while (curDestDimension->next != NULL) {
            curDestDimension = curDestDimension->next;
        }
        
        Dimension * curSourceDimension = source->denominatorDimensions;
        while (curSourceDimension != NULL) {
            Dimension * tmpDestDimension = (Dimension *) calloc(1, sizeof(Dimension));
            memcpy(tmpDestDimension, curSourceDimension, sizeof(Dimension));
            curDestDimension->next = tmpDestDimension;
            curDestDimension->next->next = NULL;

            dest->numberOfNumerators++;
            
            curDestDimension = curDestDimension->next;
            curSourceDimension = curSourceDimension->next;
        }
    }
    
    dest->numeratorPrimeProduct *= source->denominatorPrimeProduct;
}

void 
noisyConfigPhysicsCopyNumeratorToDenominatorDimensions(NoisyConfigState * N, Physics * dest, Physics * source)
{
    if (source->numeratorDimensions == NULL)
        return;
    
    if (dest->denominatorDimensions == NULL) 
    {
        Dimension * tmpDestDimension = (Dimension*) calloc(1, sizeof(Dimension));
        dest->denominatorDimensions = (Dimension *) memcpy(tmpDestDimension, source->numeratorDimensions, sizeof(Dimension) * source->numberOfNumerators);
        dest->denominatorDimensions[source->numberOfNumerators-1].next = NULL;
        dest->numberOfDenominators = source->numberOfNumerators;
    }
    else
    {
        Dimension * curDestDimension = dest->denominatorDimensions;
        while (curDestDimension->next != NULL) {
            curDestDimension = curDestDimension->next;
        }
        
        Dimension * curSourceDimension = source->numeratorDimensions;
        while (curSourceDimension != NULL) {
            Dimension * tmpDestDimension = (Dimension *) calloc(1, sizeof(Dimension));
            memcpy(tmpDestDimension, curSourceDimension, sizeof(Dimension));
            curDestDimension->next = tmpDestDimension;
            curDestDimension->next->next = NULL;

            dest->numberOfDenominators++;
            
            curDestDimension = curDestDimension->next;
            curSourceDimension = curSourceDimension->next;
        }
    }
    
    dest->denominatorPrimeProduct *= source->numeratorPrimeProduct;

}

// TODO change method name from copy to transfer
void noisyConfigPhysicsCopyNumeratorDimensions(NoisyConfigState * N, Physics * dest, Physics * source) 
{
    if (source->numeratorDimensions == NULL)
        return;
    
    if (dest->numeratorDimensions == NULL) 
    {
        Dimension * tmpDestDimension = (Dimension*) calloc(1, sizeof(Dimension));
        dest->numeratorDimensions = (Dimension *) memcpy(tmpDestDimension, source->numeratorDimensions, sizeof(Dimension) * source->numberOfNumerators);
        dest->numeratorDimensions[source->numberOfNumerators-1].next = NULL;
    }
    else
    {
        Dimension * curDestDimension = dest->numeratorDimensions;
        while (curDestDimension->next != NULL) {
            curDestDimension = curDestDimension->next;
        }
        
        Dimension * curSourceDimension = source->numeratorDimensions;
        while (curSourceDimension != NULL) {
            Dimension * tmpDestDimension = (Dimension *) calloc(1, sizeof(Dimension));
            memcpy(tmpDestDimension, curSourceDimension, sizeof(Dimension));
            curDestDimension->next = tmpDestDimension;
            curDestDimension->next->next = NULL;
            
            curDestDimension = curDestDimension->next;
            curSourceDimension = curSourceDimension->next;
        }
    }
    
    dest->numberOfNumerators += source->numberOfNumerators;
    dest->numeratorPrimeProduct *= source->numeratorPrimeProduct;
}

void noisyConfigPhysicsCopyDenominatorDimensions(NoisyConfigState * N, Physics * dest, Physics * source)
{
    if (source->denominatorDimensions == NULL)
        return;
    
    if (dest->denominatorDimensions == NULL) 
    {
        Dimension * tmpDestDimension = (Dimension *) calloc(1, sizeof(Dimension));
        dest->denominatorDimensions = (Dimension *) memcpy(tmpDestDimension, source->denominatorDimensions, sizeof(Dimension) * source->numberOfDenominators); 
        dest->denominatorDimensions[source->numberOfDenominators-1].next = NULL;
    }
    else
    {
        Dimension * curDestDimension = dest->denominatorDimensions;
        while (curDestDimension->next != NULL) {
            curDestDimension = curDestDimension->next;
        }
        
        Dimension * curSourceDimension = source->denominatorDimensions;
        while (curSourceDimension != NULL) {
            Dimension * tmpDestDimension = (Dimension *) calloc(1, sizeof(Dimension));
            memcpy(tmpDestDimension, curSourceDimension, sizeof(Dimension));
            curDestDimension->next = tmpDestDimension;
            curDestDimension->next->next = NULL;

            curDestDimension = curDestDimension->next;
            curSourceDimension = curSourceDimension->next;
        }
    }
    
    dest->numberOfDenominators += source->numberOfDenominators;
    dest->denominatorPrimeProduct *= source->denominatorPrimeProduct;
}

void 
noisyConfigPhysicsAddNumeratorDimension(NoisyConfigState * N, Physics * physics, Dimension * numerator)
{
    if (physics->numeratorDimensions == NULL) {
        Dimension * tmpDestDimension = (Dimension *) calloc(1, sizeof(Dimension));
        memcpy(tmpDestDimension, numerator, sizeof(Dimension));
        physics->numeratorDimensions = tmpDestDimension;
        physics->numeratorDimensions->next = NULL;
    } else {
        Dimension * curDimension = physics->numeratorDimensions;
        while (curDimension->next != NULL) {
            
            curDimension = curDimension->next;
        }
        
        Dimension * tmpDestDimension = (Dimension *) calloc(1, sizeof(Dimension));
        memcpy(tmpDestDimension, numerator, sizeof(Dimension));
        curDimension->next = tmpDestDimension;
        curDimension->next->next = NULL;
    }
    physics->numeratorPrimeProduct *= numerator->primeNumber;
    physics->numberOfNumerators++;
}

void 
noisyConfigPhysicsAddDenominatorDimension(NoisyConfigState * N, Physics * physics, Dimension * denominator)
{
    if (physics->denominatorDimensions == NULL) {
        Dimension * tmpDestDimension = (Dimension *) calloc(1, sizeof(Dimension));
        memcpy(tmpDestDimension, denominator, sizeof(Dimension));
        physics->denominatorDimensions = tmpDestDimension;
        physics->denominatorDimensions->next = NULL;
    } else {
        Dimension * curDimension = physics->denominatorDimensions;
        while (curDimension->next != NULL) {
            
            curDimension = curDimension->next;
        }
        
        Dimension * tmpDestDimension = (Dimension *) calloc(1, sizeof(Dimension));
        memcpy(tmpDestDimension, denominator, sizeof(Dimension));
        curDimension->next = tmpDestDimension;
        curDimension->next->next = NULL;
    }
    physics->denominatorPrimeProduct *= denominator->primeNumber;
    physics->numberOfDenominators++;
}

Dimension *
noisyConfigDimensionTableAddDimensionForToken(NoisyConfigState *  N, NoisyConfigScope *  scope, NoisyConfigToken *  token)
{
	Dimension *	newDimension;

	newDimension = (Dimension *)calloc(1, sizeof(Dimension));
	if (newDimension == NULL)
	{
		noisyConfigFatal(N, Emalloc);
	}

	newDimension->identifier	= token->stringConst;
	newDimension->sourceInfo	= token->sourceInfo;
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
noisyConfigPhysicsTableAddPhysicsForToken(NoisyConfigState *  N, NoisyConfigScope *  scope, NoisyConfigToken *  token)
{
	Physics *	newPhysics;

	newPhysics = (Physics *)calloc(1, sizeof(Physics));
	if (newPhysics == NULL)
	{
		noisyConfigFatal(N, Emalloc);
	}

	newPhysics->identifier	= token->identifier;
	newPhysics->sourceInfo	= token->sourceInfo;
	newPhysics->scope	= scope;
    
    newPhysics->numeratorPrimeProduct = 1;
    newPhysics->denominatorPrimeProduct = 1;

	newPhysics->definition	= noisyConfigPhysicsTablePhysicsForIdentifier(N, scope, token->identifier);

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
noisyConfigDimensionTableDimensionForIdentifier(NoisyConfigState *  N, NoisyConfigScope *  scope, const char *  identifier)
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

	return noisyConfigDimensionTableDimensionForIdentifier(N, scope->parent, identifier);
}

Physics *
noisyConfigPhysicsTablePhysicsForIdentifier(NoisyConfigState *  N, NoisyConfigScope *  scope, const char *  identifier)
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

	return noisyConfigPhysicsTablePhysicsForIdentifier(N, scope->parent, identifier);
}

// NoisyConfigSymbol *
// noisyConfigSymbolTableSymbolForIdentifier(NoisyConfigState *  N, NoisyConfigScope *  scope, const char *  identifier)
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
// 	NoisyConfigSymbol *	p = scope->firstSymbol;
// 	while (p != NULL)
// 	{
// 		if (!strcmp(p->identifier, identifier))
// 		{
// 			return p;
// 		}
// 		p = p->next;
// 	}
// 
// 	return noisyConfigSymbolTableSymbolForIdentifier(N, scope->parent, identifier);
// }
// 
// 
// NoisyConfigSymbol *
// noisyConfigSymbolTableAddOrLookupSymbolForToken(NoisyConfigState *  N, NoisyConfigScope *  scope, NoisyConfigToken *  token)
// {
// 	NoisyConfigSymbol *	newSymbol;
// 
// 	newSymbol = (NoisyConfigSymbol *)calloc(1, sizeof(NoisyConfigSymbol));
// 	if (newSymbol == NULL)
// 	{
// 		noisyConfigFatal(N, Emalloc);
// 	}
// 
// 	newSymbol->identifier	= token->identifier;
// 	newSymbol->sourceInfo	= token->sourceInfo;
// 	newSymbol->scope	= scope;
// 
// 	/*
// 	 *	NOTE:	An extant definition might not exist.
// 	 */
// 	newSymbol->definition	= noisyConfigSymbolTableSymbolForIdentifier(N, scope, token->identifier);
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
// 		NoisyConfigSymbol *	p = scope->firstSymbol;
// 		while (p->next != NULL)
// 		{
// 			p = p->next;
// 		}
// 		p->next = newSymbol;
// 	}
// 
// 	return newSymbol;
// }

NoisyConfigScope *
noisyConfigSymbolTableAllocScope(NoisyConfigState *  N)
{
	NoisyConfigScope *	newScope;

	newScope = (NoisyConfigScope *)calloc(1, sizeof(NoisyConfigScope));
	if (newScope == NULL)
	{
		noisyConfigFatal(N, Emalloc);
	}

	return newScope;
}


NoisyConfigScope *
noisyConfigSymbolTableOpenScope(NoisyConfigState *  N, NoisyConfigScope *  scope, NoisyConfigIrNode *  subTree)
{
	NoisyConfigScope *	newScope = noisyConfigSymbolTableAllocScope(N);

	newScope->parent = scope;
	newScope->begin = subTree->sourceInfo;
	scope->firstChild = newScope;

	return newScope;
}


void
noisyConfigSymbolTableCloseScope(NoisyConfigState *  N, NoisyConfigScope *  scope, NoisyConfigIrNode *  subTree)
{
	scope->end = subTree->sourceInfo;
}
