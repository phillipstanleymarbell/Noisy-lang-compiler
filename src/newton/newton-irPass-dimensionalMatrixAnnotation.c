/*
	Authored 2019. Phillip Stanley-Marbell, Vlad-Mihai Mandric, Kiseki Hirakawa
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
#include "common-irHelpers.h"


void
irPassDimensionalMatrixAnnotationByBody(State *  N)
{
	Invariant *	invariant = N->invariantList;

	while (invariant)
	{
		int		dimensionCount = 0;
		IrNode *	parameterbody = invariant->constraints;
		Dimension *	dimension = findNthIrNodeOfType(N, parameterbody, kNewtonIrNodeType_Tidentifier, 0)->physics->dimensions;
		int		parameterCount = countIrNodeOfType(N, parameterbody, kNewtonIrNodeType_Tidentifier);
	
		/*
		 *	First, count the number of parameters and dimensions
		 */
		Dimension * 	tempdimension = dimension;
		
		for ( ; tempdimension ; tempdimension = tempdimension->next)
		{
			dimensionCount++;
		}

		if (parameterCount == 0 || dimensionCount == 0)
		{
			fatal(N, Esanity);
		}

		/*
		 *	Next, allocate a temporary matrix to hold the full list
		 *	of parameter and dimension values.
		 *
		 *	We have a list of parameters, each of which has a list of
		 *	_all_ the dimensions defined in a Newton description,
		 *	including those that the parameter in question does not use.
		 *
		 *	We construct the temporary matrix with a row for each parameter,
		 *	and a column for each of the (possibly unused) dimensions.
		 *	Later, below, we determine which dimensions we actually use
		 *	and recreate the _true_ dimensional matrix which has
		 *	parameters in columns and dimensions in rows.
		 */
		double **	tmpMatrix = (double **)calloc(parameterCount, sizeof(double *));

		if (!tmpMatrix)
		{
			fatal(N, Emalloc);
		}

		for (int i = 0; i < parameterCount; i++)
		{
			tmpMatrix[i] = (double *)calloc(dimensionCount, sizeof(double));
			if (!tmpMatrix[i])
			{
				fatal(N, Emalloc);
			}
		}

		invariant->dimensionalMatrixColumnLabels = (char **) calloc(parameterCount, sizeof(char *));

		if (!invariant->dimensionalMatrixColumnLabels)
		{
			fatal(N, Emalloc);
		}

		/*
		 *	Build the temporary matrix and populate the array usedDimensions[]
		 *	with the indices of dimensions used by _any_ of the parameters:
		 */
		bool	usedDimensions[dimensionCount];
		bzero(usedDimensions, sizeof(usedDimensions));

		for (int i = 0; i < parameterCount; i++)
		{
			tempdimension = findNthIrNodeOfType(N,parameterbody,kNewtonIrNodeType_Tidentifier,i)->physics->dimensions;
			invariant->dimensionalMatrixColumnLabels[i] = findNthIrNodeOfType(N,parameterbody,kNewtonIrNodeType_Tidentifier,i)->physics->identifier;
			for (int j = 0; j < dimensionCount; j++)
			{
				tmpMatrix[i][j] = tempdimension->exponent;
				if (tmpMatrix[i][j])
				{
					usedDimensions[j] |= 1;
				}
				tempdimension = tempdimension->next;
			}
		}

		/*
		 *	We already know the dimensional matrix column count, since
		 *	that is the parameter count. We determine the row count
		 *	from inspecting usedDimensions[].
		 */
		invariant->dimensionalMatrixColumnCount = parameterCount;
		invariant->dimensionalMatrixRowCount = 0;
		for (int i = 0; i < dimensionCount; i++)
		{
			if (usedDimensions[i])
			{
				invariant->dimensionalMatrixRowCount++;
			}
		}

		if (invariant->dimensionalMatrixRowCount == 0)
		{
			fatal(N, Esanity);
		}

		invariant->dimensionalMatrixRowLabels = (char **) calloc(invariant->dimensionalMatrixRowCount, sizeof(char *));
		if (!invariant->dimensionalMatrixRowLabels)
		{
			fatal(N, Emalloc);
		}

		/*
		 *	Next, use usedDimensions[] to determine how many rows
		 *	we need in the dimensional matrix. At the same time,
		 *	we also collect the dimension string names:
		 */
		int	copiedRowLabelCount = 0;

		tempdimension = dimension;
		for (int i = 0; i < dimensionCount; i++)
		{
			if (usedDimensions[i])
			{
				invariant->dimensionalMatrixRowLabels[copiedRowLabelCount++] = tempdimension->name;
			}
			tempdimension = tempdimension->next;
		}

		/*
		 *	Now, finally, allocate the dimensional matrix:
		 */
		invariant->dimensionalMatrix = (double *)calloc(invariant->dimensionalMatrixRowCount*invariant->dimensionalMatrixColumnCount, sizeof(double));
		if (!invariant->dimensionalMatrix)
		{
			fatal(N, Emalloc);
		}

		/*
		 *	Copy over the temporary matrix in the correct form
		 *	(rows are dimensions, columns are parameters).
		 */
		int	copiedDimensionCount;
		for (int i = 0; i < parameterCount; i++)
		{
			copiedDimensionCount = 0;
			for (int j = 0; j < dimensionCount; j++)
			{
				if (usedDimensions[j])
				{
					invariant->dimensionalMatrix[invariant->dimensionalMatrixColumnCount*(copiedDimensionCount++) + i] = tmpMatrix[i][j];
				}
			}
		}

		free(tmpMatrix);

		invariant = invariant->next;
	}
}


void
irPassDimensionalMatrixAnnotation(State *  N)
{
	Invariant *	invariant = N->invariantList;

	while (invariant)
	{
		int		parameterCount = 0, dimensionCount = 0;
		IrNode *	parameter = invariant->parameterList;
		Dimension *	dimension = parameter->irLeftChild->physics->dimensions;


		/*
		 *	First, count the number of parameters and dimensions
		 */
		for ( ; parameter ; parameter = parameter->irRightChild)
		{
			parameterCount++;
		}

		for ( ; dimension ; dimension = dimension->next)
		{
			dimensionCount++;
		}

		if (parameterCount == 0 || dimensionCount == 0)
		{
			fatal(N, Esanity);
		}

		/*
		 *	Next, allocate a temporary matrix to hold the full list
		 *	of parameter and dimension values.
		 *
		 *	We have a list of parameters, each of which has a list of
		 *	_all_ the dimensions defined in a Newton description,
		 *	including those that the parameter in question does not use.
		 *
		 *	We construct the temporary matrix with a row for each parameter,
		 *	and a column for each of the (possibly unused) dimensions.
		 *	Later, below, we determine which dimensions we actually use
		 *	and recreate the _true_ dimensional matrix which has
		 *	parameters in columns and dimensions in rows.
		 */
		double **	tmpMatrix = (double **)calloc(parameterCount, sizeof(double *));
		if (!tmpMatrix)
		{
			fatal(N, Emalloc);
		}

		for (int i = 0; i < parameterCount; i++)
		{
			tmpMatrix[i] = (double *)calloc(dimensionCount, sizeof(double));
			if (!tmpMatrix[i])
			{
				fatal(N, Emalloc);
			}
		}

		invariant->dimensionalMatrixColumnLabels = (char **) calloc(parameterCount, sizeof(char *));
		if (!invariant->dimensionalMatrixColumnLabels)
		{
			fatal(N, Emalloc);
		}

		/*
		 *	Build the temporary matrix and populate the array usedDimensions[]
		 *	with the indices of dimensions used by _any_ of the parameters:
		 */
		bool	usedDimensions[dimensionCount];
		bzero(usedDimensions, sizeof(usedDimensions));

		parameter = invariant->parameterList;
		for (int i = 0; i < parameterCount; i++)
		{
			dimension = parameter->irLeftChild->physics->dimensions;
			invariant->dimensionalMatrixColumnLabels[i] = parameter->irLeftChild->physics->identifier;
			for (int j = 0; j < dimensionCount; j++)
			{
				tmpMatrix[i][j] = dimension->exponent;
				if (tmpMatrix[i][j])
				{
					usedDimensions[j] |= 1;
				}

				dimension = dimension->next;
			}
			parameter = parameter->irRightChild;
		}

		/*
		 *	We already know the dimensional matrix column count, since
		 *	that is the parameter count. We determine the row count
		 *	from inspecting usedDimensions[].
		 */
		invariant->dimensionalMatrixColumnCount = parameterCount;
		invariant->dimensionalMatrixRowCount = 0;
		for (int i = 0; i < dimensionCount; i++)
		{
			if (usedDimensions[i])
			{
				invariant->dimensionalMatrixRowCount++;
			}
		}

		if (invariant->dimensionalMatrixRowCount == 0)
		{
			fatal(N, Esanity);
		}

		invariant->dimensionalMatrixRowLabels = (char **) calloc(invariant->dimensionalMatrixRowCount, sizeof(char *));
		if (!invariant->dimensionalMatrixRowLabels)
		{
			fatal(N, Emalloc);
		}

		/*
		 *	Next, use usedDimensions[] to determine how many rows
		 *	we need in the dimensional matrix. At the same time,
		 *	we also collect the dimension string names:
		 */
		parameter = invariant->parameterList;
		dimension = parameter->irLeftChild->physics->dimensions;
		int	copiedRowLabelCount = 0;
		for (int i = 0; i < dimensionCount; i++)
		{
			if (usedDimensions[i])
			{
				invariant->dimensionalMatrixRowLabels[copiedRowLabelCount++] = dimension->name;
			}
			dimension = dimension->next;
		}

		/*
		 *	Now, finally, allocate the dimensional matrix:
		 */
		invariant->dimensionalMatrix = (double *)calloc(invariant->dimensionalMatrixRowCount*invariant->dimensionalMatrixColumnCount, sizeof(double));
		if (!invariant->dimensionalMatrix)
		{
			fatal(N, Emalloc);
		}

		/*
		 *	Copy over the temporary matrix in the correct form
		 *	(rows are dimensions, columns are parameters).
		 */
		int	copiedDimensionCount;
		for (int i = 0; i < parameterCount; i++)
		{
			copiedDimensionCount = 0;
			for (int j = 0; j < dimensionCount; j++)
			{
				if (usedDimensions[j])
				{
					invariant->dimensionalMatrix[invariant->dimensionalMatrixColumnCount*(copiedDimensionCount++) + i] = tmpMatrix[i][j];
				}
			}
		}

		free(tmpMatrix);

		invariant = invariant->next;
	}
}
