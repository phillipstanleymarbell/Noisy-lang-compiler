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

// #include <errno.h>
#include <stdio.h>
#include <stdbool.h>
// #include <assert.h>
#include <stdlib.h>
#include <setjmp.h>
// #include <sys/stat.h>
// #include <sys/time.h>
// #include <time.h>
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
// #include "noisy-parser.h"
// #include "newton-parser.h"
// #include "noisy-lexer.h"
// #include "newton-lexer.h"
// #include "common-irPass-helpers.h"
// #include "common-lexers-helpers.h"
#include "common-irHelpers.h"
// #include "common-symbolTable.h"
#include "newton-types.h"
#include "newton-symbolTable.h"
#include "newton-irPass-cBackend.h"


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
// irPassEstimatorSynthesisADAnnotate(State *  N, IrNode *  root, char * parentTokenString)
autoDiffAnnotate(State *  N, IrNode *  root, char * parentTokenString)
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
				autoDiffAnnotate(N, RL(root), root->tokenString);
			}
			else
			{
				autoDiffAnnotate(N, root->irLeftChild, root->tokenString);
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
				autoDiffAnnotate(N, currXSeq->irLeftChild, root->tokenString);
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
// irPassEstimatorSynthesisADGenExpressionSSA(State *  N, IrNode *  root)
void
autoDiffGenSSA(State *  N, IrNode *  root)
{
	switch (root->type)
	{
		case kNewtonIrNodeType_Pquantity:
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "%s %s = %s;\n", N->estimatorOutputVariableType,root->tokenString, irPassCNodeToStr(N, root->irLeftChild));
			break;
		}

		case kNewtonIrNodeType_PquantityFactor:
		{
			if (R(root) && RL(root)->type == kNewtonIrNodeType_PexponentiationOperator)
			{
				autoDiffGenSSA(N, L(root));
				flexprint(N->Fe, N->Fm, N->Fpc, "%s %s = pow(%s, %f);\n", N->estimatorOutputVariableType,root->tokenString, root->irLeftChild->tokenString, RRL(root)->value);
			}
			else if (root->irLeftChild->type == kNewtonIrNodeType_Ptranscendental)
			{
				autoDiffGenSSA(N, RL(root));
				flexprint(N->Fe, N->Fm, N->Fpc, "%s %s = %s(%s);\n", N->estimatorOutputVariableType,root->tokenString, irPassCNodeToStr(N, LL(root)), RL(root)->tokenString);
			}
			else
			{
				autoDiffGenSSA(N, root->irLeftChild);
				flexprint(N->Fe, N->Fm, N->Fpc, "%s %s = %s;\n", N->estimatorOutputVariableType,root->tokenString, root->irLeftChild->tokenString);
			}
			break;
		}

		case kNewtonIrNodeType_PquantityExpression:
		case kNewtonIrNodeType_PquantityTerm:
		{
			IrNode *  currXSeq = NULL;

			for (currXSeq = root; currXSeq != NULL; currXSeq = currXSeq->irRightChild)
			{
				autoDiffGenSSA(N, currXSeq->irLeftChild);
			}

			flexprint(N->Fe, N->Fm, N->Fpc, "%s %s = ", N->estimatorOutputVariableType,root->tokenString);
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
// irPassEstimatorSynthesisADGenReverse(State *  N, IrNode *  root)
void
autoDiffGenReverseSSA(State *  N, IrNode *  root)
{
	switch (root->type)
	{
		case kNewtonIrNodeType_PquantityExpression:
		{
			IrNode *  currXSeq = NULL;

			flexprint(N->Fe, N->Fm, N->Fpc, "%s g%s = g%s;\n\n", N->estimatorOutputVariableType,root->irLeftChild->tokenString, root->tokenString);
			autoDiffGenReverseSSA(N, root->irLeftChild);
			flexprint(N->Fe, N->Fm, N->Fpc, "\n");
			for (currXSeq = root->irRightChild; currXSeq != NULL; currXSeq = currXSeq->irRightChild->irRightChild)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "%s g%s = %sg%s;\n", N->estimatorOutputVariableType,currXSeq->irRightChild->irLeftChild->tokenString,
																		 irPassCNodeToStr(N, currXSeq->irLeftChild->irLeftChild),
																		 root->tokenString);
				autoDiffGenReverseSSA(N, currXSeq->irRightChild->irLeftChild);
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

				flexprint(N->Fe, N->Fm, N->Fpc, "%s g%s = ", N->estimatorOutputVariableType,currXSeq->irLeftChild->tokenString);
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
				autoDiffGenReverseSSA(N, currXSeq->irLeftChild);
				flexprint(N->Fe, N->Fm, N->Fpc, "\n");
			}

			break;
		}

		case kNewtonIrNodeType_PquantityFactor:
		{
			if (R(root) && RL(root)->type == kNewtonIrNodeType_PexponentiationOperator)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "%s g%1$s = g%2$s * %3$f * pow(%1$s, %4$f);\n", N->estimatorOutputVariableType,L(root)->tokenString, root->tokenString, RRL(root)->value, RRL(root)->value - 1);
				autoDiffGenReverseSSA(N, root->irLeftChild);
			}
			else if (root->irLeftChild->type == kNewtonIrNodeType_Ptranscendental)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "%s g%s = g%s * ", N->estimatorOutputVariableType,RL(root)->tokenString, root->tokenString);
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
				autoDiffGenReverseSSA(N, RL(root));
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "%s g%s = g%s;\n", N->estimatorOutputVariableType,root->irLeftChild->tokenString, root->tokenString);
				autoDiffGenReverseSSA(N, root->irLeftChild);
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
// irPassEstimatorSynthesisGenAutoDiffExpression(State *  N, IrNode *  expressionXSeq, char * parentTokenString)
autoDiffGenExpression(State *  N, IrNode *  expressionXSeq, char * parentTokenString)
{
	expressionXSeq->tokenString = parentTokenString;
	autoDiffAnnotate(N, expressionXSeq, expressionXSeq->tokenString);
	autoDiffGenSSA(N, expressionXSeq);

	/*
	 *	Initialize ds/d(id) values for the symbols of the expression (comment)
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "// Reverse calculation of derivatives\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "// g%1$s_i ≡ d%1$s/d%1$s_i\n\n", parentTokenString);
	flexprint(N->Fe, N->Fm, N->Fpc, "\n");

	autoDiffGenReverseSSA(N, expressionXSeq);

	/*
	 *	Pass desired derivatives back
	 */

	return;
}

/*
 *	Given an expression AST ($expressionXSeq), generate a C
 *	function body that computes the expression in SSA form,
 *	as well as the reverse SSA  of its derivatives. 
 *
 *	NOTE: This function is coupled with the estimator
 *	synthesis backend in that it takes C enumerator names in
 *	$wrtNames.
 */
// irPassEstimatorSynthesisGenAutoDiffBody(State *  N, IrNode *  expressionXSeq, char ** wrtNames, Symbol **  wrtSymbols, int wrtSymbolsLength)
void
autoDiffGenBody(State *  N, IrNode *  expressionXSeq, char ** wrtNames, Symbol **  wrtSymbols, int wrtSymbolsLength)
{
	flexprint(N->Fe, N->Fm, N->Fpc, "// Original expression:\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "//");
	irPassCConstraintTreeWalk(N, expressionXSeq);
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "// W.R.T Symbols\n");
	for (int i = 0; i < wrtSymbolsLength; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "%s g%s = 0;\n", N->estimatorOutputVariableType,wrtSymbols[i]->identifier);
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

		flexprint(N->Fe, N->Fm, N->Fpc, "%s g%s = 0;\n", N->estimatorOutputVariableType,currSymbolNode->symbol->identifier);
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "\n// ---\n");

	flexprint(N->Fe, N->Fm, N->Fpc, "\n%s gad_ = 1;\n\n", N->estimatorOutputVariableType);
	autoDiffGenExpression(N, expressionXSeq, "ad_");

	for (int i = 0; i < wrtSymbolsLength; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "Ji[%s] = g%s;\n", wrtNames[i], wrtSymbols[i]->identifier);
	}

	flexprint(N->Fe, N->Fm, N->Fpc, "\nreturn ad_;\n");
}