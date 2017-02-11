NoisyState * newtonApiInit(char *  newtonFileName);
Physics* getPhysicsTypeByName(NoisyState* N, char* nameOfType);
NewtonAPIReport* satisfiesConstraints(NoisyState* N, NoisyIrNode* parameterTreeRoot);
