char * test_newtonApiInit_notNull();
char * test_newtonApiInit_notNullInvariant();
char * test_newtonApiGetInvariantByParameters_Valid();
char * test_newtonApiGetPhysicsTypeByName_Valid();
char * test_newtonApiPhysicsTypeUsageExample();

/* just a small helper method */
NoisyIrNode *
makeNoisyIrNodeSetToken(
                        NoisyState * N,
                        NoisyIrNodeType nodeType,
                        char * identifier,
                        char * stringConst,
                        double realConst
                        );
