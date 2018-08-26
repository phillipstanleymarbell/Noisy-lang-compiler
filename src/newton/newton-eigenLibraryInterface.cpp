/*
	Authored 2018. Vlad Mihai Mandric.

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
	static long
	factorialHelper(long n, long N)
	{
		if (n)
		{
			return factorialHelper(n-1L, N*n);
		}
		else
		{
			return N;
		}
	}

	static long
	factorial(long i)
	{
		return factorialHelper(i,1L);
	}

	static long
	choose(long n, long r)
	{
		if(r <= n)
		{
			/*
			 * minimize the size of intermediate results to avoid potential overflow
			 * and use long to be on the safe side. perms is nPr
			 */
			if(r <= (n - r))
			{
				long perms = 1L;
				for(long i = n - r + 1L; i <= n; i++)
				{
					perms *= i;
				}
				long rFact = factorial(r);
				return perms / rFact;
			}
			else
			{
				return choose(n, n - r);
			}
		}
		else
		{
			return choose(n,r);
		}
	}

	/*
	 *	Computes the row reduced echelon form (RREF)
	 */
	static void
	computeRREF(MatrixXd & m, int rowCount, int columnCount, int indices[])
	{
		int i = 0, j = 0, r = 0;
		
		while (i < rowCount && j < columnCount)
		{
			/*
			 *	Step 1
			 */
			while (m(i, j) == 0)
			{
				bool swapped = false;
				for (int p = i + 1; i + p < rowCount; p++)
				{

					if (m(p, j))
					{
						m.row(i).swap(m.row(p));
						swapped = true;
						break;
					}
				}

				if (!swapped)
				{
					indices[r] = j;
					r++;
					j++;
				}

				if(j == columnCount)
				{
					return;
				}
			}

			/*
			 *	Step 2
			 */
			m.row(i) /= m(i, j);

			/*
			 *	Step 3
			 */
			for (int q = 0; q < rowCount; q++)
			{
				if (q != i)
				{
					m.row(q) -= m.row(i) * m(q, j) / m(i, j); 
				}
			}

			/*
			 *	Step 4
			 */
			i++;
			j++;
		}

		return;
	}

	static void
	computePiGroup(MatrixXd m, int rowCount, int columnCount, int rank, int x[], MatrixXd kernels[], int element)
	{
		assert(columnCount - rank != 0);
		int	indices[columnCount - rank];

		for (int i = 1; i <= rank; i++)
		{
			if(x[i] > i)
			{
				m.col(i-1).swap(m.col(x[i]-1));
			}
		}

		cout << m << endl << endl;

		computeRREF(m, rowCount, columnCount, indices);
		
		MatrixXd nonPivot(rank, columnCount - rank);
		for (int i = 0; i < columnCount - rank; i++)
		{
			/*
			 *	Builds the matrix out of the non-pivot columns
			 */
			nonPivot.block(0, i, rank, 1) = m.block(0, indices[i], rank, 1);
		}

		/*
		 *	Multiply the matrix by -1
		 */
		nonPivot *= -1;

		kernels[element].resize(columnCount, columnCount - rank);
		MatrixXd I = MatrixXd::Identity(columnCount - rank, columnCount - rank);
		
		int r = 0, p = 0;
		for (int i = 0; i < columnCount; i++)
		{
			if (r < columnCount - rank && i == indices[r])
			{
				kernels[element].row(i) = I.row(r);

				/*
				 *	Append the identity matrix to the non-pivot matrix
				 */
				r++;
			}
			else
			{
				kernels[element].row(i) = nonPivot.row(p);
				p++; 
			}
		}

		return;
	}

	static int element = 0;

	static void
	generateAllPiGroups(MatrixXd dimensionalMatrix, int rowCount, int columnCount, int rank, int x[], int k, MatrixXd kernels[])
	{	
		/*
		 *	Generate all the possible combinations of columns in lexicographic order
		 */

		if (k == rank + 1)
		{
			computePiGroup(dimensionalMatrix, rowCount, columnCount, rank, x, kernels, element);
			element++;
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

	/*
	 *	N = #rows, M = #columns
	 */
	double **
	newtonEigenLibraryInterfaceGetPiGroups(double *  dimensionalMatrix, int rowCount, int columnCount)
	{
		Map<MatrixXd>	eigenInterfaceDimensionalMatrix (dimensionalMatrix, columnCount, rowCount);
		MatrixXd	eigenInterfaceTransposedDimensionalMatrix = eigenInterfaceDimensionalMatrix.transpose();
		int		rank = eigenInterfaceTransposedDimensionalMatrix.fullPivLu().rank();
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
