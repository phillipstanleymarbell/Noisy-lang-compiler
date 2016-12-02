
NoisyIrNode * noisyConfigParse(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode * noisyConfigParseConfigFile(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode * noisyConfigParseDimensionTypeNameScope(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseDimensionTypeNameStatementList(NoisyState * N, NoisyScope * scope);
NoisyIrNode * noisyConfigParseDimensionTypeNameStatement(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode * noisyConfigParseLawScope(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseLawStatementList(NoisyState * N, NoisyScope * scope);
NoisyIrNode * noisyConfigParseLawStatement(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode * noisyConfigParseExpression(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode * noisyConfigParseTerm(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode * noisyConfigParseFactor(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode * noisyConfigParseDimensionAliasScope(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseDimensionAliasStatementList(NoisyState * N, NoisyScope * scope);
NoisyIrNode * noisyConfigParseDimensionAliasStatement(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode * noisyConfigParseVectorIntegralScope(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseVectorIntegralLists(NoisyState * N, NoisyScope * scope);
NoisyIrNode * noisyConfigParseVectorIntegralList(NoisyState * N, NoisyScope * scope);

NoisyIrNode * noisyConfigParseScalarIntegralScope(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseScalarIntegralLists(NoisyState * N, NoisyScope * scope);
NoisyIrNode * noisyConfigParseScalarIntegralList(NoisyState * N, NoisyScope * scope);

NoisyIrNode * noisyConfigParseVectorScalarPairScope(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseVectorScalarPairStatementList(NoisyState * N, NoisyScope * scope);
NoisyIrNode * noisyConfigParseVectorScalarPairStatement(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode * noisyConfigParseLowPrecedenceBinaryOp(NoisyState *  N, NoisyScope * currentScope);
NoisyIrNode * noisyConfigParseHighPrecedenceBinaryOp(NoisyState *  N, NoisyScope * currentScope);
NoisyIrNode * noisyConfigParseUnaryOp(NoisyState *  N, NoisyScope * currentScope);
NoisyIrNode * noisyConfigParseVectorOp(NoisyState *  N, NoisyScope * currentScope);
NoisyIrNode * noisyConfigParseAssignOp(NoisyState *  N, NoisyScope * currentScope);


NoisyIrNode * noisyConfigParseTerminal(NoisyState *  N, NoisyIrNodeType expectedType, NoisyScope * currentScope);
NoisyIrNode * noisyConfigParseIdentifier(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode * noisyConfigParseIdentifierDefinitionTerminal(NoisyState *  N, NoisyIrNodeType  expectedType, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseIdentifierUsageTerminal(NoisyState *  N, NoisyIrNodeType expectedType, NoisyScope *  scope);
