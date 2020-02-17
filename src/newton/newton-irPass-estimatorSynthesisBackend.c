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
#include "newton-irPass-cBackend.h"

Invariant *
findInvariantByIdentifier(State *  N, const char *  identifier)
{
	Symbol *	invariantSym = commonSymbolTableSymbolForIdentifier(N, N->newtonIrTopScope, identifier);
	IrNode *	invariantNode = invariantSym->typeTree->irParent;

	/*
	 *	We are, in fact, comparing addresses here.
	 */
	Invariant *	invariant = N->invariantList;
	while (invariant != NULL && invariant->identifier != invariantNode->irLeftChild->tokenString)
	{
		invariant = invariant->next;
	}

	if (invariant == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Invariant with identifier \"%s\" not found in input files.\n", identifier);
		fatal(N, Esanity);
	}
	
	return invariant;
}

/*
 *	Finds the number of the subdimensions by counting how many 
 *	consecutive Physics notes have the same identifier.
 */
int
newtonPhysicsLength(Physics *  physics)
{
	int length = 0;
	char *	identifier = physics->identifier;
	while (physics && physics->identifier == identifier)
	{
		length++;
		physics = physics->next;
	}
	return length;
}


void
irPassEstSynthCollectUsedIdentifiers(State *  N, IrNode *  root)
{
	if (root == NULL)
	{
		return;
	}

	if (root->type == kNewtonIrNodeType_Tidentifier)
	{
		// TODO: Record usage
		return;
	}

	irPassEstSynthCollectUsedIdentifiers(N, root->irLeftChild);
	irPassEstSynthCollectUsedIdentifiers(N, root->irRightChild);

	return;
}

void
irPassEstimatorSynthesisProcessInvariantList(State *  N)
{
	/*
	 *	Check if the file called is simply an include.nt
	 */
	if (N->invariantList == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fprtl, "/*\n *\tPlease specify a valid file\n */\n");
		fatal(N, Esanity);
	}

	/*
	 *	Find the invariants  
	 */
	Invariant *	processInvariant = findInvariantByIdentifier(N, N->estimatorProcessModel);
	Invariant *	measureInvariant = findInvariantByIdentifier(N, N->estimatorMeasurementModel);
	
	/*
	 *	Deduce the state variables from the process model invariant 
	 */
	if (processInvariant->constraints == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Process invariant is empty.");
		fatal(N, Esanity);		
	}

	IrNode *	constraintXSeq = NULL;
	Physics *	stateVariablePhysics = NULL;

	/*
	 *	Find state vector dimension
	 */
	int stateDimension = 0;
	for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{	
		IrNode *  leafLeftAST = constraintXSeq->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild;
		stateVariablePhysics = newtonPhysicsTablePhysicsForIdentifier(N, N->newtonIrTopScope, leafLeftAST->physics->identifier);
		stateDimension += newtonPhysicsLength(stateVariablePhysics);
	}

	/*
	 *	Generate state variable names
	 */
	char * stateVariableNames[stateDimension];
	int counter = 0;
	for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{	
		IrNode *  leafLeftAST = constraintXSeq->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild;
		stateVariablePhysics = newtonPhysicsTablePhysicsForIdentifier(N, N->newtonIrTopScope, leafLeftAST->physics->identifier);
		int subdimensionLength = newtonPhysicsLength(stateVariablePhysics);

		for (int i = 0; i < subdimensionLength; i++)
		{
			int	needed = snprintf(NULL, 0, "STATE_%s_%d", leafLeftAST->physics->identifier, i) + 1;
			stateVariableNames[counter+i] = malloc(needed);
			snprintf(stateVariableNames[counter+i], needed, "STATE_%s_%d", leafLeftAST->physics->identifier, i);
		}
		counter += subdimensionLength;
	}

	flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tGenerated .c file from Newton\n */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n#include <stdlib.h>");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n#include <stdio.h>");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n#include <math.h>\n\n");

	/*
	 *	Generate state indexing enumerator
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\ntypedef enum\n{\n");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s,\n", stateVariableNames[i]);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\tSTATE_DIMENSION\n} filterCoreStateIdx;\n\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "typedef struct {\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t*\tState\n\t*/\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble S[STATE_DIMENSION];\n\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t*\tState covariance matrix\n\t*/\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble P[STATE_DIMENSION][STATE_DIMENSION];\n");
	// TODO: Populate covariance matrix?
	flexprint(N->Fe, N->Fm, N->Fpc, "} CoreState;\n\n");

	/*
	 *	Generate process model functions
	 */
	counter = 0;
	for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{	
		IrNode *  leafLeftAST = constraintXSeq->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild;
		stateVariablePhysics = newtonPhysicsTablePhysicsForIdentifier(N, N->newtonIrTopScope, leafLeftAST->physics->identifier);
		int subdimensionLength = newtonPhysicsLength(stateVariablePhysics);

		for (int i = 0; i < subdimensionLength; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "double\nprocess_%s ", stateVariableNames[counter+i]);
			irPassCGenFunctionArgument(N, constraintXSeq->irLeftChild, false);
			flexprint(N->Fe, N->Fm, N->Fpc, "\n");
			irPassCGenFunctionBody(N, constraintXSeq->irLeftChild, false);
		}
		counter += subdimensionLength;
	}

	/*
	 *	Generate init function
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterInit (CoreState * cState, double S0[STATE_DIMENSION], double P0[STATE_DIMENSION][STATE_DIMENSION]) {\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tfor (int i = 0; i < STATE_DIMENSION; i++)\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tcState->S[i] = S0[i];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tfor (int i = 0; i < STATE_DIMENSION; i++)\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tfor (int j = 0; j < STATE_DIMENSION; j++)\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t\tcState->P[i][j] = P0[i][j];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "}\n");

	/*
	 *	Generate predict function
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterPredict (double step) {\n");
	counter = 0;
	for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{	
		IrNode *  leafLeftAST = constraintXSeq->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild;
		stateVariablePhysics = newtonPhysicsTablePhysicsForIdentifier(N, N->newtonIrTopScope, leafLeftAST->physics->identifier);
		int subdimensionLength = newtonPhysicsLength(stateVariablePhysics);

		for (int i = 0; i < subdimensionLength; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\tprocess_%s", stateVariableNames[counter+i]);
			// TODO: This is where I need the relation matrix.
			flexprint(N->Fe, N->Fm, N->Fpc, ";\n");
		}
		counter += subdimensionLength;
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "}\n");

	
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\nint \nmain(int argc, char *argv[])\n{\n\treturn 0;\n}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n/*\n *\tEnd of the generated .c file\n */\n");
	
	/// DEBUG CODE to be able to compile with -Wunused-variable
	measureInvariant++;
}

void
irPassEstimatorSynthesisBackend(State *  N)
{
    if (N->estimatorProcessModel == NULL || N->estimatorMeasurementModel == NULL) 
    {
        flexprint(N->Fe, N->Fm, N->Fperr, "Please specify the invariant identifiers for the process and measurement model.\n");
		fatal(N, Esanity);
    }

	FILE *	apiFile;

	irPassEstimatorSynthesisProcessInvariantList(N);

	if (N->outputEstimatorSynthesisFilePath) 
	{
		apiFile = fopen(N->outputEstimatorSynthesisFilePath, "w"); 

		if (apiFile == NULL)
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\n%s: %s.\n", Eopen, N->outputEstimatorSynthesisFilePath);
			consolePrintBuffers(N);
		}

		fprintf(apiFile, "%s", N->Fpc->circbuf);
		fclose(apiFile);
	}
	
}