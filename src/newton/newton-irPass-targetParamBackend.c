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

void
irPassTargetParamSearchAndCreateArgList(State *  N, IrNode *  root, IrNodeType expectedType, char **argList, int argumentIndex)
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

	irPassTargetParamSearchAndCreateArgList(N, root->irLeftChild, expectedType, argList, argumentIndex);
	
	return;
}

void
irPassTargetParamProcessInvariantList(State *  N)
{
	/*
	 *	Check if the file called is simply an include.nt
	 */
	if (N->invariantList == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tN->invariantList is NULL\n */\n");
		return;
	}

	Invariant *	invariant = N->invariantList;

	IrNode *	parameterListXSeq = invariant->parameterList->irParent->irLeftChild;

	char ** argumentsList;
		
	int index = 0;

	//int countFunction = 0;

	while(invariant)
	{
		argumentsList = (char **) malloc(invariant->dimensionalMatrixColumnCount * sizeof(char *));

		while (parameterListXSeq != NULL) 
		{
			irPassTargetParamSearchAndCreateArgList(N, parameterListXSeq->irLeftChild, kNewtonIrNodeType_Tidentifier, argumentsList, index);
			parameterListXSeq = parameterListXSeq->irRightChild;
			index++;
		}
		
		/*
		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "double %s", argumentsList[index]);
			if (index < invariant->dimensionalMatrixColumnCount - 1)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ", ");
			}
		}
		*/

		/*
		 *	We construct a temporary array to re-locate the positions of the permuted parameters
		 */
		int *		tmpPosition = (int *)calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));
		index = 0;

		flexprint(N->Fe, N->Fm, N->Fpc, "\n\tPhi(");
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
						flexprint(N->Fe, N->Fm, N->Fpc, "%s^%.2f, ", argumentsList[index],invariant->nullSpace[countKernel][tmpPosition[row]][col]);
					}
					index++;
				}
			}
		}
		flexprint(N->Fe, N->Fm, N->Fpc, ") = 0\n\n");

		/*
		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\tdouble %s", argumentsList[index]);
			if (index < invariant->dimensionalMatrixColumnCount - 1)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ";\n");
			}
		}
		
		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "%s ", argumentsList[index]);
		}

		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "\t%s = atof(argv[%d]);\n", argumentsList[index], index+1);
		}

		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fpc, "%s", argumentsList[index]);
			if (index < invariant->dimensionalMatrixColumnCount - 1)
			{
				flexprint(N->Fe, N->Fm, N->Fpc, ", ");
			}
		}
		*/

		free(tmpPosition);
		
		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			free(argumentsList[index]);
		}
		free(argumentsList);

		invariant = invariant->next;
	}
}

void
irPassTargetParamDimensionalMatrixKernelPrinter(State *  N)
{
	Invariant *	invariant = N->invariantList;

	int targetParamIndex = atoi(N->targetParam);
	int targetParamApperanceCnt;

	while (invariant)
	{
		/*
		 *	We construct a temporary array to re-locate the positions of the permuted parameters
		 */
		int *		tmpPosition = (int *)calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));

		if (invariant->numberOfUniqueKernels == 0)
		{
			flexprint(N->Fe, N->Fm, N->Fpinfo, "\t(No kernel for invariant \"%s\")\n", invariant->identifier);
		}
		else
		{
			flexprint(N->Fe, N->Fm, N->Fpinfo, "Invariant \"%s\" has %d unique kernels, each with %d column(s). My target param is %s\n\n",
							invariant->identifier, invariant->numberOfUniqueKernels, invariant->kernelColumnCount, N->targetParam);

			for (int countKernel = 0; countKernel < invariant->numberOfUniqueKernels; countKernel++)
			{

				flexprint(N->Fe, N->Fm, N->Fpinfo, "\tKernel %d:\n", countKernel);

				for (int j = 0; j < invariant->dimensionalMatrixColumnCount; j++)
				{
					tmpPosition[invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + j]] = j;
				}

				/*
				 *	Prints out the a table of the symbolic expressions implied by the Pi groups derived from the kernels.	
				 */
				
				/*
				 *	PermutedIndexArray consists of the indices to show how exactly the matrix is permuted.
				 *	It stores all the permutation results for all the different kernels.
				 */
				// flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\t\tThe ordering of parameters is:\t", countKernel);
				// for (int i = 0; i < invariant->dimensionalMatrixColumnCount; i++)
				// {
				// 	flexprint(N->Fe, N->Fm, N->Fpinfo, "%c%c ", 'P'+(invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + i]/10), 
				// 							'0'+invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + i]%10);
				// }
				// flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\n");

				targetParamApperanceCnt = 0;
				for (int col = 0; col < invariant->kernelColumnCount; col++)
				{
					targetParamApperanceCnt += abs((int) invariant->nullSpace[countKernel][tmpPosition[targetParamIndex]][col]); /*FIXME */
					/*
					for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
					{
						flexprint(N->Fe, N->Fm, N->Fpinfo, "^(%2g)  ", invariant->nullSpace[countKernel][tmpPosition[row]][col]);
					}
					*/
				}

				for (int col = 0; col < invariant->kernelColumnCount; col++)
				{
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\t\tPi group %d, Pi %d is:\t", countKernel, col);
					for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
					{
						flexprint(N->Fe, N->Fm, N->Fpinfo, "%c%c", 'P'+(row/10), '0'+ (row%10) );
						flexprint(N->Fe, N->Fm, N->Fpinfo, "^(%2g)  ", invariant->nullSpace[countKernel][tmpPosition[row]][col]);
					}
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\n");
				}
				//flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");

				if (targetParamApperanceCnt == 1) 
				{
					// for (int col = 0; col < invariant->kernelColumnCount; col++)
					// {
					// 	flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\t\tPi group %d, Pi %d is:\t", countKernel, col);
					// 	for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
					// 	{
					// 		flexprint(N->Fe, N->Fm, N->Fpinfo, "%c%c", 'P'+(row/10), '0'+ (row%10) );
					// 		flexprint(N->Fe, N->Fm, N->Fpinfo, "^(%2g)  ", invariant->nullSpace[countKernel][tmpPosition[row]][col]);
					// 	}
					// 	flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\n");
					// }
					// flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\t\tTargeParam YES\n\n");
				} else {flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\t\tTargeParam NO\n\n");}
			}

			flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
		}
		
		flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");

		free(tmpPosition);
		invariant = invariant->next;
	}
}


void
irPassTargetParamBackend(State *  N)
{
	//FILE *	cFile;

	//irPassTargetParamProcessInvariantList(N);
	irPassTargetParamDimensionalMatrixKernelPrinter(N);

	/*
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
	*/
}
