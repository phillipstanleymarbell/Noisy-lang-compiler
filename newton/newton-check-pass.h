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
    IrNode * leftTerm,
    IrNode * rightTerm,
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

double
checkQuantityExpression(
    State * N,
    IrNode * constraintTreeRoot,
    IrNode * parameterTreeRoot,
    char * errorMessage,
    ConstraintReport * report
);

double
checkQuantityTerm(
    State * N,
    IrNode * constraintTreeRoot,
    IrNode * parameterTreeRoot,
	bool * containsUnaryOp,
    char * errorMessage,
    ConstraintReport * report
);

void
checkQuantityFactor(
    State * N,
    IrNode * constraintTreeRoot,
    IrNode * parameterTreeRoot,
    char * errorMessage,
    ConstraintReport * report
);

void
addConstraintReportToNewtonAPIReport(NewtonAPIReport * newtonReport, ConstraintReport * constraintReport);
