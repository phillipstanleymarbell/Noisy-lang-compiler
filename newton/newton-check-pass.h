void
newtonCheckCompareOp(
    NoisyState* N,
    NoisyIrNode * leftExpression, 
    NoisyIrNode * rightExpression,
    NoisyIrNodeType compareOpType,
    NewtonAPIReport * report
);

void
newtonCheckBinOp(
    NoisyState* N,
    NoisyIrNode * leftTerm, 
    NoisyIrNode * rightTerm,
    NoisyIrNodeType binOpType,
    NewtonAPIReport * report
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
    NewtonAPIReport * report
);

double
checkQuantityTerm(
    NoisyState * N,
    NoisyIrNode * constraintTreeRoot,
    NoisyIrNode * parameterTreeRoot,
    NewtonAPIReport * report
);

void
checkQuantityFactor(
    NoisyState * N,
    NoisyIrNode * constraintTreeRoot,
    NoisyIrNode * parameterTreeRoot,
    NewtonAPIReport * report
);

