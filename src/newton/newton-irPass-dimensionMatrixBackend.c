/*
	Authored 2018. Phillip Stanley-Marbell. To be extended by Vlad-Mihai Mandric... (Vlad: please clean up this comment and add your name to the authors list when you get here --- Phillip)
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
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "newton-symbolTable.h"
#include "common-irPass-helpers.h"
#include "common-astTransform.h"
#include "newton-irPass-dotBackend.h"
#include "newton-irPass-dimensionMatrixBackend.h"
#include "newton-types.h"
#include "newton.h"

/*
 *	This is a template for Vlad to build on. See https://github.com/phillipstanleymarbell/Noisy-lang-compiler/pull/301#issuecomment-409131120
 */

extern char *	gNewtonAstNodeStrings[];

static void
irPassDimensionMatrixProcessInvariantList(State *  N)
{
	Invariant *	invariant = N->invariantList;

	while (invariant)
	{
		fprintf(stderr, "invariant: [%s]\n", invariant->identifier);
		
		IrNode *	parameter = invariant->parameterList;
		while (parameter)
		{
			if (parameter->irLeftChild && parameter->irLeftChild->physics)
			{
				fprintf(stderr, "\tParameter: [%s]\n", parameter->irLeftChild->physics->identifier);
				fprintf(stderr, "\tDimensions:\n");

				Dimension *	dimension = parameter->irLeftChild->physics->dimensions;
				while (dimension)
				{
					fprintf(stderr, "\t\t%s^%1.f:\n", dimension->identifier, dimension->exponent);
					dimension = dimension->next;
				}

				fprintf(stderr, "\n");
			}
			parameter = parameter->irRightChild;
		}
		invariant = invariant->next;
	}

	return;
}


void
irPassDimensionMatrixBackend(State *  N)
{
	irPassDimensionMatrixProcessInvariantList(N);

	return;
}
