/*
	Authored 2018. Phillip Stanley-Marbell

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

#include "newton-eigenLibraryInterface.h"

#include <Eigen/Eigen>
#include <iostream>
using namespace std;
using namespace Eigen;

extern "C"
{
	void rref(MatrixXf & m, int N, int M, VectorXi & indices){
		int i = 0, j = 0, r = 0;
		
		while(i < N && j < M){
			//Step 1
			int p = 1;
			int k = j;
			while (k < N){
				if(m(i, k) == 0){
					do{
						m.row(i).swap(m.row(i+p));
						p++;
					}
					while(m(i, k) == 0);
				}

				if(m(i, k) == 0){
					indices(r) = k;
					r++;
					k++;
				}
				else{
					break;
				}
			}
			//Step 2
			m.array().row(i) /= m(i, j);

			//Step 3
			for(int q = 0; q < N; q++){
				if(q != i){
					m.array().row(q) -= m.array().row(i) * m(q, j) / m(i, j); 
				}
			}
			
			//Step 4
			i++;
			j++;
		}

		return;
	}

	void kernelPiGroups(MatrixXf m, int rank, int x[], int N, int M){
		cout << endl << endl << endl;
		cout << "The column basis is" << endl;
		for(int i = 1; i <= rank; i++){
			cout << x[i] << " ";
			if(x[i] > i){
				m.col(i-1).swap(m.col(x[i]-1));
			}
		}
		cout << endl << endl;
		cout << "Dimensional Matrix" << endl;
		cout << m << endl << endl;

		VectorXi indices(M - rank);
		rref(m, N, M, indices);
		cout << "The RREF form of the matrix is " << endl;
		cout << m << endl << endl;
		
		MatrixXf nonPivot(N, M - rank);
		for(int i = 0; i < M - rank; i++){
			nonPivot.col(i) = m.col(indices(i)); 
		}
		cout << nonPivot << endl;
		nonPivot.array() = nonPivot.array() * -1;

		MatrixXf ker(M, M - rank);
		MatrixXf I = MatrixXf::Identity(M - rank, M - rank);
		
		int r = 0, p = 0;
		for(int i = 0; i < N; i++){
			if(i == indices(r)){
				ker.row(i) = nonPivot.row(i);
				r++;
			}
			else{
				ker.row(i) = I.row(p);
				p++; 
			}
		}

		cout << "The Kernel is" << endl;
		cout << ker << endl << endl;
		cout << "m * ker = " << endl << m * ker << endl;

		return;
	}
	void constructPiGroups(int k, int N, int M, MatrixXf m, int x[], int rank){
		if(k == rank + 1){
			kernelPiGroups(m, rank, x, M, N);
		}
		else{
			for(int i = x[k-1] + 1; i <= M -rank + k; i++){
				x[k] = i;
				constructPiGroups(k+1, M, N, m, x, rank);
			}
		}
	}

	void getPiGroups(float *m, int N, int M){
		Map<MatrixXf> tmp (m, M, N);
		MatrixXf mat = tmp.transpose();

		MatrixXf n(3, 5);
		n << -2, -3, -1, 1, 1,
			 -2, 0, -1, 0, -1,
			 1, 1, 1, 0, 0;

		int rank = mat.fullPivLu().rank();
		cout << "The rank of the matrix is " << rank << endl;
		
		int x[rank+1];
		x[0] = 0;
		int k = 1;
		constructPiGroups(k, N, M, mat, x, rank);
	
		return;
	}

	
} /* extern "C" */
