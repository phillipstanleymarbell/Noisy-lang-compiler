/*
	Authored 2018. Youchao Wang.
	Updated 2020. Orestis Kaparounakis.

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

extern const char *		gNewtonTokenDescriptions[kCommonIrNodeTypeMax];

bool
irPassCIsExpectedTypePresentInRightChild(State *  N, IrNode *  parentNode, IrNodeType expectedType)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassCIsExpectedTypePresentInRightChild);

	bool	isExpectedTypePresent = false;

	if (parentNode->irRightChild == NULL)
	{
		return isExpectedTypePresent;
	}

	if (parentNode->irRightChild->irLeftChild->type == expectedType)
	{
		isExpectedTypePresent = true;
	}

	return isExpectedTypePresent;
}

bool
irPassCIsConstraintHumanWritten(State *  N, IrNode *  parentNode)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassCIsConstraintHumanWritten);

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
 *	This function maps a node to a token string for C.
 *	Adapted from smtBackEnd, author Zhengyang Gu.
 */
char *
irPassCNodeToStr(State *  N, IrNode *  node)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassCNodeToStr);

	char *	output = NULL;
	switch(node->type)
	{
		case kNewtonIrNodeType_PnumericConst:
		{
			/*
			 *	Either realConst or integerConst, whose case is below.
			 *	Recursion to avoid direct jump.
			 */
			output = irPassCNodeToStr(N, node->irLeftChild);
			break;
		}

		case kNewtonIrNodeType_TrealConst:
		{
			int needed = snprintf(NULL, 0, "%f", node->value) + 1;
			output = malloc(needed);
			snprintf(output, needed, "%f", node->value);
			break;
		}

		case kNewtonIrNodeType_TintegerConst:
		{
			int needed = snprintf(NULL, 0, "%d", node->integerValue) + 1;
			output = malloc(needed);
			snprintf(output, needed, "%d", node->integerValue);
			break;
		}

		case kNewtonIrNodeType_Tidentifier:
		{	
			if (node->physics->isConstant == true) {
				int needed = snprintf(NULL, 0, "%f", node->value) + 1;
				output = malloc(needed);
				snprintf(output, needed, "%f", node->value);
			} else {
				int needed = snprintf(NULL, 0, "%s", node->tokenString) + 1;
				output = malloc(needed);
				snprintf(output, needed, "%s", node->tokenString);
			}
			break;
		}

		case kNewtonIrNodeType_Tplus:
		{
			output = malloc(4*sizeof(char));
			strcpy(output, " + ");
			break;
		}

		case kNewtonIrNodeType_Tminus:
		{
			output = malloc(4*sizeof(char));
			strcpy(output, " - ");
			break;
		}

		case kNewtonIrNodeType_Tdiv:
		{
			output = malloc(4*sizeof(char));
			strcpy(output, " / ");
			break;
		}

		case kNewtonIrNodeType_Tmul:
		{
			output = malloc(4*sizeof(char));
			strcpy(output, " * ");
			break;
		}
		/*
		 *	There is no operator for power exponents
		 *	operation in C. We therefore must use
		 *	double pow( double para, double exponent)
		 *	This switch case prints out the ','
		 */
		case kNewtonIrNodeType_Texponentiation:
		{
			output = malloc(4*sizeof(char));
			strcpy(output, ",");
			break;
		}

		case kNewtonIrNodeType_Tlog2:
		case kNewtonIrNodeType_Tlog10:
		case kNewtonIrNodeType_Tln:
		case kNewtonIrNodeType_Tsqrt:
		case kNewtonIrNodeType_Texp:
		case kNewtonIrNodeType_Ttanh:
		case kNewtonIrNodeType_Tcosh:
		case kNewtonIrNodeType_Tsinh:
		case kNewtonIrNodeType_Tarctan:
		case kNewtonIrNodeType_Tarccos:
		case kNewtonIrNodeType_Tarcsin:
		case kNewtonIrNodeType_Ttan:
		case kNewtonIrNodeType_Tcos:
		case kNewtonIrNodeType_Tsin:
		{			
			output = strdup(gNewtonTokenDescriptions[node->type]);
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
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassCSearchAndPrintNodeType);

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

void
irPassCSearchAndCreateArgList(State *  N, IrNode *  root, IrNodeType expectedType, char **argList, int argumentIndex)
{
	if (root == NULL)
	{
		return;
	}

	if (root->irRightChild == NULL && root->irLeftChild == NULL
		&& root->type == expectedType)
	{
		argList[argumentIndex] = (char *) malloc((strlen(root->tokenString) + 1) * sizeof(char));
		strcpy(argList[argumentIndex], root->tokenString);
		//flexprint(N->Fe, N->Fm, N->Fpc, "%s", root->tokenString);
		
		return;
	}

	irPassCSearchAndCreateArgList(N, root->irLeftChild, expectedType, argList, argumentIndex);
	
	return;
}

int
irPassCCountRemainingParameters(State *  N, IrNode *  root, int depth)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassCCountRemainingParameters);

	if (root == NULL)
	{
		return depth;
	}

	if (root->irRightChild == NULL && root->irLeftChild == NULL
		&& root->type == kNewtonIrNodeType_Tidentifier)
	{
		depth += 1;
		return depth;
	}

	depth = irPassCCountRemainingParameters(N, root->irLeftChild, depth);
	depth = irPassCCountRemainingParameters(N, root->irRightChild, depth);

	return depth;
}

void
irPassCConstraintTreeWalk(State *  N, IrNode *  root)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassCConstraintTreeWalk);

	/*
	 *	This branch should be reached for return.
	 *	It completes a preorder tree traversal.
	 */
	if (root == NULL)
	{
		return;
	}

	static bool	isPowLeftBracketPrinted = false;

	if (root->type == kNewtonIrNodeType_PquantityExpression)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, " (");
	}
	

	/*
	 *	There is no operator for power exponents
	 *	operation in C. We therefore must use
	 *	double pow( double para, double exponent)
	 */
	if (irPassCIsExpectedTypePresentInRightChild(N, root, kNewtonIrNodeType_PexponentiationOperator) == true)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, " pow(");
		isPowLeftBracketPrinted = true;
	}

	if (root->irRightChild == NULL && root->irLeftChild == NULL)
	{
		char *		nodeTokenString = NULL;
		nodeTokenString = irPassCNodeToStr(N, root);

		flexprint(N->Fe, N->Fm, N->Fpc, " %s", nodeTokenString);
		/*
		 *	Print out the right bracket of pow() function.
		 */
		if (isPowLeftBracketPrinted == true && 
		   (root->type == kNewtonIrNodeType_PnumericConst ||
		    root->type == kNewtonIrNodeType_TrealConst	||
		    root->type == kNewtonIrNodeType_TintegerConst))
		{
			flexprint(N->Fe, N->Fm, N->Fpc, ")");
			isPowLeftBracketPrinted = false;
		}

		return;
	}

	irPassCConstraintTreeWalk(N, root->irLeftChild);
	irPassCConstraintTreeWalk(N, root->irRightChild);
	
	if (root->type == kNewtonIrNodeType_PquantityExpression)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, " )");
	}
}

/*
 *	Generate format:
 *	{
 *		double calculatedValue = 0.0;
 		calculatedValue = pow(parameter1 , exponent1) * parameter2 ... ;
 *		return calculatedValue;
 *	}
 */
void
irPassCGenFunctionBody(State *  N, IrNode *  constraint, bool isLeft)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassCGenFunctionBody);

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
		irPassCConstraintTreeWalk(N, constraint->irRightChild->irRightChild);
	}
	else
	{
		irPassCConstraintTreeWalk(N, constraint->irLeftChild);
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
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassCGenFunctionArgument);

	flexprint(N->Fe, N->Fm, N->Fpc, "(");

	IrNode *	constraintsXSeq;

	if (isLeft == false)
	{
		/*
		 * Right child of constraint is XSeq, the left child of which
		 * is the RHS quantityExpression. 
		 */
		constraintsXSeq = constraint->irRightChild->irRightChild->irLeftChild;
	}
	else
	{
		constraintsXSeq = constraint->irLeftChild;
	}

	while (constraintsXSeq->irRightChild != NULL)
	{
		irPassCSearchAndPrintNodeType(N, constraintsXSeq->irLeftChild, kNewtonIrNodeType_Tidentifier, false, -1);
		constraintsXSeq = constraintsXSeq->irRightChild;
	}

	irPassCSearchAndPrintNodeType(N, constraintsXSeq->irLeftChild, kNewtonIrNodeType_Tidentifier, true,
					irPassCCountRemainingParameters(N, constraintsXSeq->irLeftChild, 0));
	flexprint(N->Fe, N->Fm, N->Fpc, ")");
}

/*
 *	Generate format: double functionName
 */
void
irPassCGenFunctionName(State *  N, IrNode *  constraints, int countFunction)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassCGenFunctionName);

	if (irPassCIsConstraintHumanWritten(N, constraints))
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
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassCProcessInvariantList);

	/*
	 *	Check if the file called is simply an include.nt
	 */
	if (N->invariantList == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tPlease specify a valid file\n */\n");
		return;
	}

	Invariant *	invariant = N->invariantList;

	if (invariant->constraints == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tNo constraints created\n */\n");
		return;
	}

	IrNode *	constraintXSeq = invariant->constraints;

	IrNode *	parameterListXSeq = invariant->parameterList->irParent->irLeftChild;

	char ** argumentsList;
		
	int index = 0;

	int countFunction = 0;

	flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tGenerated .c file from Newton\n */\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n#include <stdlib.h>\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n#include <stdio.h>\n");
	flexprint(N->Fe, N->Fm, N->Fpc, "\n#include <math.h>\n\n");

	while(invariant)
	{
		for (countFunction = 0; constraintXSeq != NULL; countFunction++, constraintXSeq = constraintXSeq->irRightChild)
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
			if (irPassCIsConstraintHumanWritten(N, constraintXSeq->irLeftChild))
			{
				flexprint(N->Fe, N->Fm, N->Fpc, "double\nhumanWrittenConstraintLHS%d", countFunction);
				irPassCGenFunctionArgument(N, constraintXSeq->irLeftChild, true);
				irPassCGenFunctionBody(N, constraintXSeq->irLeftChild, true);
			}

			irPassCGenFunctionName(N, constraintXSeq->irLeftChild, countFunction);
			irPassCGenFunctionArgument(N, constraintXSeq->irLeftChild, false);
			irPassCGenFunctionBody(N, constraintXSeq->irLeftChild, false);
		}
		
		
		argumentsList = (char **) malloc(invariant->dimensionalMatrixColumnCount * sizeof(char *));

		/*
		*	Print calculation function
		*/
		countFunction = 0;	
		flexprint(N->Fe, N->Fm, N->Fpc, "double\n%s%d",
				invariant->identifier, countFunction);

		flexprint(N->Fe, N->Fm, N->Fpc, "(");
		
		while (parameterListXSeq != NULL) 
		{
			irPassCSearchAndCreateArgList(N, parameterListXSeq->irLeftChild, kNewtonIrNodeType_Tidentifier, argumentsList, index);
			parameterListXSeq = parameterListXSeq->irRightChild;
			index++;
		}
		
		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "double %s", argumentsList[index]);
			if (index < invariant->dimensionalMatrixColumnCount - 1)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ", ");
			}
		}

		flexprint(N->Fe, N->Fm, N->Fpc, ")\n");

		/*
		*	Declare
		*/	
		flexprint(N->Fe, N->Fm, N->Fpc, "{\n\tdouble calculatedProportion = 0.0;\n");

		/*
		*	Calculation
		*/	
		flexprint(N->Fe, N->Fm, N->Fpc, "\n\tcalculatedProportion = ");

		/*
		 *	We construct a temporary array to re-locate the positions of the permuted parameters
		 */
		int *		tmpPosition = (int *)calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));

		index = 0;
		for (int countKernel = 0; countKernel < invariant->numberOfUniqueKernels; countKernel++)
		{
			for (int j = 0; j < invariant->dimensionalMatrixColumnCount; j++)
			{
				tmpPosition[invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + j]] = j;
			}

			for (int col = 0; col < invariant->kernelColumnCount; col++)
			{
				for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
				{
					if (invariant->nullSpace[countKernel][tmpPosition[row]][col] != 0) 
					{
						//flexprint(N->Fe, N->Fm, N->Fpc, "pow(%c%c, ", 'P'+(row/10), '0'+ (row%10) );
						flexprint(N->Fe, N->Fm, N->Fpc, "pow(" );
						flexprint(N->Fe, N->Fm, N->Fpc, "%s", argumentsList[index]);
						flexprint(N->Fe, N->Fm, N->Fpc, ", %f) * ", invariant->nullSpace[countKernel][tmpPosition[row]][col]);
					}
					index++;
				}
			}
			/*
			*   FIXME avoid printing 1.0 to complete the sentence;
			*/
			flexprint(N->Fe, N->Fm, N->Fpc, "1.0;");
		}

		/*
		*	Return
		*/	
		flexprint(N->Fe, N->Fm, N->Fpc, "\n\n\treturn calculatedProportion;");

		flexprint(N->Fe, N->Fm, N->Fpc, "\n}\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "\n\nint \nmain(int argc, char *argv[])\n{\n");

		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble %s", argumentsList[index]);
			if (index < invariant->dimensionalMatrixColumnCount - 1)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ";\n");
			}
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ";");

		/*
		*	Declare
		*/	
		flexprint(N->Fe, N->Fm, N->Fpc, "\n\tdouble calculatedProportion = 0.0;\n\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "\tif (argc < %d) {\n",invariant->dimensionalMatrixColumnCount+1);
		flexprint(N->Fe, N->Fm, N->Fpc, "\t\tprintf(\"Usage is exec_name ");

		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "%s ", argumentsList[index]);
		}

		flexprint(N->Fe, N->Fm, N->Fpc, "\\n\");\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\t\treturn -1;\n\t}\n\n");

		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\t%s = atof(argv[%d]);\n", argumentsList[index], index+1);
		}

		/*
		*	Calculation
		*/	
		flexprint(N->Fe, N->Fm, N->Fpc, "\n\tcalculatedProportion = ");

		countFunction = 0;	
		flexprint(N->Fe, N->Fm, N->Fpc, "%s%d", invariant->identifier, countFunction);

		flexprint(N->Fe, N->Fm, N->Fpc, "(");

		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "%s", argumentsList[index]);
			if (index < invariant->dimensionalMatrixColumnCount - 1)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ", ");
			}
		}

		flexprint(N->Fe, N->Fm, N->Fpc, ");\n\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "\tprintf(\"Calculated proportion is %%f.\\n\", calculatedProportion);\n\n");

		flexprint(N->Fe, N->Fm, N->Fpc, "\treturn 0;\n");
		flexprint(N->Fe, N->Fm, N->Fpc, "\n}\n\n");

		free(tmpPosition);
		
		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			free(argumentsList[index]);
		}
		free(argumentsList);

		invariant = invariant->next;
	}
	
	flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tEnd of the generated .c file\n */\n");
}

void
irPassCBackend(State *  N)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassCBackend);

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
