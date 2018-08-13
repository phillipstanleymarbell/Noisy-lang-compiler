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
	void kernelPiGroups(MatrixXf m, int rank, int x[]){
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

		MatrixXf ker = m.fullPivLu().kernel();
		cout << "The Kernel is" << endl;
		cout << ker << endl << endl;
		cout << "m * ker = " << endl << m * ker << endl;

		return;
	}
	void constructPiGroups(int k, int M, int N, MatrixXf m, int x[], int rank){
		if(k == rank + 1){
			kernelPiGroups(m, rank, x);
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

		int rank = mat.fullPivLu().rank();
		cout << "The rank of the matrix is " << rank << endl;
		
		int x[rank+1];
		x[0] = 0;
		int k = 1;
		constructPiGroups(k, M, N, mat, x, rank);
	
		return;
	}

	
} /* extern "C" */
