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
		stillLinear &= irPassEstimatorSynthesisExpressionLinear(N, L(constraintXSeq));
		stillLinear &= irPassEstimatorSynthesisExpressionLinear(N, LRRL(constraintXSeq));
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
			if (root->irLeftChild->type == kNewtonIrNodeType_Ptranscendental)
			{
				// TODO: RL(root) is a quantityExpression and is not assigned tokenString
				irPassEstimatorSynthesisADAnnotate(N, RL(root), root->tokenString);
			}
			else
			{
				irPassEstimatorSynthesisADAnnotate(N, root->irLeftChild, root->tokenString);
			}
			break;
		}

		case kNewtonIrNodeType_PquantityExpression:
		case kNewtonIrNodeType_PquantityTerm:
		{
			if (root->type == kNewtonIrNodeType_PquantityExpression &&
				root->tokenString == NULL &&
				parentTokenString != NULL)
			{
				int	needed = snprintf(NULL, 0, "%s_%d", parentTokenString, dUid) + 1;
				root->tokenString = malloc(needed);
				snprintf(root->tokenString, needed, "%s_%d", parentTokenString, dUid++);
			}

			IrNode * currXSeq = NULL;
			dUid = 1;
			for (currXSeq = root; currXSeq != NULL; currXSeq = currXSeq->irRightChild)
			{
				if (currXSeq->irLeftChild->type == kNewtonIrNodeType_PquantityTerm ||
					currXSeq->irLeftChild->type == kNewtonIrNodeType_PquantityFactor )
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
			/*
			 *	Code relies on irrelevant cases (e.g. operators)
			 *	hitting the default rule.
			 */
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
			if (R(root) && RL(root)->type == kNewtonIrNodeType_PexponentiationOperator)
			{
				irPassEstimatorSynthesisADGenExpressionSSA(N, L(root));
				flexprint(N->Fe, N->Fm, N->Fpc, "double %s = pow(%s, %f);\n", root->tokenString, root->irLeftChild->tokenString, RRL(root)->value);
			}
			else if (root->irLeftChild->type == kNewtonIrNodeType_Ptranscendental)
			{
				irPassEstimatorSynthesisADGenExpressionSSA(N, RL(root));
				flexprint(N->Fe, N->Fm, N->Fpc, "double %s = %s(%s);\n", root->tokenString, irPassCNodeToStr(N, LL(root)), RL(root)->tokenString);
			}
			else
			{
				irPassEstimatorSynthesisADGenExpressionSSA(N, root->irLeftChild);
				flexprint(N->Fe, N->Fm, N->Fpc, "double %s = %s;\n", root->tokenString, root->irLeftChild->tokenString);
			}
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
			if (root->irLeftChild->type == kNewtonIrNodeType_PunaryOp)
			{
				/*
				 *	root contains the unary operator
				 */
				flexprint(N->Fe, N->Fm, N->Fpc, "%s", irPassCNodeToStr(N, LL(root)));
			}
			else
			{
				/*
				 *	root contains actual first factor
				 */
				flexprint(N->Fe, N->Fm, N->Fpc, " %s ", root->irLeftChild->tokenString);
			}

			for (currXSeq = root->irRightChild; currXSeq != NULL; currXSeq = currXSeq->irRightChild)
			{
					if (currXSeq->irLeftChild->type == kNewtonIrNodeType_PlowPrecedenceOperator)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "%s", irPassCNodeToStr(N, currXSeq->irLeftChild->irLeftChild));
					}
					else if (currXSeq->irLeftChild->type == kNewtonIrNodeType_PhighPrecedenceQuantityOperator)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "%s", irPassCNodeToStr(N, LLL(currXSeq)));
					}
					else
					{
						flexprint(N->Fe, N->Fm, N->Fpc, " %s ", currXSeq->irLeftChild->tokenString);
					}
			}
			flexprint(N->Fe, N->Fm, N->Fpc, ";\n\n");
		}

		default:
			/*
			 *	Code relies on irrelevant cases (e.g. operators)
			 *	hitting the default rule.
			 */
			break;
	}
}

/**
 *	Generate reverse mode derivative SSA expression.
 *	g(id) = ds/d(id), where s is the function output
 */
void
irPassEstimatorSynthesisADGenReverse(State *  N, IrNode *  root)
{
	switch (root->type)
	{
		case kNewtonIrNodeType_PquantityExpression:
		{
			IrNode *  currXSeq = NULL;

			flexprint(N->Fe, N->Fm, N->Fpc, "double g%s = g%s;\n\n", root->irLeftChild->tokenString, root->tokenString);
			irPassEstimatorSynthesisADGenReverse(N, root->irLeftChild);
			flexprint(N->Fe, N->Fm, N->Fpc, "\n");
			for (currXSeq = root->irRightChild; currXSeq != NULL; currXSeq = currXSeq->irRightChild->irRightChild)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "double g%s = %sg%s;\n", currXSeq->irRightChild->irLeftChild->tokenString,
				                                                         irPassCNodeToStr(N, currXSeq->irLeftChild->irLeftChild),
				                                                         root->tokenString);
				irPassEstimatorSynthesisADGenReverse(N, currXSeq->irRightChild->irLeftChild);
				flexprint(N->Fe, N->Fm, N->Fpc, "\n");
			}

			break;
		}

		case kNewtonIrNodeType_PquantityTerm:
		{
			IrNode * firstFactor = root;
			IrNode * firstOperator = root->irRightChild;
			bool startsWithUnaryOp = false;
			if (root->irLeftChild->type == kNewtonIrNodeType_PunaryOp)
			{
				firstFactor = root->irRightChild;
				firstOperator = RR(root);
				startsWithUnaryOp = true;
			}
			
			for (IrNode * currXSeq = firstFactor; currXSeq != NULL; currXSeq = currXSeq->irRightChild)
			{
				if (currXSeq->irLeftChild->type == kNewtonIrNodeType_PhighPrecedenceQuantityOperator)
				{
					continue;
				}

				flexprint(N->Fe, N->Fm, N->Fpc, "double g%s = ", currXSeq->irLeftChild->tokenString);
				if (startsWithUnaryOp == true)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "%s", irPassCNodeToStr(N, LL(root)));
				}
				flexprint(N->Fe, N->Fm, N->Fpc, "g%s", root->tokenString);

				if (currXSeq == firstFactor)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, " * 1");
				}
				else
				{
					flexprint(N->Fe, N->Fm, N->Fpc, " * %s", firstFactor->irLeftChild->tokenString);
				}

				/*
				 *	Iterate over operators only (notice index update clause)
				 */
				for (IrNode *  currXSeqOp = firstOperator; currXSeqOp != NULL; currXSeqOp = RR(currXSeqOp))
				{
					if (currXSeq == currXSeqOp->irRightChild)
					{
						/*
						 *	Derivation is w.r.t. R(currXSeqOp)
						 */
						if (LLL(currXSeqOp)->type == kNewtonIrNodeType_Tdiv) {
							flexprint(N->Fe, N->Fm, N->Fpc, " * (-1/pow(%s, 2))", RL(currXSeqOp)->tokenString);
						}
						else
						{
							flexprint(N->Fe, N->Fm, N->Fpc, " * 1");
						}
					}
					else
					{
						/*
						 *	R(currXSeqOp) is simply a factor.
						 */
						flexprint(N->Fe, N->Fm, N->Fpc, "%s%s", irPassCNodeToStr(N, LLL(currXSeqOp)), RL(currXSeqOp)->tokenString);
					}
				}
				flexprint(N->Fe, N->Fm, N->Fpc, ";\n");
				irPassEstimatorSynthesisADGenReverse(N, currXSeq->irLeftChild);
				flexprint(N->Fe, N->Fm, N->Fpc, "\n");
			}

			break;
		}

		case kNewtonIrNodeType_PquantityFactor:
		{
			if (R(root) && RL(root)->type == kNewtonIrNodeType_PexponentiationOperator)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "double g%1$s = g%2$s * %3$f * pow(%1$s, %4$f);\n", L(root)->tokenString, root->tokenString, RRL(root)->value, RRL(root)->value - 1);
				irPassEstimatorSynthesisADGenReverse(N, root->irLeftChild);
			}
			else if (root->irLeftChild->type == kNewtonIrNodeType_Ptranscendental)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "double g%s = g%s * ", RL(root)->tokenString, root->tokenString);
				switch (LL(root)->type)
				{
					case kNewtonIrNodeType_Tsin:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "cos(%s);\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Tcos:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "(-sin(%s));\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Ttan:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "(1/(cos(%1$s)*cos(%1$s));\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Tcotan:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "((-sin(%1$s)*sin(%1$s)-cos(%1$s)*cos(%1$s))/(sin(%1$s)*sin(%1$s)));\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Tsec:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "(sin(%1$s/(cos(%1$s)*cos(%1$s)));\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Tcosec:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "((-cos(%1$s)/(sin(%1$s)*sin(%1$s)));\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Tarcsin:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "(1/sqrt(1 - %1$s*%1$s));\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Tarccos:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "(-1/sqrt(1 - %1$s*%1$s));\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Tarctan:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "(1/(1 + %1$s*%1$s));\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Tarccotan:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "(-1/(1 + %1$s*%1$s));\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Tarcsec:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "(1/(abs(%1$s)*sqrt(%1$s*%1$s - 1)));\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Tarccosec:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "(-1/(abs(%1$s)*sqrt(%1$s*%1$s - 1)));\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Tsinh:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "cosh(%s);\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Tcosh:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "sinh(%s);\n", RL(root)->tokenString);
						break;
					}
					case kNewtonIrNodeType_Ttanh:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, "(1 - tanh(%1$s)*tanh(%1$s));\n", RL(root)->tokenString);
						break;
					}
					default:
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ";\n");
						flexprint(N->Fe, N->Fm, N->Fperr, "Unhandled transcendental case type number '%d'.\n", LL(root)->type);
						break;
					}
				}
				irPassEstimatorSynthesisADGenReverse(N, RL(root));
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "double g%s = g%s;\n", root->irLeftChild->tokenString, root->tokenString);
				irPassEstimatorSynthesisADGenReverse(N, root->irLeftChild);
			}

			break;
		}

		case kNewtonIrNodeType_Pquantity:
		{
			if (root->irLeftChild->type == kNewtonIrNodeType_Tidentifier &&
				root->irLeftChild->physics->isConstant == false)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "g%s += g%s;\n", irPassCNodeToStr(N, root->irLeftChild), root->tokenString);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "// %1$s is a constant, g%1$s undefined\n", irPassCNodeToStr(N, root->irLeftChild));
			}
			break;
		}

		default:
			/*
			 *	Code relies on irrelevant cases (e.g. operators)
			 *	hitting the default rule.
			 */
			break;
	}
	return;
}

/*
 *	Generate reverse mode AutoDiff-aumented C code for given expression.
 */
void
irPassEstimatorSynthesisGenAutoDiffExpression(State *  N, IrNode *  expressionXSeq, char * parentTokenString)
{
	expressionXSeq->tokenString = parentTokenString;
	irPassEstimatorSynthesisADAnnotate(N, expressionXSeq, expressionXSeq->tokenString);
	irPassEstimatorSynthesisADGenExpressionSSA(N, expressionXSeq);

	/*
	 *	Initialize ds/d(id) values for the symbols of the expression
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "// Reverse calculation of derivatives\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "// g%1$s_i ≡ d%1$s/d%1$s_i\n\n", parentTokenString);
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	irPassEstimatorSynthesisADGenReverse(N, expressionXSeq);

	/*
	 *	Pass desired derivatives back
	 */

	return;
}

void
irPassEstimatorSynthesisGenAutoDiffBody(State *  N, IrNode *  expressionXSeq, char ** wrtNames, Symbol **  wrtSymbols, int wrtSymbolsLength)
{
	flexprint(N->Fe, N->Fm, N->Fpc, "// Original expression:\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "//");
	irPassCConstraintTreeWalk(N, expressionXSeq);
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "// W.R.T Symbols\n");
	for (int i = 0; i < wrtSymbolsLength; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "double g%s = 0;\n", wrtSymbols[i]->identifier);
	}

	flexprint(N->Fe, N->Fm, N->Fpc, "\n// Rest of Symbols from expression\n");
	IrNode *	expressionSymbols = findExpressionIdentifiers(N, expressionXSeq);
	for (IrNode * currSymbolNode = expressionSymbols; currSymbolNode != NULL; currSymbolNode = currSymbolNode->irRightChild)
	{
		bool inWrtSymbols = false;
		for (int i = 0; i < wrtSymbolsLength; i++)
		{
			if (wrtSymbols[i] == currSymbolNode->symbol)
			{
				inWrtSymbols = true;
				break;
			}
		}
		if ((inWrtSymbols == true) || (currSymbolNode->physics->isConstant))
		{
			continue;
		}

		flexprint(N->Fe, N->Fm, N->Fpc, "double g%s = 0;\n", currSymbolNode->symbol->identifier);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\n// ---\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\ndouble gw = 1;\n\n");
	irPassEstimatorSynthesisGenAutoDiffExpression(N, expressionXSeq, "w");

	for (int i = 0; i < wrtSymbolsLength; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "Ji[%s] = g%s;\n", wrtNames[i], wrtSymbols[i]->identifier);
	}

	flexprint(N->Fe, N->Fm, N->Fpc, "\nreturn w;\n");
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
	flexprint(N->Fe, N->Fm, N->Fpc, "cState->Pm->data = &cState->P[0][0];\n\n");

	for (int i = 0; i < stateDimension; i++)
	{
		for (int j = 0; j < stateDimension; j++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "cState->Q[%d][%d] =", i, j);
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
	flexprint(N->Fe, N->Fm, N->Fpc, "cState->Qm = makeMatrix(STATE_DIMENSION, STATE_DIMENSION);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "cState->Qm->data = &cState->Q[0][0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	for (int i = 0; i < measureDimension; i++)
	{
		for (int j = 0; j < measureDimension; j++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "cState->R[%d][%d] =", i, j);
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
	flexprint(N->Fe, N->Fm, N->Fpc, "cState->Rm = makeMatrix(MEASURE_DIMENSION, MEASURE_DIMENSION);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "cState->Rm->data = &cState->R[0][0];\n\n");

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
			flexprint(N->Fe, N->Fm, N->Fpc, ", double %s", stateExtraParamSymbols[i]->identifier);
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ") {\n");
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
					if (currArg != lastArg || N->autodiff == true)
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}
			if (N->autodiff == true)
			{
				IrNode *  RHSExpression = constraintXSeq->irLeftChild->irRightChild->irRightChild->irLeftChild;
				flexprint(N->Fe, N->Fm, N->Fpc, "double Ji[STATE_DIMENSION])\n{\n");

				irPassEstimatorSynthesisGenAutoDiffBody(N, RHSExpression, stateVariableNames, stateVariableSymbols, stateDimension);

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
						flexprint(N->Fe, N->Fm, N->Fpc, "double calculatedValue = 0.0;\n");

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
						flexprint(N->Fe, N->Fm, N->Fpc, ") ) / h );\nreturn calculatedValue;\n}\n\n");
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
			flexprint(N->Fe, N->Fm, N->Fpc, ", double %s", stateExtraParamSymbols[i]->identifier);
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ") {\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "double newState[STATE_DIMENSION];\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "double fMatrix[STATE_DIMENSION][STATE_DIMENSION] = {0};\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "\n{\n");
		for (int i = 0; i < stateDimension; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "double %s = cState->S[%s];\n", stateVariableSymbols[i]->identifier, stateVariableNames[i]);
		}


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
			flexprint(N->Fe, N->Fm, N->Fpc, "double h = 0.0005;");
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
					flexprint(N->Fe, N->Fm, N->Fpc, "h);\n");
				}
			}
		}
		flexprint(N->Fe, N->Fm, N->Fpc, "\n}\n");
	}

	/*
	 *	Generate predict state
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix Fm = {.height = STATE_DIMENSION, .width = STATE_DIMENSION, .data = &fMatrix[0][0]};\n");
	if (linearProcess)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  FSm = multiplyMatrix(&Fm, cState->Sm);\n");
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
	flexprint(N->Fe, N->Fm, N->Fpc, "double *  q = cState->Qm->data;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "for (int i = 0; i < STATE_DIMENSION*STATE_DIMENSION; i++)\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "{\n\t*p = *fpf + *q;\n\tp++;\n\tfpf++;\n\tq++;\n}\n");

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
		flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterUpdate (CoreState *  cState, double Z[MEASURE_DIMENSION]");
		for (int i = 0; i < measureExtraParams; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, ", double %s", measureExtraParamSymbols[i]->identifier);
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ") {\n");

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

			// int currArg = 0;
			for (int currArg = 0; currArg < stateDimension; currArg++)
			{
				if (relationMatrix[counter][currArg] == true)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "double %s", measureInvariantStateVariableSymbols[currArg]->identifier);
					if (currArg != functionLastArg[counter])
					{
						flexprint(N->Fe, N->Fm, N->Fpc, ", ");
					}
				}
			}

			if (N->autodiff == true)
			{
				IrNode *  RHSExpression = constraintXSeq->irLeftChild->irRightChild->irRightChild->irLeftChild;
				flexprint(N->Fe, N->Fm, N->Fpc, ", double Ji[STATE_DIMENSION])\n{\n");

				irPassEstimatorSynthesisGenAutoDiffBody(N, RHSExpression, stateVariableNames, stateVariableSymbols, stateDimension);

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
						flexprint(N->Fe, N->Fm, N->Fpc, "double\nd_measure_%s_d%s ", measureVariableNames[counter], stateVariableSymbols[currDeriv]->identifier);
						flexprint(N->Fe, N->Fm, N->Fpc, "(");
						for (int currArg = 0; currArg < stateDimension; currArg++)
						{
							if (relationMatrix[counter][currArg] == true)
							{
								flexprint(N->Fe, N->Fm, N->Fpc, "double %s, ", measureInvariantStateVariableSymbols[currArg]->identifier);
							}
						}
						flexprint(N->Fe, N->Fm, N->Fpc, "double h)\n{\n");
						flexprint(N->Fe, N->Fm, N->Fpc, "double calculatedValue = 0.0;\n");

						flexprint(N->Fe, N->Fm, N->Fpc, "calculatedValue = ((");
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
						flexprint(N->Fe, N->Fm, N->Fpc, ") ) / h );\nreturn calculatedValue;\n}\n\n");
					}
				}
			}
		}

		flexprint(N->Fe, N->Fm, N->Fpc, "void\nfilterUpdate (CoreState *  cState, double Z[MEASURE_DIMENSION]");
		for (int i = 0; i < measureExtraParams; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, ", double %s", measureExtraParamSymbols[i]->identifier);
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ") {\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "double HS[MEASURE_DIMENSION];\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "double hMatrix[MEASURE_DIMENSION][STATE_DIMENSION] = {0};\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "\n{\n");
		for (int i = 0; i < stateDimension; i++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "double %s = cState->S[%s];\n", measureInvariantStateVariableSymbols[i]->identifier, stateVariableNames[i]);
		}

		counter = 0;
		for (counter = 0; counter < measureDimension; counter++)
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "HS[%s] = measure_%s(", measureVariableNames[counter], measureVariableNames[counter]);
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
			flexprint(N->Fe, N->Fm, N->Fpc, "double h = 0.0005;");

			for (int i = 0; i < measureDimension; i++)
			{
				for (int j = 0; j < stateDimension; j++)
				{
					flexprint(N->Fe, N->Fm, N->Fpc, "hMatrix[%d][%d] = ", i, j);

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
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix Hm = { .height = MEASURE_DIMENSION, .width = STATE_DIMENSION, .data = &hMatrix[0][0] };\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix Zm = { .height = MEASURE_DIMENSION, .width = 1, .data = Z };\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "// Kg = PH^T * (HPH^T + Q)^(-1)\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  Hm_T = transposeMatrix(&Hm);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  PHm_T = multiplyMatrix(cState->Pm, Hm_T);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  HPHm_T = multiplyMatrix(&Hm, PHm_T);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "double *  hph = HPHm_T->data;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "double *  r = cState->Rm->data;\n");
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
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix HSm_s = { .height = MEASURE_DIMENSION, .width = 1, .data = &HS[0] };\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "matrix *  HSm = &HSm_s;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "double *  hs = &HS[0];\n");
	}

	flexprint(N->Fe, N->Fm, N->Fpc, "double *  z = &Z[0];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "for (int i = 0; i < MEASURE_DIMENSION; i++)\n{\n");
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
	flexprint(N->Fe, N->Fm, N->Fpc, "}\n\n");


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

	flexprint(N->Fe, N->Fm, N->Fpc, "double initState[STATE_DIMENSION];\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "double time = 0;\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "//scanf(\"%%lf\", &time);\n");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "initState[%s] = 0;\n", stateVariableNames[i]);
		flexprint(N->Fe, N->Fm, N->Fpc, "//scanf(\",%%lf\", &initState[%d]);\n", i);
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


	flexprint(N->Fe, N->Fm, N->Fpc, "double dt;\n");
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
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "dt = time-prevtime;\n\n");
	// flexprint(N->Fe, N->Fm, N->Fpc, "filterPredict(&cs, time - prevtime);\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "filterPredict (&cs");
	for (int i = 0; i < stateExtraParams; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, ", %s", stateExtraParamSymbols[i]->identifier);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, ");\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "printf(\"Predict: %%lf\", time);\n");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "printf(\", %%lf\", cs.S[%s]);\n", stateVariableNames[i]);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "printf(\"\\n\");\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "filterUpdate(&cs, measure");
	for (int i = 0; i < measureExtraParams; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, ", %s", measureExtraParamSymbols[i]->identifier);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, ");\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "printf(\"Update: %%lf\", time);\n");
	for (int i = 0; i < stateDimension; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "printf(\", %%lf\", cs.S[%s]);\n", stateVariableNames[i]);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "printf(\"\\n\");\n");
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
