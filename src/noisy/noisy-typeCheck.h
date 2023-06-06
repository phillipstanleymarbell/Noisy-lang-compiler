void noisyTypeCheck(State * N);
bool noisySignatureIsMatching(State * N, IrNode * definitionSignature, IrNode * declarationSignature);
NoisyType getNoisyTypeFromTypeExpr(State * N, IrNode * typeExpr);
NoisyType getNoisyTypeFromBasicType(IrNode * basicType);
bool noisyIsOfType(NoisyType typ1,NoisyBasicType typeSuperSet);
bool noisyIsSigned(NoisyType typ);
void noisySemanticErrorRecovery(State *  N);
void noisySemanticError(State *  N, IrNode * currentlyParsingNode, char *  details);