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
irPassTargetParamDimensionalMatrixKernelPrinter(State *  N)
{
	Invariant *	invariant = N->invariantList;
	IrNode *	parameterListXSeq = invariant->parameterList->irParent->irLeftChild;

	int targetParamIndex = -1; //atoi(N->targetParam);
	int targetParamApperanceCnt, targetParamUniquePiGroup=-1, index=0;
	char ** argumentsList;

	/* FIXME check if invariant or invariant->list are null */
	argumentsList = (char **) malloc(invariant->dimensionalMatrixColumnCount * sizeof(char *));

	/* FIXME check if this while is needed */
	while (parameterListXSeq != NULL) 
	{
		irPassTargetParamSearchAndCreateArgList(N, parameterListXSeq->irLeftChild, kNewtonIrNodeType_Tidentifier, argumentsList, index);
		parameterListXSeq = parameterListXSeq->irRightChild;
		index++;
	}

	/* FIXME check if argumentsList is null */
	for (int i=0; i<invariant->dimensionalMatrixColumnCount; i++) 
	{
		if (!strcmp(N->targetParam,argumentsList[i])) {
			targetParamIndex = i;
			break;
		}
	}

	if (targetParamIndex == -1) {
		flexprint(N->Fe, N->Fm, N->Fpinfo, "Target parameter %s not found. Exiting.\n", N->targetParam);
		return;
	}

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
			flexprint(N->Fe, N->Fm, N->Fpinfo, "Invariant \"%s\" has %d unique kernels, each with %d column(s). Target param is %s\n\n",
							invariant->identifier, invariant->numberOfUniqueKernels, invariant->kernelColumnCount, N->targetParam);

			for (int countKernel = 0; countKernel < invariant->numberOfUniqueKernels; countKernel++)
			{

				flexprint(N->Fe, N->Fm, N->Fpinfo, "Kernel %d:\n", countKernel);

				for (int j = 0; j < invariant->dimensionalMatrixColumnCount; j++)
				{
					tmpPosition[invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + j]] = j;
				}

				targetParamApperanceCnt = 0;
				targetParamUniquePiGroup = -1;

				for (int piIndex = 0; piIndex < invariant->kernelColumnCount; piIndex++)
				{
					if (abs((int) invariant->nullSpace[countKernel][tmpPosition[targetParamIndex]][piIndex]) == 1) /*FIXME for fractional values*/
					{
						targetParamUniquePiGroup = piIndex;
						targetParamApperanceCnt++; /* If there are more than one instances this will be overwritten but not taken into account later */
					}
				}

				for (int piIndex = 0; piIndex < invariant->kernelColumnCount; piIndex++)
				{
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\tPi group %d, Pi %d is:\t", countKernel, piIndex);
					for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
					{
						flexprint(N->Fe, N->Fm, N->Fpinfo, "%c%c", 'P'+(row/10), '0'+ (row%10) );
						flexprint(N->Fe, N->Fm, N->Fpinfo, "^(%2g)  ", invariant->nullSpace[countKernel][tmpPosition[row]][piIndex]);
					}
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\n");
				}
				//flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");

				if (targetParamApperanceCnt == 1) 
				{
					//flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\t\tTargeParam YES\n\n");
					N->targetParamLocatedKernel = countKernel;
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\tDFS: ");

					index = 0;
					for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
					{
						if (invariant->nullSpace[countKernel][tmpPosition[row]][targetParamUniquePiGroup] != 0) 
						{
							if (index == 0) /* Has smth been printed? */
							{
								flexprint(N->Fe, N->Fm, N->Fpinfo, "%s^%.2f", argumentsList[row],invariant->nullSpace[countKernel][tmpPosition[row]][targetParamUniquePiGroup]);
								index++;
							} else {
								flexprint(N->Fe, N->Fm, N->Fpinfo, " * %s^%.2f", argumentsList[row],invariant->nullSpace[countKernel][tmpPosition[row]][targetParamUniquePiGroup]);
							}
						}
					}

					flexprint(N->Fe, N->Fm, N->Fpinfo, " = Phi (");

					for (int piIndex = 0; piIndex < invariant->kernelColumnCount; piIndex++)
					{
						if (piIndex != targetParamUniquePiGroup)
						{		
							index = 0;
							for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
							{
								if (invariant->nullSpace[countKernel][tmpPosition[row]][piIndex] != 0) 
								{
									if (index == 0) /* Has smth been printed? */
									{
										flexprint(N->Fe, N->Fm, N->Fpinfo, "%s^%.2f", argumentsList[row],invariant->nullSpace[countKernel][tmpPosition[row]][piIndex]);
										index++;
									} else {
										flexprint(N->Fe, N->Fm, N->Fpinfo, " * %s^%.2f", argumentsList[row],invariant->nullSpace[countKernel][tmpPosition[row]][piIndex]);
									}
								}
							}
							flexprint(N->Fe, N->Fm, N->Fpinfo, ", ");
						}	
					}
					flexprint(N->Fe, N->Fm, N->Fpinfo, ")\n\n");
					
				} else {
					//flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\t\tTargeParam NO\n\n");
				}
			}

			//flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
		}
		
		//flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");

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
