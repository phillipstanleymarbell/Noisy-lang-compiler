NoisyIrNode *
newtonParse(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode *
newtonParseFile(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode *
newtonParseRuleList(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode *
newtonParseRule(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseBaseSignal(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseName(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseSymbol(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseDerivation(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode *
newtonParseTerminal(NoisyState *  N, NoisyIrNodeType expectedType, NoisyScope * currentScope);

NoisyIrNode *
newtonParseIdentifier(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode *
newtonParseIdentifierUsageTerminal(NoisyState *  N, NoisyIrNodeType expectedType, NoisyScope *  scope);

NoisyIrNode *
newtonParseIdentifierDefinitionTerminal(NoisyState *  N, NoisyIrNodeType  expectedType, NoisyScope *  scope);


