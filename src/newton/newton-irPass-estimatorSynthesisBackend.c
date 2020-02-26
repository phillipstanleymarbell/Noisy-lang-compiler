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
	- Linear model must by factorised (e.g. 4*P+X and not 3*P+X+P).
	- LHS of invariants is assumed to contain solely the state and measurement identifiers.
	- System takes no input (B=0).
	- Invariant parameter lists start with the state variables, in the same order
	  with which they appear in the invariant body.
	- Measure invariant does not skip unused state variables.
	
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

/**
 *	Detect symbol appearance in expression.
 *	Return the symbol's identifier node in the expression.
 */
IrNode *
irPassEstimatorSynthesisDetectSymbol(State *  N, IrNode *  ExpressionXSeq, Symbol *  stateVariableSymbol)
{
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

	return foundMatchingIrNode;
}

/*
 *	Annotate expression tree's nodes' tokenString field.
 *	Each node (intermediate value) gets its own id token.
 *	Prefix supplied with parentTokenString.
 *	e.g. P*V + G*H*sin(G) + K, with root's tokenString="w",
 *  *should* look like:
 *	w
 *	├── w_1 ── w_1_1 ── w_1_1_1 ── P
 *	│	│
 *	│   └── w_1_2 ── w_1_2_1 ── V
 *	│
 *	├── w_2 ── w_2_1 ── w_2_1_1 ── G
 *	│	│
 *	│   ├── w_2_2 ── w_2_2_1 ── H
 *	│	│
 *	│   └── w_2_3 ── w_2_3_1 ── G
 *	│ 
 *	└── w_3 ── w_3_1 ── w_3_1_1 ── K
 */
void
irPassEstimatorSynthesisADAnnotate(State *  N, IrNode *  root, char * parentTokenString)
{
	int dUid = 1;

	switch (root->type)
	{
		case kNewtonIrNodeType_Pquantity:
		{
			/*
			*	Self-assign name for Static Single Assignment expression
			*/
			int	needed = snprintf(NULL, 0, "%s_%d", parentTokenString, dUid) + 1;
			root->tokenString = malloc(needed);
			snprintf(root->tokenString, needed, "%s_%d", parentTokenString, dUid++);
			break;
		}

		case kNewtonIrNodeType_PquantityFactor:
		{
			irPassEstimatorSynthesisADAnnotate(N, root->irLeftChild, root->tokenString);
			// flexprint(N->Fe, N->Fm, N->Fpc, "double %s = %s;\n", root->tokenString, root->irLeftChild->tokenString);
			// TODO: This can introduce another quantityExpression.
			// 		 e.g exp(), sin(), cos(), tan()
			break;
		} 

		case kNewtonIrNodeType_PquantityExpression:
		case kNewtonIrNodeType_PquantityTerm:
		{
			IrNode * currXSeq = NULL;
			dUid = 1;
			for (currXSeq = root; currXSeq != NULL; currXSeq = currXSeq->irRightChild)
			{
				if (currXSeq->irLeftChild->type == kNewtonIrNodeType_PquantityTerm ||
					currXSeq->irLeftChild->type == kNewtonIrNodeType_PquantityFactor)
				{
					/*
					 *	Assign each child a name for Static Single Assignment expression
					 */
					int	needed = snprintf(NULL, 0, "%s_%d", root->tokenString, dUid) + 1;
					currXSeq->irLeftChild->tokenString = malloc(needed);
					snprintf(currXSeq->irLeftChild->tokenString, needed, "%s_%d", root->tokenString, dUid++);
				}
			}

			for (currXSeq = root; currXSeq != NULL; currXSeq = currXSeq->irRightChild)
			{
				irPassEstimatorSynthesisADAnnotate(N, currXSeq->irLeftChild, root->tokenString);
			}
		}
		default:
			break;
	}
}

/*
 *	Generate Static Single Assignment (SSA) form of given expression
 *	based on existing annotations (node->tokenString).
 */
void
irPassEstimatorSynthesisADGenExpressionSSA(State *  N, IrNode *  root)
{
	switch (root->type)
	{
		case kNewtonIrNodeType_Pquantity:
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "double %s = %s;\n", root->tokenString, irPassCNodeToStr(N, root->irLeftChild));		
			break;
		}

		case kNewtonIrNodeType_PquantityFactor:
		{
			irPassEstimatorSynthesisADGenExpressionSSA(N, root->irLeftChild);
			flexprint(N->Fe, N->Fm, N->Fpc, "double %s = %s;\n", root->tokenString, root->irLeftChild->tokenString);
			// TODO: exp(), sin(), cos(), tan()
			break;
		}

		case kNewtonIrNodeType_PquantityExpression:
		case kNewtonIrNodeType_PquantityTerm:
		{
			IrNode *  currXSeq = NULL;

			for (currXSeq = root; currXSeq != NULL; currXSeq = currXSeq->irRightChild)
			{
				irPassEstimatorSynthesisADGenExpressionSSA(N, currXSeq->irLeftChild);
			}

			flexprint(N->Fe, N->Fm, N->Fpc, "double %s = ", root->tokenString);
			flexprint(N->Fe, N->Fm, N->Fpc, " %s ", root->irLeftChild->tokenString);
			for (currXSeq = root->irRightChild; currXSeq != NULL; currXSeq = currXSeq->irRightChild)
			{
					if (currXSeq->irLeftChild->type == kNewtonIrNodeType_PlowPrecedenceOperator)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "%s", irPassCNodeToStr(N, currXSeq->irLeftChild->irLeftChild));
					}
					else if (currXSeq->irLeftChild->type == kNewtonIrNodeType_PhighPrecedenceQuantityOperator)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "%s", irPassCNodeToStr(N, currXSeq->irLeftChild->irLeftChild->irLeftChild));
					}
					else
					{
						flexprint(N->Fe, N->Fm, N->Fpc, " %s ", currXSeq->irLeftChild->tokenString);					
					}
			}
			flexprint(N->Fe, N->Fm, N->Fpc, ";\n\n");
		}
		
		default:
			break;
	}
}

/*
 *	Generate reverse mode AutoDiff-aumented C code for given expression.
 */
void
irPassEstimatorSynthesisGenAutoDiffExpression(State *  N, IrNode *  ExpressionXSeq, Symbol *  wrtSymbols, char * parentTokenString)
{
	ExpressionXSeq->tokenString = "w";
	irPassEstimatorSynthesisADAnnotate(N, ExpressionXSeq, ExpressionXSeq->tokenString);
	irPassEstimatorSynthesisADGenExpressionSSA(N, ExpressionXSeq);
	return;
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
	 *	PRE-CODE-GEN WORK
	 */

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

	if (measureInvariant->constraints == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Measurement invariant is empty. Unobservable system.");
		fatal(N, Esanity);		
	}
	
	// AUTODIFF TESTBED START
	// ######################
	irPassEstimatorSynthesisGenAutoDiffExpression(N, processInvariant->constraints->irLeftChild->irRightChild->irRightChild->irLeftChild, NULL, "w");
	return;
	// ####################
	// AUTODIFF TESTBED END

	flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tGenerated .c file from Newton\n */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "#include <stdlib.h>\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "#include <stdio.h>\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "#include <math.h>\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "#include \"C-Linear-Algebra/matrix.h\"\n#include \"C-Linear-Algebra/matrixadv.h\"\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	IrNode *	constraintXSeq = NULL;
	// Physics *	stateVariablePhysics = NULL;

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

	constraintXSeq = NULL;
	// stateVariablePhysics = NULL;
	/*
	 *	Find measurement vector dimension (Z)
	 */
	int measureDimension = 0;
	for (constraintXSeq = measureInvariant->constraints; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{	
		measureDimension++;
	}

	/*
	 *	Generate state variable names and
	 *	book-keep corresponding symbols.
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
	 *	Generate measure variable names
	 */
	char * measureVariableNames[measureDimension];
	counter = 0;
	for (constraintXSeq = measureInvariant->constraints; constraintXSeq != NULL; counter++, constraintXSeq = constraintXSeq->irRightChild)
	{	
		IrNode *  leafLeftAST = constraintXSeq->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild;
		
		int	needed = snprintf(NULL, 0, "MEASURE_%s_%d", leafLeftAST->tokenString, 0) + 1;
		measureVariableNames[counter] = malloc(needed);
		snprintf(measureVariableNames[counter], needed, "MEASURE_%s_%d", leafLeftAST->tokenString, 0);
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

	/*
	 *	Generate measure indexing enumerator
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\ntypedef enum\n{\n");
	for (int i = 0; i < measureDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s,\n", measureVariableNames[i]);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\tMEASURE_DIMENSION\n} filterMeasureIdx;\n\n");

	/*
	 *	Generate core filter-state struct
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "typedef struct {\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t *\tState\n\t */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble S[STATE_DIMENSION];\n\tmatrix *\tSm;\n\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t *\tState covariance matrix\n\t */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble P[STATE_DIMENSION][STATE_DIMENSION];\n\tmatrix *\tPm;\n");
	// TODO: Populate covariance matrix?
	flexprint(N->Fe, N->Fm, N->Fpc, "} CoreState;\n\n");

	/*
	 *	GENERATE INIT FUNCTION
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
	 *	GENERATE PREDICT FUNCTION
	 */
	
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
		flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterPredict (CoreState *  cState, double step) {\n");
		/*	If linear:
		 *	Deduce state transition matrix F
		 */
		IrNode * fMatrixIrNodes[stateDimension][stateDimension];
		int fRow = 0;
		flexprint(N->Fe, N->Fm, N->Fpc, "double fMatrix[STATE_DIMENSION][STATE_DIMENSION] = \n");
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

	}
	else 
	{
		/*	
		 *	NON-LINEAR PREDICT
		 */

		/*
		 *	Find process invariant parameter list length
		 */
		int processParameterLength = 0;
		for (constraintXSeq = processInvariant->parameterList; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
		{
			processParameterLength++;
		}

		/*
		 *	Book-keep process parameter list symbols
		 */
		Symbol *  parameterVariableSymbols[processParameterLength];
		counter = 0;
		for (constraintXSeq = processInvariant->parameterList; constraintXSeq != NULL; counter++, constraintXSeq = constraintXSeq->irRightChild)
		{
			parameterVariableSymbols[counter] = constraintXSeq->irLeftChild->irLeftChild->symbol;
		}

		/*
		 *	Create relation matrix
		 *	Each symbol in stateVariableSymbols has its own row
		 *	relating it to the parameters of the invariant.
		 */
		bool relationMatrix[stateDimension][processParameterLength];
		counter = 0;
		for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; counter++, constraintXSeq = constraintXSeq->irRightChild)
		{
			IrNode *  RHSExpressionXSeq = constraintXSeq->irLeftChild->irRightChild->irRightChild->irLeftChild;
			for (int mColumn = 0; mColumn < processParameterLength; mColumn++)
			{
				/**
				 *	Check if symbol from parameter list appears in RHS expression
				 */
				if (irPassEstimatorSynthesisDetectSymbol(N, RHSExpressionXSeq, parameterVariableSymbols[mColumn]) != NULL)
				{
					relationMatrix[counter][mColumn] = true;
				}
				else
				{
					relationMatrix[counter][mColumn] = false;
				}
			}
		}

		/*
		 *	Generate single state variable prediction functions.
		 *  This is: f_1(s1,s2, ... s_j), ... f_i(x,y, ... s_j)
		 */
		counter = 0;
		int functionLastArg[stateDimension];
		for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; counter++, constraintXSeq = constraintXSeq->irRightChild)
		{	
			flexprint(N->Fe, N->Fm, N->Fpc, "double\nprocess_%s ", stateVariableNames[counter]);
			flexprint(N->Fe, N->Fm, N->Fpc, "(");

			int lastArg = 0;
			for (lastArg = processParameterLength-1; lastArg >= 0; lastArg--)
			{
				if (relationMatrix[counter][lastArg] == true)
				{
					functionLastArg[counter] = lastArg;
					break;
				} 	
			}

			int currArg = 0;
			for (currArg = 0; currArg < processParameterLength; currArg++)
			{
				if (relationMatrix[counter][currArg] == true)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "double %s", parameterVariableSymbols[currArg]->identifier);
					if (currArg != lastArg)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}
			flexprint(N->Fe, N->Fm, N->Fpc, ")\n");
			flexprint(N->Fe, N->Fm, N->Fpc, "\n");
			irPassCGenFunctionBody(N, constraintXSeq->irLeftChild, false);

			/**
			 *	Generate partial derivatives
			 */
			for (int currDeriv = 0; currDeriv < stateDimension; currDeriv++)
			{
				if (relationMatrix[counter][currDeriv] == true)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "double\nd_process_%s_d%s ", stateVariableNames[counter], stateVariableSymbols[currDeriv]->identifier);
					flexprint(N->Fe, N->Fm, N->Fpc, "(");
					for (currArg = 0; currArg < processParameterLength; currArg++)
					{
						if (relationMatrix[counter][currArg] == true)
						{
							flexprint(N->Fe, N->Fm, N->Fpc, "double %s, ", parameterVariableSymbols[currArg]->identifier);
						}
					}
					flexprint(N->Fe, N->Fm, N->Fpc, "double h)\n{\n");
					flexprint(N->Fe, N->Fm, N->Fpc, "double calculatedValue = 0.0;");

					flexprint(N->Fe, N->Fm, N->Fpc, "calculatedValue = (( process_%s(", stateVariableNames[counter]);

					for (currArg = 0; currArg < processParameterLength; currArg++)
					{
						if (relationMatrix[counter][currArg] == true)
						{	
							if (currArg == currDeriv)
							{
								flexprint(N->Fe, N->Fm, N->Fpc, "%s+h", parameterVariableSymbols[currArg]->identifier);	
							}
							else
							{
								flexprint(N->Fe, N->Fm, N->Fpc, "%s", parameterVariableSymbols[currArg]->identifier);
							}
							
							if (currArg != functionLastArg[counter])
							{
								flexprint(N->Fe, N->Fm, N->Fpc, ", ");
							}
						}
					}
					flexprint(N->Fe, N->Fm, N->Fpc, ")");
					
					flexprint(N->Fe, N->Fm, N->Fpc, " - process_%s(", stateVariableNames[counter]);
					currArg = 0;
					for (currArg = 0; currArg < processParameterLength; currArg++)
					{
						if (relationMatrix[counter][currArg] == true)
						{	
							flexprint(N->Fe, N->Fm, N->Fpc, "%s", parameterVariableSymbols[currArg]->identifier);
							if (currArg != functionLastArg[counter])
							{
								flexprint(N->Fe, N->Fm, N->Fpc, ", ");
							}
						}
					}
					flexprint(N->Fe, N->Fm, N->Fpc, ") ) / h );\n}\n");

				}
			}
		
		}
		

		/*
		 *	Generate predict function
		 */
		flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterPredict (CoreState *  cState, double step)\n{\n");
		
		for (int i = 0; i < stateDimension; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "double %s = cState->S[%s];\n", stateVariableSymbols[i]->identifier, stateVariableNames[i]);
		}
		
		flexprint(N->Fe, N->Fm, N->Fpc, "double newState[STATE_DIMENSION];\n");
		counter = 0;
		for (counter = 0; counter < stateDimension; counter++)
		{	
			flexprint(N->Fe, N->Fm, N->Fpc, "newState[%s] = process_%s(", stateVariableNames[counter], stateVariableNames[counter]);

			int currArg = 0;
			for (currArg = 0; currArg < processParameterLength; currArg++)
			{
				if (relationMatrix[counter][currArg] == true)
				{	
					flexprint(N->Fe, N->Fm, N->Fpc, "%s", parameterVariableSymbols[currArg]->identifier);
					if (currArg != functionLastArg[counter])
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}
			flexprint(N->Fe, N->Fm, N->Fpc, ");\n");
		}

		flexprint(N->Fe, N->Fm, N->Fpc, "double h = 0.0005;");
		flexprint(N->Fe, N->Fm, N->Fpc, "double fMatrix[STATE_DIMENSION][STATE_DIMENSION];\n");

		/*
		 *	Generate calculation of Jacobian of f()
		 */

		for (int i = 0; i < stateDimension; i++)
		{
			for (int j = 0; j < stateDimension; j++)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "fMatrix[%d][%d] = ", i, j);
				
				if (relationMatrix[i][j] == false)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "0;");
					continue;
				}
				
				flexprint(N->Fe, N->Fm, N->Fpc, "d_process_%s_d%s(", stateVariableNames[i], stateVariableSymbols[j]->identifier);
				for (int currArg = 0; currArg < processParameterLength; currArg++)
				{
					if (relationMatrix[i][currArg] == true)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "%s, ", parameterVariableSymbols[currArg]->identifier);
					}
				}
				flexprint(N->Fe, N->Fm, N->Fpc, "h);");
			}
		}
	}

	/*
	 *	Generate predict state
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix Fm = {.height = STATE_DIMENSION, .width = STATE_DIMENSION, .data = &fMatrix[0][0]};\n");
	if (linearProcess) 
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  FSm = multiplyMatrix(&Fm, cState->Sm);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "cState->Sm = copyMatrix(FSm);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "double *  sn = FSm->data;\n");
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "double *  sn = &newState[0];\n");		
	}	
	flexprint(N->Fe, N->Fm, N->Fpc, "double *  s = &cState->S[0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "for (int i = 0; i < STATE_DIMENSION; i++)\n{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "*s = *sn;\ns++;\nsn++;\n}\n");
	/*
	 *	Generate covariance propagation
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  Fm_T = transposeMatrix(&Fm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  FPm = multiplyMatrix(&Fm, cState->Pm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  FPFm_T = multiplyMatrix(FPm, Fm_T);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "double *  p = cState->Pm->data;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "double *  fpf = FPFm_T->data;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "double *  q = Qm->data;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "for (int i = 0; i < STATE_DIMENSION*STATE_DIMENSION; i++)\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "{\n\t*p = *fpf + *q;\n\tp++;\n\tfpf++;\n\tq++;\n}\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "}\n");

	/*
	 *	GENERATE UPDATE FUNCTION
	 */

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

	if (linearMeasurement == true)
	{
		/*	
		*	If linear:
		*	Deduce measurement matrix H 
		*/
		flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterUpdate (CoreState *  cState, double Z[MEASURE_DIMENSION], double step) {\n");
		IrNode * hMatrixIrNodes[measureDimension][stateDimension];
		flexprint(N->Fe, N->Fm, N->Fpc, "double hMatrix[MEASURE_DIMENSION][STATE_DIMENSION] = \n");
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

		
	}
	else
	{
		/*
		 *	NON-LINEAR UPDATE
		 */

		/*
		 *	Create relation matrix
		 *	Each measurement equation has its own row
		 *	relating it to the state variables*.
		 *	*Here: Mapping of measurementSymbols to stateVariableSymbols.
		 */
		bool relationMatrix[measureDimension][stateDimension];
		counter = 0;
		for (constraintXSeq = measureInvariant->constraints; constraintXSeq != NULL; counter++, constraintXSeq = constraintXSeq->irRightChild)
		{
			IrNode *  RHSExpressionXSeq = constraintXSeq->irLeftChild->irRightChild->irRightChild->irLeftChild;
			for (int mColumn = 0; mColumn < stateDimension; mColumn++)
			{
				/*
				 *	Check if symbol from parameter list appears in RHS expression
				 */
				if (irPassEstimatorSynthesisDetectSymbol(N, RHSExpressionXSeq, measurementSymbols[mColumn]) != NULL)
				{
					relationMatrix[counter][mColumn] = true;
				}
				else
				{
					relationMatrix[counter][mColumn] = false;
				}
			}
		}

		/*
		 *	Generate single state variable measurement functions.
		 *  This is: h_1(s1,s2, ... s_j), ... h_i(x,y, ... s_j)
		 */
		counter = 0;
		int functionLastArg[measureDimension];
		for (constraintXSeq = measureInvariant->constraints; constraintXSeq != NULL; counter++, constraintXSeq = constraintXSeq->irRightChild)
		{	
			flexprint(N->Fe, N->Fm, N->Fpc, "double\nmeasure_%s ", measureVariableNames[counter]);
			flexprint(N->Fe, N->Fm, N->Fpc, "(");

			int lastArg = 0;
			for (lastArg = stateDimension-1; lastArg >= 0; lastArg--)
			{
				if (relationMatrix[counter][lastArg] == true)
				{
					functionLastArg[counter] = lastArg;
					break;
				}
			}

			int currArg = 0;
			for (currArg = 0; currArg < stateDimension; currArg++)
			{
				if (relationMatrix[counter][currArg] == true)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "double %s", measurementSymbols[currArg]->identifier);
					if (currArg != functionLastArg[counter])
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}
			flexprint(N->Fe, N->Fm, N->Fpc, ")\n");
			irPassCGenFunctionBody(N, constraintXSeq->irLeftChild, false);
			flexprint(N->Fe, N->Fm, N->Fpc, "\n");

			/*
			 *	Generate derivative functions of h()
			 */
			for (int currDeriv = 0; currDeriv < stateDimension; currDeriv++)
			{
				if (relationMatrix[counter][currDeriv] == true)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "double\nd_measure_%s_d%s ", measureVariableNames[counter], stateVariableSymbols[currDeriv]->identifier);
					flexprint(N->Fe, N->Fm, N->Fpc, "(");
					for (int currArg = 0; currArg < stateDimension; currArg++)
					{
						if (relationMatrix[counter][currArg] == true)
						{
							flexprint(N->Fe, N->Fm, N->Fpc, "double %s, ", measurementSymbols[currArg]->identifier);
						}
					}
					flexprint(N->Fe, N->Fm, N->Fpc, "double h)\n{\n");
					flexprint(N->Fe, N->Fm, N->Fpc, "double calculatedValue = 0.0;");

					flexprint(N->Fe, N->Fm, N->Fpc, "calculatedValue = ((");
					flexprint(N->Fe, N->Fm, N->Fpc, "measure_%s(", measureVariableNames[currDeriv]);

					for (int currArg = 0; currArg < stateDimension; currArg++)
					{
						if (relationMatrix[counter][currArg] == true)
						{	
							if (currArg == currDeriv)
							{
								flexprint(N->Fe, N->Fm, N->Fpc, "%s+h", measurementSymbols[currArg]->identifier);	
							}
							else
							{
								flexprint(N->Fe, N->Fm, N->Fpc, "%s", measurementSymbols[currArg]->identifier);
							}
							
							if (currArg != functionLastArg[counter])
							{
								flexprint(N->Fe, N->Fm, N->Fpc, ", ");
							}
						}
					}
					flexprint(N->Fe, N->Fm, N->Fpc, ")");
					
					flexprint(N->Fe, N->Fm, N->Fpc, " - measure_%s(", measureVariableNames[counter]);
					currArg = 0;
					for (currArg = 0; currArg < stateDimension; currArg++)
					{
						if (relationMatrix[counter][currArg] == true)
						{	
							flexprint(N->Fe, N->Fm, N->Fpc, "%s", measurementSymbols[currArg]->identifier);
							if (currArg != functionLastArg[counter])
							{
								flexprint(N->Fe, N->Fm, N->Fpc, ", ");
							}
						}
					}
					flexprint(N->Fe, N->Fm, N->Fpc, ") ) / h );\n}\n\n");
				}
			}
		}

		flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterUpdate (CoreState *  cState, double Z[MEASURE_DIMENSION], double step) {\n");

		for (int i = 0; i < stateDimension; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "double %s = cState->S[%s];\n", measurementSymbols[i]->identifier, stateVariableNames[i]);
		}

		flexprint(N->Fe, N->Fm, N->Fpc, "double HS[MEASURE_DIMENSION];\n");
		counter = 0;
		for (counter = 0; counter < measureDimension; counter++)
		{	
			flexprint(N->Fe, N->Fm, N->Fpc, "HS[%s] = measure_%s(", measureVariableNames[counter], measureVariableNames[counter]);
			int currArg = 0;
			for (currArg = 0; currArg < stateDimension; currArg++)
			{
				if (relationMatrix[counter][currArg] == true)
				{	
					flexprint(N->Fe, N->Fm, N->Fpc, "%s", measurementSymbols[currArg]->identifier);
					if (currArg != functionLastArg[counter])
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}
			flexprint(N->Fe, N->Fm, N->Fpc, ");\n");
		}

		flexprint(N->Fe, N->Fm, N->Fpc, "double h = 0.0005;");
		flexprint(N->Fe, N->Fm, N->Fpc, "double hMatrix[MEASURE_DIMENSION][STATE_DIMENSION];\n");

		/*
		 *	Generate calculation of Jacobian of h()
		 */

		for (int i = 0; i < measureDimension; i++)
		{
			for (int j = 0; j < stateDimension; j++)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "hMatrix[%d][%d] = ", i, j);
				
				if (relationMatrix[i][j] == false)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "0;");
					continue;
				}
				
				flexprint(N->Fe, N->Fm, N->Fpc, "d_measure_%s_d%s ", measureVariableNames[i], stateVariableSymbols[j]->identifier);
				flexprint(N->Fe, N->Fm, N->Fpc, "(");
				for (int currArg = 0; currArg < stateDimension; currArg++)
				{
					if (relationMatrix[i][currArg] == true)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "%s, ", measurementSymbols[currArg]->identifier);
					}
				}
				flexprint(N->Fe, N->Fm, N->Fpc, "h);");
			}
		}

	}

	/*
	 *	Generate update matrix operations
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix Hm = { .height = MEASURE_DIMENSION, .width = STATE_DIMENSION, .data = &hMatrix[0][0] };\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix Zm = { .height = MEASURE_DIMENSION, .width = 1, .data = Z };\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "// Kg = PH^T * (HPH^T + Q)^(-1)\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  Hm_T = transposeMatrix(&Hm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  PHm_T = multiplyMatrix(cState->Pm, Hm_T);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  HPHm_T = multiplyMatrix(&Hm, PHm_T);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "double *  hph = HPHm_T->data;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "double *  r = Rm->data;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "for (int i = 0; i < MEASURE_DIMENSION * MEASURE_DIMENSION; i++)\n{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "*hph += *r;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "hph++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "r++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  HPHm_T_inv = inverseMatrix(HPHm_T);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  Kg = multiplyMatrix(PHm_T, HPHm_T_inv);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "// S <- S + Kg (Z - HS)\n");

	if (linearMeasurement)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  HSm = multiplyMatrix(&Hm, cState->Sm);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "double *  hs = HSm->data;\n");
	}
	else
	{
		// TODO: *hs simulates current state if measured
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix HSm_s = { .height = MEASURE_DIMENSION, .width = 1, .data = &HS[0] };\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  HSm = &HSm_s;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "double *  hs = &HS[0];\n");
	}		
	
	flexprint(N->Fe, N->Fm, N->Fpc, "double *  z = &Z[0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "for (int i = 0; i < 3; i++)\n{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "*hs = *z - *hs;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "hs++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "z++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  KgZHS = multiplyMatrix(Kg, HSm);\n");	
	flexprint(N->Fe, N->Fm, N->Fpc, "double *  s = &cState->S[0];\n");	
	flexprint(N->Fe, N->Fm, N->Fpc, "double *  kgzhs = KgZHS->data;\n");	
	flexprint(N->Fe, N->Fm, N->Fpc, "for (int i = 0; i < STATE_DIMENSION; i++)\n{\n");	
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
	
	
	/*
	 *	Generate main for testing purposes
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\nint \nmain(int argc, char *argv[])\n{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "int noOfValues = 1 + STATE_DIMENSION + MEASURE_DIMENSION;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "int nread;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "size_t nlen = 0;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "char *  line;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "CoreState cs;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "// First line is column names\n");
	// flexprint(N->Fe, N->Fm, N->Fpc, "nread = getline(&line, &nlen, stdin);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "double initState[STATE_DIMENSION];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "double time;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "scanf(\"%%lf\", &time);\n");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "scanf(\",%%lf\", &initState[%d]);\n", i);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "double initCov[STATE_DIMENSION][STATE_DIMENSION] = {");
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
	flexprint(N->Fe, N->Fm, N->Fpc, "double measure[MEASURE_DIMENSION];\n");
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
	flexprint(N->Fe, N->Fm, N->Fpc, "printf(\"Predict: %%lf\", time);\n");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "printf(\", %%lf\", cs.S[%s]);\n", stateVariableNames[i]);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "printf(\"\\n\");\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "filterUpdate(&cs, measure, time - prevtime);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "printf(\"Update: %%lf\", time);\n");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "printf(\", %%lf\", cs.S[%s]);\n", stateVariableNames[i]);
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
