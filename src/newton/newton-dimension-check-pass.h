ConstraintReport*
newtonDimensionCheckExpressionOrStatement(
    State * N,
    IrNode * leftExpression
);


void
newtonDimensionCheckQuantityExpression(
	State * N,
	IrNode * expressionRoot,
	char * errorMessage,
	ConstraintReport * report
);

void
newtonDimensionCheckQuantityTerm(
	State * N,
	IrNode * termRoot,
	char * errorMessage,
	ConstraintReport * report
);

void
newtonDimensionCheckQuantityFactor(
	State *N,
	IrNode *factorRoot,
	char * errorMessage,
	ConstraintReport * report
);

void
newtonDimensionCheckExponentialExpression(
    State * N,
	IrNode * baseNode,
    IrNode * expressionRoot,
    char * errorMessage,
    ConstraintReport * report
);

double
newtonDimensionCheckNumericExpression(
    State * N,
    IrNode * expressionRoot,
    char * errorMessage,
    ConstraintReport * report
);

double
newtonDimensionCheckNumericTerm(
    State * N,
    IrNode * termRoot,
    char * errorMessage,
    ConstraintReport * report
);

void
newtonDimensionCheckNumericFactor(
    State * N,
    IrNode * factorRoot,
    char * errorMessage,
    ConstraintReport * report
);
