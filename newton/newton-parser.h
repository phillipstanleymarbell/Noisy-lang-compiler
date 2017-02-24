NoisyIrNode * 
newtonParse(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode * 
newtonParseFile(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode * 
newtonParseRuleList(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode * 
newtonParseRule(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode * 
newtonParseInvariant(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode * 
newtonParseParameter(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode * 
newtonParseParameterTuple(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode * 
newtonParseConstraint(NoisyState * N, NoisyScope * currentScope);

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

NoisyIrNode * 
newtonParseConstant(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode * 
newtonParseCompareOp(NoisyState * N, NoisyScope * currentScope);

bool
newtonIsConstant(Physics * physics);

unsigned long long int
newtonGetInvariantIdByParameters(NoisyState * N, NoisyIrNode * parameterTreeRoot, unsigned long long int invariantId);

int
newtonGetPhysicsId(NoisyState * N, Physics * physics);

char *
newtonParseGetPhysicsTypeStringByBoundIdentifier(NoisyState * N, NoisyIrNode * root, char* boundVariableIdentifier);

/* TODO currently this is unused... remove if still unused */
char *
newtonParseGetIdentifierByBoundPhysicsString(NoisyState * N, NoisyIrNode * root, char* physicsTypeString);

NoisyIrNode *
newtonParseFindNodeByPhysicsId(NoisyState *N, NoisyIrNode * root, int physicsId);

NoisyIrNode *
newtonParseFindNodeByBoundPhysicsString(NoisyState *N, NoisyIrNode * root, char* physicsTypeString);

