/*
 *	LibFlex's FlexList and FlexTuple etc. require user to tag type with their own enums.
 */
typedef enum
{
	kNoisyConfigFlexListTypeString,
	kNoisyConfigFlexListTypeNoisyConfigScopePointer,
} NoisyConfigFlexListType;



#define noisyConfigValidFlexTupleCheckMacro(x)	(((x)->siblings != NULL) && ((x)->siblings->hd != NULL))




NoisyConfigIrNode *	noisyConfigParse(NoisyConfigState *  N, NoisyConfigScope *  currentScope);
NoisyConfigIrNode *	noisyConfigParseConfigFile(NoisyConfigState *  N, NoisyConfigScope *  currentScope);

NoisyConfigIrNode * noisyConfigParseDimensionTypeNameScope(NoisyConfigState *  N, NoisyConfigScope *  scope);
NoisyConfigIrNode * noisyConfigParseDimensionTypeNameStatementList(NoisyConfigState * N, NoisyConfigScope * scope);
NoisyConfigIrNode * noisyConfigParseDimensionTypeNameStatement(NoisyConfigState *  N, NoisyConfigScope *  currentScope);

NoisyConfigIrNode * noisyConfigParseLawScope(NoisyConfigState *  N, NoisyConfigScope *  scope);
NoisyConfigIrNode * noisyConfigParseLawStatementList(NoisyConfigState * N, NoisyConfigScope * scope);
NoisyConfigIrNode * noisyConfigParseLawStatement(NoisyConfigState *  N, NoisyConfigScope *  currentScope);
NoisyConfigIrNode * noisyConfigParseExpression(NoisyConfigState *  N, NoisyConfigScope *  currentScope);
NoisyConfigIrNode * noisyConfigParseTerm(NoisyConfigState *  N, NoisyConfigScope *  currentScope);
NoisyConfigIrNode * noisyConfigParseFactor(NoisyConfigState * N, NoisyConfigScope * currentScope);

NoisyConfigIrNode * noisyConfigParseDimensionAliasScope(NoisyConfigState *  N, NoisyConfigScope *  scope);
NoisyConfigIrNode * noisyConfigParseDimensionAliasStatementList(NoisyConfigState * N, NoisyConfigScope * scope);
NoisyConfigIrNode * noisyConfigParseDimensionAliasStatement(NoisyConfigState *  N, NoisyConfigScope *  currentScope);

NoisyConfigIrNode * noisyConfigParseVectorIntegralScope(NoisyConfigState *  N, NoisyConfigScope *  scope);
NoisyConfigIrNode * noisyConfigParseVectorIntegralLists(NoisyConfigState * N, NoisyConfigScope * scope);
NoisyConfigIrNode * noisyConfigParseVectorIntegralList(NoisyConfigState * N, NoisyConfigScope * scope);

NoisyConfigIrNode * noisyConfigParseScalarIntegralScope(NoisyConfigState *  N, NoisyConfigScope *  scope);
NoisyConfigIrNode * noisyConfigParseScalarIntegralLists(NoisyConfigState * N, NoisyConfigScope * scope);
NoisyConfigIrNode * noisyConfigParseScalarIntegralList(NoisyConfigState * N, NoisyConfigScope * scope);

NoisyConfigIrNode * noisyConfigParseVectorScalarPairScope(NoisyConfigState *  N, NoisyConfigScope *  scope);
NoisyConfigIrNode * noisyConfigParseVectorScalarPairStatementList(NoisyConfigState * N, NoisyConfigScope * scope);
NoisyConfigIrNode * noisyConfigParseVectorScalarPairStatement(NoisyConfigState *  N, NoisyConfigScope *  currentScope);

NoisyConfigIrNode * noisyConfigParseLowPrecedenceBinaryOp(NoisyConfigState *  N, NoisyConfigScope * currentScope);
NoisyConfigIrNode * noisyConfigParseHighPrecedenceBinaryOp(NoisyConfigState *  N, NoisyConfigScope * currentScope);
NoisyConfigIrNode * noisyConfigParseUnaryOp(NoisyConfigState *  N, NoisyConfigScope * currentScope);
NoisyConfigIrNode * noisyConfigParseVectorOp(NoisyConfigState *  N, NoisyConfigScope * currentScope);
NoisyConfigIrNode * noisyConfigParseAssignOp(NoisyConfigState *  N, NoisyConfigScope * currentScope);


NoisyConfigIrNode * noisyConfigParseTerminal(NoisyConfigState *  N, NoisyConfigIrNodeType expectedType, NoisyConfigScope * currentScope);
NoisyConfigIrNode * noisyConfigParseIdentifier(NoisyConfigState *  N, NoisyConfigScope *  currentScope);
NoisyConfigIrNode * noisyConfigParseIdentifierDefinitionTerminal(NoisyConfigState *  N, NoisyConfigIrNodeType  expectedType, NoisyConfigScope *  scope);
NoisyConfigIrNode * noisyConfigParseIdentifierUsageTerminal(NoisyConfigState *  N, NoisyConfigIrNodeType expectedType, NoisyConfigScope *  scope);
