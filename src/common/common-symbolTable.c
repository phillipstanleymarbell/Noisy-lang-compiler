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
#include "common-symbolTable.h"


/*
 *	See issue #295.
 */

Scope *
commonSymbolTableAllocScope(State *  N)
{
//	TimeStampTraceMacro(kNoisyTimeStampKeySymbolTableAllocScope);

	Scope *	newScope;

	newScope = (Scope *)calloc(1, sizeof(Scope));
	if (newScope == NULL)
	{
		fatal(N, Emalloc);
	}

	return newScope;
}

Symbol *
commonSymbolTableAddOrLookupSymbolForToken(State *  N, Scope *  scope, Token *  token)
{
//	TimeStampTraceMacro(kNoisyTimeStampKeySymbolTableAddOrLookupSymbolForToken);

	Symbol *	newSymbol;

	newSymbol = (Symbol *)calloc(1, sizeof(Symbol));
	if (newSymbol == NULL)
	{
		fatal(N, Emalloc);
	}

	newSymbol->identifier	= token->identifier;
	newSymbol->sourceInfo	= token->sourceInfo;
	newSymbol->scope	= scope;

	/*
	 *	NOTE:	An extant definition might not exist.
	 */
	newSymbol->definition	= commonSymbolTableSymbolForIdentifier(N, scope, token->identifier);

	/*
	 *	NOTE:	Caller sets (1) intconst/etc. fields, (2) type, based on context.
	 *		Caller sets the typesig based on the parsed typeexpr for defns.
	 */
	if (scope->firstSymbol == NULL)
	{
		scope->firstSymbol = newSymbol;
	}
	else
	{
		Symbol *	p = scope->firstSymbol;
		while (p->next != NULL)
		{
			p = p->next;
		}
		p->next = newSymbol;
	}

	return newSymbol;
}


Symbol *
commonSymbolTableSymbolForIdentifier(State *  N, Scope *  scope, const char *  identifier)
{
//	TimeStampTraceMacro(kNoisyTimeStampKeySymbolTableSymbolForIdentifier);

	/*
	 *	Recursion falls out when we reach root which has nil parent,
	 *	and this is the point at which we check the module scopes:
	 */
	if (scope == NULL)
	{
		Scope *	moduleScope = N->moduleScopes;

		// while (moduleScope != NULL)
		// {
		if (moduleScope != NULL)
		{
			Symbol *	moduleSym = moduleScope->firstSymbol;

			while (moduleSym != NULL)
			{
				if (!strcmp(moduleSym->identifier, identifier))
				{
					return moduleSym;
				}
				moduleSym = moduleSym->next;
			}
		}
			// moduleScope = moduleScope->next;
		// }

		return NULL;
	}

	/*
	 *	Search current and parent (not siblings or children)
	 */
	Symbol *	sym = scope->firstSymbol;
	while (sym != NULL)
	{
		if (!strcmp(sym->identifier, identifier))
		{
			return sym;
		}
		sym = sym->next;
	}

	return commonSymbolTableSymbolForIdentifier(N, scope->parent, identifier);
}


Scope *
commonSymbolTableOpenScope(State *  N, Scope *  scope, IrNode *  subTree)
{
//	TimeStampTraceMacro(kNoisyTimeStampKeySymbolTableOpenScope);

	Scope *		newScope = commonSymbolTableAllocScope(N);
	Scope *		siblingScope = scope->firstChild;

	newScope->parent = scope;
	newScope->begin = subTree->sourceInfo;
	newScope->next = NULL;

	if (siblingScope == NULL)
	{
		/*
		 *	Parent has no other children
		 */
		siblingScope = newScope;
		newScope->prev = NULL;
	}
	else
	{
		/*
		 *	Find last sibling (if any)
		 */
		while (siblingScope->next != NULL)
		{
			siblingScope = siblingScope->next;
		}		
		siblingScope->next = newScope;
		newScope->prev = siblingScope;
	}

	return newScope;
}

Scope *
commonSymbolTableGetScopeWithName(State * N, Scope * scope, const char * identifier)
{
	Scope * iter = scope;
	while (iter != NULL)
	{
		if (!strcmp(iter->identifier,identifier))
		{
			return iter;
		}
		iter = iter->next;
	}
	return NULL;
}

void
commonSymbolTableCloseScope(State *  N, Scope *  scope, IrNode *  subTree)
{
//	TimeStampTraceMacro(kNoisyTimeStampKeySymbolTableCloseScope);

	scope->end = subTree->sourceInfo;
}
