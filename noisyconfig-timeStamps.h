/*
 *	NOTE:   Add entries here for new locations we want to time stamp; all
 *	functions in the implementation should ideally have entries here.
 */
typedef enum
{
	/*
	 *	Parser global routines in noisy-parser.c.
	 */
	kNoisyConfigTimeStampKeyParse,
	kNoisyConfigTimeStampKeyParseProgram,
	kNoisyConfigTimeStampKeyParseProgtypeDeclaration,
	kNoisyConfigTimeStampKeyParseProgtypeBody,
	kNoisyConfigTimeStampKeyParseProgtypeTypenameDeclaration,
	kNoisyConfigTimeStampKeyParseConstantDeclaration,
	kNoisyConfigTimeStampKeyParseTypeDeclaration,
	kNoisyConfigTimeStampKeyParseAdtTypeDeclaration,
	kNoisyConfigTimeStampKeyParseNamegenDeclaration,
	kNoisyConfigTimeStampKeyParseIdentifierOrNil,
	kNoisyConfigTimeStampKeyParseIdentifierOrNilList,
	kNoisyConfigTimeStampKeyParseIdentifierList,
	kNoisyConfigTimeStampKeyParseTypeExpression,
	kNoisyConfigTimeStampKeyParseTypeName,
	kNoisyConfigTimeStampKeyParseTolerance,
	kNoisyConfigTimeStampKeyParseErrorMagnitudeTolerance,
	kNoisyConfigTimeStampKeyParseLossTolerance,
	kNoisyConfigTimeStampKeyParseLatencyTolerance,
	kNoisyConfigTimeStampKeyParseBasicType,
	kNoisyConfigTimeStampKeyParseRealType,
	kNoisyConfigTimeStampKeyParseFixedType,
	kNoisyConfigTimeStampKeyParseAnonAggregateType,
	kNoisyConfigTimeStampKeyParseArrayType,
	kNoisyConfigTimeStampKeyParseListType,
	kNoisyConfigTimeStampKeyParseTupleType,
	kNoisyConfigTimeStampKeyParseSetType,
	kNoisyConfigTimeStampKeyParseInitList,
	kNoisyConfigTimeStampKeyParseIdxInitList,
	kNoisyConfigTimeStampKeyParseStarInitList,
	kNoisyConfigTimeStampKeyParseElement,
	kNoisyConfigTimeStampKeyParseNamegenDefinition,
	kNoisyConfigTimeStampKeyParseScopedStatementList,
	kNoisyConfigTimeStampKeyParseStatementList,
	kNoisyConfigTimeStampKeyParseStatement,
	kNoisyConfigTimeStampKeyParseAssignOp,
	kNoisyConfigTimeStampKeyParseMatchStatement,
	kNoisyConfigTimeStampKeyParseIterStatement,
	kNoisyConfigTimeStampKeyParseGuardBody,
	kNoisyConfigTimeStampKeyParseExpression,
	kNoisyConfigTimeStampKeyParseListCastExpression,
	kNoisyConfigTimeStampKeyParseSetCastExpression,
	kNoisyConfigTimeStampKeyParseArrayCastExpression,
	kNoisyConfigTimeStampKeyParseAnonAggregateCastExpression,
	kNoisyConfigTimeStampKeyParseChanEventExpression,
	kNoisyConfigTimeStampKeyParseChan2nameExpression,
	kNoisyConfigTimeStampKeyParseVar2nameExpression,
	kNoisyConfigTimeStampKeyParseName2chanExpression,
	kNoisyConfigTimeStampKeyParseTerm,
	kNoisyConfigTimeStampKeyParseFactor,
	kNoisyConfigTimeStampKeyParseTupleValue,
	kNoisyConfigTimeStampKeyParseFieldSelect,
	kNoisyConfigTimeStampKeyParseHighPrecedenceBinaryO,
	kNoisyConfigTimeStampKeyParseLowPrecedenceBinaryOp,
	kNoisyConfigTimeStampKeyParseCmpOp,
	kNoisyConfigTimeStampKeyParseBooleanOp,
	kNoisyConfigTimeStampKeyParseUnaryOp,
	kNoisyConfigTimeStampKeyParseTerminal,
	kNoisyConfigTimeStampKeyParseIdentifierUsageTerminal,
	kNoisyConfigTimeStampKeyParseIdentifierDefinitionTerminal,

	/*
	 *	Parser local routines in noisy-parser.c.
	 */
	kNoisyConfigTimeStampKeyParserSyntaxError,
	kNoisyConfigTimeStampKeyParserSemanticError,
	kNoisyConfigTimeStampKeyParserErrorRecovery,
	kNoisyConfigTimeStampKeyParserProgtypeName2scope,
	kNoisyConfigTimeStampKeyParserErrorUseBeforeDefinition,
	kNoisyConfigTimeStampKeyParserErrorMultiDefinition,
	kNoisyConfigTimeStampKeyParserPeekCheck,
	kNoisyConfigTimeStampKeyLexPeekPrint,
	kNoisyConfigTimeStampKeyParserDepthFirstWalk,
	kNoisyConfigTimeStampKeyParserAddLeaf,
	kNoisyConfigTimeStampKeyParserAddLeafWithChainingSeq,
	kNoisyConfigTimeStampKeyParserAddToProgtypeScopes,
	kNoisyConfigTimeStampKeyParserAssignTypes,


	/*
	 *	Symbol table routines in noisy-symbolTable.c.
	 */
	kNoisyConfigTimeStampKeySymbolTableAllocScope,
	kNoisyConfigTimeStampKeySymbolTableAddOrLookupSymbolForToken,
	kNoisyConfigTimeStampKeySymbolTableSymbolForIdentifier,
	kNoisyConfigTimeStampKeySymbolTableOpenScope,
	kNoisyConfigTimeStampKeySymbolTableCloseScope,


	/*
	 *	Ir helper routines in noisy-irHelpers.c.
	 */
	kNoisyConfigTimeStampKeyGenNoisyIrNode,


	/*
	 *	Lexer public routines in noisy-lexer.c.
	 */
	kNoisyConfigTimeStampKeyLexAllocateSourceInfo,
	kNoisyConfigTimeStampKeyLexAllocateToken,
	kNoisyConfigTimeStampKeyLexPut,
	kNoisyConfigTimeStampKeyLexGet,
	kNoisyConfigTimeStampKeyLexPeek,
	kNoisyConfigTimeStampKeyLexInit,
	kNoisyConfigTimeStampKeyLexPrintToken,
	kNoisyConfigTimeStampKeyLexDebugPrintToken,

	/*
	 *	Lexer local routines in noisy-lexer.c.
	 */
	kNoisyConfigTimeStampKeyLexerCheckTokenLength,
	kNoisyConfigTimeStampKeyLexerCur,
	kNoisyConfigTimeStampKeyLexerGobble,
	kNoisyConfigTimeStampKeyLexerDone,
	kNoisyConfigTimeStampKeyLexerEqf,
	kNoisyConfigTimeStampKeyLexerCheckComment,
	kNoisyConfigTimeStampKeyLexerCheckWeq,
	kNoisyConfigTimeStampKeyLexerCheckWeq3,
	kNoisyConfigTimeStampKeyLexerCheckSingle,
	kNoisyConfigTimeStampKeyLexerCheckDot,
	kNoisyConfigTimeStampKeyLexerCheckGt,
	kNoisyConfigTimeStampKeyLexerCheckLt,
	kNoisyConfigTimeStampKeyLexerCheckSingleQuote,
	kNoisyConfigTimeStampKeyLexerCheckDoubleQuote,
	kNoisyConfigTimeStampKeyLexerCheckMinus,
	kNoisyConfigTimeStampKeyLexerFinishToken,
	kNoisyConfigTimeStampKeyLexerMakeNumericConst,
	kNoisyConfigTimeStampKeyLexerIsDecimal,
	kNoisyConfigTimeStampKeyLexerStringAtLeft,
	kNoisyConfigTimeStampKeyLexerStringAtRight,
	kNoisyConfigTimeStampKeyLexerIsDecimalSeparatedWithChar,
	kNoisyConfigTimeStampKeyLexerIsRadixConst,
	kNoisyConfigTimeStampKeyLexerIsRealConst,
	kNoisyConfigTimeStampKeyLexerIsEngineeringRealConst,
	kNoisyConfigTimeStampKeyLexerStringToRadixConst,
	kNoisyConfigTimeStampKeyLexerStringToRealConst,
	kNoisyConfigTimeStampKeyLexerStringToEngineeringRealConst,
	kNoisyConfigTimeStampKeyLexerIsOperatorOrSeparator,


	/*
	 *	First/Follow set routines in noisy-firstAndFollow.c.
	 */
	kNoisyConfigTimeStampKeyInFirst,
	kNoisyConfigTimeStampKeyInFollow,


	/*
	 *	Type-related routines in noisy-types.c
	 */
	kNoisyConfigTimeStampKeyTypeValidateIrSubtree,
	kNoisyConfigTimeStampKeyTypeEqualsSubtreeTypes,
	kNoisyConfigTimeStampKeyTypeMakeTypeSignature,


	/*
	 *	Miscellaneous platform and glue routines in noisy.c.
	 */
	kNoisyConfigTimeStampKeyNoisyInit,
	kNoisyConfigTimeStampKeyRunPasses,
	kNoisyConfigTimeStampKeyCheckRss,
	kNoisyConfigTimeStampKeyConsolePrintBuffers,
	kNoisyConfigTimeStampKeyPrintToFile,
	kNoisyConfigTimeStampKeyRenderDotInFile,
	kNoisyConfigTimeStampKeyCheckCgiCompletion,
	kNoisyConfigTimeStampKeyFatal,
	kNoisyConfigTimeStampKeyError,


	/*
	 *	Public routines in Dotbackend
	 */
	kNoisyConfigTimeStampKeyIrPassDotAstDotFmt,
	kNoisyConfigTimeStampKeyIrPassDotSymbotTableDotFmt,
	kNoisyConfigTimeStampKeyIrPassAstDotPrintWalk,
	kNoisyConfigTimeStampKeyIrPassSymbolTableDotPrintWalk,
	kNoisyConfigTimeStampKeyIrPassDotBackend,

	/*
	 *	Private routines in Dotbackend
	 */
	kNoisyConfigTimeStampKeyIrPassDotIsType,
	kNoisyConfigTimeStampKeyIrPassDotScope2Id,
	kNoisyConfigTimeStampKeyIrPassDotScope2Id2,
	kNoisyConfigTimeStampKeyIrPassDotSymbol2Id,


	/*
	 *	Public routines Protobuf backend
	 */
	kNoisyConfigTimeStampKeyIrPassProtobufSymbotTableEmitter,
	kNoisyConfigTimeStampKeyIrPassProtobufAstEmitter,
	kNoisyConfigTimeStampKeyIrPassProtobufAstSerializeWalk,
	kNoisyConfigTimeStampKeyIrPassProtobufSymbolTableSerializeWalk,
	kNoisyConfigTimeStampKeyIrPassTypeChecker,


	/*
	 *	IR pass helpers. TODO: should merge this into IR helpers...
	 */
	kNoisyConfigTimeStampKeyIrPassHelperColorIr,
	kNoisyConfigTimeStampKeyIrPassHelperColorSymbolTable,
	kNoisyConfigTimeStampKeyIrPassHelperIrSize,
	kNoisyConfigTimeStampKeyIrPassHelperSymbolTableSize,

	/*
	 *	Used to tag un-tracked time.
	 */
	kNoisyConfigTimeStampKeyUnknown,


	/*
	 *	Code depends on this being last.
	 */
	kNoisyConfigTimeStampKeyMax,
} NoisyConfigTimeStampKey;

typedef struct
{
	uint64_t		nanoseconds;
	NoisyConfigTimeStampKey	key;
} NoisyConfigTimeStamp;


#ifdef NoisyOsMacOSX
#include <mach/mach.h>
#include <mach/mach_time.h>

/*
 *	NOTE: The final N->callAggregateTotal and N->timestampCount won't match
 *	because we roll timestampCount modulo number of slots. 
 *
 *	We get complementary information from DTrace (see precommitStatisticsHook.sh).
 *	However, having this manual tracing ability that we can statically compile in
 *	lets us validate our DTrace setup, while also providing a quick statistics in
 *	a self-contained mechanism.
 */
#define NoisyConfigTimeStampTraceMacro(routineKey)		if (N->mode & kNoisyConfigModeCallStatistics)\
							{\
								uint64_t	now = mach_absolute_time();\
								\
								N->timestamps[N->timestampCount].nanoseconds = now;\
								N->timestamps[N->timestampCount].key = (routineKey);\
								\
								if (N->callAggregateTotal > 0)\
								{\
									N->timeAggregates[N->timeAggregatesLastKey] += (now - N->timeAggregatesLastTimestamp);\
									N->timeAggregateTotal += (now - N->timeAggregatesLastTimestamp);\
								}\
								\
								N->timeAggregatesLastKey = (routineKey);\
								N->timeAggregatesLastTimestamp = now;\
								N->timestampCount = (N->timestampCount + 1) % N->timestampSlots;\
								\
								N->callAggregates[routineKey]++;\
								N->callAggregateTotal++;\
							}\

#else
#define NoisyConfigTimeStampTraceMacro(routineKey) 
#endif /* #ifdef NoisyOsMacOSX */

extern const char *     noisyConfigTimeStampKeyStrings[kNoisyConfigTimeStampKeyMax];
