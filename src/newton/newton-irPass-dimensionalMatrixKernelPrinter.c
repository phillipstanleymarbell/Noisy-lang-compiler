/*
	Authored 2018. Phillip Stanley-Marbell, Youchao Wang.

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
#include "noisy-lexer.h"
#include "common-irPass-helpers.h"
#include "newton-types.h"
#include "newton-symbolTable.h"

void
irPassDimensionalMatrixKernelPrinter(State *  N)
{
	Invariant *	invariant = N->invariantList;

	while (invariant)
	{
		/*
		 *	We construct a temporary array to re-locate the positions of the permuted parameters
		 */
		int *		tmpPosition = (int *)calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));

		flexprint(N->Fe, N->Fm, N->Fpinfo, "The corresponding kernels:\n\n");

		if (invariant->numberOfUniqueKernels == 0)
		{
			flexprint(N->Fe, N->Fm, N->Fpinfo, "\t(No kernel for invariant \"%s\")\n", invariant->identifier);
		}
		else
		{
			flexprint(N->Fe, N->Fm, N->Fpinfo, "\tInvariant \"%s\" has %d unique kernels, each with %d column(s)...\n\n",
							invariant->identifier, invariant->numberOfUniqueKernels, invariant->kernelColumnCount);
			
			for (int countKernel = 0; countKernel < invariant->numberOfUniqueKernels; countKernel++)
			{

				flexprint(N->Fe, N->Fm, N->Fpinfo, "\tKernel %d is a valid kernel\n", countKernel);

				/*
				 *	The number of rows of the kernel equals number of columns of the dimensional matrix.
				 */
				for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
				{
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\t");
					for (int col = 0; col < invariant->kernelColumnCount; col++)
					{
						flexprint(N->Fe, N->Fm, N->Fpinfo, "%4g",
								invariant->nullSpace[countKernel][row][col],
								(col == invariant->kernelColumnCount - 1 ? "" : " "));
					}
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
				}


				/*
				 *	PermutedIndexArray consists of the indices to show how exactly the matrix is permuted.
				 *	It stores all the permutation results for all the different kernels.
				 */
				flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\tThe ordering of parameters are as follows\n", countKernel);
				flexprint(N->Fe, N->Fm, N->Fpinfo, "\t");
				
				for (int i = 0; i < invariant->dimensionalMatrixColumnCount; i++)
				{
					flexprint(N->Fe, N->Fm, N->Fpinfo, " #%c%c ", 'A'+(invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + i]/10), 
											'0'+invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + i]%10);
				}
				flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\n");

	
				/*
				 *	Prints out the a table of the symbolic expressions implied by the Pi groups derived from the kernels.	
				 */
				for (int j = 0; j < invariant->dimensionalMatrixColumnCount; j++)
				{
					tmpPosition[invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + j]] = j;
				}

				flexprint(N->Fe, N->Fm, N->Fpinfo, "\tThere are in total %d Pi-groups\n", invariant->kernelColumnCount);
				for (int col = 0; col < invariant->kernelColumnCount; col++)
				{
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\tPi-group %d is\n", col);
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\t");
					for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
					{
						flexprint(N->Fe, N->Fm, N->Fpinfo, "#%c%c", 'A'+(row/10), '0'+ (row%10) );
						flexprint(N->Fe, N->Fm, N->Fpinfo, "^(%2g)  ", invariant->nullSpace[countKernel][tmpPosition[row]][col]);
					}
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
				}
				flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\n");

			}
		}
		flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");

		free(tmpPosition);

		invariant = invariant->next;
	}
}
