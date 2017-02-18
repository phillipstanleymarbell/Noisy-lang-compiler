NoisyState * newtonApiInit(char *  newtonFileName);
Physics* newtonApiGetPhysicsTypeByName(NoisyState* N, char* nameOfType);
NewtonAPIReport* newtonApiSatisfiesConstraints(NoisyState* N, NoisyIrNode* parameterTreeRoot);
Invariant * newtonApiGetInvariantByParameters(NoisyState* N, NoisyIrNode* parameterTreeRoot);

void newtonCheckSingleInvariant(
    NoisyState* N, 
    Invariant* invariant,
    NoisyIrNode* parameterTreeRoot
);

void
iterateConstraints(
    NoisyState * N,
    NoisyIrNode * constraintsTreeRoot,
    NoisyIrNode* parameterTreeRoot,
    NewtonAPIReport* report
);

