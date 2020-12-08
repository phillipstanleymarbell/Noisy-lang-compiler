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
	- System takes no input in linear case (B=0). // TODO: This is fixable.
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
#include "newton-irPass-autoDiff.h"

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
 *	Looks up in the Newton Physics table for the identifier of the
 *	argument Physics *p. Then, looks up in the Common Symbol Table
 *	for "var" in the found signal's Physics uncertainty scope.
 *	Returns the value of the variance or 0 if no variance declaration was found.
 */
double
getIdentifierSignalUncertainty(State *  N, Physics *p)
{
	static const double defaultUncertainty = 1e-6;
	/*
	 *	NOTE: We do not yet have a data structure for representing uncertainties
	 *	and/or probability distributions.
	 */
	Physics *	signalPhysics = newtonPhysicsTablePhysicsForIdentifier(N, N->newtonIrTopScope, p->identifier);
	if (signalPhysics == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "No Physics entry found for identifier '%s'.\n", p->identifier);
		flexprint(N->Fe, N->Fm, N->Fperr, "This should not be able to happen. Please contact the developers or open an bug issue providing the input that was used.\n");
		/*
		 *	The reason this should not be able to happen at this stage is because
		 *	it probably is a variable usage before declaration, which is caught in earlier passes.
		 */
		fatal(N, Efatal);
	}
	Symbol * varSymbol = commonSymbolTableSymbolForIdentifier(N, signalPhysics->uncertaintyScope, "var");
	if (varSymbol == NULL)
	{
		error(N, "No Symbol entry found for identifier 'var'.");
		// TODO: Write *what* you are setting to $defaultUncertainty
		flexprint(N->Fe, N->Fm, N->Fperr, "Variance for signal '%s' not found. Setting to %f.\n", p->identifier, defaultUncertainty);
		return defaultUncertainty;
	}
	return varSymbol->typeTree->irParent->irRightChild->value;
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
				stillLinear = stillLinear && irPassEstimatorSynthesisExpressionLinear(N, expressionXSeq->irLeftChild);
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

			/*
			 *	Check for identifiers inside transcendentals.
			 */
			IrNode* found = NULL;
			for (int n = 0;  (found = findNthIrNodeOfType(N, expressionXSeq, kNewtonIrNodeType_Ptranscendental, n)) != NULL; n++)
			{
				IrNode* foundIdentifier = NULL;
				for (int k = 0;  (foundIdentifier = findNthIrNodeOfType(N, found->irParent, kNewtonIrNodeType_Tidentifier, k)) != NULL; k++)
				if (!foundIdentifier->physics->isConstant)
				{
					return false;
				}
			}
			// TODO: kNewtonIrNodeType_Tidentifier > 1 is linear ONLY for identifier*step
		} break;

		default:
			break;
	}

	return true;
}

bool
irPassEstimatorSynthesisInvariantLinear(State *  N, Invariant *  invariant)
{
	bool stillLinear = true;
	for (IrNode *  constraintXSeq = invariant->constraints; stillLinear && (constraintXSeq != NULL); constraintXSeq = constraintXSeq->irRightChild)
	{
		stillLinear = stillLinear && irPassEstimatorSynthesisExpressionLinear(N, L(constraintXSeq));
		stillLinear = stillLinear && irPassEstimatorSynthesisExpressionLinear(N, LRRL(constraintXSeq));
	}
	return stillLinear;
}


/*
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
		flexprint(N->Fe, N->Fm, N->Fperr, "Warning: Symbol \"%s\" detected in multiple terms.\n", stateVariableSymbol->identifier);
	}

	if (foundMatchingIrNode != NULL)
	{
		/*
		 *	Remove identifier from sub-tree.
		 *	Hack: Substitute with identity element.
		 *	Problem: Mutates the tree.
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
		// TODO: Constant propagation. -- No. Let gcc/clang handle that.
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

	flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tGenerated .c file from Newton\n */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "#include <stdlib.h>\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "#include <stdio.h>\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "#include <math.h>\n\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "#include \"../C-Linear-Algebra/matrix.h\"\n#include \"../C-Linear-Algebra/matrixadv.h\"\n\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "#define DEG2RAD (3.1415926535/180)\n");

	IrNode *	constraintXSeq = NULL;
	/*
	 *	Find state vector dimension (N)
	 */
	int stateDimension = 0;
	for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{
		stateDimension++;
	}

	constraintXSeq = NULL;
	/*
	 *	Find number of invariant parameters that are not state variables.
	 */
	int stateExtraParams = 0;
	for (constraintXSeq = processInvariant->parameterList; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{
		stateExtraParams++;
	}
	stateExtraParams = stateExtraParams - stateDimension;

	constraintXSeq = NULL;
	/*
	 *	Find measurement vector dimension (Z)
	 */
	int measureDimension = 0;
	for (constraintXSeq = measureInvariant->constraints; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{
		measureDimension++;
	}

	constraintXSeq = NULL;
	int measureExtraParams = 0;
	for (constraintXSeq = measureInvariant->parameterList; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{
		measureExtraParams++;
	}
	measureExtraParams = measureExtraParams - stateDimension - measureDimension;

	/*
	 *	Generate state variable names and
	 *	book-keep corresponding symbols and uncertainties.
	 */
	char *	stateVariableNames[stateDimension];
	Symbol *	stateVariableSymbols[stateDimension];
	double	stateVariableUncertainties[stateDimension];

	int counter = 0;
	for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; counter++, constraintXSeq = constraintXSeq->irRightChild)
	{
		IrNode *  leafLHS = LLL(LLL(constraintXSeq));

		int	needed = snprintf(NULL, 0, "STATE_%s_%d", leafLHS->tokenString, 0) + 1;
		stateVariableNames[counter] = malloc(needed);
		snprintf(stateVariableNames[counter], needed, "STATE_%s_%d", leafLHS->tokenString, 0);

		stateVariableSymbols[counter] = leafLHS->symbol;

		stateVariableUncertainties[counter] = getIdentifierSignalUncertainty(N, leafLHS->physics);
	}

	Symbol *	stateExtraParamSymbols[stateExtraParams];
	counter = 0;
	for (constraintXSeq = processInvariant->parameterList; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{
		Symbol *	parameterSymbol = LL(constraintXSeq)->symbol;
		bool		isStateVariable = false;

		for (int i = 0; i < stateDimension; i++)
		{
			if (parameterSymbol == stateVariableSymbols[i])
			{
				isStateVariable = true;
				break;
			}
		}

		if (isStateVariable)
		{
			continue;
		}

		if (counter >= stateExtraParams)
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "Process invariant identifiers that are state variables appear to be less than constraints. This should not be able to happen. Please contact the developers or open an bug issue.\n");
			/*
			 *	The reason this should not be able to happen at this stage is because
			 *	it probably is a variable usage before declaration, which is caught in earlier passes.
			 */
			fatal(N, Efatal);
		}

		stateExtraParamSymbols[counter] = parameterSymbol;
		counter++;
	}


	/*
	 *	Generate measure variable names
	 */
	char *	measureVariableNames[measureDimension];
	Symbol *	measureVariableSymbols[measureDimension];
	double	measureVariableUncertainties[measureDimension];

	counter = 0;
	for (constraintXSeq = measureInvariant->constraints; constraintXSeq != NULL; counter++, constraintXSeq = constraintXSeq->irRightChild)
	{
		IrNode *  leafLHS = LLL(LLL(constraintXSeq));

		int	needed = snprintf(NULL, 0, "MEASURE_%s_%d", leafLHS->tokenString, 0) + 1;
		measureVariableNames[counter] = malloc(needed);
		snprintf(measureVariableNames[counter], needed, "MEASURE_%s_%d", leafLHS->tokenString, 0);

		measureVariableSymbols[counter] = leafLHS->symbol;

		measureVariableUncertainties[counter] = getIdentifierSignalUncertainty(N, leafLHS->physics);
	}


	/*
	 *	Generate state indexing enumerator
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\nenum filterCoreStateIdx\n{\n");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s,\n", stateVariableNames[i]);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\tSTATE_DIMENSION\n};\n\n");

	/*
	 *	Generate measure indexing enumerator
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\nenum filterMeasureIdx\n{\n");
	for (int i = 0; i < measureDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s,\n", measureVariableNames[i]);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\tMEASURE_DIMENSION\n};\n\n");

	/*
	 *	Generate core filter-state struct
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "typedef struct CoreState	CoreState;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "struct CoreState \n{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t *\tState\n\t */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s S[STATE_DIMENSION];\n\tmatrix *\tSm;\n\n",N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t *\tState covariance matrix\n\t */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s P[STATE_DIMENSION][STATE_DIMENSION];\n\tmatrix *\tPm;\n\n",N->estimatorOutputVariableType);
	// TODO: Populate covariance matrix?
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t *\tProcess noise matrix\n\t */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s Q[STATE_DIMENSION][STATE_DIMENSION];\n\tmatrix *\tQm;\n\n",N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t *\tProcess noise matrix\n\t */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s R[MEASURE_DIMENSION][MEASURE_DIMENSION];\n\tmatrix *\tRm;\n\n",N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "};\n\n");

	/*
	 *	GENERATE INIT FUNCTION
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterInit (CoreState * cState, %1$s S0[STATE_DIMENSION], %1$s P0[STATE_DIMENSION][STATE_DIMENSION]) \n{\n",N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\tfor (int i = %s; i < STATE_DIMENSION; i++)\n", stateVariableNames[0]);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tcState->S[i] = S0[i];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Sm = makeMatrix(1, STATE_DIMENSION);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Sm->data = &cState->S[0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\n\tfor (int i = %s; i < STATE_DIMENSION; i++)\n", stateVariableNames[0]);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tfor (int j = %s; j < STATE_DIMENSION; j++)\n", stateVariableNames[0]);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t\tcState->P[i][j] = P0[i][j];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Pm = makeMatrix(STATE_DIMENSION, STATE_DIMENSION);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Pm->data = &cState->P[0][0];\n\n");

	for (int i = 0; i < stateDimension; i++)
	{
		for (int j = 0; j < stateDimension; j++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Q[%d][%d] =", i, j);
			if (i != j) {
				flexprint(N->Fe, N->Fm, N->Fpc, " %g;\n", pow(10, (log10(stateVariableUncertainties[i]*stateVariableUncertainties[j])/2))*1e-1);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, " %g;\n", stateVariableUncertainties[i]);
			}
		}
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Qm = makeMatrix(STATE_DIMENSION, STATE_DIMENSION);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Qm->data = &cState->Q[0][0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	for (int i = 0; i < measureDimension; i++)
	{
		for (int j = 0; j < measureDimension; j++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->R[%d][%d] =", i, j);
			if (i != j) {
				flexprint(N->Fe, N->Fm, N->Fpc, " %g;\n", pow(10, (log10(measureVariableUncertainties[i]*measureVariableUncertainties[j])/2))*1e-1);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, " %g;\n", measureVariableUncertainties[i]);
			}
		}
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Rm = makeMatrix(MEASURE_DIMENSION, MEASURE_DIMENSION);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Rm->data = &cState->R[0][0];\n\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "}\n\n");


	/*
	 *	GENERATE PREDICT FUNCTION
	 */

	/*
	 *	Determine linearity of process model
	 */
	bool linearProcess = irPassEstimatorSynthesisInvariantLinear(N, processInvariant);;

	if (linearProcess == true)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterPredict (CoreState *  cState");
		for (int i = 0; i < stateExtraParams; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, ", %s %s",N->estimatorOutputVariableType ,stateExtraParamSymbols[i]->identifier);
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ")\n{\n");
		/*	If linear:
		 *	Deduce state transition matrix F
		 */
		IrNode * fMatrixIrNodes[stateDimension][stateDimension];
		int fRow = 0;
		flexprint(N->Fe, N->Fm, N->Fpc, "%s fMatrix[STATE_DIMENSION][STATE_DIMENSION] = \n",N->estimatorOutputVariableType);
		flexprint(N->Fe, N->Fm, N->Fpc, "{ ");
		for (constraintXSeq = processInvariant->constraints; constraintXSeq != NULL; fRow++, constraintXSeq = constraintXSeq->irRightChild)
		{
			/*
			 *	Right Hand Side Expression of the constraint:
			 *	RHSExpression = constraintlist-> constraint -> right hand side -> bypass comparison -> quantity expression
			 */
			IrNode *  RHSExpressionXSeq = LRRL(constraintXSeq);

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
			flexprint(N->Fe, N->Fm, N->Fpc, "%s\nprocess_%s ", N->estimatorOutputVariableType,stateVariableNames[counter]);
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
					flexprint(N->Fe, N->Fm, N->Fpc, "%s %s", N->estimatorOutputVariableType, parameterVariableSymbols[currArg]->identifier);
					if (currArg != lastArg || N->autodiff == true)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}
			if (N->autodiff == true)
			{
				IrNode *  RHSExpression = constraintXSeq->irLeftChild->irRightChild->irRightChild->irLeftChild;
				flexprint(N->Fe, N->Fm, N->Fpc, "%s Ji[STATE_DIMENSION])\n{\n",N->estimatorOutputVariableType);

				autoDiffGenBody(N, RHSExpression, stateVariableNames, stateVariableSymbols, stateDimension);

				flexprint(N->Fe, N->Fm, N->Fpc, "\n}\n\n");
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ")\n");
				irPassCGenFunctionBody(N, constraintXSeq->irLeftChild, false);
				/*
				 *	Generate partial derivatives
				 */
				for (int currDeriv = 0; currDeriv < stateDimension; currDeriv++)
				{
					if (relationMatrix[counter][currDeriv] == true)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "%s\nd_process_%s_d%s ", N->estimatorOutputVariableType, stateVariableNames[counter], stateVariableSymbols[currDeriv]->identifier);
						flexprint(N->Fe, N->Fm, N->Fpc, "(");
						for (currArg = 0; currArg < processParameterLength; currArg++)
						{
							if (relationMatrix[counter][currArg] == true)
							{
								flexprint(N->Fe, N->Fm, N->Fpc, "%s %s, ", N->estimatorOutputVariableType, parameterVariableSymbols[currArg]->identifier);
							}
						}
						flexprint(N->Fe, N->Fm, N->Fpc, "%s h)\n{\n", N->estimatorOutputVariableType);
						flexprint(N->Fe, N->Fm, N->Fpc, "\t%s calculatedValue = 0.0;\n",N->estimatorOutputVariableType);

						flexprint(N->Fe, N->Fm, N->Fpc, "\tcalculatedValue = (( process_%s(", stateVariableNames[counter]);

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
						flexprint(N->Fe, N->Fm, N->Fpc, ") ) / h );\n\n\treturn calculatedValue;\n}\n\n");
					}
				}
			}
		}

		/*
		 *	Generate predict function
		 */
		flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterPredict (CoreState *  cState");
		for (int i = 0; i < stateExtraParams; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, ", %s %s", N->estimatorOutputVariableType, stateExtraParamSymbols[i]->identifier);
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ")\n{\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s newState[STATE_DIMENSION];\n", N->estimatorOutputVariableType);
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s fMatrix[STATE_DIMENSION][STATE_DIMENSION] = {0};\n\n", N->estimatorOutputVariableType);

		for (int i = 0; i < stateDimension; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\t%s %s = cState->S[%s];\n", N->estimatorOutputVariableType, stateVariableSymbols[i]->identifier, stateVariableNames[i]);
		}


		counter = 0;
		for (counter = 0; counter < stateDimension; counter++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\tnewState[%s] = process_%s(", stateVariableNames[counter], stateVariableNames[counter]);

			int currArg = 0;
			for (currArg = 0; currArg < processParameterLength; currArg++)
			{
				if (relationMatrix[counter][currArg] == true)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "%s", parameterVariableSymbols[currArg]->identifier);
					if (currArg != functionLastArg[counter] || N->autodiff == true)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}
			if (N->autodiff == true)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "fMatrix[%s]);\n", stateVariableNames[counter]);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ");\n");
			}
		}

		if (N->autodiff != true)
		{
			/*
			 *	Generate calculation of Jacobian of f() with standard diff
			 */
			flexprint(N->Fe, N->Fm, N->Fpc, "\t%s h = 0.0005;\n",N->estimatorOutputVariableType);
			for (int i = 0; i < stateDimension; i++)
			{
				for (int j = 0; j < stateDimension; j++)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "\tfMatrix[%d][%d] = ", i, j);

					if (relationMatrix[i][j] == false)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "0;");
						continue;
					}

					flexprint(N->Fe, N->Fm, N->Fpc, "\td_process_%s_d%s(", stateVariableNames[i], stateVariableSymbols[j]->identifier);
					for (int currArg = 0; currArg < processParameterLength; currArg++)
					{
						if (relationMatrix[i][currArg] == true)
						{
							flexprint(N->Fe, N->Fm, N->Fpc, "%s, ", parameterVariableSymbols[currArg]->identifier);
						}
					}
					flexprint(N->Fe, N->Fm, N->Fpc, "h);\n");
				}
			}
		}
	}

	/*
	 *	Generate predict state
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\tmatrix Fm = {.height = STATE_DIMENSION, .width = STATE_DIMENSION, .data = &fMatrix[0][0]};\n");
	if (linearProcess)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  FSm = multiplyMatrix(&Fm, cState->Sm);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  sn = FSm->data;\n", N->estimatorOutputVariableType);
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  sn = &newState[0];\n", N->estimatorOutputVariableType);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  s = &cState->S[0];\n",N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\tfor (int i = 0; i < STATE_DIMENSION; i++)\n\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t*s = *sn;\n\t\ts++;\n\t\tsn++;\n\t}\n\n");
	/*
	 *	Generate covariance propagation
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  Fm_T = transposeMatrix(&Fm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  FPm = multiplyMatrix(&Fm, cState->Pm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  FPFm_T = multiplyMatrix(FPm, Fm_T);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  p = cState->Pm->data;\n", N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  fpf = FPFm_T->data;\n", N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  q = cState->Qm->data;\n", N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\tfor (int i = 0; i < STATE_DIMENSION*STATE_DIMENSION; i++)\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t{\n\t\t*p = *fpf + *q;\n\t\tp++;\n\t\tfpf++;\n\t\tq++;\n\t}\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "}\n\n");

	/*
	 *	GENERATE UPDATE FUNCTION
	 */

	/*
	 *	Find correspondence between state variable Symbol identifiers and
	 *	measurement Invariant parameter identifiers. Essentially map
	 *	stateVariableSymbols to symbols in the measurement Invariant.
	 */

	/*
	 *	Declared Measurement Invariant Symbols that correspond to state variables.
	 */
	Symbol * measureInvariantStateVariableSymbols[stateDimension];
	counter = 0;
	for (IrNode * parameterXSeq = measureInvariant->parameterList; parameterXSeq != NULL; parameterXSeq = parameterXSeq->irRightChild)
	{
		IrNode *  parameterIdentifier = parameterXSeq->irLeftChild->irLeftChild;
		for (int i = 0; i < stateDimension; i++)
		{
			if (strcmp(parameterIdentifier->symbol->identifier, stateVariableSymbols[i]->identifier) == 0)
			{
				measureInvariantStateVariableSymbols[i] = parameterIdentifier->symbol;
				counter++;
				break;
			}

		}
	}

	Symbol *	measureExtraParamSymbols[measureExtraParams];
	counter = 0;
	for (constraintXSeq = measureInvariant->parameterList; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{
		Symbol *	parameterSymbol = LL(constraintXSeq)->symbol;
		bool		isMeasureOrStateVariable = false;

		for (int i = 0; i < stateDimension; i++)
		{
			if (parameterSymbol == measureInvariantStateVariableSymbols[i])
			{
				isMeasureOrStateVariable = true;
				break;
			}
		}
		if (isMeasureOrStateVariable)
		{
			continue;
		}

		for (int i = 0; i < measureDimension; i++)
		{
			if (parameterSymbol == measureVariableSymbols[i])
			{
				isMeasureOrStateVariable = true;
				break;
			}
		}
		if (isMeasureOrStateVariable)
		{
			continue;
		}

		if (counter >= measureExtraParams)
		{
			fatal(N, "Measurement Invariant Parameter identifiers that are either measurement variables or state variables appear to be less than #measurement_constraints + #process_constraints. Are *all* state variables present in the Measurement Invariant's parameter?\n");
		}

		measureExtraParamSymbols[counter] = parameterSymbol;
		counter++;
	}


	/*
	 *	Determine linearity of measurement model
	 */
	bool linearMeasurement = irPassEstimatorSynthesisInvariantLinear(N, measureInvariant);

	if (linearMeasurement == true)
	{
		/*
		 *	If linear:
		 *	Deduce measurement matrix H
		 */
		flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterUpdate (CoreState *  cState, %s Z[MEASURE_DIMENSION]",N->estimatorOutputVariableType);
		for (int i = 0; i < measureExtraParams; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, ", %s %s", N->estimatorOutputVariableType ,measureExtraParamSymbols[i]->identifier);
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ")\n{\n");

		IrNode * hMatrixIrNodes[measureDimension][stateDimension];
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s hMatrix[MEASURE_DIMENSION][STATE_DIMENSION] = \n", N->estimatorOutputVariableType);
		flexprint(N->Fe, N->Fm, N->Fpc, "\t{\n");
		int hRow = 0;
		for (constraintXSeq = measureInvariant->constraints; constraintXSeq != NULL; hRow++, constraintXSeq = constraintXSeq->irRightChild)
		{
			/*
			 *	Find the Right Hand Side Expression of the constraint:
			 *	RHSExpression = constraintlist-> constraint -> right hand side -> bypass comparison -> quantity expression
			 */
			IrNode *  RHSExpressionXSeq = constraintXSeq->irLeftChild->irRightChild->irRightChild->irLeftChild;
			flexprint(N->Fe, N->Fm, N->Fpc, "\t\t{ ");
			for (int hColumn = 0; hColumn < stateDimension; hColumn++)
			{
				hMatrixIrNodes[hRow][hColumn] = irPassEstimatorSynthesisIsolateSymbolFactors(N, RHSExpressionXSeq, measureInvariantStateVariableSymbols[hColumn]);
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
		flexprint(N->Fe, N->Fm, N->Fpc, "\t};\n");
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
		 *	*Here a mapping of measureInvariantStateVariableSymbols to stateVariableSymbols.
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
				if (irPassEstimatorSynthesisDetectSymbol(N, RHSExpressionXSeq, measureInvariantStateVariableSymbols[mColumn]) != NULL)
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
			flexprint(N->Fe, N->Fm, N->Fpc, "\t%s\nmeasure_%s ", N->estimatorOutputVariableType ,measureVariableNames[counter]);
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

			// int currArg = 0;
			for (int currArg = 0; currArg < stateDimension; currArg++)
			{
				if (relationMatrix[counter][currArg] == true)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "\t%s %s", N->estimatorOutputVariableType ,measureInvariantStateVariableSymbols[currArg]->identifier);
					if (currArg != functionLastArg[counter])
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}

			if (N->autodiff == true)
			{
				IrNode *  RHSExpression = constraintXSeq->irLeftChild->irRightChild->irRightChild->irLeftChild;
				flexprint(N->Fe, N->Fm, N->Fpc, ", %s Ji[STATE_DIMENSION])\n{\n", N->estimatorOutputVariableType);

				autoDiffGenBody(N, RHSExpression, stateVariableNames, stateVariableSymbols, stateDimension);

				flexprint(N->Fe, N->Fm, N->Fpc, "\n}\n\n");
			}
			else
			{
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
						flexprint(N->Fe, N->Fm, N->Fpc, "\t%s\nd_measure_%s_d%s ", N->estimatorOutputVariableType ,measureVariableNames[counter], stateVariableSymbols[currDeriv]->identifier);
						flexprint(N->Fe, N->Fm, N->Fpc, "(");
						for (int currArg = 0; currArg < stateDimension; currArg++)
						{
							if (relationMatrix[counter][currArg] == true)
							{
								flexprint(N->Fe, N->Fm, N->Fpc, "%s %s, ", N->estimatorOutputVariableType, measureInvariantStateVariableSymbols[currArg]->identifier);
							}
						}
						flexprint(N->Fe, N->Fm, N->Fpc, "\t%s h)\n{\n", N->estimatorOutputVariableType);
						flexprint(N->Fe, N->Fm, N->Fpc, "\t%s calculatedValue = 0.0;\n", N->estimatorOutputVariableType);

						flexprint(N->Fe, N->Fm, N->Fpc, "\tcalculatedValue = ((");
						flexprint(N->Fe, N->Fm, N->Fpc, "measure_%s(", measureVariableNames[counter]);

						for (int currArg = 0; currArg < stateDimension; currArg++)
						{
							if (relationMatrix[counter][currArg] == true)
							{
								if (currArg == currDeriv)
								{
									flexprint(N->Fe, N->Fm, N->Fpc, "%s+h", measureInvariantStateVariableSymbols[currArg]->identifier);
								}
								else
								{
									flexprint(N->Fe, N->Fm, N->Fpc, "%s", measureInvariantStateVariableSymbols[currArg]->identifier);
								}

								if (currArg != functionLastArg[counter])
								{
									flexprint(N->Fe, N->Fm, N->Fpc, ", ");
								}
							}
						}
						flexprint(N->Fe, N->Fm, N->Fpc, ")");

						flexprint(N->Fe, N->Fm, N->Fpc, " - measure_%s(", measureVariableNames[counter]);
						for (int currArg = 0; currArg < stateDimension; currArg++)
						{
							if (relationMatrix[counter][currArg] == true)
							{
								flexprint(N->Fe, N->Fm, N->Fpc, "%s", measureInvariantStateVariableSymbols[currArg]->identifier);
								if (currArg != functionLastArg[counter])
								{
									flexprint(N->Fe, N->Fm, N->Fpc, ", ");
								}
							}
						}
						flexprint(N->Fe, N->Fm, N->Fpc, ") ) / h );\n\n\treturn calculatedValue;\n}\n\n");
					}
				}
			}
		}

		flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterUpdate (CoreState *  cState, %s Z[MEASURE_DIMENSION]",N->estimatorOutputVariableType);
		for (int i = 0; i < measureExtraParams; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, ", %s %s", N->estimatorOutputVariableType ,measureExtraParamSymbols[i]->identifier);
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ")\n{\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s HS[MEASURE_DIMENSION];\n", N->estimatorOutputVariableType);
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s hMatrix[MEASURE_DIMENSION][STATE_DIMENSION] = {0};\n", N->estimatorOutputVariableType);

		flexprint(N->Fe, N->Fm, N->Fpc, "\n{\n");
		for (int i = 0; i < stateDimension; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\t%s %s = cState->S[%s];\n", N->estimatorOutputVariableType, measureInvariantStateVariableSymbols[i]->identifier, stateVariableNames[i]);
		}

		counter = 0;
		for (counter = 0; counter < measureDimension; counter++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\tHS[%s] = measure_%s(", measureVariableNames[counter], measureVariableNames[counter]);
			for (int currArg = 0; currArg < stateDimension; currArg++)
			{
				if (relationMatrix[counter][currArg] == true)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "%s", measureInvariantStateVariableSymbols[currArg]->identifier);
					if (currArg != functionLastArg[counter])
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}
			if (N->autodiff == true)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ", hMatrix[%s]);\n", measureVariableNames[counter]);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ");\n");
			}
		}

		if (N->autodiff != true)
		{
			/*
			 *	Generate calculation of Jacobian of h()
			 */
			flexprint(N->Fe, N->Fm, N->Fpc, "\t%s h = 0.0005;\n",N->estimatorOutputVariableType);

			for (int i = 0; i < measureDimension; i++)
			{
				for (int j = 0; j < stateDimension; j++)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "\thMatrix[%d][%d] = ", i, j);

					if (relationMatrix[i][j] == false)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "0;\n");
						continue;
					}

					flexprint(N->Fe, N->Fm, N->Fpc, "d_measure_%s_d%s ", measureVariableNames[i], stateVariableSymbols[j]->identifier);
					flexprint(N->Fe, N->Fm, N->Fpc, "(");
					for (int currArg = 0; currArg < stateDimension; currArg++)
					{
						if (relationMatrix[i][currArg] == true)
						{
							flexprint(N->Fe, N->Fm, N->Fpc, "%s, ", measureInvariantStateVariableSymbols[currArg]->identifier);
						}
					}
					flexprint(N->Fe, N->Fm, N->Fpc, "h);\n");
				}
			}
		}
		flexprint(N->Fe, N->Fm, N->Fpc, "\n}\n");
	}

	/*
	 *	Generate update matrix operations
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix Hm = { .height = MEASURE_DIMENSION, .width = STATE_DIMENSION, .data = &hMatrix[0][0] };\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix Zm = { .height = MEASURE_DIMENSION, .width = 1, .data = Z };\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\t// Kg = PH^T * (HPH^T + R)^(-1)\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  Hm_T = transposeMatrix(&Hm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  PHm_T = multiplyMatrix(cState->Pm, Hm_T);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  HPHm_T = multiplyMatrix(&Hm, PHm_T);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  hph = HPHm_T->data;\n", N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  r = cState->Rm->data;\n", N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\tfor (int i = 0; i < MEASURE_DIMENSION * MEASURE_DIMENSION; i++)\n\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t*hph += *r;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\thph++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tr++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  HPHm_T_inv = inverseMatrix(HPHm_T);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  Kg = multiplyMatrix(PHm_T, HPHm_T_inv);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t// S <- S + Kg (Z - HS)\n");

	if (linearMeasurement)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  HSm = multiplyMatrix(&Hm, cState->Sm);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  hs = HSm->data;\n", N->estimatorOutputVariableType);
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix HSm_s = { .height = MEASURE_DIMENSION, .width = 1, .data = &HS[0] };\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  HSm = &HSm_s;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  hs = &HS[0];\n", N->estimatorOutputVariableType);
	}

	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  z = &Z[0];\n", N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\tfor (int i = 0; i < MEASURE_DIMENSION; i++)\n\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t*hs = *z - *hs;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\ths++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tz++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  KgZHS = multiplyMatrix(Kg, HSm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  s = &cState->S[0];\n", N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  kgzhs = KgZHS->data;\n", N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\tfor (int i = 0; i < STATE_DIMENSION; i++)\n\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t*s += *kgzhs;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\ts++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tkgzhs++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t// P <- P - KgHP\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  HPm = multiplyMatrix(&Hm, cState->Pm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  KgHPm = multiplyMatrix(Kg, HPm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  p = &cState->P[0][0];\n", N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s *  kghp = KgHPm->data;\n", N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\tfor (int i = 0; i < STATE_DIMENSION*STATE_DIMENSION; i++)\n\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t*p -= *kghp;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tp++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tkghp++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "}\n\n");


	/*
	 *	Generate main for testing purposes
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\nint \nmain(int argc, char *argv[])\n{\n");
	// flexprint(N->Fe, N->Fm, N->Fpc, "int noOfValues = 1 + STATE_DIMENSION + MEASURE_DIMENSION;\n");
	// flexprint(N->Fe, N->Fm, N->Fpc, "int nread;\n");
	// flexprint(N->Fe, N->Fm, N->Fpc, "size_t nlen = 0;\n");
	// flexprint(N->Fe, N->Fm, N->Fpc, "\tchar *  line;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\tCoreState cs;\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s initState[STATE_DIMENSION];\n", N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s time = 0;\n", N->estimatorOutputVariableType);
	// flexprint(N->Fe, N->Fm, N->Fpc, "//scanf(\"%%lf\", &time);\n");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\tinitState[%s] = 0;\n", stateVariableNames[i]);
		// flexprint(N->Fe, N->Fm, N->Fpc, "//scanf(\",%%lf\", &initState[%d]);\n", i);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s initCov[STATE_DIMENSION][STATE_DIMENSION] = {", N->estimatorOutputVariableType);
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

	flexprint(N->Fe, N->Fm, N->Fpc, "\tfilterInit(&cs, initState, initCov);\n");


	// flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble dt;\n");
	for (int i = 0; i < stateExtraParams; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s %s;\n", N->estimatorOutputVariableType, stateExtraParamSymbols[i]->identifier);
	}
	
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s prevtime = time;\n", N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t%s measure[MEASURE_DIMENSION];\n", N->estimatorOutputVariableType);
	flexprint(N->Fe, N->Fm, N->Fpc, "\twhile (scanf(\"%s\", &time) > 0)\n",N->estimatorOutputVariableFormat);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t{\n");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t\tscanf(\",%%*lf\");\n", i);
	}
	for (int i = 0; i < measureDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t\tscanf(\",%s\", &measure[%d]);\n", N->estimatorOutputVariableFormat,i);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tdt = time-prevtime;\n\n");
	// flexprint(N->Fe, N->Fm, N->Fpc, "filterPredict(&cs, time - prevtime);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tfilterPredict (&cs");
	for (int i = 0; i < stateExtraParams; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, ", %s", stateExtraParamSymbols[i]->identifier);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, ");\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprintf(\"Predict: %s\", time);\n", N->estimatorOutputVariableFormat);
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprintf(\", %s\", cs.S[%s]);\n", N->estimatorOutputVariableFormat,  stateVariableNames[i]);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprintf(\"\\n\");\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tfilterUpdate(&cs, measure");
	for (int i = 0; i < measureExtraParams; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, ", %s", measureExtraParamSymbols[i]->identifier);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, ");\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprintf(\"Update: %s\", time);\n", N->estimatorOutputVariableFormat);
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprintf(\", %s\", cs.S[%s]);\n", N->estimatorOutputVariableFormat, stateVariableNames[i]);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprintf(\"\\n\");\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprevtime = time;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\treturn 0;\n}\n");
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

	int needed = snprintf(NULL, 0, "double") + 1;
	N->estimatorOutputVariableType = malloc(needed);
	snprintf(N->estimatorOutputVariableType, needed, "double");

	needed = snprintf(NULL, 0, "%%lf") + 1;
	N->estimatorOutputVariableFormat = malloc(needed);
	snprintf(N->estimatorOutputVariableFormat, needed, "%%lf");
	
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
