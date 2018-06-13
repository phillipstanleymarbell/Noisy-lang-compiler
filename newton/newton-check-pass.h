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
