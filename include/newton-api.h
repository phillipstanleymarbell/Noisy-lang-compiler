State * newtonApiInit(char *  newtonFileName);
Physics* newtonApiGetPhysicsTypeByName(State* N, char* nameOfType);
NewtonAPIReport* newtonApiSatisfiesConstraints(State* N, IrNode* parameterTreeRoot);
Invariant * newtonApiGetInvariantByParameters(State* N, IrNode* parameterTreeRoot);


void
iterateConstraints(
    State * N,
    IrNode * constraintsTreeRoot,
    IrNode* parameterTreeRoot,
    NewtonAPIReport* report
);


void		newtonApiAddLeaf(State *  N, IrNode *  parent, IrNode *  newNode);
void		newtonApiAddLeafWithChainingSeqNoLexer(State *  N, IrNode *  parent, IrNode *  newNode);
void    newtonApiNumberParametersZeroToN(State * N, IrNode * parameterTreeRoot);
