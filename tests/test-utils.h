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
IrNode * makeTestParameterTuplePendulumCase();
IrNode * makeSampleCorrectTestExpressionPendulumCase();
IrNode * makeSampleCorrectTestStatementPendulumCase();
IrNode * makeSampleIncorrectTestExpressionPendulumCase();
IrNode * makeSampleIncorrectTestStatementPendulumCase();

/* These methods are for pressure_sensors.nt */
IrNode * makeTestParameterTuplePressureCaseBoyles();
IrNode * makeTestParameterTuplePressureCaseGayLussac();
IrNode * makeTestParameterTuplePressureCaseAvogadro();
IrNode * makeSampleCorrectTestExpressionPressureCase();
IrNode * makeSampleCorrectTestStatementPressureCase();
IrNode * makeSampleIncorrectTestExpressionPressureCase();
IrNode * makeSampleIncorrectTestStatementPressureCase();

/* These methods are for electricity.nt */
IrNode * makeTestParameterTupleElectricityCaseCurrent();
IrNode * makeTestParameterTupleElectricityCaseCapacitance();
IrNode * makeTestParameterTupleElectricityCasePower();
IrNode * makeSampleCorrectTestExpressionElectricityCase();
IrNode * makeSampleCorrectTestStatementElectricityCase();
IrNode * makeSampleIncorrectTestExpressionElectricityCase();
IrNode * makeSampleIncorrectTestStatementElectricityCase();
