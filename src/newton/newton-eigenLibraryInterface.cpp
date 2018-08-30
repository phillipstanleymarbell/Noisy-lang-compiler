/*
	Authored 2018. Vlad Mihai Mandric & James Rhodes

	Based on skeleton implementation of Eigen interface by Phillip Stanley-Marbell.

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

#include <Eigen/Eigen>
#include <iostream>
using namespace std;
using namespace Eigen;

extern "C"
{
	static int gKernelNumber = 0;

	/*
	 *	A tail recursive implementation of n!
	 */
	static long
	factorial(long n, long N = 1)
	{
		if (n)
		{
			return factorial(n-1L, N*n);
		}
		else
		{
			return N;
		}
	}

	/*
	 *	Computes nCr, in a way designed to prevent overflow
	 */
	static long
	choose(long n, long r)
	{
		/*
		 *	First check arguments are in correct order, and reverse if not
		 */
		if (r > n)
		{
			return choose(r, n);
		}

		/*
		 *	Minimize the size of intermediate results to avoid potential overflow
		 */
		if (r < (n - r))
		{
			return choose(n, n - r);
		}

		/*
		 *	nPr = n! / r! = (r+1)*(r+2)*...*(n-1)*n
		 */
		long permutationsNR = 1L;
		for (long i = r + 1L; i <= n; i++)
		{
			permutationsNR *= i;
		}

		/*
		 *	nCr = nPr / (n-r)!
		 */
		long nMinusRFactorial = factorial(n - r);
		return permutationsNR / nMinusRFactorial;
	}

	/*
	 *	This is used by computeRREF to make a swap if necessary and returns the column
	 *	of the matrix at which it terminated
	 */
	static int
	makeRREFSwapIfNecessary(MatrixXd & matrix, int nonPivotIndices[], int matrixRank, int currentRow, int currentColumn)
	{
		int rowCount = matrix.rows();
		int columnCount = matrix.cols();

		if (currentColumn >= columnCount)
		{
			return currentColumn;
		}

		int nonPivotPosition = 0;
		while (nonPivotIndices[nonPivotPosition] != -1 && nonPivotPosition < columnCount - matrixRank - 1)
		{
			nonPivotPosition++;
		}
		
		/*
		 *	Only attempt to swap if current position is non-zero
		 */
		if (matrix(currentRow, currentColumn) == 0)
		{
			/*
			 *	Now search the matrix for a row with non-zero entry in this column
			 */
			for (int compareRow = currentRow + 1; compareRow < rowCount; compareRow++)
			{
				if (matrix(compareRow,currentColumn))
				{
					/*
					 *	Entry in compareRow is non-zero so we can make a swap
					 */
					matrix.row(currentRow).swap(matrix.row(compareRow));
					return currentColumn;
				}
			}

			/*
			 *	At this point we have not made a swap, try again with the next column
			 */
			nonPivotIndices[nonPivotPosition] = currentColumn;
			return makeRREFSwapIfNecessary(matrix, nonPivotIndices, matrixRank, currentRow, currentColumn+1);
		}
		else
		{
			return currentColumn;
		}
	}

	/*
	 *	Computes the row reduced echelon form (RREF)
	 */
	static void
	computeRREF(MatrixXd & matrix, int nonPivotIndices[], int matrixRank, int currentRow = 0, int currentColumn = 0)
	{
		int rowCount = matrix.rows();
		int columnCount = matrix.cols();

		if (currentRow >= rowCount)
		{
			/*
			 *	The last columns may not have been checked for swaps and should be non-pivots
			 */
			int columnNumber = columnCount - 1;
			int nonPivotIndex = columnCount - matrixRank - 1;
			while (nonPivotIndices[nonPivotIndex] == -1)
			{
				nonPivotIndices[nonPivotIndex] = columnNumber;
				columnNumber--;
				nonPivotIndex--;
			}

			return;
		}

		/*
		 *	Make the current entry non-zero
		 */
		currentColumn = makeRREFSwapIfNecessary(matrix, nonPivotIndices, matrixRank, currentRow, currentColumn);
		if (currentColumn >= columnCount)
		{
			return;
		}

		/*
		 *	Divide the current row through by the current entry
		 */
		matrix.row(currentRow) /= matrix(currentRow, currentColumn);

		/*
		 *	Make all other entries in current column zero by subtracting multiples of the current row
		 *	(which has a value of 1 at the current position)
		 */
		for (int q = 0; q < rowCount; q++)
		{
			if (q != currentRow)
			{
				matrix.row(q) -= matrix.row(currentRow) * matrix(q, currentColumn);
			}
		}

		/*
		 *	Repeat the algorithm, starting at the next position
		 */
		return computeRREF(matrix, nonPivotIndices, matrixRank, currentRow + 1, currentColumn + 1);
	}

	static void
	computePiGroup(MatrixXd matrix, int rowCount, int columnCount, int rank, int x[], MatrixXd kernels[], int kernelNumber)
	{
		int	nonPivotIndices[columnCount - rank];

		for (int i = 0; i < columnCount - rank; i++)
		{
			nonPivotIndices[i] = -1;
		}

		for (int i = 1; i <= rank; i++)
		{
			if (x[i] > i)
			{
				matrix.col(i-1).swap(matrix.col(x[i]-1));
			}
		}

		computeRREF(matrix, nonPivotIndices, rank);

		for (int i = 0; i < columnCount - rank; i++)
		{
			assert(nonPivotIndices[i] != -1);
		}

		MatrixXd nonPivotMatrix(rank, columnCount - rank);

		for (int i = 0; i < columnCount - rank; i++)
		{
			/*
			 *	Builds the matrix out of the non-pivot columns
			 */
			nonPivotMatrix.col(i) = matrix.col(nonPivotIndices[i]);
		}
		
		/*
		 *	Multiply the matrix by -1
		 */
		nonPivotMatrix *= -1;

		kernels[kernelNumber].resize(columnCount, columnCount - rank);
		int kernelRows = columnCount;
		MatrixXd identityMatrix = MatrixXd::Identity(columnCount - rank, columnCount - rank);
		
		int identityRow = 0, nonPivotRow = 0;
		int identitySize = columnCount - rank;
		for (int kernelRow = 0; kernelRow < kernelRows; kernelRow++)
		{
			if (identityRow < identitySize && kernelRow == nonPivotIndices[identityRow])
			{
				/*
				 *	Append the identity matrix to the non-pivot matrix
				 */
				kernels[kernelNumber].row(kernelRow) = identityMatrix.row(identityRow);
				identityRow++;
			}
			else
			{
				kernels[kernelNumber].row(kernelRow) = nonPivotMatrix.row(nonPivotRow);
				nonPivotRow++;
			}
		}
	}

	static void
	generateAllPiGroups(MatrixXd dimensionalMatrix, int rowCount, int columnCount, int rank, int x[], int k, MatrixXd kernels[])
	{	
		/*
		 *	Generate all the possible combinations of columns in lexicographic order
		 */

		if (k == rank + 1)
		{
			computePiGroup(dimensionalMatrix, rowCount, columnCount, rank, x, kernels, gKernelNumber);
			gKernelNumber++;
		}
		else
		{
			for (int i = x[k-1] + 1; i <= columnCount - rank + k; i++)
			{
				x[k] = i;
				generateAllPiGroups(dimensionalMatrix, rowCount, columnCount, rank, x, k + 1, kernels);
			}
		}
	}

	double **
	newtonEigenLibraryInterfaceGetPiGroups(double *  dimensionalMatrix, int rowCount, int columnCount)
	{
		Map<MatrixXd>	eigenInterfaceDimensionalMatrix (dimensionalMatrix, columnCount, rowCount);
		MatrixXd	eigenInterfaceTransposedDimensionalMatrix = eigenInterfaceDimensionalMatrix.transpose();
		int		rank = eigenInterfaceTransposedDimensionalMatrix.fullPivLu().rank();

		if (columnCount - rank == 0)
		{
			return NULL;
		}

		int 		numberOfCircuitSets = choose(columnCount,rank);
		MatrixXd	eigenInterfaceKernels[numberOfCircuitSets];
		double **	cInterfaceKernels;
		int		x[rank+1];
		int		k = 1;

		x[0] = 0;
		generateAllPiGroups(eigenInterfaceTransposedDimensionalMatrix, rowCount, columnCount, rank, x, k, eigenInterfaceKernels);

		cInterfaceKernels = (double **)calloc(numberOfCircuitSets*rowCount, sizeof(double *));
		assert(cInterfaceKernels != NULL);

		for (int i = 0; i < numberOfCircuitSets; i++)
		{
			cout << eigenInterfaceKernels[i] << endl << endl;
			cInterfaceKernels[i] = eigenInterfaceKernels[i].data();
		}

		return cInterfaceKernels;
	}
} /* extern "C" */
