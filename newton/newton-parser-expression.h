IrNode *
newtonParseQuantityExpression(State * N, Scope * currentScope);

IrNode *
newtonParseQuantityTerm(State * N, Scope * currentScope);

IrNode *
newtonParseQuantityFactor(State * N, Scope * currentScope);

IrNode *
newtonParseNumericExpression(State * N, Scope * currentScope);

IrNode *
newtonParseNumericTerm(State * N, Scope * currentScope);

IrNode *
newtonParseNumericFactor(State * N, Scope * currentScope);

IrNode *
newtonParseHighPrecedenceBinaryOp(State * N, Scope * currentScope);

IrNode * 
newtonParseLowPrecedenceBinaryOp(State * N, Scope * currentScope);

IrNode *
newtonParseUnaryOp(State * N, Scope * currentScope);

IrNode *
newtonParseCompareOp(State * N, Scope * currentScope);

IrNode *
newtonParseMidPrecedenceBinaryOp(State * N, Scope * currentScope);

IrNode *
newtonParseInteger(State * N, Scope * currentScope);

IrNode *
newtonParseExponentialExpression(State * N, Scope * currentScope, IrNode * exponentBase);
