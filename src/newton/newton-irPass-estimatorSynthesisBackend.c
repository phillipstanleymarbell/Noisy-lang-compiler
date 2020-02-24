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

	This code makes the following assumptions for ease of implementation:
	- No signals with subdimensions.
	- Model must by factorised (e.g. 4*P+X and not 3*P+X+P).
	- LHS of invariants is assumed to contain solely the state identifiers.
	- System takes no input (B=0).
	
	Many of these can and will be lifted as development progresses.

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

bool
irPassEstimatorSynthesisExpressionLinear(State *  N, IrNode *  expressionXSeq) {
	bool  stillLinear = true;

	switch (expressionXSeq->type)
	{
		case kNewtonIrNodeType_PquantityExpression:
		{	
			while (stillLinear && expressionXSeq != NULL)
			{
				stillLinear &= irPassEstimatorSynthesisExpressionLinear(N, expressionXSeq->irLeftChild);
				expressionXSeq = expressionXSeq->irRightChild;
			}
			return stillLinear;
		} break;

		case kNewtonIrNodeType_PquantityTerm:
		{
			if (countIrNodeOfType(N, expressionXSeq, kNewtonIrNodeType_PexponentiationOperator) > 0	||
			    countIrNodeOfType(N, expressionXSeq, kNewtonIrNodeType_Texponentiation) > 0	||
				countIrNodeOfType(N, expressionXSeq, kNewtonIrNodeType_TExponential) > 0 ||
				countIrNodeOfType(N, expressionXSeq, kNewtonIrNodeType_Tidentifier) > 2)
			{
				return false;
			}
			// TODO: Also check for transcendentals
			// TODO: kNewtonIrNodeType_Tidentifier > 1 is linear ONLY for identifier*step
		} break;
	
		default:
			break;
	}

	return true;	
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
	int timesDetected = 0;
	IrNode *  foundMatchingIrNode = NULL;
	IrNode *  foundIdentifierIrNode = NULL;
	foundIdentifierIrNode = findNthIrNodeOfType(N, ExpressionXSeq, kNewtonIrNodeType_Tidentifier, nth);
	while(foundIdentifierIrNode != NULL)
	{
		if (foundIdentifierIrNode->symbol == stateVariableSymbol)
		{
			timesDetected++;
			foundMatchingIrNode = foundIdentifierIrNode;
		}
		nth++;
		foundIdentifierIrNode = findNthIrNodeOfType(N, ExpressionXSeq, kNewtonIrNodeType_Tidentifier, nth);
	}
	if (timesDetected > 1) {
		// TODO: Actually handle factorisation
		flexprint(N->Fe, N->Fm, N->Fperr, "Warning: Symbol %s detected in multiple terms.\n", stateVariableSymbol->identifier);
	}
	
	if (foundMatchingIrNode != NULL)
	{	
		/*
		 *	Remove identifier from sub-tree
		 *	Hack: Substitute with identity element
		 */
		foundMatchingIrNode->type = kNewtonIrNodeType_TintegerConst;
		foundMatchingIrNode->integerValue = 1;
		/*
		 *	Get the identifier's quantityTerm
		 */
		while (foundMatchingIrNode->type != kNewtonIrNodeType_PquantityTerm)
		{
			foundMatchingIrNode = foundMatchingIrNode->irParent;
		}
		addLeaf(N, symbolFactors, foundMatchingIrNode);
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
		flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tPlease specify a valid input file\n */\n");
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
	flexprint(N->Fe, N->Fm, N->Fpc, "#include <stdlib.h>\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "#include <stdio.h>\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "#include <math.h>\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "#include \"matrix.h\"\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

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
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble S[STATE_DIMENSION];\n\tmatrix *\tSm;\n\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t *\tState covariance matrix\n\t */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble P[STATE_DIMENSION][STATE_DIMENSION];\n\tmatrix *\tPm;\n");
	// TODO: Populate covariance matrix?
	flexprint(N->Fe, N->Fm, N->Fpc, "} CoreState;\n\n");

	/*
	 *	Generate filterInit function
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterInit (CoreState * cState, double S0[STATE_DIMENSION], double P0[STATE_DIMENSION][STATE_DIMENSION]) \n{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tfor (int i = %s; i < STATE_DIMENSION; i++)\n", stateVariableNames[0]);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tcState->S[i] = S0[i];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "cState->Sm = makeMatrix(1, STATE_DIMENSION);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "cState->Sm->data = &cState->S[0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\tfor (int i = %s; i < STATE_DIMENSION; i++)\n", stateVariableNames[0]);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tfor (int j = %s; j < STATE_DIMENSION; j++)\n", stateVariableNames[0]);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t\tcState->P[i][j] = P0[i][j];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "cState->Pm = makeMatrix(STATE_DIMENSION, STATE_DIMENSION);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "cState->Pm->data = &cState->P[0][0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "}\n");


	/*
	 *	Generate filterPredict function
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterPredict (CoreState *  cState, double step) {\n");

	/*
	 *	Determine linearity of process model
	 */
	bool linearProcess = true;
	for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{
		linearProcess &= irPassEstimatorSynthesisExpressionLinear(N, constraintXSeq->irLeftChild->irRightChild->irRightChild->irLeftChild);
	}
	
	if (linearProcess == true)
	{
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

		/**
		 *	Declare matrix struct;
		 */
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix Fm = { .height = (int) STATE_DIMENSION, ");
		flexprint(N->Fe, N->Fm, N->Fpc, ".width = (int) STATE_DIMENSION, ");
		flexprint(N->Fe, N->Fm, N->Fpc, ".data = &fmatrix[0][0] };\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "matrix * FSm = multiplyMatrix(&Fm, cState->Sm);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "cState->Sm = copyMatrix(FSm);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix * Fm_T = transposeMatrix(&Fm);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix * FPm = multiplyMatrix(&Fm, cState->Pm);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix * FPFm_T = multiplyMatrix(FPm, Fm_T);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "double *  p = cState->Pm->data;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "double *fpf = FPFm_T->data;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "double *  q = Qm->data;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "for (int i = 0; i < STATE_DIMENSION*STATE_DIMENSION; i++)\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "{\n\t*p = *fpf + *q;\n\tp++;\n\tfpf++;\n\tq++;\n}\n");

	}
	else 
	{
		/*	If non-linear:
		*	Generate process model equations
		*/
		counter = 0;
		for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; counter++, constraintXSeq = constraintXSeq->irRightChild)
		{	
			flexprint(N->Fe, N->Fm, N->Fpc, "double\nprocess_%s ", stateVariableNames[counter]);
			irPassCGenFunctionArgument(N, constraintXSeq->irLeftChild, false);
			flexprint(N->Fe, N->Fm, N->Fpc, "\n");
			irPassCGenFunctionBody(N, constraintXSeq->irLeftChild, false);
		}
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

	/*
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
	 *	Determine linearity of measurement model
	 */
	bool linearMeasurement = true;
	for (constraintXSeq = measureInvariant->constraints; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{
		linearMeasurement &= irPassEstimatorSynthesisExpressionLinear(N, constraintXSeq->irLeftChild->irRightChild->irRightChild->irLeftChild);
	}

	flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterUpdate (CoreState *  cState, double Z[%d], double step) {\n", measureDimension);
	if (linearMeasurement == true)
	{
		/*	
		*	If linear:
		*	Deduce measurement matrix H 
		*/
		IrNode * hMatrixIrNodes[measureDimension][stateDimension];
		flexprint(N->Fe, N->Fm, N->Fpc, "double hMatrix[%d][STATE_DIMENSION] = \n", measureDimension);
		flexprint(N->Fe, N->Fm, N->Fpc, "{ ");
		int hRow = 0;
		for (constraintXSeq = measureInvariant->constraints; constraintXSeq != NULL; hRow++, constraintXSeq = constraintXSeq->irRightChild)
		{	
			/*
			*	Find the Right Hand Side Expression of the constraint:
			*	RHSExpression = constraintlist-> constraint -> right hand side -> bypass comparison -> quantity expression
			*/
			IrNode *  RHSExpressionXSeq = constraintXSeq->irLeftChild->irRightChild->irRightChild->irLeftChild;
			flexprint(N->Fe, N->Fm, N->Fpc, "{ ");
			for (int hColumn = 0; hColumn < stateDimension; hColumn++)
			{	


				hMatrixIrNodes[hRow][hColumn] = irPassEstimatorSynthesisIsolateSymbolFactors(N, RHSExpressionXSeq, measurementSymbols[hColumn]);
				if (hMatrixIrNodes[hRow][hColumn]->irRightChild == NULL &&
					hMatrixIrNodes[hRow][hColumn]->irLeftChild == NULL) {
					flexprint(N->Fe, N->Fm, N->Fpc, "( 0 ), ");
				}
				else
				{
					irPassCConstraintTreeWalk(N, hMatrixIrNodes[hRow][hColumn]);				
					flexprint(N->Fe, N->Fm, N->Fpc, ", ");
				}
			
			}

			flexprint(N->Fe, N->Fm, N->Fpc, "},\n");
		}
		flexprint(N->Fe, N->Fm, N->Fpc, "};\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "matrix Hm = { .height = %d, .width = STATE_DIMENSION, .data = &hMatrix[0][0] };\n", measureDimension);
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix Zm = { .height = %d, .width = 1, .data = Z };\n", measureDimension);
		flexprint(N->Fe, N->Fm, N->Fpc, "\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "// Kg = PH^T * (HPH^T + Q)^(-1)\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  Hm_T = transposeMatrix(&Hm);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  PHm_T = multiplyMatrix(cState->Pm, Hm_T);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  HPHm_T = multiplyMatrix(&Hm, PHm_T);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "double *hph = HPHm_T->data;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "double *r = Rm->data;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "for (int i = 0; i < %d * %d; i++)\n{\n", measureDimension, measureDimension);
		flexprint(N->Fe, N->Fm, N->Fpc, "*hph += *r;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "hph++;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "r++;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "}\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  HPHm_T_inv = matrixInverse(HPHm_T);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  Kg = multiplyMatrix(PHm_T, HPHm_T_inv);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "// S <- S + Kg (Z - HS)\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  HSm = multiplyMatrix(&Hm, cState->Sm);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "double *  hs = HSm->data;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "double *  z = &Z[0];\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "for (int i = 0; i < 3; i++)\n{\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "*hs = *z - *hs;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "hs++;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "z++;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "}\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix * KgZHS = multiplyMatrix(Kg, HSm);\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "double *  s = &cState->S[0];\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "double *  kgzhs = KgZHS->data;\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "for (int i = 0; i < (int) STATE_DIMENSION; i++)\n{\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "*s += *kgzhs;\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "s++;\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "kgzhs++;\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "}\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "\n");	

		flexprint(N->Fe, N->Fm, N->Fpc, "// P <- P - KgHP\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  HPm = multiplyMatrix(&Hm, cState->Pm);\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  KgHPm = multiplyMatrix(Kg, HPm);\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "double *  p = &cState->P[0][0];\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "double *  kghp = KgHPm->data;\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "for (int i = 0; i < STATE_DIMENSION*STATE_DIMENSION; i++)\n{\n");	
		flexprint(N->Fe, N->Fm, N->Fpc, "*p -= *kghp;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "p++;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "kghp++;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "}\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "}\n");
	}
	else
	{
		
	}
	
	
	/*
	 *	Generate main for testing purposes
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\nint \nmain(int argc, char *argv[])\n{\n\treturn 0;\n}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "int noOfValues = 1 + STATE_DIMENSION + MEASURE_DIMENSION;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "int nread;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "size_t nlen = 0;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "char *  line;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "CoreState cs;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "// First line is column names\n");
	// flexprint(N->Fe, N->Fm, N->Fpc, "nread = getline(&line, &nlen, stdin);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "double initState[%d];\n", measureDimension);
	flexprint(N->Fe, N->Fm, N->Fpc, "double time;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "scanf(\"%%lf\", &time);\n");
	for (int i = 0; i < measureDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "scanf(\",%%lf\", &initState[%d]);\n", i);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "double initCov[STATE_DIMENSION][STATE_DIMENSION] = ");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "{");
		for (int j = 0; j < stateDimension; j++)
		{
			if (i == j)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "100,");
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "1,");
			}		
		}
		flexprint(N->Fe, N->Fm, N->Fpc, "},");		
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "};\n");
	
	flexprint(N->Fe, N->Fm, N->Fpc, "filterInit(&cs, initState, initCov);\n");


	flexprint(N->Fe, N->Fm, N->Fpc, "double prevtime = time;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "double measure[%d];\n", stateDimension);
	flexprint(N->Fe, N->Fm, N->Fpc, "while (scanf(\"%%lf\", &time) > 0)\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "{\n");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "scanf(\",%%*lf\");\n", i);
	}
	for (int i = 0; i < measureDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "scanf(\",%%lf\", &measure[%d]);\n", i);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "filterPredict(&cs, time - prevtime);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "printf(\"Predict: %%lf, time);\n");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "printf(\", %%lf\", cs.S[%d]);\n");
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "printf(\"\\n\");\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "filterUpdate(&cs, measure, time - prevtime);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "printf(\"Update: %%lf, time);\n");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "printf(\", %%lf\", cs.S[%d]);\n");
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "prevtime = time;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "return 0;\n}\n");
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