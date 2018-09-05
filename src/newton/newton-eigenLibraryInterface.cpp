/*
	Authored 2018, Vlad Mihai Mandric, James Rhodes, Phillip Stanley-Marbell.

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
#include <stdint.h>
#include <float.h>
using namespace std;
using namespace Eigen;

extern "C"
{
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
	 *	This is used by transformMatrixToRREF to make a swap if necessary
	 *	and returns the column of the matrix at which it terminated
	 */
	static int
	makeRREFSwapIfNecessary(MatrixXd &  matrix, int nonPivotColumnIndices[], int matrixRank, int currentRow, int currentColumn)
	{
		int rowCount = matrix.rows();
		int columnCount = matrix.cols();

		if (currentColumn >= columnCount)
		{
			return currentColumn;
		}

		int nonPivotPosition = 0;
		while (nonPivotColumnIndices[nonPivotPosition] != -1 && nonPivotPosition < columnCount - matrixRank - 1)
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
			nonPivotColumnIndices[nonPivotPosition] = currentColumn;
			return makeRREFSwapIfNecessary(matrix, nonPivotColumnIndices, matrixRank, currentRow, currentColumn+1);
		}
		else
		{
			return currentColumn;
		}
	}

	/*
	 *	Transforms matrix to the row reduced echelon form (RREF)
	 */
	static void
	transformMatrixToRREF(MatrixXd &  matrix, int nonPivotColumnIndices[], int matrixRank, int currentRow = 0, int currentColumn = 0)
	{
		int rowCount = matrix.rows();
		int columnCount = matrix.cols();

		if (currentRow >= rowCount)
		{
			/*
			 *	The last columns may not have been checked for swaps
			 *	and should be non-pivots.
			 */
			int columnNumber = columnCount - 1;
			int nonPivotIndex = columnCount - matrixRank - 1;
			while (nonPivotColumnIndices[nonPivotIndex] == -1)
			{
				nonPivotColumnIndices[nonPivotIndex] = columnNumber;
				columnNumber--;
				nonPivotIndex--;
			}

			return;
		}

		/*
		 *	Make the current entry non-zero
		 */
		currentColumn = makeRREFSwapIfNecessary(matrix, nonPivotColumnIndices, matrixRank, currentRow, currentColumn);
		if (currentColumn >= columnCount)
		{
			return;
		}

		/*
		 *	Divide the current row through by the current entry
		 */
		matrix.row(currentRow) /= matrix(currentRow, currentColumn);

		/*
		 *	Make all other entries in current column zero by subtracting
		 *	multiples of the current row (which has a value of 1 at the
		 *	current position).
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
		return transformMatrixToRREF(matrix, nonPivotColumnIndices, matrixRank, currentRow + 1, currentColumn + 1);
	}

	static void
	computePiGroup(MatrixXd matrix, int rowCount, int columnCount, int rank, int nonPivotColumnIndices[], MatrixXd &  kernel)
	{
		MatrixXd nonPivotMatrix(matrix.rows(), columnCount - rank);

		for (int i = 0; i < columnCount - rank; i++)
		{
			/*
			 *	Builds the matrix out of the non-pivot columns
			 */
			nonPivotMatrix.col(i) = matrix.col(nonPivotColumnIndices[i]);
		}

		/*
		 *	Multiply the matrix by -1
		 */
		nonPivotMatrix *= -1;

		int		kernelRowCount	= columnCount;
		int		identitySize	= columnCount - rank;
		MatrixXd	identityMatrix	= MatrixXd::Identity(identitySize, identitySize);

		/*
		 *	The final kernel will have as many rows as there are columns
		 *	in the original matrix:
		 */
		kernel.resize(kernelRowCount, columnCount - rank);

		/*
		 *	Place the identity matrix in the _rows_ corresponding to the
		 *	nonpivot _columns_ in the original matrix. All other rows get
		 *	the rows from the non-pivot part of the RREF matrix
		 */
		int	nextNonPivotColumnIndex = 0;
		int	nextIdentityMatrixRowToCopy = 0;
		for (int i = 0; i < kernelRowCount; i++)
		{
			if ((nextNonPivotColumnIndex < (columnCount - rank)) && (i == nonPivotColumnIndices[nextNonPivotColumnIndex]))
			{
				kernel.row(i) = identityMatrix.row(nextNonPivotColumnIndex++);
			}
			else
			{
				kernel.row(i) = nonPivotMatrix.row(nextIdentityMatrixRowToCopy++);
			}
		}
	}



	/*
	 *	For a dimensional matrix with n parameters (n columns) and rank r,
	 *	(i.e., equivalently, with r pivot columns ==> r linearly-independent
	 *	columns) there are choose(n, r) ways in which we can rearrange the
	 *	columns of the dimensional matrix to yield a unique Pi group (see
	 *	Harald Hanche-Olsen, 2004 and E. Buckingham, 1914).
	 *
	 *	For each of these orderings of the r pivot columns, we generate 
	 *	all the possible n-bit words which have r '1's and we place the 
	 *	r pivot column indices at those positions.
	 *
	 *	If we also wanted to account not just for placement of the pivot
	 *	columns, but also for their permutations relative to each other,
	 *	we could use Heap's algorithm (B. R. Heap, "Permutations by 
	 *	interchanges". The Computer Journal. 6 (3): 293–4) to find all the
	 *	r! permutations of the r pivot column indices. 
	 *
	 *	Limitations:
	 *	(1)	We assume inputMatrix has less than 64 columns as we use a
	 *		uint64_t for the pivot position bitmap.
	 */

	static int
	onesCount(uint64_t word)
	{
		int	count = 0;
		for (size_t i = 0; i < sizeof(word)*8; i++)
		{
			count += (word >> i) & 0x1;
		}

		return count;
	}

	static uint64_t
	getKthNbitWordWithRankBitsSet(int n, int kth, int rank)
	{
		assert(n <= 64);

		int		which = 0;
		uint64_t	max = (1ULL << n) - 1ULL;
		for (uint64_t i = 0; i < max; i++)
		{
			if (onesCount(i) == rank)
			{
				if (which == kth)
				{
					return i;
				}

				which++;
			}
		}

		return which;
	}

	static void
	permuteWithBitMask(MatrixXd &  permutableMatrix, uint64_t permuteMask, int pivotColumnIndices[])
	{
		assert (permutableMatrix.cols() <= 64);

		int	nextPivot = 0;
		for (int i = 0; i < permutableMatrix.cols(); i++)
		{
			if ((permuteMask >> (permutableMatrix.cols() - 1 - i)) & 0x1)
			{
				permutableMatrix.col(i).swap(permutableMatrix.col(pivotColumnIndices[nextPivot++]));
			}
		}
	}

	static void
	resetWithNegativeOnes(int array[], int arrayLength)
	{
		for (int i = 0; i < arrayLength; i++)
		{
			array[i] = -1;
		}
	}

	static void
	sanityCheckForNoNegativeOnes(int array[], int arrayLength)
	{
		for (int i = 0; i < arrayLength; i++)
		{
			assert(array[i] != -1);
		}
	}

	double **
	newtonEigenLibraryInterfaceGetPiGroups(double *  dimensionalMatrix, int rowCount, int columnCount, int *  kernelColumnCount, int *  numberOfUniqueKernels)
	{
		Map<MatrixXd>	tmp (dimensionalMatrix, columnCount, rowCount);
		MatrixXd	eigenInterfaceDimensionalMatrix = tmp.transpose();
		int		rank = eigenInterfaceDimensionalMatrix.fullPivLu().rank();

		if (columnCount - rank == 0)
		{
			return NULL;
		}

		assert(rank > 0);

		int		nonPivotColumnIndices[columnCount - rank];
		int		pivotColumnIndices[rank];
		int		numberOfPivots = 0;
		int		numberOfCircuitSets = choose(columnCount, rank);
		MatrixXd	*eigenInterfaceKernels = new MatrixXd[numberOfCircuitSets];
		double **	cInterfaceKernels;

		/*
		 *	Initialize the non-pivot column indices array. It will get
		 *	populated by transformMatrixToRREF()
		 */
		resetWithNegativeOnes(nonPivotColumnIndices, columnCount - rank);
		
		/*
		 *	Transform eigenInterfaceDimensionalMatrix in-place into its RREF:
		 */
		transformMatrixToRREF(eigenInterfaceDimensionalMatrix, nonPivotColumnIndices, rank);

		/*
		 *	Sanity check: Make sure transformMatrixToRREF() set the total
		 *	of columnCount - rank non-pivot indices
		 */
		sanityCheckForNoNegativeOnes(nonPivotColumnIndices, columnCount - rank);

		/*
		 *	Construct an array of the pivot indices from the non-pivot indices:
		 */
		for (int i = 0; i < columnCount; i++)
		{
			bool	indexIsNonPivot = false;
			for (int j = 0; j < columnCount - rank; j++)
			{
				if (nonPivotColumnIndices[j] == i)
				{
					indexIsNonPivot = true;
				}
			}
			if (!indexIsNonPivot)
			{
				pivotColumnIndices[numberOfPivots++] = i;
			}
		}

		/*
		 *	Sanity check: Number of pivots should equal rank.
		 */
		assert(numberOfPivots == rank);

		/*
		 *	Allocate the C-array which we will send back to the Newton core.
		 */
		cInterfaceKernels = (double **)calloc(numberOfCircuitSets, sizeof(double *));
		assert(cInterfaceKernels != NULL);

		/*
		 *	Now that we know which indices are the pivots, we start again,
		 *	(1) permuting the pivot columns of the original matrix
		 *	(2) computing the RREF, (3) computing the null space.
		 */
		for (int i = 0; i < numberOfCircuitSets; i++)
		{
			MatrixXd	permutableMatrix = tmp.transpose();
			uint64_t	permuteMask = getKthNbitWordWithRankBitsSet(columnCount /* n */, i /* kth */, rank);

			permuteWithBitMask(permutableMatrix, permuteMask, pivotColumnIndices);


			/*
			 *	TODO: NOTE: For the computed kernels to be usable, we also need to
			 *	store information on what the permutationMask and pivotColumnIndices
			 *	was. We currently do not yet do that.
			 */



			/*
			 *	Initialize the non-pivot column indices array. It will get
			 *	populated by transformMatrixToRREF()
			 */
			resetWithNegativeOnes(nonPivotColumnIndices, columnCount - rank);

			/*
			 *	Transform the matrix in-place to row-reduced echelon form (RREF)
			 */
			transformMatrixToRREF(permutableMatrix, nonPivotColumnIndices, rank);

			/*
			 *	Sanity check: Make sure transformMatrixToRREF() set the total
			 *	of columnCount - rank non-pivot indices.
			 */
			sanityCheckForNoNegativeOnes(nonPivotColumnIndices, columnCount - rank);

			computePiGroup(permutableMatrix, rowCount, columnCount, rank, nonPivotColumnIndices, eigenInterfaceKernels[i]);

			bool	isDuplicateKernel = false;
			for (int j = 0; j < i; j++)
			{
				if (eigenInterfaceKernels[j] == eigenInterfaceKernels[i])
				{
					isDuplicateKernel = true;
				}
			}

			/*
			 *	If the new kernel does not duplicate an existing one, then bump the kernel count
			 */
			if (!isDuplicateKernel)
			{
				/*
				 *	Make the matrix non-fractional by multiplying by reciprocal
				 *	of smallest coefficient. Could do even better by multiplying
				 *	by LCM.
				 */
				double	minCoefficient	= DBL_MAX;
				int	nRows		= eigenInterfaceKernels[i].rows();
				int	nCols		= eigenInterfaceKernels[i].cols();
				for (int row = 0; row < nRows; row++)
				{
					for (int col = 0; col < nCols; col++)
					{
						if (abs(eigenInterfaceKernels[i](row, col)) > 0)
						{
							minCoefficient = min(abs(eigenInterfaceKernels[i](row, col)), minCoefficient);
						}
					}
				}
					
				eigenInterfaceKernels[i] *= (1.0 / minCoefficient);

				cInterfaceKernels[*numberOfUniqueKernels] = eigenInterfaceKernels[i].data();
				*numberOfUniqueKernels += 1;

// TODO: This print statement should be removed once we have a separate pass to print out the kernels, like we do for printing out the dimensional matrix. See issue #354
cout << "Kernel " << i << " is a new unique kernel:" << endl << eigenInterfaceKernels[i] << endl << endl;
			}
		}
// TODO: This print statement should be removed once we have a separate pass to print out the kernels, like we do for printing out the dimensional matrix. See issue #354
cout << "Number of unique kernels is " << *numberOfUniqueKernels << endl << endl;

		return cInterfaceKernels;
	}
} /* extern "C" */
