void
newtonDimensionPassParse(State *  N, Scope *  currentScope);

void
newtonDimensionPassParseFile(State *  N, Scope *  currentScope);

void
newtonDimensionPassParseRuleList(State *  N, Scope *  currentScope);

void
newtonDimensionPassParseRule(State * N, Scope * currentScope);

void
newtonDimensionPassParseBaseSignal(State * N, Scope * currentScope);

void
newtonDimensionPassParseSubindexTuple(State * N, Scope * currentScope);

void
newtonDimensionPassParseSubindex(State * N, Scope * currentScope);

IrNode *
newtonDimensionPassParseName(State * N, Scope * currentScope);

IrNode *
newtonDimensionPassParseSymbol(State * N, Scope * currentScope);



