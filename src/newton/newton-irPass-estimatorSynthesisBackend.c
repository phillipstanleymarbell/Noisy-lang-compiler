/*
	Authored 2020. Orestis Kaparounakis.

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

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "version.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "noisy-parser.h"
#include "newton-parser.h"
#include "noisy-lexer.h"
#include "newton-lexer.h"
#include "common-irPass-helpers.h"
#include "common-lexers-helpers.h"
#include "common-irHelpers.h"
#include "common-symbolTable.h"
#include "newton-types.h"
#include "newton-symbolTable.h"

Invariant *
findInvariantByIdentifier(State *  N, const char *  identifier)
{
	Symbol *	invariantSym = commonSymbolTableSymbolForIdentifier(N, N->newtonIrTopScope, identifier);
	IrNode *	invariantNode = invariantSym->typeTree->irParent;

	/*
	 *	We are, in fact, comparing addresses here.
	 */
	Invariant *	invariant = N->invariantList;
	while (invariant != NULL && invariant->identifier != invariantNode->tokenString)
	{
		invariant = invariant->next;
	}

	if (invariant == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Process invariant identifier not found in input.\n");
		fatal(N, Esanity);		
	}
	
	return invariant;
}

void
irPassEstimatorSynthesisProcessInvariantList(State *  N)
{
	/*
	 *	Find the invariants  
	 */
	Invariant *	processInvariant = findInvariantByIdentifier(N, N->estimatorProcessModel);
	Invariant *	measureInvariant = findInvariantByIdentifier(N, N->estimatorMeasurementModel);
	processInvariant++;
	measureInvariant++;
	/*
	 *	Deduce the state variables from the process model invariant 
	 */

}

void
irPassEstimatorSynthesisBackend(State *  N)
{
    if (N->estimatorProcessModel == NULL || N->estimatorMeasurementModel == NULL) 
    {
        flexprint(N->Fe, N->Fm, N->Fperr, "Please specify the invariant identifiers for the process and measurement model.\n");
		fatal(N, Esanity);
    }

	// FILE *	rtlFile;

	irPassEstimatorSynthesisProcessInvariantList(N);	
	
}