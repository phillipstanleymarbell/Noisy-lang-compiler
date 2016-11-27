
void		errorUseBeforeDefinition(NoisyConfigState *  N, const char *  identifier);
void		errorMultiDefinition(NoisyConfigState *  N, struct NoisyConfigSymbol *  symbol);

bool		peekCheck(NoisyConfigState *  N, int lookAhead, NoisyConfigIrNodeType expectedType);
struct NoisyConfigIrNode *	depthFirstWalk(NoisyConfigState *  N, NoisyConfigIrNode *  node);
void		addLeaf(NoisyConfigState *  N, NoisyConfigIrNode *  parent, NoisyConfigIrNode *  newNode, NoisyConfigScope * currentScope);
void		addLeafWithChainingSeq(NoisyConfigState *  N, NoisyConfigIrNode *  parent, NoisyConfigIrNode *  newNode, NoisyConfigScope * currentScope);
void		addToProgtypeScopes(NoisyConfigState *  N, char *  identifier, NoisyConfigScope *  progtypeScope);
void		assignTypes(NoisyConfigState *  N, NoisyConfigIrNode *  node, NoisyConfigIrNode *  typeExpression);
void        noisyConfigParserSyntaxError(
	NoisyConfigState *  N, 
	NoisyConfigIrNodeType currentlyParsingTokenOrProduction, 
    NoisyConfigIrNodeType expectedProductionOrToken
);
