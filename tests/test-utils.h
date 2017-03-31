/* just a small helper method */
IrNode *
makeIrNodeSetValue(
	State * N,
	IrNodeType nodeType,
	char * identifier,
	double realConst
	);

int numberOfConstraintsPassed(NewtonAPIReport* newtonReport);
IrNode * setupNthIrNodeType(State* noisy);

// TODO come up with a way to do this cleaner....
/* These methods are for invariant.nt */
IrNode * makeTestParameterTuple();
IrNode * makeSampleCorrectTestExpression();
IrNode * makeSampleCorrectTestStatement();
IrNode * makeSampleIncorrectTestExpression();
IrNode * makeSampleIncorrectTestStatement();


/* These methods are for pendulum_acceleration.nt */
IrNode *
makeTestParameterTuplePendulumCase();
IrNode * makeSampleCorrectTestExpressionPendulumCase();
IrNode * makeSampleCorrectTestStatementPendulumCase();
IrNode * makeSampleIncorrectTestExpressionPendulumCase();
IrNode * makeSampleIncorrectTestStatementPendulumCase();
