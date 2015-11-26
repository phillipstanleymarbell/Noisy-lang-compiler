/*
	Authored 2015. Phillip Stanley-Marbell.

	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	*	Redistributions of source code must retain the above
		copyright notice, this list of conditions and the following
		disclaimer.

	*	Redistributions in binary form must reproduce the above
		copyright notice, this list of conditions and the following
		disclaimer in the documentation and/or other materials
		provided with the distribution.

	*	Neither the name of the author nor the names of its
		contributors may be used to endorse or promote products
		derived from this software without specific prior written
		permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/



/*
 *	NOTE:   Add entries here for new locations we want to time stamp; all
 *	functions in the implementation should ideally have entries here.
 */
typedef enum
{
	/*
	 *	Parser global routines in noisy-parser.c.
	 */
	kNoisyTimeStampKeyParse,
	kNoisyTimeStampKeyParseProgram,
	kNoisyTimeStampKeyParseProgtypeDeclaration,
	kNoisyTimeStampKeyParseProgtypeBody,
	kNoisyTimeStampKeyParseProgtypeTypenameDeclaration,
	kNoisyTimeStampKeyParseConstantDeclaration,
	kNoisyTimeStampKeyParseTypeDeclaration,
	kNoisyTimeStampKeyParseAdtTypeDeclaration,
	kNoisyTimeStampKeyParseNamegenDeclaration,
	kNoisyTimeStampKeyParseIdentifierOrNil,
	kNoisyTimeStampKeyParseIdentifierOrNilList,
	kNoisyTimeStampKeyParseIdentifierList,
	kNoisyTimeStampKeyParseTypeExpression,
	kNoisyTimeStampKeyParseTypeName,
	kNoisyTimeStampKeyParseTolerance,
	kNoisyTimeStampKeyParseErrorMagnitudeTolerance,
	kNoisyTimeStampKeyParseLossTolerance,
	kNoisyTimeStampKeyParseLatencyTolerance,
	kNoisyTimeStampKeyParseBasicType,
	kNoisyTimeStampKeyParseRealType,
	kNoisyTimeStampKeyParseFixedType,
	kNoisyTimeStampKeyParseAnonAggregateType,
	kNoisyTimeStampKeyParseArrayType,
	kNoisyTimeStampKeyParseListType,
	kNoisyTimeStampKeyParseTupleType,
	kNoisyTimeStampKeyParseSetType,
	kNoisyTimeStampKeyParseInitList,
	kNoisyTimeStampKeyParseIdxInitList,
	kNoisyTimeStampKeyParseStarInitList,
	kNoisyTimeStampKeyParseElement,
	kNoisyTimeStampKeyParseNamegenDefinition,
	kNoisyTimeStampKeyParseScopedStatementList,
	kNoisyTimeStampKeyParseStatementList,
	kNoisyTimeStampKeyParseStatement,
	kNoisyTimeStampKeyParseAssignOp,
	kNoisyTimeStampKeyParseMatchStatement,
	kNoisyTimeStampKeyParseIterStatement,
	kNoisyTimeStampKeyParseGuardBody,
	kNoisyTimeStampKeyParseExpression,
	kNoisyTimeStampKeyParseListCastExpression,
	kNoisyTimeStampKeyParseSetCastExpression,
	kNoisyTimeStampKeyParseArrayCastExpression,
	kNoisyTimeStampKeyParseAnonAggregateCastExpression,
	kNoisyTimeStampKeyParseChanEventExpression,
	kNoisyTimeStampKeyParseChan2nameExpression,
	kNoisyTimeStampKeyParseVar2nameExpression,
	kNoisyTimeStampKeyParseName2chanExpression,
	kNoisyTimeStampKeyParseTerm,
	kNoisyTimeStampKeyParseFactor,
	kNoisyTimeStampKeyParseFieldSelect,
	kNoisyTimeStampKeyParseHighPrecedenceBinaryO,
	kNoisyTimeStampKeyParseLowPrecedenceBinaryOp,
	kNoisyTimeStampKeyParseCmpOp,
	kNoisyTimeStampKeyParseBooleanOp,
	kNoisyTimeStampKeyParseUnaryOp,
	kNoisyTimeStampKeyParseTerminal,
	kNoisyTimeStampKeyParseIdentifierUsageTerminal,
	kNoisyTimeStampKeyParseIdentifierDefinitionTerminal,

	/*
	 *	Parser local routines in noisy-parser.c.
	 */
	kNoisyTimeStampKeyParserSyntaxError,
	kNoisyTimeStampKeyParserSemanticError,
	kNoisyTimeStampKeyParserErrorRecovery,
	kNoisyTimeStampKeyParserProgtypeName2scope,
	kNoisyTimeStampKeyParserErrorUseBeforeDefinition,
	kNoisyTimeStampKeyParserErrorMultiDefinition,
	kNoisyTimeStampKeyParserTermSyntaxError,
	kNoisyTimeStampKeyParserTermErrorRecovery,
	kNoisyTimeStampKeyParserPeekCheck,
	kNoisyTimeStampKeyLexPeekPrint,
	kNoisyTimeStampKeyParserDepthFirstWalk,
	kNoisyTimeStampKeyParserAddLeaf,
	kNoisyTimeStampKeyParserAddLeafWithChainingSeq,
	kNoisyTimeStampKeyParserAddToProgtypeScopes,
	kNoisyTimeStampKeyParserAssignTypes,


	/*
	 *	Symbol table routines in noisy-symbolTable.c.
	 */
	kNoisyTimeStampKeySymbolTableAllocScope,
	kNoisyTimeStampKeySymbolTableAddOrLookupSymbolForToken,
	kNoisyTimeStampKeySymbolTableSymbolForIdentifier,
	kNoisyTimeStampKeySymbolTableOpenScope,
	kNoisyTimeStampKeySymbolTableCloseScope,


	/*
	 *	Ir helper routines in noisy-irHelpers.c.
	 */
	kNoisyTimeStampKeyGenNoisyIrNode,


	/*
	 *	Lexer public routines in noisy-lexer.c.
	 */
	kNoisyTimeStampKeyLexAllocateSourceInfo,
	kNoisyTimeStampKeyLexAllocateToken,
	kNoisyTimeStampKeyLexPut,
	kNoisyTimeStampKeyLexGet,
	kNoisyTimeStampKeyLexPeek,
	kNoisyTimeStampKeyLexInit,
	kNoisyTimeStampKeyLexPrintToken,
	kNoisyTimeStampKeyLexDebugPrintToken,

	/*
	 *	Lexer local routines in noisy-lexer.c.
	 */
	kNoisyTimeStampKeyLexerCheckTokenLength,
	kNoisyTimeStampKeyLexerCur,
	kNoisyTimeStampKeyLexerGobble,
	kNoisyTimeStampKeyLexerDone,
	kNoisyTimeStampKeyLexerEqf,
	kNoisyTimeStampKeyLexerCheckComment,
	kNoisyTimeStampKeyLexerCheckWeq,
	kNoisyTimeStampKeyLexerCheckWeq3,
	kNoisyTimeStampKeyLexerCheckSingle,
	kNoisyTimeStampKeyLexerCheckDot,
	kNoisyTimeStampKeyLexerCheckGt,
	kNoisyTimeStampKeyLexerCheckLt,
	kNoisyTimeStampKeyLexerCheckSingleQuote,
	kNoisyTimeStampKeyLexerCheckDoubleQuote,
	kNoisyTimeStampKeyLexerCheckMinus,
	kNoisyTimeStampKeyLexerFinishToken,
	kNoisyTimeStampKeyLexerMakeNumericConst,
	kNoisyTimeStampKeyLexerIsDecimal,
	kNoisyTimeStampKeyLexerStringAtLeft,
	kNoisyTimeStampKeyLexerStringAtRight,
	kNoisyTimeStampKeyLexerIsDecimalSeparatedWithChar,
	kNoisyTimeStampKeyLexerIsRadixConst,
	kNoisyTimeStampKeyLexerIsRealConst,
	kNoisyTimeStampKeyLexerIsEngineeringRealConst,
	kNoisyTimeStampKeyLexerStringToRadixConst,
	kNoisyTimeStampKeyLexerStringToRealConst,
	kNoisyTimeStampKeyLexerStringToEngineeringRealConst,
	kNoisyTimeStampKeyLexerIsOperatorOrSeparator,


	/*
	 *	First/Follow set routines in noisy-firstAndFollow.c.
	 */
	kNoisyTimeStampKeyInFirst,
	kNoisyTimeStampKeyInFollow,


	/*
	 *	Type-related routines in noisy-types.c
	 */
	kNoisyTimeStampKeyTypeValidateIrSubtree,
	kNoisyTimeStampKeyTypeEqualsSubtreeTypes,
	kNoisyTimeStampKeyTypeMakeTypeSignature,


	/*
	 *	Miscellaneous platform and glue routines in noisy.c.
	 */
	kNoisyTimeStampKeyNoisyInit,
	kNoisyTimeStampKeyRunPasses,
	kNoisyTimeStampKeyCheckRss,
	kNoisyTimeStampKeyConsolePrintBuffers,
	kNoisyTimeStampKeyPrintToFile,
	kNoisyTimeStampKeyRenderDotInFile,
	kNoisyTimeStampKeyCheckCgiCompletion,
	kNoisyTimeStampKeyFatal,
	kNoisyTimeStampKeyError,


	/*
	 *	Public routines in Dotbackend
	 */
	kNoisyTimeStampKeyIrPassDotAstDotFmt,
	kNoisyTimeStampKeyIrPassDotSymbotTableDotFmt,
	kNoisyTimeStampKeyIrPassAstDotPrintWalk,
	kNoisyTimeStampKeyIrPassSymbolTableDotPrintWalk,
	kNoisyTimeStampKeyIrPassDotBackend,

	/*
	 *	Private routines in Dotbackend
	 */
	kNoisyTimeStampKeyIrPassDotIsType,
	kNoisyTimeStampKeyIrPassDotScope2Id,
	kNoisyTimeStampKeyIrPassDotScope2Id2,
	kNoisyTimeStampKeyIrPassDotSymbol2Id,

	/*
	 *	IR pass helpers. TODO: should merge this into IR helpers...
	 */
	kNoisyTimeStampKeyIrPassHelperColorTree,
	kNoisyTimeStampKeyIrPassHelperTreeSize,

	/*
	 *	Used to tag un-tracked time.
	 */
	kNoisyTimeStampKeyUnknown,


	/*
	 *	Code depends on this being last.
	 */
	kNoisyTimeStampKeyMax,
} NoisyTimeStampKey;

typedef struct
{
	uint64_t		nanoseconds;
	NoisyTimeStampKey	key;
} NoisyTimeStamp;


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
#define NoisyTimeStampTraceMacro(routineKey)		if (N->mode & kNoisyModeCallStatistics)\
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
#define NoisyTimeStampTraceMacro(routineKey) 
#endif /* #ifdef NoisyOsMacOSX */

extern const char *     NoisyTimeStampKeyStrings[kNoisyTimeStampKeyMax];
