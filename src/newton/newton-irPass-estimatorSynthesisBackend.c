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

/*
 *	Isolate factors of stateVariableSymbols found in ExpressionXSeq in 
 *	an IrNode subtree of additions. Essentially 1st-level factorisation.
 */
IrNode *
irPassEstimatorSynthesisIsolateSymbolFactors(State *  N, IrNode *  ExpressionXSeq, Symbol *  stateVariableSymbol)
{
	IrNode *  symbolFactors = genIrNode(N, kNewtonIrNodeType_PquantityExpression,
										NULL /* left child */,
										NULL /* right child */,
										NULL /* source info */
									);
	int nth = 0;
	IrNode *  foundIdentifierIrNode;
	foundIdentifierIrNode = findNthIrNodeOfType(N, ExpressionXSeq, kNewtonIrNodeType_Tidentifier, nth);
	while(foundIdentifierIrNode != NULL && foundIdentifierIrNode->symbol != stateVariableSymbol)
	{
		nth++;
		foundIdentifierIrNode = findNthIrNodeOfType(N, ExpressionXSeq, kNewtonIrNodeType_Tidentifier, nth);
	}
	
	if (foundIdentifierIrNode != NULL)
	{	
		/*
		 *	Get the identifier's quantityTerm
		 */
		while (foundIdentifierIrNode->type != kNewtonIrNodeType_PquantityTerm)
		{
			foundIdentifierIrNode = foundIdentifierIrNode->irParent;
		}
		addLeaf(N, symbolFactors, foundIdentifierIrNode);
		/*
		 *	Note that foundIdentifierIrNode still contains the identifier somewhere
		 */
		// TODO: Remove identifier from sub-tree
		// TODO: Constant propagation
	}
	

	return symbolFactors;	
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
	 *	PROCESS MODEL
	 */
	
	/*
	 *	Deduce the state variables from the process model invariant 
	 */
	if (processInvariant->constraints == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Process invariant is empty.");
		fatal(N, Esanity);		
	}

	flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tGenerated .c file from Newton\n */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n#include <stdlib.h>");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n#include <stdio.h>");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n#include <math.h>\n\n");

	IrNode *	constraintXSeq = NULL;
	Physics *	stateVariablePhysics = NULL;

	/*
	 *	Find state vector dimension (N)
	 */
	int stateDimension = 0;
	for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{	
		// IrNode *  leafLeftAST = constraintXSeq->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild;
		// TODO: For multidimensional signals:
		// stateVariablePhysics = newtonPhysicsTablePhysicsForIdentifier(N, N->newtonIrTopScope, leafLeftAST->physics->identifier);
		// int subdimensionLength = newtonPhysicsLength(stateVariablePhysics);
		stateDimension++;
	}

	/*
	 *	Generate state variable names
	 */
	char * stateVariableNames[stateDimension];
	Symbol * stateVariableSymbols[stateDimension];

	int counter = 0;
	for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; counter++, constraintXSeq = constraintXSeq->irRightChild)
	{	
		IrNode *  leafLeftAST = constraintXSeq->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild;
		
		int	needed = snprintf(NULL, 0, "STATE_%s_%d", leafLeftAST->tokenString, 0) + 1;
		stateVariableNames[counter] = malloc(needed);
		snprintf(stateVariableNames[counter], needed, "STATE_%s_%d", leafLeftAST->tokenString, 0);

		stateVariableSymbols[counter] = leafLeftAST->symbol;
	}

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
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t *\tState\n\t */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble S[STATE_DIMENSION];\n\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t *\tState covariance matrix\n\t */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble P[STATE_DIMENSION][STATE_DIMENSION];\n");
	// TODO: Populate covariance matrix?
	flexprint(N->Fe, N->Fm, N->Fpc, "} CoreState;\n\n");

	/*	If linear:
	 *	Deduce state transition matrix F
	 */
	IrNode * fMatrixIrNodes[stateDimension][stateDimension];
	int fRow = 0;
	flexprint(N->Fe, N->Fm, N->Fpc, "double fmatrix[STATE_DIMENSION][STATE_DIMENSION] = \n");
	flexprint(N->Fe, N->Fm, N->Fpc, "{ ");
	for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; fRow++, constraintXSeq = constraintXSeq->irRightChild)
	{	
		/*
		 *	Find the Right Hand Side Expression of the constraint:
		 *	RHSExpression = constraintlist-> constraint -> right hand side -> bypass comparison -> quantity expression
		 */
		IrNode *  RHSExpressionXSeq = constraintXSeq->irLeftChild->irRightChild->irRightChild->irLeftChild;
		// flexprint(N->Fe, N->Fm, N->Fpc, "//\tFrom expression:\n");
		// irPassCConstraintTreeWalk(N, RHSExpressionXSeq);
		// flexprint(N->Fe, N->Fm, N->Fpc, "\n//\tGenerated:\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "{ ");
		for (int fColumn = 0; fColumn < stateDimension; fColumn++)
		{	
			fMatrixIrNodes[fRow][fColumn] = irPassEstimatorSynthesisIsolateSymbolFactors(N, RHSExpressionXSeq, stateVariableSymbols[fColumn]);
			if (fMatrixIrNodes[fRow][fColumn]->irRightChild == NULL &&
				fMatrixIrNodes[fRow][fColumn]->irLeftChild == NULL) {
				flexprint(N->Fe, N->Fm, N->Fpc, "( 0 ), ");
			}
			else
			{
				irPassCConstraintTreeWalk(N, fMatrixIrNodes[fRow][fColumn]);				
				flexprint(N->Fe, N->Fm, N->Fpc, ", ");
			}
			
		}
		
		flexprint(N->Fe, N->Fm, N->Fpc, "},\n");
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "};\n");


	/*	If non-linear:
	 *	Generate process model functions
	 */
	counter = 0;
	for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; counter++, constraintXSeq = constraintXSeq->irRightChild)
	{	
		flexprint(N->Fe, N->Fm, N->Fpc, "double\nprocess_%s ", stateVariableNames[counter]);
		irPassCGenFunctionArgument(N, constraintXSeq->irLeftChild, false);
		flexprint(N->Fe, N->Fm, N->Fpc, "\n");
		irPassCGenFunctionBody(N, constraintXSeq->irLeftChild, false);
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

	/*
	 *	MEASUREMENT MODEL
	 */

	if (measureInvariant->constraints == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Measurement invariant is empty. Unobservable system.");
		fatal(N, Esanity);		
	}
	constraintXSeq = NULL;
	stateVariablePhysics = NULL;
	/*
	 *	Find measurement vector dimension (Z)
	 */
	int measureDimension = 0;
	for (constraintXSeq = measureInvariant->constraints; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{	
		measureDimension++;
	}

	/**
	 *	Map stateVariableSymbols to measurementSymbols
	 */
	Symbol * measurementSymbols[stateDimension];
	counter = 0;
	for (IrNode * parameterXSeq = measureInvariant->parameterList; parameterXSeq != NULL; parameterXSeq = parameterXSeq->irRightChild)
	{
			IrNode *  parameterIdentifier = parameterXSeq->irLeftChild->irLeftChild;
			for (int i = 0; i < stateDimension; i++)
			{
				if (strcmp(parameterIdentifier->symbol->identifier, stateVariableSymbols[i]->identifier) == 0)
				{
					measurementSymbols[i] = parameterIdentifier->symbol;
					counter++;
					break;
				}
				
			}
	}

	/*	
	 *	If linear:
	 *	Deduce measurement matrix H 
	 */
	IrNode * hMatrixIrNodes[measureDimension][stateDimension];
	fRow = 0;
	flexprint(N->Fe, N->Fm, N->Fpc, "double hMatrix[%d][STATE_DIMENSION] = \n", measureDimension);
	flexprint(N->Fe, N->Fm, N->Fpc, "{ ");
	for (constraintXSeq = measureInvariant->constraints; constraintXSeq != NULL; fRow++, constraintXSeq = constraintXSeq->irRightChild)
	{	
		/*
		 *	Find the Right Hand Side Expression of the constraint:
		 *	RHSExpression = constraintlist-> constraint -> right hand side -> bypass comparison -> quantity expression
		 */
		IrNode *  RHSExpressionXSeq = constraintXSeq->irLeftChild->irRightChild->irRightChild->irLeftChild;
		flexprint(N->Fe, N->Fm, N->Fpc, "{ ");
		for (int fColumn = 0; fColumn < stateDimension; fColumn++)
		{	


			hMatrixIrNodes[fRow][fColumn] = irPassEstimatorSynthesisIsolateSymbolFactors(N, RHSExpressionXSeq, measurementSymbols[fColumn]);
			if (hMatrixIrNodes[fRow][fColumn]->irRightChild == NULL &&
				hMatrixIrNodes[fRow][fColumn]->irLeftChild == NULL) {
				flexprint(N->Fe, N->Fm, N->Fpc, "( 0 ), ");
			}
			else
			{
				irPassCConstraintTreeWalk(N, hMatrixIrNodes[fRow][fColumn]);				
				flexprint(N->Fe, N->Fm, N->Fpc, ", ");
			}
			
		}
		
		flexprint(N->Fe, N->Fm, N->Fpc, "},\n");
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "};\n");

	
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\nint \nmain(int argc, char *argv[])\n{\n\treturn 0;\n}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n/*\n *\tEnd of the generated .c file\n */\n");
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