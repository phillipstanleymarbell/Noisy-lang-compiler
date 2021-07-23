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
#include "newton-irPass-estimatorSynthesisBackend.h"

/*
*	Helper data structures for the estimator backend
*/

typedef struct ConstraintNode
{
	IrNode *	constraint;
	int			stateVariableId;
	int			caseId;
	struct ConstraintNode *  next;
} ConstraintNode;

typedef ConstraintNode *  ConstraintList;

typedef struct estimatorSynthesisState
{
	int		processParams;
	int		stateExtraParams;
	int 		stateDimension;
	int		measureParams;
	int		measureExtraParams;
	int		measureDimension;
	int		processConstraints;
	int		measureConstraints;
	ConstraintList	processConstraintList;
	char **		stateVariableNames;
	Symbol **	stateVariableSymbols;
	double *	stateVariableUncertainties;
	Symbol **	stateExtraParamSymbols;
	Symbol **  	parameterVariableSymbols;
	ConstraintList	measureConstraintList;
	char **		measureVariableNames;
	Symbol **	measureVariableSymbols;
	double *	measureVariableUncertainties;
	Symbol **	measureInvariantStateVariableSymbols;
	Symbol **	measureExtraParamSymbols;
	bool **		relationMatrix;
	bool **		measureRelationMatrix;
	int *		functionLastArg;
	int *		measureFunctionLastArg;
} estimatorSynthesisState;

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
	int	length = 0;
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
	bool	stillLinear = true;

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
irPassEstimatorSynthesisInvariantLinear(State *  N, ConstraintList listHead)
{
	bool	stillLinear = true;
	for (ConstraintList  iter = listHead; stillLinear && (iter != NULL); iter=iter->next)
	{
		stillLinear = stillLinear && irPassEstimatorSynthesisExpressionLinear(N, iter->constraint);
		stillLinear = stillLinear && irPassEstimatorSynthesisExpressionLinear(N, RRL(iter->constraint));
	}
	return stillLinear;
}

/*
*	Helper function that uses in-order traversal of the AST, starting from the kNewtonIrNodeType_PconstraintList node
*	of an invariant and inserts all the constraints of the invariant in a simply linked list. The new elements
*	are appended to the list.
*	Input : currentNode (at the first call a kNewtonIrNodeType_PconstraintList node is expected),
*		the head of the simply linked list.
*	Return:	The list that contains all the constraints of the invariant.
*/
ConstraintList
irPassEstimatorSynthesisCreateConstraintList(IrNode *  currentNode, ConstraintList listHead)
{
	static int	caseId = 0;

	if (currentNode == NULL)
	{
		return listHead;
	}
	switch (currentNode->type)
	{
	case kNewtonIrNodeType_PconstraintList:
	case kNewtonIrNodeType_PcaseStatementList:
	case kNoisyIrNodeType_Xseq:
	{
		listHead = irPassEstimatorSynthesisCreateConstraintList(currentNode->irLeftChild, listHead);
		listHead = irPassEstimatorSynthesisCreateConstraintList(currentNode->irRightChild, listHead);
	}	break;
	case kNewtonIrNodeType_PpiecewiseConstraint:
	{
		listHead = irPassEstimatorSynthesisCreateConstraintList(currentNode->irLeftChild, listHead);
	}	break;
	case kNewtonIrNodeType_Pconstraint:
	{	
		if (currentNode->irLeftChild->type == kNewtonIrNodeType_PpiecewiseConstraint)
		{
			listHead = irPassEstimatorSynthesisCreateConstraintList(currentNode->irLeftChild, listHead);
		}
		else
		{
			if (listHead == NULL)
			{
				listHead = (ConstraintList)calloc(1, sizeof(ConstraintNode));
				listHead->constraint = currentNode;
				listHead->caseId = caseId;
			}
			else
			{
				ConstraintList iter;
				for (iter = listHead; iter->next != NULL; iter = iter->next);
				iter->next = (ConstraintList)calloc(1, sizeof(ConstraintNode));
				iter->next->constraint = currentNode;
				iter->next->caseId = caseId;
			}
		}
	}	break;
	case kNewtonIrNodeType_PcaseStatement:
		caseId++;
		listHead = irPassEstimatorSynthesisCreateConstraintList(currentNode->irRightChild, listHead);
		break;
		/*
		 *	Skips the condition constraint.
		 */
	default:
		break;
	}

	return listHead;
}

/*
*	Helper function that takes a ConstraintList and frees its memory.
*	NOTE : It does not free the memory of the IrNodes(so we expect nothing to go wrong),
*	only the structs created for the constraintList.
*/
ConstraintList
irPassEstimatorSynthesisFreeConstraintList(ConstraintList listHead)
{
	ConstraintList	del;
	while (listHead != NULL)
	{
		del = listHead;
		listHead = listHead->next;
		free(del);
	}
	return listHead;
}

/*
 *	Detect symbol appearance in expression.
 *	Return the symbol's identifier node in the expression.
 */
IrNode *
irPassEstimatorSynthesisDetectSymbol(State *  N, IrNode *  ExpressionXSeq, Symbol *  stateVariableSymbol)
{
	int	nth = 0;
	int	timesDetected = 0;
	IrNode *	foundMatchingIrNode = NULL;
	IrNode *	foundIdentifierIrNode = NULL;
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
	int	nth = 0;
	int	timesDetected = 0;
	IrNode *	foundMatchingIrNode = NULL;
	IrNode *	foundIdentifierIrNode = NULL;
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

/*
*	Traverses the process constraint list and counts the state dimensions(Ν). Also
*	stores all the necessary information regarding LHS symbols, names and uncertainties.
*/
void
irPassEstimatorSynthesisCountStateDimensions(estimatorSynthesisState * E,State * N)
{
	ConstraintList	iter;
	/*
	*	Allocate memory for the stateVariable names and symbols arrays and initialize them.
	*/
	E->stateVariableNames = (char**) malloc(E->processParams*sizeof(char*));
	E->stateVariableSymbols = (Symbol**) malloc(E->processParams*sizeof(Symbol*));
	E->stateVariableUncertainties = (double *)malloc(E->processParams*(sizeof(double)));

	for (int i = 0; i < E->processParams; i++)
	{
		E->stateVariableNames[i]="";	// This does not do what you think it does.
		E->stateVariableSymbols[i]=NULL;
	}
	E->processConstraints = 0;

	for (iter= E->processConstraintList; iter != NULL; iter = iter->next)
	{
		IrNode *  leafLHS = LLL(LL(iter->constraint));

		E->processConstraints++;

		for(int i = 0;i < E->processParams; i++)
		{
			if (E->stateVariableSymbols[i] == NULL)
			{
				int	needed = snprintf(NULL, 0, "STATE_%s_%d", leafLHS->tokenString, 0) + 1;
				E->stateVariableNames[i] = malloc(needed);
				snprintf(E->stateVariableNames[i], needed, "STATE_%s_%d", leafLHS->tokenString, 0);

				E->stateVariableSymbols[i] = leafLHS->symbol;

				E->stateVariableUncertainties[i] = getIdentifierSignalUncertainty(N, leafLHS->physics);

				E->stateDimension++;

				iter->stateVariableId = i;
				break;
			}
			else if (!strcmp(E->stateVariableSymbols[i]->identifier,leafLHS->tokenString))
			{
				iter->stateVariableId = i;
				break;
			}
		}

	}
}

/*
*	Traverses the measure constraint list and counts the measure dimensions(Z). Also
*	stores all the necessary information regarding LHS symbols, names and uncertainties.
*/
void
irPassEstimatorSynthesisCountMeasureDimensions(estimatorSynthesisState * E,State * N,ConstraintList listHead)
{
	ConstraintList iter;
	/*
	*	Allocate memory for the measure variable names and symbols arrays and initialize them.
	*/
	E->measureVariableNames = (char**) malloc(E->measureParams*sizeof(char*));
	E->measureVariableSymbols = (Symbol**) malloc(E->measureParams*sizeof(Symbol*));
	E->measureVariableUncertainties = (double *)malloc(E->measureParams*(sizeof(double)));

	for (int i = 0; i < E->measureParams; i++)
	{
		E->measureVariableNames[i]="";
		E->measureVariableSymbols[i]=NULL;
	}

	E->measureConstraints = 0;

	for (iter= listHead; iter != NULL; iter = iter->next)
	{
		IrNode *  leafLHS = LLL(LL(iter->constraint));

		E->measureConstraints++;

		for(int i = 0; i < E->measureParams; i++)
		{
			if (E->measureVariableSymbols[i] == NULL)
			{
				int	needed = snprintf(NULL, 0, "MEASURE_%s_%d", leafLHS->tokenString, 0) + 1;
				E->measureVariableNames[i] = malloc(needed);
				snprintf(E->measureVariableNames[i], needed, "MEASURE_%s_%d", leafLHS->tokenString, 0);

				E->measureVariableSymbols[i] = leafLHS->symbol;

				E->measureVariableUncertainties[i] = getIdentifierSignalUncertainty(N, leafLHS->physics);

				E->measureDimension++;

				iter->stateVariableId = i;
				break;
			}
			else if (!strcmp(E->measureVariableSymbols[i]->identifier,leafLHS->tokenString))
			{
				iter->stateVariableId = i;
				break;
			}
		}

	}
}

/*
*	Counts the parameters of process model that are not state variables and stores their symbols.
*/
void
irPassEstimatorSynthesisCountExtraParams(estimatorSynthesisState * E,State * N,IrNode * parameterList)
{
	E->stateExtraParamSymbols = (Symbol**) malloc(E->stateExtraParams * sizeof(Symbol*));
	int counter = 0;
	IrNode * constraintXSeq;

	for (constraintXSeq = parameterList; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{
		Symbol *	parameterSymbol = LL(constraintXSeq)->symbol;
		bool		isStateVariable = false;

		for (int i = 0; i < E->stateDimension; i++)
		{
			if (parameterSymbol == E->stateVariableSymbols[i])
			{
				isStateVariable = true;
				break;
			}
		}

		if (isStateVariable)
		{
			continue;
		}

		if (counter >= E->stateExtraParams)
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "Process invariant identifiers that are state variables appear to be less than constraints. This should not be able to happen. Please contact the developers or open an bug issue.\n");
			/*
			 *	The reason this should not be able to happen at this stage is because
			 *	it probably is a variable usage before declaration, which is caught in earlier passes.
			 */
			fatal(N, Efatal);
		}

		E->stateExtraParamSymbols[counter] = parameterSymbol;
		counter++;
	}
}

/*
*	Helper function that calculates the indendation for pretty printing the if statements.
*	Takes as argument the ammount of tabs expected to be printed.
*	Returns a string with the correct ammount of tabs needed.
*/
char *
irPassEstimatorSynthesisGetIndentation(int indent)
{
	char * str = (char*) malloc(indent+1);
	for (int i = 0; i < indent; i++)
	{
		str[i] = '\t';
	}
	str[indent] = '\0';
	return str;
}

/*
*	Recursive walk of the AST that generates the predict function code.
*	Needs to start from the process invariant's constraint list.
*/
void
irPassEstimatorSynthesisGeneratePredict(estimatorSynthesisState * E,State * N,IrNode * currentNode)
{
	static int counter = 0;
	static ConstraintList iter = NULL;
	static int indent = 1;

	if (currentNode == NULL)
	{
		return ;
	}

	switch (currentNode->type)
	{
	case kNewtonIrNodeType_PconstraintList:
	case kNoisyIrNodeType_Xseq:
		irPassEstimatorSynthesisGeneratePredict(E,N,currentNode->irLeftChild);
		irPassEstimatorSynthesisGeneratePredict(E,N,currentNode->irRightChild);
		break;
	case kNewtonIrNodeType_PpiecewiseConstraint:
		irPassEstimatorSynthesisGeneratePredict(E,N,currentNode->irLeftChild);
		break;
	case kNewtonIrNodeType_Pconstraint:
		if (currentNode->irLeftChild->type == kNewtonIrNodeType_PpiecewiseConstraint)
		{
			irPassEstimatorSynthesisGeneratePredict(E,N,currentNode->irLeftChild);
		}
		else
		{
			if (counter == 0)
			{
				iter = E->processConstraintList;
			}
			else
			{
				iter = iter->next;
			}

			flexprint(N->Fe, N->Fm, N->Fpc, "%snewState[%s] = process_%s_%d(", irPassEstimatorSynthesisGetIndentation(indent),E->stateVariableNames[iter->stateVariableId], E->stateVariableNames[iter->stateVariableId],iter->caseId);

			int currArg = 0;
			for (currArg = 0; currArg < E->processParams; currArg++)
			{
				if (E->relationMatrix[counter][currArg] == true)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "%s", E->parameterVariableSymbols[currArg]->identifier);
					if (currArg != E->functionLastArg[counter] || N->autodiff == true)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}
			if (N->autodiff == true)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "fMatrix[%s]);\n", E->stateVariableNames[iter->stateVariableId]);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ");\n");
			}
			if (N->autodiff != true)
			{
				/*
				*	Generate calculation of Jacobian of f() with standard diff
				*/
				for (int j = 0; j < E->stateDimension; j++)
				{

					if (E->relationMatrix[counter][j] == false)
					{
						continue;
					}
					else
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "%sfMatrix[%d][%d] = ", irPassEstimatorSynthesisGetIndentation(indent),iter->stateVariableId, j);
					}

					flexprint(N->Fe, N->Fm, N->Fpc, "d_process_%s_%d_d%s(",E->stateVariableNames[iter->stateVariableId], iter->caseId,E->stateVariableSymbols[j]->identifier);
					for (int currArg = 0; currArg < E->processParams; currArg++)
					{
						if (E->relationMatrix[counter][currArg] == true)
						{
							flexprint(N->Fe, N->Fm, N->Fpc, "%s, ", E->parameterVariableSymbols[currArg]->identifier);
						}
					}
					flexprint(N->Fe, N->Fm, N->Fpc, "h);\n");
				}
			}
			counter++;
		}
		break;
	case kNewtonIrNodeType_PcaseStatementList:
		flexprint(N->Fe,N->Fm,N->Fpc,"%s",irPassEstimatorSynthesisGetIndentation(indent));
		irPassEstimatorSynthesisGeneratePredict(E,N,currentNode->irLeftChild);
		flexprint(N->Fe,N->Fm,N->Fpc,"%selse ",irPassEstimatorSynthesisGetIndentation(indent));
		irPassEstimatorSynthesisGeneratePredict(E,N,currentNode->irRightChild);
		break;
	case kNewtonIrNodeType_PcaseStatement:
		if (currentNode->irLeftChild->type == kNewtonIrNodeType_Totherwise)
		{
			flexprint(N->Fe,N->Fm,N->Fpc,"\n%s{\n",irPassEstimatorSynthesisGetIndentation(indent));
		}
		else {
			flexprint(N->Fe,N->Fm,N->Fpc,"if ( ");
			irPassCConstraintTreeWalk(N,LL(currentNode));
			flexprint(N->Fe,N->Fm,N->Fpc," %s",irPassCNodeToStr(N,LRL(currentNode)->irLeftChild));
			irPassCConstraintTreeWalk(N,LRR(currentNode));
			flexprint(N->Fe,N->Fm,N->Fpc,")\n%s{\n",irPassEstimatorSynthesisGetIndentation(indent));
		}
		indent++;
		irPassEstimatorSynthesisGeneratePredict(E,N,currentNode->irRightChild);
		indent--;
		flexprint(N->Fe,N->Fm,N->Fpc,"%s}\n",irPassEstimatorSynthesisGetIndentation(indent));
		break;
		/*
		*	Skips the condition-constraint.
		*/
	default:
		break;
	}

	return ;
}

/*
*	Recursive walk of the AST that generates the update function code.
*	Needs to start from the measure invariant's constraint list.
*/
void
irPassEstimatorSynthesisGenerateUpdate(estimatorSynthesisState * E,State * N,IrNode * currentNode)
{
	static int counter = 0;
	static ConstraintList iter = NULL;
	static int indent = 1;

	if (currentNode == NULL)
	{
		return ;
	}

	switch (currentNode->type)
	{
	case kNewtonIrNodeType_PconstraintList:
	case kNoisyIrNodeType_Xseq:
		irPassEstimatorSynthesisGenerateUpdate(E,N,currentNode->irLeftChild);
		irPassEstimatorSynthesisGenerateUpdate(E,N,currentNode->irRightChild);
		break;
	case kNewtonIrNodeType_PpiecewiseConstraint:
		irPassEstimatorSynthesisGenerateUpdate(E,N,currentNode->irLeftChild);
		break;
	case kNewtonIrNodeType_Pconstraint:
		if (currentNode->irLeftChild->type == kNewtonIrNodeType_PpiecewiseConstraint)
		{
			irPassEstimatorSynthesisGenerateUpdate(E,N,currentNode->irLeftChild);
		}
		else
		{
			if (counter == 0)
			{
				iter = E->measureConstraintList;
			}
			else
			{
				iter = iter->next;
			}

			flexprint(N->Fe, N->Fm, N->Fpc, "%sHS[%s] = measure_%s_%d(", irPassEstimatorSynthesisGetIndentation(indent),E->measureVariableNames[iter->stateVariableId], E->measureVariableNames[iter->stateVariableId],iter->caseId);

			for (int currArg = 0; currArg < E->stateDimension; currArg++)
			{
				if (E->measureRelationMatrix[counter][currArg] == true)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "%s", E->measureInvariantStateVariableSymbols[currArg]->identifier);
					if (currArg != E->measureFunctionLastArg[counter])
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}
			if (N->autodiff == true)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ", hMatrix[%s]);\n", E->measureVariableNames[iter->stateVariableId]);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ");\n");
			}

			if (N->autodiff != true)
			{
				/*
				*	Generate calculation of Jacobian of h()
				*/
				for (int j = 0; j < E->stateDimension; j++)
				{
					if (E->measureRelationMatrix[counter][j] == false)
					{
						continue;
					}
					else
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "%shMatrix[%d][%d] = ", irPassEstimatorSynthesisGetIndentation(indent),iter->stateVariableId, j);
					}

					flexprint(N->Fe, N->Fm, N->Fpc, "d_measure_%s_%d_d%s ", E->measureVariableNames[iter->stateVariableId], iter->caseId,E->stateVariableSymbols[j]->identifier);
					flexprint(N->Fe, N->Fm, N->Fpc, "(");
					for (int currArg = 0; currArg < E->stateDimension; currArg++)
					{
						if (E->measureRelationMatrix[counter][currArg] == true)
						{
							flexprint(N->Fe, N->Fm, N->Fpc, "%s, ", E->measureInvariantStateVariableSymbols[currArg]->identifier);
						}
					}
					flexprint(N->Fe, N->Fm, N->Fpc, "h);\n");
				}
			}
			counter++;
		}
		break;
	case kNewtonIrNodeType_PcaseStatementList:
		flexprint(N->Fe,N->Fm,N->Fpc,"%s",irPassEstimatorSynthesisGetIndentation(indent));
		irPassEstimatorSynthesisGenerateUpdate(E,N,currentNode->irLeftChild);
		flexprint(N->Fe,N->Fm,N->Fpc,"%selse ",irPassEstimatorSynthesisGetIndentation(indent));
		irPassEstimatorSynthesisGenerateUpdate(E,N,currentNode->irRightChild);
		break;
	case kNewtonIrNodeType_PcaseStatement:
		if (currentNode->irLeftChild->type == kNewtonIrNodeType_Totherwise)
		{
			flexprint(N->Fe,N->Fm,N->Fpc,"\n%s{\n",irPassEstimatorSynthesisGetIndentation(indent));
		}
		else {
			flexprint(N->Fe,N->Fm,N->Fpc,"if ( ");
			irPassCConstraintTreeWalk(N,LL(currentNode));
			flexprint(N->Fe,N->Fm,N->Fpc," %s",irPassCNodeToStr(N,LRL(currentNode)->irLeftChild));
			irPassCConstraintTreeWalk(N,LRR(currentNode));
			flexprint(N->Fe,N->Fm,N->Fpc,")\n%s{\n",irPassEstimatorSynthesisGetIndentation(indent));
		}
		indent++;
		irPassEstimatorSynthesisGenerateUpdate(E,N,currentNode->irRightChild);
		indent--;
		flexprint(N->Fe,N->Fm,N->Fpc,"%s}\n",irPassEstimatorSynthesisGetIndentation(indent));
		break;
	default:
		break;
	}



}

void
irPassEstimatorSynthesisFreeState(estimatorSynthesisState * E)
{
	for(int i = 0; i < E->processParams; i++)
	{
		if (!strcmp(E->stateVariableNames[i],""))
		{
			continue;
		}
		free(E->stateVariableNames[i]);
	}
	free(E->stateVariableNames);

	free(E->stateVariableSymbols);
	free(E->stateVariableUncertainties);
	free(E->stateExtraParamSymbols);
	free(E->parameterVariableSymbols);

	for(int i = 0; i < E->measureParams; i++)
	{
		if (!strcmp(E->measureVariableNames[i],""))
		{
			continue;
		}
		free(E->measureVariableNames[i]);
	}
	free(E->measureVariableNames);

	free(E->measureVariableSymbols);
	free(E->measureVariableUncertainties);
	free(E->measureInvariantStateVariableSymbols);
	free(E->measureExtraParamSymbols);

	if (E->relationMatrix != NULL)
	{
		for (int i = 0; i < E->processConstraints;i++)
		{
			free(E->relationMatrix[i]);
		}
		free(E->relationMatrix);
		free(E->functionLastArg);
	}


	if (E->measureRelationMatrix != NULL)
	{
		for (int i = 0; i < E->measureConstraints;i++)
		{
			free(E->measureRelationMatrix[i]);
		}
		free(E->measureRelationMatrix);
		free(E->measureFunctionLastArg);
	}


	E->processConstraintList =  irPassEstimatorSynthesisFreeConstraintList(E->processConstraintList);
	E->measureConstraintList = irPassEstimatorSynthesisFreeConstraintList(E->measureConstraintList);

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
	estimatorSynthesisState * E = (estimatorSynthesisState *)malloc(sizeof(estimatorSynthesisState));


	E->processConstraintList = irPassEstimatorSynthesisCreateConstraintList(processInvariant->constraints,NULL);

	constraintXSeq = NULL;
	/*
	 *	Find number of invariant parameters.
	 */
	E->processParams = 0;
	for (constraintXSeq = processInvariant->parameterList; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{
		E->processParams++;
	}

	/*
	 *	Find state vector dimension (N) and book-keep process symbols.
	 */

	irPassEstimatorSynthesisCountStateDimensions(E,N);

	E->stateExtraParams = E->processParams - E->stateDimension;

	E->measureConstraintList = irPassEstimatorSynthesisCreateConstraintList(measureInvariant->constraints,NULL);

	/*
	 *	Find number of invariant parameters.
	 */

	constraintXSeq = NULL;
	E->measureParams = 0;
	for (constraintXSeq = measureInvariant->parameterList; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{
		E->measureParams++;
	}

	/*
	 *	Find measurement vector dimension (Z) and book-keep measurements' symbols.
	 */

	irPassEstimatorSynthesisCountMeasureDimensions(E,N,E->measureConstraintList);

	// E->measureExtraParams = E->measureParams - E->stateDimension - E->measureDimension;

	/*
	 *	Find and book-keep extra params of process/
	 */
	irPassEstimatorSynthesisCountExtraParams(E,N,processInvariant->parameterList);

	/*
	 *	Generate state indexing enumerator
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\nenum filterCoreStateIdx\n{\n");
	for (int i = 0; i < E->stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s,\n", E->stateVariableNames[i]);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\tSTATE_DIMENSION\n};\n\n");

	/*
	 *	Generate measure indexing enumerator
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\nenum filterMeasureIdx\n{\n");
	for (int i = 0; i < E->measureDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t%s,\n", E->measureVariableNames[i]);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\tMEASURE_DIMENSION\n};\n\n");

	/*
	 *	Generate core filter-state struct
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "typedef struct CoreState	CoreState;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "struct CoreState \n{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t *\tState\n\t */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble S[STATE_DIMENSION];\n\tmatrix *\tSm;\n\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t *\tState covariance matrix\n\t */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble P[STATE_DIMENSION][STATE_DIMENSION];\n\tmatrix *\tPm;\n\n");
	// TODO: Populate covariance matrix?
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t *\tProcess noise matrix\n\t */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble Q[STATE_DIMENSION][STATE_DIMENSION];\n\tmatrix *\tQm;\n\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t/*\n\t *\tProcess noise matrix\n\t */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble R[MEASURE_DIMENSION][MEASURE_DIMENSION];\n\tmatrix *\tRm;\n\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "};\n\n");

	/*
	 *	GENERATE INIT FUNCTION
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterInit (CoreState * cState, double S0[STATE_DIMENSION], double P0[STATE_DIMENSION][STATE_DIMENSION]) \n{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tfor (int i = %s; i < STATE_DIMENSION; i++)\n", E->stateVariableNames[0]);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tcState->S[i] = S0[i];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Sm = makeMatrix(1, STATE_DIMENSION);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Sm->data = &cState->S[0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\n\tfor (int i = %s; i < STATE_DIMENSION; i++)\n", E->stateVariableNames[0]);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tfor (int j = %s; j < STATE_DIMENSION; j++)\n", E->stateVariableNames[0]);
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t\tcState->P[i][j] = P0[i][j];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Pm = makeMatrix(STATE_DIMENSION, STATE_DIMENSION);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Pm->data = &cState->P[0][0];\n\n");

	for (int i = 0; i < E->stateDimension; i++)
	{
		for (int j = 0; j < E->stateDimension; j++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Q[%d][%d] =", i, j);
			if (i != j) {
				flexprint(N->Fe, N->Fm, N->Fpc, " %g;\n", pow(10, (log10(E->stateVariableUncertainties[i]*E->stateVariableUncertainties[j])/2))*1e-1);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, " %g;\n", E->stateVariableUncertainties[i]);
			}
		}
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Qm = makeMatrix(STATE_DIMENSION, STATE_DIMENSION);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->Qm->data = &cState->Q[0][0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	for (int i = 0; i < E->measureDimension; i++)
	{
		for (int j = 0; j < E->measureDimension; j++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\tcState->R[%d][%d] =", i, j);
			if (i != j) {
				flexprint(N->Fe, N->Fm, N->Fpc, " %g;\n", pow(10, (log10(E->measureVariableUncertainties[i]*E->measureVariableUncertainties[j])/2))*1e-1);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, " %g;\n", E->measureVariableUncertainties[i]);
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
	bool linearProcess = irPassEstimatorSynthesisInvariantLinear(N, E->processConstraintList);

	if (linearProcess == true)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterPredict (CoreState *  cState");
		for (int i = 0; i < E->stateExtraParams; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, ", double %s", E->stateExtraParamSymbols[i]->identifier);
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ")\n{\n");
		/*	If linear:
		 *	Deduce state transition matrix F
		 */
		IrNode * fMatrixIrNodes[E->stateDimension][E->stateDimension];
		int fRow = 0;
		flexprint(N->Fe, N->Fm, N->Fpc, "double fMatrix[STATE_DIMENSION][STATE_DIMENSION] = \n");
		flexprint(N->Fe, N->Fm, N->Fpc, "{ ");
		for (ConstraintList iter = E->processConstraintList; iter != NULL; fRow++, iter = iter->next)
		{
			/*
			 *	Right Hand Side Expression of the constraint:
			 *	RHSExpression = constraintlist-> constraint -> right hand side -> bypass comparison -> quantity expression
			 */
			IrNode *  RHSExpressionXSeq = RRL(iter->constraint);

			flexprint(N->Fe, N->Fm, N->Fpc, "{ ");
			for (int fColumn = 0; fColumn < E->stateDimension; fColumn++)
			{
				fMatrixIrNodes[fRow][fColumn] = irPassEstimatorSynthesisIsolateSymbolFactors(N, RHSExpressionXSeq, E->stateVariableSymbols[fColumn]);

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
		 *	Book-keep process parameter list symbols
		 */
		E->parameterVariableSymbols = (Symbol**) malloc(E->processParams*sizeof(Symbol*));
		int counter = 0;
		for (constraintXSeq = processInvariant->parameterList; constraintXSeq != NULL; counter++, constraintXSeq = constraintXSeq->irRightChild)
		{
			E->parameterVariableSymbols[counter] = constraintXSeq->irLeftChild->irLeftChild->symbol;
		}

		/*
		 *	Create relation matrix
		 *	Each symbol in stateVariableSymbols has its own row
		 *	relating it to the parameters of the invariant.
		 */

		E->relationMatrix = (bool**) malloc(E->processConstraints * sizeof(bool*));
		for (int i = 0; i < E->processConstraints;i++)
		{
			E->relationMatrix[i] = (bool*) malloc(E->stateDimension * sizeof(bool));
		}

		counter = 0;
		for (ConstraintList iter = E->processConstraintList; iter != NULL; counter++, iter=iter->next)
		{
			IrNode *  RHSExpressionXSeq = iter->constraint->irRightChild->irRightChild->irLeftChild;
			for (int mColumn = 0; mColumn < E->processParams; mColumn++)
			{
				/**
				 *	Check if symbol from parameter list appears in RHS expression
				 */
				if (irPassEstimatorSynthesisDetectSymbol(N, RHSExpressionXSeq, E->parameterVariableSymbols[mColumn]) != NULL)
				{
					E->relationMatrix[counter][mColumn] = true;
				}
				else
				{
					E->relationMatrix[counter][mColumn] = false;
				}
			}
		}

		/*
		 *	Generate single state variable prediction functions.
		 *  This is: f_1(s1,s2, ... s_j), ... f_i(x,y, ... s_j)
		 */

		counter = 0;

		E->functionLastArg = (int*) malloc(E->processConstraints * sizeof(int));
		for (ConstraintList iter = E->processConstraintList; iter != NULL; counter++, iter = iter->next)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "double\nprocess_%s_%d ", E->stateVariableNames[iter->stateVariableId],iter->caseId);
			flexprint(N->Fe, N->Fm, N->Fpc, "(");

			int lastArg = 0;
			for (lastArg = E->processParams-1; lastArg >= 0; lastArg--)
			{
				if (E->relationMatrix[counter][lastArg] == true)
				{
					E->functionLastArg[counter] = lastArg;
					break;
				}
			}

			int currArg = 0;
			for (currArg = 0; currArg < E->processParams; currArg++)
			{
				if (E->relationMatrix[counter][currArg] == true)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "double %s", E->parameterVariableSymbols[currArg]->identifier);
					if (currArg != lastArg || N->autodiff == true)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}
			if (N->autodiff == true)
			{
				IrNode *  RHSExpression = RRL(iter->constraint);
				flexprint(N->Fe, N->Fm, N->Fpc, "double Ji[STATE_DIMENSION])\n{\n");

				autoDiffGenBody(N, RHSExpression, E->stateVariableNames, E->stateVariableSymbols, E->stateDimension);

				flexprint(N->Fe, N->Fm, N->Fpc, "\n}\n\n");
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ")\n");
				irPassCGenFunctionBody(N, iter->constraint, false);
				/*
				 *	Generate partial derivatives
				 */
				for (int currDeriv = 0; currDeriv < E->stateDimension; currDeriv++)
				{
					if (E->relationMatrix[counter][currDeriv] == true)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "double\nd_process_%s_%d_d%s ", E->stateVariableNames[iter->stateVariableId], iter->caseId,E->stateVariableSymbols[currDeriv]->identifier);
						flexprint(N->Fe, N->Fm, N->Fpc, "(");
						for (currArg = 0; currArg < E->processParams; currArg++)
						{
							if (E->relationMatrix[counter][currArg] == true)
							{
								flexprint(N->Fe, N->Fm, N->Fpc, "double %s, ", E->parameterVariableSymbols[currArg]->identifier);
							}
						}
						flexprint(N->Fe, N->Fm, N->Fpc, "double h)\n{\n");
						flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble calculatedValue = 0.0;\n");

						flexprint(N->Fe, N->Fm, N->Fpc, "\tcalculatedValue = (( process_%s_%d(",E->stateVariableNames[iter->stateVariableId],iter->caseId);

						for (currArg = 0; currArg < E->processParams; currArg++)
						{
							if (E->relationMatrix[counter][currArg] == true)
							{
								if (currArg == currDeriv)
								{
									flexprint(N->Fe, N->Fm, N->Fpc, "%s+h", E->parameterVariableSymbols[currArg]->identifier);
								}
								else
								{
									flexprint(N->Fe, N->Fm, N->Fpc, "%s", E->parameterVariableSymbols[currArg]->identifier);
								}

								if (currArg != E->functionLastArg[counter])
								{
									flexprint(N->Fe, N->Fm, N->Fpc, ", ");
								}
							}
						}
						flexprint(N->Fe, N->Fm, N->Fpc, ")");

						flexprint(N->Fe, N->Fm, N->Fpc, " - process_%s_%d(", E->stateVariableNames[iter->stateVariableId],iter->caseId);
						currArg = 0;
						for (currArg = 0; currArg < E->processParams; currArg++)
						{
							if (E->relationMatrix[counter][currArg] == true)
							{
								flexprint(N->Fe, N->Fm, N->Fpc, "%s", E->parameterVariableSymbols[currArg]->identifier);
								if (currArg != E->functionLastArg[counter])
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
		for (int i = 0; i < E->stateExtraParams; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, ", double %s", E->stateExtraParamSymbols[i]->identifier);
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ")\n{\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble newState[STATE_DIMENSION];\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble fMatrix[STATE_DIMENSION][STATE_DIMENSION] = {0};\n\n");

		for (int i = 0; i < E->stateDimension; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble %s = cState->S[%s];\n", E->stateVariableSymbols[i]->identifier, E->stateVariableNames[i]);
		}

		if (N->autodiff != true)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble h = 0.0005;\n");
		}

		irPassEstimatorSynthesisGeneratePredict(E,N,processInvariant->constraints);

	}

	/*
	 *	Generate predict state
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\tmatrix Fm = {.height = STATE_DIMENSION, .width = STATE_DIMENSION, .data = &fMatrix[0][0]};\n");
	if (linearProcess)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  FSm = multiplyMatrix(&Fm, cState->Sm);\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  sn = FSm->data;\n");
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  sn = &newState[0];\n");
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  s = &cState->S[0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\tfor (int i = 0; i < STATE_DIMENSION; i++)\n\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t*s = *sn;\n\t\ts++;\n\t\tsn++;\n\t}\n\n");
	/*
	 *	Generate covariance propagation
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  Fm_T = transposeMatrix(&Fm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  FPm = multiplyMatrix(&Fm, cState->Pm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  FPFm_T = multiplyMatrix(FPm, Fm_T);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  p = cState->Pm->data;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  fpf = FPFm_T->data;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  q = cState->Qm->data;\n");
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
	E->measureInvariantStateVariableSymbols = (Symbol**) malloc(E->stateDimension * sizeof(Symbol*));
	int counter = 0;
	for (IrNode * parameterXSeq = measureInvariant->parameterList; parameterXSeq != NULL; parameterXSeq = parameterXSeq->irRightChild)
	{
		IrNode *  parameterIdentifier = parameterXSeq->irLeftChild->irLeftChild;
		for (int i = 0; i < E->stateDimension; i++)
		{
			if (strcmp(parameterIdentifier->symbol->identifier, E->stateVariableSymbols[i]->identifier) == 0)
			{
				E->measureInvariantStateVariableSymbols[i] = parameterIdentifier->symbol;
				counter++;
				break;
			}

		}
	}
	/*
	*	We declare ExtraParamSymbols[measureParams] because we have not counted the extra params yet.
	*/
	E->measureExtraParamSymbols = (Symbol**) malloc(E->measureParams * sizeof(Symbol*));
	counter = 0;
	for (constraintXSeq = measureInvariant->parameterList; constraintXSeq != NULL; constraintXSeq = constraintXSeq->irRightChild)
	{
		Symbol *	parameterSymbol = LL(constraintXSeq)->symbol;
		bool		isMeasureOrStateVariable = false;

		for (int i = 0; i < E->stateDimension; i++)
		{
			if (parameterSymbol == E->measureInvariantStateVariableSymbols[i])
			{
				isMeasureOrStateVariable = true;
				break;
			}
		}
		if (isMeasureOrStateVariable)
		{
			continue;
		}

		for (int i = 0; i < E->measureDimension; i++)
		{
			if (parameterSymbol == E->measureVariableSymbols[i])
			{
				isMeasureOrStateVariable = true;
				break;
			}
		}
		if (isMeasureOrStateVariable)
		{
			continue;
		}

		// if (counter >= E->measureExtraParams)
		// {
		// 	fatal(N, "Measurement Invariant Parameter identifiers that are either measurement variables or state variables appear to be less than #measurement_constraints + #process_constraints. Are *all* state variables present in the Measurement Invariant's parameter?\n");
		// }

		E->measureExtraParamSymbols[E->measureExtraParams] = parameterSymbol;
		E->measureExtraParams++;
	}


	/*
	 *	Determine linearity of measurement model
	 */
	bool linearMeasurement = irPassEstimatorSynthesisInvariantLinear(N, E->measureConstraintList);

	if (linearMeasurement == true)
	{
		/*
		 *	If linear:
		 *	Deduce measurement matrix H
		 */
		flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterUpdate (CoreState *  cState, double Z[MEASURE_DIMENSION]");
		for (int i = 0; i < E->measureExtraParams; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, ", double %s", E->measureExtraParamSymbols[i]->identifier);
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ")\n{\n");

		IrNode * hMatrixIrNodes[E->measureDimension][E->stateDimension];
		flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble hMatrix[MEASURE_DIMENSION][STATE_DIMENSION] = \n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\t{\n");
		int hRow = 0;
		for (ConstraintList iter = E->measureConstraintList; iter != NULL; hRow++, iter=iter->next)
		{
			/*
			 *	Find the Right Hand Side Expression of the constraint:
			 *	RHSExpression = constraintlist-> constraint -> right hand side -> bypass comparison -> quantity expression
			 */
			IrNode *  RHSExpressionXSeq = RRL(iter->constraint);
			flexprint(N->Fe, N->Fm, N->Fpc, "\t\t{ ");
			for (int hColumn = 0; hColumn < E->stateDimension; hColumn++)
			{
				hMatrixIrNodes[hRow][hColumn] = irPassEstimatorSynthesisIsolateSymbolFactors(N, RHSExpressionXSeq, E->measureInvariantStateVariableSymbols[hColumn]);
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
		E->measureRelationMatrix = (bool**) malloc(E->measureConstraints * sizeof(bool*));
		for (int i = 0; i < E->measureConstraints;i++)
		{
			E->measureRelationMatrix[i] = (bool*) calloc(E->measureParams, sizeof(bool));
		}

		counter = 0;
		for (ConstraintList iter = E->measureConstraintList; iter != NULL; counter++,iter=iter->next)
		{
			IrNode *  RHSExpressionXSeq = RRL(iter->constraint);
			for (int mColumn = 0; mColumn < E->measureParams; mColumn++)
			{
				/*
				 *	Check if symbol from parameter list appears in RHS expression
				 */
				if (irPassEstimatorSynthesisDetectSymbol(N, RHSExpressionXSeq, E->measureInvariantStateVariableSymbols[mColumn]) != NULL)
				{
					E->measureRelationMatrix[counter][mColumn] = true;
				}
				else
				{
					E->measureRelationMatrix[counter][mColumn] = false;
				}
			}
		}

		/*
		 *	Generate single state variable measurement functions.
		 *  This is: h_1(s1,s2, ... s_j), ... h_i(x,y, ... s_j)
		 */
		counter = 0;
		E->measureFunctionLastArg = (int*) malloc(E->measureConstraints * sizeof(int));
		for (ConstraintList iter = E->measureConstraintList; iter != NULL; counter++, iter=iter->next)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "double\nmeasure_%s_%d ", E->measureVariableNames[iter->stateVariableId],iter->caseId);
			flexprint(N->Fe, N->Fm, N->Fpc, "(");

			int lastArg = 0;
			for (lastArg = E->measureParams-1; lastArg >= 0; lastArg--)
			{
				if (E->measureRelationMatrix[counter][lastArg] == true)
				{
					E->measureFunctionLastArg[counter] = lastArg;
					break;
				}
			}

			// int currArg = 0;
			for (int currArg = 0; currArg < E->measureParams; currArg++)
			{
				if (E->measureRelationMatrix[counter][currArg] == true)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "double %s", E->measureInvariantStateVariableSymbols[currArg]->identifier);
					if (currArg != E->measureFunctionLastArg[counter])
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}

			if (N->autodiff == true)
			{
				IrNode *  RHSExpression = RRL(iter->constraint);
				flexprint(N->Fe, N->Fm, N->Fpc, ", double Ji[STATE_DIMENSION])\n{\n");

				autoDiffGenBody(N, RHSExpression, E->stateVariableNames, E->stateVariableSymbols, E->stateDimension);

				flexprint(N->Fe, N->Fm, N->Fpc, "\n}\n\n");
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ")\n");
				irPassCGenFunctionBody(N,iter->constraint, false);
				flexprint(N->Fe, N->Fm, N->Fpc, "\n");

				/*
				 *	Generate derivative functions of h()
				 */
				for (int currDeriv = 0; currDeriv < E->stateDimension; currDeriv++)
				{
					if (E->measureRelationMatrix[counter][currDeriv] == true)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "double\nd_measure_%s_%d_d%s ", E->measureVariableNames[iter->stateVariableId], iter->caseId, E->stateVariableSymbols[currDeriv]->identifier);
						flexprint(N->Fe, N->Fm, N->Fpc, "(");
						for (int currArg = 0; currArg < E->stateDimension; currArg++)
						{
							if (E->measureRelationMatrix[counter][currArg] == true)
							{
								flexprint(N->Fe, N->Fm, N->Fpc, "double %s, ", E->measureInvariantStateVariableSymbols[currArg]->identifier);
							}
						}
						flexprint(N->Fe, N->Fm, N->Fpc, "double h)\n{\n");
						flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble calculatedValue = 0.0;\n");

						flexprint(N->Fe, N->Fm, N->Fpc, "\tcalculatedValue = ((");
						flexprint(N->Fe, N->Fm, N->Fpc, "measure_%s_%d(", E->measureVariableNames[iter->stateVariableId],iter->caseId);

						for (int currArg = 0; currArg < E->measureParams; currArg++)
						{
							if (E->measureRelationMatrix[counter][currArg] == true)
							{
								if (currArg == currDeriv)
								{
									flexprint(N->Fe, N->Fm, N->Fpc, "%s+h", E->measureInvariantStateVariableSymbols[currArg]->identifier);
								}
								else
								{
									flexprint(N->Fe, N->Fm, N->Fpc, "%s", E->measureInvariantStateVariableSymbols[currArg]->identifier);
								}

								if (currArg != E->measureFunctionLastArg[counter])
								{
									flexprint(N->Fe, N->Fm, N->Fpc, ", ");
								}
							}
						}
						flexprint(N->Fe, N->Fm, N->Fpc, ")");

						flexprint(N->Fe, N->Fm, N->Fpc, " - measure_%s_%d(", E->measureVariableNames[iter->stateVariableId],iter->caseId);
						for (int currArg = 0; currArg < E->measureParams; currArg++)
						{
							if (E->measureRelationMatrix[counter][currArg] == true)
							{
								flexprint(N->Fe, N->Fm, N->Fpc, "%s", E->measureInvariantStateVariableSymbols[currArg]->identifier);
								if (currArg != E->measureFunctionLastArg[counter])
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

		flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterUpdate (CoreState *  cState, double Z[MEASURE_DIMENSION]");
		for (int i = 0; i < E->measureExtraParams; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, ", double %s", E->measureExtraParamSymbols[i]->identifier);
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ")\n{\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble HS[MEASURE_DIMENSION];\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble hMatrix[MEASURE_DIMENSION][STATE_DIMENSION] = {0};\n");

		// flexprint(N->Fe, N->Fm, N->Fpc, "\n{\n");
		for (int i = 0; i < E->stateDimension; i++)
		{
			if (E->measureInvariantStateVariableSymbols[i] != NULL)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble %s = cState->S[%s];\n", E->measureInvariantStateVariableSymbols[i]->identifier, E->stateVariableNames[i]);
			}
		}

		if (N->autodiff != true)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble h = 0.0005;\n");
		}

		irPassEstimatorSynthesisGenerateUpdate(E,N,measureInvariant->constraints);
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
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  hph = HPHm_T->data;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  r = cState->Rm->data;\n");
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
		flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  hs = HSm->data;\n");
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix HSm_s = { .height = MEASURE_DIMENSION, .width = 1, .data = &HS[0] };\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  HSm = &HSm_s;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  hs = &HS[0];\n");
	}

	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  z = &Z[0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\tfor (int i = 0; i < MEASURE_DIMENSION; i++)\n\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t*hs = *z - *hs;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\ths++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tz++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  KgZHS = multiplyMatrix(Kg, HSm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  s = &cState->S[0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  kgzhs = KgZHS->data;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\tfor (int i = 0; i < STATE_DIMENSION; i++)\n\t{\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\t*s += *kgzhs;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\ts++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tkgzhs++;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t// P <- P - KgHP\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  HPm = multiplyMatrix(&Hm, cState->Pm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tmatrix *  KgHPm = multiplyMatrix(Kg, HPm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  p = &cState->P[0][0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble *  kghp = KgHPm->data;\n");
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

	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble initState[STATE_DIMENSION];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble time = 0;\n");
	// flexprint(N->Fe, N->Fm, N->Fpc, "//scanf(\"%%lf\", &time);\n");
	for (int i = 0; i < E->stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\tinitState[%s] = 0;\n", E->stateVariableNames[i]);
		// flexprint(N->Fe, N->Fm, N->Fpc, "//scanf(\",%%lf\", &initState[%d]);\n", i);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble initCov[STATE_DIMENSION][STATE_DIMENSION] = {");
	for (int i = 0; i < E->stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "{");
		for (int j = 0; j < E->stateDimension; j++)
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
	for (int i = 0; i < E->stateExtraParams; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble %s;\n", E->stateExtraParamSymbols[i]->identifier);
	}

	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble prevtime = time;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble measure[MEASURE_DIMENSION];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\twhile (scanf(\"%%lf\", &time) > 0)\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t{\n");
	for (int i = 0; i < E->stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t\tscanf(\",%%*lf\");\n", i);
	}
	for (int i = 0; i < E->measureDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t\tscanf(\",%%lf\", &measure[%d]);\n", i);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tdt = time-prevtime;\n\n");
	// flexprint(N->Fe, N->Fm, N->Fpc, "filterPredict(&cs, time - prevtime);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tfilterPredict (&cs");
	for (int i = 0; i < E->stateExtraParams; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, ", %s", E->stateExtraParamSymbols[i]->identifier);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, ");\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprintf(\"Predict: %%lf\", time);\n");
	for (int i = 0; i < E->stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprintf(\", %%lf\", cs.S[%s]);\n", E->stateVariableNames[i]);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprintf(\"\\n\");\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tfilterUpdate(&cs, measure");
	for (int i = 0; i < E->measureExtraParams; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, ", %s", E->measureExtraParamSymbols[i]->identifier);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, ");\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprintf(\"Update: %%lf\", time);\n");
	for (int i = 0; i < E->stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprintf(\", %%lf\", cs.S[%s]);\n", E->stateVariableNames[i]);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprintf(\"\\n\");\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprevtime = time;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\t}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\treturn 0;\n}\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n/*\n *\tEnd of the generated .c file\n */\n");

	irPassEstimatorSynthesisFreeState(E);
	free(E);
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
