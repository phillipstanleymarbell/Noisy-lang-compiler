/*
	Authored 2018. Phillip Stanley-Marbell, Youchao Wang.
	Updated  2019. Kiseki Hirakawa

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
irPassDimensionalMatrixKernelPrinter(State *  N)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassDimensionalMatrixKernelPrinter);

	Invariant *	invariant = N->invariantList;
	bool		latexOutput = N->irBackends & kNewtonIrBackendLatex;


	while (invariant)
	{
		/*
		 *	We construct a temporary array to re-locate the positions of the permuted parameters
		 */
		int *		tmpPosition = (int *)calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));

		if (invariant->numberOfUniqueKernels == 0)
		{
			if (!latexOutput) flexprint(N->Fe, N->Fm, N->Fpinfo, "\t(No kernel for invariant \"%s\")\n", invariant->identifier);
		}
		else
		{
			if (!latexOutput) flexprint(N->Fe, N->Fm, N->Fpinfo, "Invariant \"%s\" has %d unique kernels, each with %d column(s)...\n\n",
							invariant->identifier, invariant->numberOfUniqueKernels, invariant->kernelColumnCount);

			if ((N->mode & kCommonModeCGI) || (N->irBackends & kNewtonIrBackendLatex))
			{
				flexprint(N->Fe, N->Fm, N->Fpmathjax, "\n\n$$\n");
				flexprint(N->Fe, N->Fm, N->Fpmathjax, "\\begin{aligned}\n");
			}

			for (int countKernel = 0; countKernel < invariant->numberOfUniqueKernels; countKernel++)
			{
				if (!latexOutput) flexprint(N->Fe, N->Fm, N->Fpinfo, "\tKernel %d is a valid kernel:\n\n", countKernel);

				/*
				 *	The number of rows of the kernel equals number of columns of the dimensional matrix.
				 */
				if (!latexOutput) for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
				{
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\t");
					for (int col = 0; col < invariant->kernelColumnCount; col++)
					{
						flexprint(N->Fe, N->Fm, N->Fpinfo, "%4g",
								invariant->nullSpace[countKernel][row][col],
								(col == invariant->kernelColumnCount - 1 ? "" : " "));
					}
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
				}
				if (!latexOutput) flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");

				for (int j = 0; j < invariant->dimensionalMatrixColumnCount; j++)
				{
					tmpPosition[invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + j]] = j;
				}

				/*
				 *	Prints out the a table of the symbolic expressions implied by the Pi groups derived from the kernels.	
				 */
				if ((N->mode & kCommonModeCGI) || (N->irBackends & kNewtonIrBackendLatex))
				{
					flexprint(N->Fe, N->Fm, N->Fpmathjax, "\t\\qquad\\qquad\\textcolor{DarkSlateGray}{\\mathbf{\\Pi\\text{ group }%d, \\text{ with column order }", countKernel);

					/*
					 *	PermutedIndexArray consists of the indices to show how exactly the matrix is permuted.
					 *	It stores all the permutation results for all the different kernels.
					 */

					flexprint(N->Fe, N->Fm, N->Fpmathjax, " \\left(");
					for (int i = 0; i < invariant->dimensionalMatrixColumnCount; i++)
					{
						flexprint(N->Fe, N->Fm, N->Fpmathjax, "%c%c", 'P'+(invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + i]/10),
											'0'+invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + i]%10);
						if (i < invariant->dimensionalMatrixColumnCount -1)
						{
							flexprint(N->Fe, N->Fm, N->Fpmathjax, ",");
						}
					}
					flexprint(N->Fe, N->Fm, N->Fpmathjax, "\\right)}} \\qquad&\\textcolor{DeepSkyBlue}{\\dashrightarrow}\\qquad");

					for (int col = 0; col < invariant->kernelColumnCount; col++)
					{
						flexprint(N->Fe, N->Fm, N->Fpmathjax, "\\textcolor{DarkGreen}{\\dfrac{");
						for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
						{
							if (invariant->nullSpace[countKernel][tmpPosition[row]][col] > 0)
							{
								flexprint(N->Fe, N->Fm, N->Fpmathjax, "(");

								/*
								 *	We should not use the column labels even though those are the things
								 *	we have at the moment that are well-named. We really should be using
								 *	the parameter names, or saving that, the "Pnnn" label.
								 */
								//flexprint(N->Fe, N->Fm, N->Fpmathjax, "%s", invariant->dimensionalMatrixColumnLabels[row]);
								flexprint(N->Fe, N->Fm, N->Fpmathjax, "%c%c", 'P'+(invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + row]/10),
													'0'+invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + row]%10);

								if (invariant->nullSpace[countKernel][tmpPosition[row]][col] != 1)
								{
									flexprint(N->Fe, N->Fm, N->Fpmathjax, "^{%g}", invariant->nullSpace[countKernel][tmpPosition[row]][col]);
								}
								flexprint(N->Fe, N->Fm, N->Fpmathjax, ")");
							}
						}
						flexprint(N->Fe, N->Fm, N->Fpmathjax, "}");
						
						flexprint(N->Fe, N->Fm, N->Fpmathjax, "{");
						for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
						{
							if (invariant->nullSpace[countKernel][tmpPosition[row]][col] < 0)
							{
								flexprint(N->Fe, N->Fm, N->Fpmathjax, "(");

								/*
								 *	We should not use the column labels even though those are the things
								 *	we have at the moment that are well-named. We really should be using
								 *	the parameter names, or saving that, the "Pnnn" label.
								 */
								//flexprint(N->Fe, N->Fm, N->Fpmathjax, "%s", invariant->dimensionalMatrixColumnLabels[row]);
								flexprint(N->Fe, N->Fm, N->Fpmathjax, "%c%c", 'P'+(invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + row]/10),
													'0'+invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + row]%10);

								if (invariant->nullSpace[countKernel][tmpPosition[row]][col] != -1)
								{
									flexprint(N->Fe, N->Fm, N->Fpmathjax, "^{%g}", 0 - invariant->nullSpace[countKernel][tmpPosition[row]][col]);
								}
								flexprint(N->Fe, N->Fm, N->Fpmathjax, ")");
							}
						}
						/*
						 *	Close \dfrac and \textcolor
						 */
						flexprint(N->Fe, N->Fm, N->Fpmathjax, "}}");

						if (col < invariant->kernelColumnCount - 1)
						{
							flexprint(N->Fe, N->Fm, N->Fpmathjax, ",\\quad");
						}
						else
						{
							flexprint(N->Fe, N->Fm, N->Fpmathjax, "\\\\");
						}
					}
				}
				else if (!latexOutput) 
				{
					/*
					 *	PermutedIndexArray consists of the indices to show how exactly the matrix is permuted.
					 *	It stores all the permutation results for all the different kernels.
					 */
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\t\tThe ordering of parameters is:\t", countKernel);
					for (int i = 0; i < invariant->dimensionalMatrixColumnCount; i++)
					{
						flexprint(N->Fe, N->Fm, N->Fpinfo, "%c%c ", 'P'+(invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + i]/10), 
												'0'+invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + i]%10);
					}
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\n");
						
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
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
				}
			}

			if ((N->mode & kCommonModeCGI) || (N->irBackends & kNewtonIrBackendLatex))
			{
				flexprint(N->Fe, N->Fm, N->Fpmathjax, "\n\\end{aligned}\n");
				flexprint(N->Fe, N->Fm, N->Fpmathjax, "$$\n");
			}

			if (!latexOutput) flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
		}
		if (!latexOutput) flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");

		free(tmpPosition);
		invariant = invariant->next;
	}
}

void
irPassDimensionalMatrixKernelPrinterFromBodyWithNumOfConstant(State *  N)
{
	Invariant *	invariant = N->invariantList;

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
			flexprint(N->Fe, N->Fm, N->Fpinfo, "Invariant \"%s\" has %d unique kernels, each with %d column(s)...\n\n",
							invariant->identifier, invariant->numberOfUniqueKernels, invariant->kernelColumnCount);

			if (N->mode & kCommonModeCGI)
			{
				flexprint(N->Fe, N->Fm, N->Fpmathjax, "\n\n$$\n");
				flexprint(N->Fe, N->Fm, N->Fpmathjax, "\\begin{aligned}\n");
			}

			for (int countKernel = 0; countKernel < invariant->numberOfUniqueKernels; countKernel++)
			{

				flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\n\tKernel %d is a valid kernel:\n\n", countKernel);

				/*
				 *	The number of rows of the kernel equals number of columns of the dimensional matrix.
				 */
				for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
				{
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\t");
					for (int col = 0; col < invariant->kernelColumnCount; col++)
					{
						flexprint(N->Fe, N->Fm, N->Fpinfo, "%4g",
								invariant->nullSpace[countKernel][row][col],
								(col == invariant->kernelColumnCount - 1 ? "" : " "));
					}
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
				}
				flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");

				for (int j = 0; j < invariant->dimensionalMatrixColumnCount; j++)
				{
					tmpPosition[invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + j]] = j;
				}

				/*
				 *	Prints out the a table of the symbolic expressions implied by the Pi groups derived from the kernels.	
				 */
				if (N->mode & kCommonModeCGI)
				{
					flexprint(N->Fe, N->Fm, N->Fpmathjax, "\t\\qquad\\qquad\\textcolor{DarkSlateGray}{\\mathbf{\\Pi\\text{ group }%d, \\text{ with column order }", countKernel);

					/*
					 *	PermutedIndexArray consists of the indices to show how exactly the matrix is permuted.
					 *	It stores all the permutation results for all the different kernels.
					 */

					flexprint(N->Fe, N->Fm, N->Fpmathjax, " \\left(");
					for (int i = 0; i < invariant->dimensionalMatrixColumnCount; i++)
					{
						flexprint(N->Fe, N->Fm, N->Fpmathjax, "%c%c", 'P'+(invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + i]/10), 
											'0'+invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + i]%10);
						if (i < invariant->dimensionalMatrixColumnCount -1)
						{
							flexprint(N->Fe, N->Fm, N->Fpmathjax, ",");
						}
					}
					flexprint(N->Fe, N->Fm, N->Fpmathjax, "\\right)}} \\qquad&\\textcolor{DeepSkyBlue}{\\dashrightarrow}\\qquad");

					for (int col = 0; col < invariant->kernelColumnCount; col++)
					{
						flexprint(N->Fe, N->Fm, N->Fpmathjax, "\\textcolor{DarkGreen}{\\dfrac{");
						for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
						{
							if (invariant->nullSpace[countKernel][tmpPosition[row]][col] > 0)
							{
								flexprint(N->Fe, N->Fm, N->Fpmathjax, "(");
								flexprint(N->Fe, N->Fm, N->Fpmathjax, "%s", invariant->dimensionalMatrixColumnLabels[row]);
								if (invariant->nullSpace[countKernel][tmpPosition[row]][col] > 1)
								{
									flexprint(N->Fe, N->Fm, N->Fpmathjax, "^{%g}", invariant->nullSpace[countKernel][tmpPosition[row]][col]);
								}
								flexprint(N->Fe, N->Fm, N->Fpmathjax, ")");
							}
						}
						flexprint(N->Fe, N->Fm, N->Fpmathjax, "}");
						
						flexprint(N->Fe, N->Fm, N->Fpmathjax, "{");
						for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
						{
							if (invariant->nullSpace[countKernel][tmpPosition[row]][col] < 0)
							{
								flexprint(N->Fe, N->Fm, N->Fpmathjax, "(");
								flexprint(N->Fe, N->Fm, N->Fpmathjax, "%s", invariant->dimensionalMatrixColumnLabels[row]);
								if (invariant->nullSpace[countKernel][tmpPosition[row]][col] < -1)
								{
									flexprint(N->Fe, N->Fm, N->Fpmathjax, "^{%g}", 0 - invariant->nullSpace[countKernel][tmpPosition[row]][col]);
								}
								flexprint(N->Fe, N->Fm, N->Fpmathjax, ")");
							}
						}
						/*
						 *	Close \dfrac and \textcolor
						 */
						flexprint(N->Fe, N->Fm, N->Fpmathjax, "}}");

						if (col < invariant->kernelColumnCount - 1)
						{
							flexprint(N->Fe, N->Fm, N->Fpmathjax, ",\\quad");
						}
						else
						{
							flexprint(N->Fe, N->Fm, N->Fpmathjax, "\\\\");
						}
					}
				}
				else
				{
					/*
					 *	PermutedIndexArray consists of the indices to show how exactly the matrix is permuted.
					 *	It stores all the permutation results for all the different kernels.
					 */
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\t\tThe ordering of parameters is:\t", countKernel);
					for (int i = 0; i < invariant->dimensionalMatrixColumnCount; i++)
					{
						flexprint(N->Fe, N->Fm, N->Fpinfo, "%c%c ", 'P'+(invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + i]/10), 
												'0'+invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + i]%10);
					}
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\n\n");

					for (int col = 0; col < invariant->kernelColumnCount; col++)
					{
						flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\t\tPi group %d, Pi %d is:\t", countKernel, col);
						
						for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
						{
							flexprint(N->Fe, N->Fm, N->Fpinfo, "%c%c", 'P'+(row/10), '0'+ (row%10) );
							flexprint(N->Fe, N->Fm, N->Fpinfo, "^(%2g)  ", invariant->nullSpace[countKernel][tmpPosition[row]][col]);
						}
					}
				}
			}

			if (N->mode & kCommonModeCGI)
			{
				flexprint(N->Fe, N->Fm, N->Fpmathjax, "\n\\end{aligned}\n");
				flexprint(N->Fe, N->Fm, N->Fpmathjax, "$$\n");
			}

			flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
		}
		flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");

		free(tmpPosition);
		invariant = invariant->next;
	}
}
