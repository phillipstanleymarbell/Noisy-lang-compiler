char * test_newtonApiInit_notNull();
char * test_newtonApiInit_notNullInvariant();
char * test_newtonApiGetInvariantByParameters_Valid();
char * test_newtonApiGetPhysicsTypeByName_Valid();
char * test_newtonApiPhysicsTypeUsageExample();
char * test_newtonCheckSingleInvariant();
char * test_newtonApiNumberParametersZeroToN();
char * tests_newtonApiDimensionCheckTree();
char * test_newtonApiDimensionCheckTree();

/* just a small helper method */
IrNode *
makeIrNodeSetValue(
                        State * N,
                        IrNodeType nodeType,
                        char * identifier,
                        double realConst
                        );

IrNode * makeTestParameterTuple();
IrNode * makeSampleCorrectTestExpression();
IrNode * makeSampleCorrectTestStatement();
IrNode * makeSampleIncorrectTestExpression();
IrNode * makeSampleIncorrectTestStatement();
