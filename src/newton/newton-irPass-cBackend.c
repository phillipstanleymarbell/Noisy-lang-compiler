/*
	Authored 2018. Youchao Wang.

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
#include "newton-types.h"
#include "newton-symbolTable.h"

bool
irPassCIsExpectedTypePresentInRightChild(IrNode *  parentNode, IrNodeType expectedType)
{
	bool	isExpectedTypePresent = false;

	if (parentNode->irRightChild == NULL)
	{
		return isExpectedTypePresent;
	}

	if (parentNode->irRightChild->type == expectedType)
	{
		isExpectedTypePresent = true;
	}

	return isExpectedTypePresent;
}

bool
irPassCIsLastParameter(IrNode *  parentNode)
{
	bool	isLastParameter = false;


	return isLastParameter;
}

bool
irPassCIsConstraintHumanWritten(IrNode *  parentNode)
{
	char *	checkString = NULL;
	int	needed = snprintf(NULL, 0, "%s", parentNode->sourceInfo->fileName) + 1;
	checkString = malloc(needed);
	snprintf(checkString, needed, "%s", parentNode->sourceInfo->fileName);

	bool	isConstraintHumanWritten = true;

	if (strcmp(checkString, "GeneratedByDA") == 0)
	{
		isConstraintHumanWritten = false;
	}

	return isConstraintHumanWritten;
}

/*
 *	This function maps a node to a token string for C. Returned
 *	string from this function can/should be 'free'ed if not useful anymore.
 *	Adapted from smtBackEnd, author Zhengyang Gu.
 */
char *
irPassCNodeToStr(State *  N, IrNode *  node)
{
	char *	output = NULL;
	switch(node->type)
	{
		case kNewtonIrNodeType_TnumericConst:
		{
			int needed = snprintf(NULL, 0, "%f", node->value) + 1;
			output = malloc(needed);
			snprintf(output, needed, "%f", node->value);
			break;
		}

		case kNewtonIrNodeType_Tidentifier:
		{
			int needed = snprintf(NULL, 0, "%s", node->tokenString) + 1;
			output = malloc(needed);
			snprintf(output, needed, "%s", node->tokenString);
			break;
		}

		case kNewtonIrNodeType_Tplus:
		{
			output = malloc(4);
			strcpy(output, " + ");
			break;
		}

		case kNewtonIrNodeType_Tminus:
		{
			output = malloc(4);
			strcpy(output, " - ");
			break;
		}

		case kNewtonIrNodeType_Tdiv:
		{
			output = malloc(4);
			strcpy(output, " / ");
			break;
		}

		case kNewtonIrNodeType_Tmul:
		{
			output = malloc(4);
			strcpy(output, " * ");
			break;
		}
		/*
		 *	There is no operator for power exponents
		 *	operation in C. We therefore must use
		 *	double pow( double para, double exponent)
		 *	This switch case prints out the ','
		 */
		case kNewtonIrNodeType_Texponent:
		{
			output = malloc(4);
			strcpy(output, ",");
			break;
		}

		default:
		{
			fatal(N, EtokenInSMT);
		}
	}

	return output;
}

void
irPassCSearchAndPrintNodeType(State *  N, IrNode *  root, IrNodeType expectedType, bool isLastParameter, int depthWalk)
{
	if (root == NULL)
	{
		return;
	}

	static int	depth = 0;

	if (root->irRightChild == NULL && root->irLeftChild == NULL
		&& root->type == expectedType)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "double");
		flexprint(N->Fe, N->Fm, N->Fpc, " %s", root->tokenString);
		if(isLastParameter == true)
		{
			depth += 1;
		}
		if(isLastParameter == false || (isLastParameter == true && depth != depthWalk))
		{
			flexprint(N->Fe, N->Fm, N->Fpc, ", ");
		}
		else if (isLastParameter == true && depth == depthWalk)
		{
			depth = 0;
		}
		return;
	}

	irPassCSearchAndPrintNodeType(N, root->irLeftChild, expectedType, isLastParameter, depthWalk);
	irPassCSearchAndPrintNodeType(N, root->irRightChild, expectedType, isLastParameter, depthWalk);

	return;
}

int
irPassCCountRemainingParameters(State *  N, IrNode *  root, int count)
{
	//static int	count = 0;

	if (root == NULL)
	{
		return count;
	}

	if (root->irRightChild == NULL && root->irLeftChild == NULL
		&& root->type == kNewtonIrNodeType_Tidentifier)
	{
		count += 1;
		return count;
	}

	count = irPassCCountRemainingParameters(N, root->irLeftChild, count);
	count = irPassCCountRemainingParameters(N, root->irRightChild, count);

	return count;
}

void
irPassCConstraintTreeWalk(State *  N, IrNode *  root)
{
	/*
	 *	This branch should be reached for return.
	 *	It completes a preorder tree traversal.
	 */
	if (root == NULL)
	{
		return;
	}

	static bool	isleftBracketPrinted = false;

	/*
	 *	There is no operator for power exponents
	 *	operation in C. We therefore must use
	 *	double pow( double para, double exponent)
	 */
	if (irPassCIsExpectedTypePresentInRightChild(root, kNewtonIrNodeType_PhighPrecedenceBinaryOp) == true)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "pow(");
		isleftBracketPrinted = true;
	}

	if (root->irRightChild == NULL && root->irLeftChild == NULL)
	{
		char *		nodeTokenString = NULL;
		nodeTokenString = irPassCNodeToStr(N, root);

		flexprint(N->Fe, N->Fm, N->Fpc, " %s", nodeTokenString);
		/*
		 *	Print out the right bracket of pow() function.
		 */
		if (isleftBracketPrinted == true && root->type == kNewtonIrNodeType_TnumericConst)
		{
			
			flexprint(N->Fe, N->Fm, N->Fpc, ")");
			isleftBracketPrinted = false;
		}

		return;
	}

	irPassCConstraintTreeWalk(N, root->irLeftChild);
	irPassCConstraintTreeWalk(N, root->irRightChild);
}

/*
 *	Generate format:
 *	{
 *		double calculatedValue = 0.0;
 		calculatedValue = parameter1 ^ exponent1 * parameter2 ^ exponent2 ... ;
 *		return calculatedValue;
 *	}
 */
void
irPassCGenFunctionBody(State *  N, IrNode *  constraint, bool isLeft)
{
	/*
	 *	Declare
	 */	
	flexprint(N->Fe, N->Fm, N->Fpc, "{\n\tdouble calculatedValue = 0.0;\n");

	/*
	 *	Calculation
	 */	
	flexprint(N->Fe, N->Fm, N->Fpc, "\tcalculatedValue =");

	if (isLeft == false)
	{
		irPassCConstraintTreeWalk(N, constraint->irRightChild->irRightChild->irLeftChild->irLeftChild);
	}
	else
	{
		irPassCConstraintTreeWalk(N, constraint->irLeftChild->irLeftChild);
	}

	flexprint(N->Fe, N->Fm, N->Fpc, ";\n");

	/*
	 *	Return
	 */
	flexprint(N->Fe, N->Fm, N->Fpc, "\n\treturn calculatedValue;\n}\n\n");
}

/*
 *	Generate format: (double parameter1, double parameter 2 ...)
 */
void
irPassCGenFunctionArgument(State *  N, IrNode *  constraint, bool isLeft)
{
	flexprint(N->Fe, N->Fm, N->Fpc, "(");

	IrNode *	constraintsXSeq;

	if (isLeft == false)
	{
		constraintsXSeq = constraint->irRightChild->irRightChild->irLeftChild->irLeftChild;
	}
	else
	{
		constraintsXSeq = constraint->irLeftChild->irLeftChild;
	}

	while (constraintsXSeq->irRightChild != NULL)
	{
		irPassCSearchAndPrintNodeType(N, constraintsXSeq->irLeftChild, kNewtonIrNodeType_Tidentifier, false, -1);
		constraintsXSeq = constraintsXSeq->irRightChild;
	}

	irPassCSearchAndPrintNodeType(N, constraintsXSeq->irLeftChild, kNewtonIrNodeType_Tidentifier, true,
					irPassCCountRemainingParameters(N, constraintsXSeq->irLeftChild, 0));
	flexprint(N->Fe, N->Fm, N->Fpc, ")\n");
}

/*
 *	Generate format: double functionName
 */
void
irPassCGenFunctionName(State *  N, IrNode *  constraints, int countFunction)
{
	if (irPassCIsConstraintHumanWritten(constraints))
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "double\nhumanWrittenConstraintRHS%d", countFunction);
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "double\n%sRHS%d",
				constraints->irLeftChild->irLeftChild->irLeftChild->irLeftChild->tokenString, countFunction);
	}
}

void
irPassCProcessInvariantList(State *  N)
{
	/*
	 *	Check if the file called is simply an include.nt
	 */
	if (N->invariantList == NULL)
	{
		return;
	}

	Invariant *	invariant = N->invariantList;
	IrNode *	constraintXSeq = invariant->constraints->irParent;

	flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tGenerated .c file from Newton\n */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n#include <math.h>\n\n");
	while(invariant)
	{
		for (int countFunction = 0; constraintXSeq != NULL; countFunction++, constraintXSeq = constraintXSeq->irRightChild)
		{
			assert(constraintXSeq->irLeftChild->type == kNewtonIrNodeType_Pconstraint);
			/*
			 *	(1) For human written constraints, e.g. we end up having
			 *	x + y + z < 5 * m / s ** 2, we create two functions, one 
			 *	for LHS, and another for RHS.
			 *
			 *	(2) For machine calculated pi groups, we construct only
			 *	one function to return the value of RHS.
			 */
			if (irPassCIsConstraintHumanWritten(constraintXSeq->irLeftChild))
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "double\nhumanWrittenConstraintLHS%d", countFunction);
				irPassCGenFunctionArgument(N, constraintXSeq->irLeftChild, true);
				irPassCGenFunctionBody(N, constraintXSeq->irLeftChild, true);
			}

			irPassCGenFunctionName(N, constraintXSeq->irLeftChild, countFunction);
			irPassCGenFunctionArgument(N, constraintXSeq->irLeftChild, false);
			irPassCGenFunctionBody(N, constraintXSeq->irLeftChild, false);
		}

		invariant = invariant->next;
	}
	flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tEnd of the generated .c file\n */\n");

}

void
irPassCBackend(State *  N)
{
	FILE *	cFile;

	irPassCProcessInvariantList(N);

	if (N->outputCFilePath)
	{
		cFile = fopen(N->outputCFilePath, "w");

		if (cFile == NULL)
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\n%s: %s.\n", Eopen, N->outputCFilePath);
			consolePrintBuffers(N);
		}

		fprintf(cFile, "%s", N->Fpc->circbuf);
		fclose(cFile);
	}

}
