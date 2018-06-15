/*
	Authored 2017. Jonathan Lim.  Modified 2018, Phillip Stanley-Marbell.

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

typedef struct Indices Indices;


/* only used in newton-check-pass.c */
struct Indices {
	int expressionIndex;
	int termIndex;
	int factorIndex;
	int lowBinOpIndex;
	int midBinOpIndex;
	int highBinOpIndex;
};


void
newtonCheckCompareOp(
    State * N,
    IrNode * leftExpression, 
    IrNode * rightExpression,
    char * leftErrorMessage,
    char * rightErrorMessage,
    IrNodeType compareOpType,
    ConstraintReport * report
);

void
newtonCheckBinOp(
    State* N,
	IrNode * parent,
    IrNode * left,
    IrNode * right,
    IrNodeType binOpType,
    ConstraintReport * report
);

void
checkSingleConstraint(
    State * N,
    IrNode * constraintTreeRoot,
    IrNode* parameterTreeRoot,
    NewtonAPIReport* report
);

Indices *
checkQuantityExpression(
    State * N,
    IrNode * constraintTreeRoot,
    IrNode * parameterTreeRoot,
    char * errorMessage,
    ConstraintReport * report
);

Indices *
checkQuantityTerm(
    State * N,
    IrNode * constraintTreeRoot,
    IrNode * parameterTreeRoot,
    char * errorMessage,
    ConstraintReport * report
);

Indices *
checkQuantityFactor(
    State * N,
    IrNode * constraintTreeRoot,
    IrNode * parameterTreeRoot,
    char * errorMessage,
    ConstraintReport * report
);

void
addConstraintReportToNewtonAPIReport(
    NewtonAPIReport * newtonReport, 
    ConstraintReport * constraintReport
);

void
updateDestinationTrackerIndicesFromSource(Indices * dest, Indices * source);
