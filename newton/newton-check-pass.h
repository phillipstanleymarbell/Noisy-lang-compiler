void
newtonCheckCompareOp(
    NoisyState * N,
    NoisyIrNode * leftExpression, 
    NoisyIrNode * rightExpression,
    char * leftErrorMessage,
    char * rightErrorMessage,
    NoisyIrNodeType compareOpType,
    ConstraintReport * report
);

void
newtonCheckBinOp(
    NoisyState* N,
    NoisyIrNode * leftTerm,
    NoisyIrNode * rightTerm,
    NoisyIrNodeType binOpType,
    ConstraintReport * report
);

void
checkSingleConstraint(
    NoisyState * N,
    NoisyIrNode * constraintTreeRoot,
    NoisyIrNode* parameterTreeRoot,
    NewtonAPIReport* report
);

double
checkQuantityExpression(
    NoisyState * N,
    NoisyIrNode * constraintTreeRoot,
    NoisyIrNode * parameterTreeRoot,
    char * errorMessage,
    ConstraintReport * report
);

double
checkQuantityTerm(
    NoisyState * N,
    NoisyIrNode * constraintTreeRoot,
    NoisyIrNode * parameterTreeRoot,
    char * errorMessage,
    ConstraintReport * report
);

void
checkQuantityFactor(
    NoisyState * N,
    NoisyIrNode * constraintTreeRoot,
    NoisyIrNode * parameterTreeRoot,
    char * errorMessage,
    ConstraintReport * report
);

