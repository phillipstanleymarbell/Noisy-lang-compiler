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
NoisyConfigIrNode * noisyConfigParseTerminal(NoisyConfigState *  N, NoisyConfigIrNodeType expectedType, NoisyConfigScope * currentScope);
NoisyConfigIrNode * noisyConfigParseAssignOp(NoisyConfigState *  N, NoisyConfigScope * currentScope);
NoisyConfigIrNode * noisyConfigParseIdentifier(NoisyConfigState *  N, NoisyConfigScope *  currentScope);
NoisyConfigIrNode * noisyConfigParseIdentifierDefinitionTerminal(NoisyConfigState *  N, NoisyConfigIrNodeType  expectedType, NoisyConfigScope *  scope);
