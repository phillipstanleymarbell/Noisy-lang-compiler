IrNode * 
newtonParse(State *  N, Scope *  currentScope);

IrNode * 
newtonParseFile(State *  N, Scope *  currentScope);

IrNode * 
newtonParseRuleList(State *  N, Scope *  currentScope);

IrNode * 
newtonParseRule(State * N, Scope * currentScope);

IrNode * 
newtonParseInvariant(State * N, Scope * currentScope);

IrNode * 
newtonParseParameter(State * N, Scope * currentScope, int parameterNumber);

IrNode * 
newtonParseParameterTuple(State * N, Scope * currentScope);

IrNode *
newtonParseSubindex(State * N, Scope * currentScope);

IrNode *
newtonParseSubindexTuple(State * N, Scope * currentScope);

IrNode * 
newtonParseConstraint(State * N, Scope * currentScope);

IrNode * 
newtonParseBaseSignal(State * N, Scope * currentScope);

IrNode * 
newtonParseName(State * N, Scope * currentScope);

IrNode * 
newtonParseSymbol(State * N, Scope * currentScope);

IrNode * 
newtonParseDerivation(State * N, Scope * currentScope);

IrNode * 
newtonParseTerminal(State *  N, IrNodeType expectedType, Scope * currentScope);

IrNode * 
newtonParseIdentifier(State *  N, Scope *  currentScope);

IrNode * 
newtonParseIdentifierUsageTerminal(State *  N, IrNodeType expectedType, Scope *  scope);

IrNode * 
newtonParseIdentifierDefinitionTerminal(State *  N, IrNodeType  expectedType, Scope *  scope);

IrNode * 
newtonParseConstant(State * N, Scope * currentScope);

IrNode * 
newtonParseCompareOp(State * N, Scope * currentScope);

bool
newtonIsDimensionless(Physics * physics);

void newtonParseResetPhysicsWithCorrectSubindex(
	State * N,
	IrNode * node,
	Scope * scope,
	char * identifier,
	int subindex);


unsigned long long int
newtonGetInvariantIdByParameters(State * N, IrNode * parameterTreeRoot, unsigned long long int invariantId);

int
newtonGetPhysicsId(State * N, Physics * physics);

Physics * 
newtonParseGetPhysicsByBoundIdentifier(State * N, IrNode * root, char * boundVariableIdentifier);

IrNode *
newtonParseFindNodeByPhysicsId(State * N, IrNode * root, int physicsId);

IrNode *
newtonParseFindParameterByTokenString(State * N, IrNode * parameterTreeRoot, char * tokenString);

IrNode *
newtonParseFindNodeByParameterNumberAndSubindex(State * N, IrNode * root, int parameterNumber, int subindex);

