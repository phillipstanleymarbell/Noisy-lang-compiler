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
irPassDimensionalMatrixConvertToList(State *  N)
{
	Invariant *	invariant = N->invariantList;

	while (invariant)
	{
		/*
		 *	Currently we are using the full null space to return all the Pi groups
		 *	When issue #372 is complete, we would then only need to use
		 *	invariant->uniquePiGroups to do the following lines 83-111.
		 */

		int ***		tmpPosition = (int ***)calloc(invariant->numberOfUniqueKernels, sizeof(int **));
		int ***		tmpReorderNullSpace = (int ***)calloc(invariant->numberOfUniqueKernels, sizeof(int **));

		for (int countKernel = 0; countKernel < invariant->numberOfUniqueKernels; countKernel++)
		{	
			tmpPosition[countKernel] = (int **)calloc(invariant->kernelColumnCount, sizeof(int *));
			tmpReorderNullSpace[countKernel] = (int **)calloc(invariant->kernelColumnCount, sizeof(int *));

			/*
			 *	Construct the new re-ordered null space
			 *	Using the permutedIndexArrayPointer
			 *	Two tmp three-dimensional arrays are used for better readability.
			 */
			for (int countColumn = 0; countColumn < invariant->kernelColumnCount; countColumn++)
			{
				tmpPosition[countKernel][countColumn] = (int *)calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));
				tmpReorderNullSpace[countKernel][countColumn] = (int *)calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));
				
				for (int countRow = 0; countRow < invariant->dimensionalMatrixColumnCount; countRow++)
				{
					tmpPosition[countKernel][countColumn][invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + countRow]] = countRow;
				}

				for (int countRow = 0; countRow < invariant->dimensionalMatrixColumnCount; countRow++)
				{
					tmpReorderNullSpace[countKernel][countColumn][countRow] = invariant->nullSpace[countKernel][tmpPosition[countKernel][countColumn][countRow]][countColumn];
				}
			}
		}

		/*
		 *	Construct the new matrix to bring into the invariant list
		 *	First, we assign the new value to dimensionalMatrixColumnCounts
		 */

		invariant->dimensionalMatrixWithPiGroupsColumnCount 
					= invariant->dimensionalMatrixColumnCount + invariant->numberOfUniqueKernels * invariant->kernelColumnCount;

		/*
		 *	Re-create the new columnlabels
		 *	We keep the dimensionalMatrixRowCount as is, and the RowLabel unchanged,
		 *	since we are not introducing any new fundamental units into the dimensional matrix. 
		 */
		invariant->dimensionalMatrixWithPiGroupsColumnLabels = (char **) calloc(invariant->dimensionalMatrixWithPiGroupsColumnCount, sizeof(char *));

		for (int i = 0; i < invariant->dimensionalMatrixColumnCount; i++)
		{
			invariant->dimensionalMatrixWithPiGroupsColumnLabels[i] = invariant->dimensionalMatrixColumnLabels[i];
		}

		for (int j = invariant->dimensionalMatrixColumnCount; j < invariant->dimensionalMatrixWithPiGroupsColumnCount; j++)
		{
			invariant->dimensionalMatrixWithPiGroupsColumnLabels[j] = "Pi group";
		}

		/*
		 *	Finally, we bring back in the pi groups into the dimensional matrix,
		 *	and since pi groups are dimensionless, we assign zeros to their corresponding
		 *	rows and columns within the dimensionalMatrix
		 */

		invariant->dimensionalMatrixWithPiGroups = (double *)calloc(invariant->dimensionalMatrixRowCount * invariant->dimensionalMatrixWithPiGroupsColumnCount, sizeof(double));

		for (int countColumn = 0; countColumn < invariant->dimensionalMatrixWithPiGroupsColumnCount; countColumn++)
		{
			for (int countRow = 0; countRow < invariant->dimensionalMatrixRowCount; countRow++)
			{
				if (countColumn < invariant->dimensionalMatrixColumnCount)
				{
					invariant->dimensionalMatrixWithPiGroups[invariant->dimensionalMatrixWithPiGroupsColumnCount * countRow + countColumn]
								= invariant->dimensionalMatrix[invariant->dimensionalMatrixColumnCount * countRow + countColumn];
				}
				else
				{
					invariant->dimensionalMatrixWithPiGroups[invariant->dimensionalMatrixWithPiGroupsColumnCount * countRow + countColumn] = 0;
				}
			}
		}

		/*
		 *	A few more steps so that we could bring things back to the MatrixPrinter.
		 *	However, this overwrites the original dimensional matrix and well as all of its properties.
		 *	Keep this or remove this, depending on the situation.
		 */
		free(invariant->dimensionalMatrix);

		invariant->dimensionalMatrix = (double *)calloc(invariant->dimensionalMatrixRowCount * invariant->dimensionalMatrixWithPiGroupsColumnCount, sizeof(double));
		invariant->dimensionalMatrixColumnCount = invariant->dimensionalMatrixWithPiGroupsColumnCount;

		for (int countColumn = 0; countColumn < invariant->dimensionalMatrixColumnCount; countColumn++)
		{
			for (int countRow = 0; countRow < invariant->dimensionalMatrixRowCount; countRow++)
			{
				invariant->dimensionalMatrix[invariant->dimensionalMatrixColumnCount * countRow + countColumn]
								= invariant->dimensionalMatrixWithPiGroups[invariant->dimensionalMatrixColumnCount * countRow + countColumn];
			}
		}


		free(invariant->dimensionalMatrixColumnLabels);
		invariant->dimensionalMatrixColumnLabels = (char **) calloc(invariant->dimensionalMatrixColumnCount, sizeof(char *));

		for (int i = 0; i < invariant->dimensionalMatrixColumnCount; i++)
		{
			invariant->dimensionalMatrixColumnLabels[i] = invariant->dimensionalMatrixWithPiGroupsColumnLabels[i];
		}

		invariant = invariant->next;

		free(tmpPosition);
		free(tmpReorderNullSpace);
	}
}
