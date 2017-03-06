NoisyState * newtonApiInit(char *  newtonFileName);
Physics* newtonApiGetPhysicsTypeByName(NoisyState* N, char* nameOfType);
NewtonAPIReport* newtonApiSatisfiesConstraints(NoisyState* N, NoisyIrNode* parameterTreeRoot);
Invariant * newtonApiGetInvariantByParameters(NoisyState* N, NoisyIrNode* parameterTreeRoot);


void
iterateConstraints(
    NoisyState * N,
    NoisyIrNode * constraintsTreeRoot,
    NoisyIrNode* parameterTreeRoot,
    NewtonAPIReport* report
);


void		newtonApiAddLeaf(NoisyState *  N, NoisyIrNode *  parent, NoisyIrNode *  newNode);
void		newtonApiAddLeafWithChainingSeqNoLexer(NoisyState *  N, NoisyIrNode *  parent, NoisyIrNode *  newNode);
void    newtonApiNumberParametersZeroToN(NoisyState * N, NoisyIrNode * parameterTreeRoot);
