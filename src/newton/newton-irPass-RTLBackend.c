/*
	Authored 2019. Vasileios Tsoutsouras.

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

#define Q_PARAMETER 15
#define N_PARAMETER 32

bool
irPassRTLIsExpectedTypePresentInRightChild(IrNode *  parentNode, IrNodeType expectedType)
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

/*
 *	This function maps a node to a token string for C.
 *	Adapted from smtBackEnd, author Zhengyang Gu.
 */
char *
irPassRTLNodeToStr(State *  N, IrNode *  node)
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
irPassRTLSearchAndPrintNodeType(State *  N, IrNode *  root, IrNodeType expectedType, bool isLastParameter, int depthWalk)
{
	if (root == NULL)
	{
		return;
	}

	static int	depth = 0;

	if (root->irRightChild == NULL && root->irLeftChild == NULL
		&& root->type == expectedType)
	{
		flexprint(N->Fe, N->Fm, N->Fprtl, "double");
		flexprint(N->Fe, N->Fm, N->Fprtl, " %s", root->tokenString);
		if(isLastParameter == true)
		{
			depth += 1;
		}
		if(isLastParameter == false || (isLastParameter == true && depth != depthWalk))
		{
			flexprint(N->Fe, N->Fm, N->Fprtl, ", ");
		}
		else if (isLastParameter == true && depth == depthWalk)
		{
			depth = 0;
		}
		return;
	}

	irPassRTLSearchAndPrintNodeType(N, root->irLeftChild, expectedType, isLastParameter, depthWalk);
	irPassRTLSearchAndPrintNodeType(N, root->irRightChild, expectedType, isLastParameter, depthWalk);

	return;
}

void
irPassRTLSearchAndCreateArgList(State *  N, IrNode *  root, IrNodeType expectedType, char **argList, int argumentIndex)
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
		//flexprint(N->Fe, N->Fm, N->Fprtl, "%s", root->tokenString);
		
		return;
	}

	irPassRTLSearchAndCreateArgList(N, root->irLeftChild, expectedType, argList, argumentIndex);
	
	return;
}

int
irPassRTLCountRemainingParameters(State *  N, IrNode *  root, int depth)
{
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

	depth = irPassRTLCountRemainingParameters(N, root->irLeftChild, depth);
	depth = irPassRTLCountRemainingParameters(N, root->irRightChild, depth);

	return depth;
}

void
irPassRTLConstraintTreeWalk(State *  N, IrNode *  root)
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
	if (irPassRTLIsExpectedTypePresentInRightChild(root, kNewtonIrNodeType_PhighPrecedenceBinaryOp) == true)
	{
		flexprint(N->Fe, N->Fm, N->Fprtl, "pow(");
		isleftBracketPrinted = true;
	}

	if (root->irRightChild == NULL && root->irLeftChild == NULL)
	{
		char *		nodeTokenString = NULL;
		nodeTokenString = irPassRTLNodeToStr(N, root);

		flexprint(N->Fe, N->Fm, N->Fprtl, " %s", nodeTokenString);
		/*
		 *	Print out the right bracket of pow() function.
		 */
		if (isleftBracketPrinted == true && root->type == kNewtonIrNodeType_TnumericConst)
		{
			
			flexprint(N->Fe, N->Fm, N->Fprtl, ")");
			isleftBracketPrinted = false;
		}

		return;
	}

	irPassRTLConstraintTreeWalk(N, root->irLeftChild);
	irPassRTLConstraintTreeWalk(N, root->irRightChild);
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
irPassRTLGenFunctionBody(State *  N, IrNode *  constraint, bool isLeft)
{
	/*
	 *	Declare
	 */	
	flexprint(N->Fe, N->Fm, N->Fprtl, "{\n\tdouble calculatedValue = 0.0;\n");

	/*
	 *	Calculation
	 */	
	flexprint(N->Fe, N->Fm, N->Fprtl, "\tcalculatedValue =");

	if (isLeft == false)
	{
		irPassRTLConstraintTreeWalk(N, constraint->irRightChild->irRightChild->irLeftChild->irLeftChild);
	}
	else
	{
		irPassRTLConstraintTreeWalk(N, constraint->irLeftChild->irLeftChild);
	}

	flexprint(N->Fe, N->Fm, N->Fprtl, ";\n");

	/*
	 *	Return
	 */
	flexprint(N->Fe, N->Fm, N->Fprtl, "\n\treturn calculatedValue;\n}\n\n");
}

/*
 *	Generate format: (double parameter1, double parameter 2 ...)
 */
void
irPassRTLGenFunctionArgument(State *  N, IrNode *  constraint, bool isLeft)
{
	flexprint(N->Fe, N->Fm, N->Fprtl, "(");

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
		irPassRTLSearchAndPrintNodeType(N, constraintsXSeq->irLeftChild, kNewtonIrNodeType_Tidentifier, false, -1);
		constraintsXSeq = constraintsXSeq->irRightChild;
	}

	irPassRTLSearchAndPrintNodeType(N, constraintsXSeq->irLeftChild, kNewtonIrNodeType_Tidentifier, true,
					irPassRTLCountRemainingParameters(N, constraintsXSeq->irLeftChild, 0));
	flexprint(N->Fe, N->Fm, N->Fprtl, ")\n");
}

/*
 *	Generate format: double functionName
 */
void
irPassRTLGenFunctionName(State *  N, IrNode *  constraints, int countFunction)
{
	flexprint(N->Fe, N->Fm, N->Fprtl, "double\n%sRHS%d",
				constraints->irLeftChild->irLeftChild->irLeftChild->irLeftChild->tokenString, countFunction);
}

void
irPassRTLProcessInvariantList(State *  N)
{
	/*
	 *	Check if the file called is simply an include.nt
	 */
	if (N->invariantList == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fprtl, "/*\n *\tPlease specify a valid file\n */\n");
		return;
	}

	Invariant *	invariant = N->invariantList;

	if (invariant->constraints == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fprtl, "/*\n *\tNo constraints created\n */\n");
		return;
	}

    /* Constraints list removed */
	IrNode *	parameterListXSeq = invariant->parameterList->irParent->irLeftChild;

	char ** argumentsList;
		
	int index = 0;

	int countFunction = 0;

    /* FIXME add checks - to CBackend as well if current assumptions are met */

	flexprint(N->Fe, N->Fm, N->Fprtl, "/*\n *\tGenerated .v file from Newton\n */\n\n");
	//flexprint(N->Fe, N->Fm, N->Fprtl, "\n#include <math.h>\n\n");

	while(invariant)
	{
		argumentsList = (char **) malloc(invariant->dimensionalMatrixColumnCount * sizeof(char *));

		/*
		*	Print calculation module -- FIXME more clear support of various architectures
		*/
		countFunction = 0;	
		flexprint(N->Fe, N->Fm, N->Fprtl, "module %s%dSerial",
				invariant->identifier, countFunction);

		flexprint(N->Fe, N->Fm, N->Fprtl, " #(\n//Parameterized values\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "parameter Q = %d,\n", Q_PARAMETER);
        flexprint(N->Fe, N->Fm, N->Fprtl, "parameter N = %d", N_PARAMETER);
        flexprint(N->Fe, N->Fm, N->Fprtl, "\n)\n(\n", N_PARAMETER);

        flexprint(N->Fe, N->Fm, N->Fprtl, "input\ti_clk,");

		while (parameterListXSeq != NULL) /* Create a list of function parameters */
		{
			irPassRTLSearchAndCreateArgList(N, parameterListXSeq->irLeftChild, kNewtonIrNodeType_Tidentifier, argumentsList, index);
			parameterListXSeq = parameterListXSeq->irRightChild;
			index++;
		}
		
		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fprtl, "input\t[N-1:0] %s_sig,\n", argumentsList[index]);
			/*
            if (index < invariant->dimensionalMatrixColumnCount - 1)
			{
				flexprint(N->Fe, N->Fm, N->Fprtl, ", ");
			}
            */
		}

        flexprint(N->Fe, N->Fm, N->Fprtl, "output\t[N-1:0] ratio_sig\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, ");\n");

		/*
		*	Declare
		*/	
		flexprint(N->Fe, N->Fm, N->Fprtl, "{\n\tdouble calculatedProportion = 0.0;\n");

		/*
		*	Calculation
		*/	
		flexprint(N->Fe, N->Fm, N->Fprtl, "\n\tcalculatedProportion = ");

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
						//flexprint(N->Fe, N->Fm, N->Fprtl, "pow(%c%c, ", 'P'+(row/10), '0'+ (row%10) );
						flexprint(N->Fe, N->Fm, N->Fprtl, "pow(" );
						flexprint(N->Fe, N->Fm, N->Fprtl, "%s", argumentsList[index]);
						flexprint(N->Fe, N->Fm, N->Fprtl, ", %f) * ", invariant->nullSpace[countKernel][tmpPosition[row]][col]);
					}
					index++;
				}
			}
			/*
			*   FIXME avoid printing 1.0 to complete the sentence;
			*/
			flexprint(N->Fe, N->Fm, N->Fprtl, "1.0;");
		}

		free(tmpPosition);

		/*
		*	Return
		*/	
		flexprint(N->Fe, N->Fm, N->Fprtl, "\n\n\treturn calculatedProportion;");

		flexprint(N->Fe, N->Fm, N->Fprtl, "\n}\n");

		flexprint(N->Fe, N->Fm, N->Fprtl, "\n\nint \nmain(int argc, char *argv[])\n{\n");

		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fprtl, "\tdouble %s", argumentsList[index]);
			if (index < invariant->dimensionalMatrixColumnCount - 1)
			{
				flexprint(N->Fe, N->Fm, N->Fprtl, ";\n");
			}
		}
		flexprint(N->Fe, N->Fm, N->Fprtl, ";");

		/*
		*	Declare
		*/	
		flexprint(N->Fe, N->Fm, N->Fprtl, "\n\tdouble calculatedProportion = 0.0;\n\n");

		flexprint(N->Fe, N->Fm, N->Fprtl, "\tif (argc < %d) {\n",invariant->dimensionalMatrixColumnCount+1);
		flexprint(N->Fe, N->Fm, N->Fprtl, "\t\tprintf(\"Usage is exec_name ");

		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fprtl,  "%s ", argumentsList[index]);
		}

		flexprint(N->Fe, N->Fm, N->Fprtl, "\\n\");\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\t\treturn -1;\n\t}\n\n");

		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fprtl, "\t%s = atof(argv[%d]);\n", argumentsList[index], index+1);
		}

		/*
		*	Calculation
		*/	
		flexprint(N->Fe, N->Fm, N->Fprtl, "\n\tcalculatedProportion = ");

		countFunction = 0;	
		flexprint(N->Fe, N->Fm, N->Fprtl, "%s%d", invariant->identifier, countFunction);

		flexprint(N->Fe, N->Fm, N->Fprtl, "(");

		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fprtl, "%s", argumentsList[index]);
			if (index < invariant->dimensionalMatrixColumnCount - 1)
			{
				flexprint(N->Fe, N->Fm, N->Fprtl, ", ");
			}
		}

		flexprint(N->Fe, N->Fm, N->Fprtl, ");\n\n");

		flexprint(N->Fe, N->Fm, N->Fprtl, "\tprintf(\"Calculated proportion is %%f.\\n\", calculatedProportion);\n\n");

		flexprint(N->Fe, N->Fm, N->Fprtl, "\treturn 0;\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\n}\n\n");

		/*
		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			free(argumentsList[index]);
		}
		free(argumentsList);
		*/

		invariant = invariant->next;
	}
	
	flexprint(N->Fe, N->Fm, N->Fprtl, "/*\n *\tEnd of the generated .v file\n */\n");
}

void
irPassRTLBackend(State *  N)
{
	FILE *	rtlFile;

	irPassRTLProcessInvariantList(N);

	if (N->outputRTLFilePath) 
	{
		rtlFile = fopen(N->outputRTLFilePath, "w"); 

		if (rtlFile == NULL)
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\n%s: %s.\n", Eopen, N->outputRTLFilePath);
			consolePrintBuffers(N);
		}

		fprintf(rtlFile, "%s", N->Fprtl->circbuf);
		fclose(rtlFile);
	}
}
