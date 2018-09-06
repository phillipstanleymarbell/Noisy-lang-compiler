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
		flexprint(N->Fe, N->Fm, N->Fpinfo, "The corresponding kernels:\n\n");

		/*
		 *	TODO: this should be done in the dimensionalMatrixKernelPrinter pass.
		 *	This is here temporarily until issue #354 is completed.
		 */
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

				flexprint(N->Fe, N->Fm, N->Fpinfo, "\tKernel %d is a new unique kernel\n", countKernel);

				/*
				 *	The number of rows of the kernel equals number of columns of the dimensional matrix.
				 */
				for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
				{
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\t");
					for (int col = 0; col < invariant->kernelColumnCount; col++)
					{
						flexprint(N->Fe, N->Fm, N->Fpinfo, "%4g",//"%4.1f%s",
								invariant->nullSpace[countKernel][row][col],
								(col == invariant->kernelColumnCount - 1 ? "" : " "));
					}
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
				}
				flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");

				for (int j = 0; j < invariant->dimensionalMatrixColumnCount; j++)
				{
					flexprint(N->Fe, N->Fm, N->Fpinfo, " %d ", invariant->permutedIndexArray[countKernel][j]);
				}
				flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
				
			
				flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\tThe ordering of parameters are as follows\n", countKernel);
				flexprint(N->Fe, N->Fm, N->Fpinfo, "\t");
				for (int i = 0; i < invariant->dimensionalMatrixColumnCount; i++)
				{
					flexprint(N->Fe, N->Fm, N->Fpinfo, " %s ", invariant->dimensionalMatrixColumnLabels[i]);
				}
				flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
			}
		}
		flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");

		//for ( )

		invariant = invariant->next;
	}
}
