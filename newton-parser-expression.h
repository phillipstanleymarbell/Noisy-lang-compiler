NoisyIrNode *
newtonParseQuantityExpression(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseQuantityTerm(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseQuantityFactor(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseHighPrecedenceBinaryOp(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode * 
newtonParseLowPrecedenceBinaryOp(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseUnaryOp(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseCompareOp(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseMidPrecedenceBinaryOp(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseTimeOp(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseVectorOp(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseInteger(NoisyState * N, NoisyScope * currentScope);
