void noisyTypeCheck(State * N);
bool noisySignatureIsMatching(State * N, IrNode * definitionSignature, IrNode * declarationSignature);
NoisyType getNoisyTypeFromTypeExpr(State * N, IrNode * typeExpr);
NoisyType getNoisyTypeFromBasicType(IrNode * basicType);