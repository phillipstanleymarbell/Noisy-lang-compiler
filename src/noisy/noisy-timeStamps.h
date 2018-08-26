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
	kNoisyTimeStampKeyParseAccuracyTolerance,
	kNoisyTimeStampKeyParseAdtTypeDecl,
	kNoisyTimeStampKeyParseAnonAggrCastExpr,
	kNoisyTimeStampKeyParseAnonAggregateType,
	kNoisyTimeStampKeyParseArith2BoolOp,
	kNoisyTimeStampKeyParseArithConst,
	kNoisyTimeStampKeyParseArrayCastExpr,
	kNoisyTimeStampKeyParseArrayType,
	kNoisyTimeStampKeyParseAssignOp,
	kNoisyTimeStampKeyParseAssignmentStatement,
	kNoisyTimeStampKeyParseBaseConst,
	kNoisyTimeStampKeyParseBasicSignal,
	kNoisyTimeStampKeyParseBasicSignalDimension,
	kNoisyTimeStampKeyParseBasicSignalUnits,
	kNoisyTimeStampKeyParseBasicType,
	kNoisyTimeStampKeyParseChanEventExpr,
	kNoisyTimeStampKeyParseCmpOp,
	kNoisyTimeStampKeyParseComplexCastExpr,
	kNoisyTimeStampKeyParseComplexType,
	kNoisyTimeStampKeyParseConstSetExpr,
	kNoisyTimeStampKeyParseConstantDecl,
	kNoisyTimeStampKeyParseDimensionArithExpr,
	kNoisyTimeStampKeyParseDimensionArithFactor,
	kNoisyTimeStampKeyParseDimensionArithTerm,
	kNoisyTimeStampKeyParseDimensionsDesignation,
	kNoisyTimeStampKeyParseElement,
	kNoisyTimeStampKeyParseErrorMagnitudeTolerance,
	kNoisyTimeStampKeyParseExpression,
	kNoisyTimeStampKeyParseFactor,
	kNoisyTimeStampKeyParseFieldSelect,
	kNoisyTimeStampKeyParseFixedType,
	kNoisyTimeStampKeyParseFunctionDecl,
	kNoisyTimeStampKeyParseGuardedExpressionList,
	kNoisyTimeStampKeyParseGuardedStatementList,
	kNoisyTimeStampKeyParseHighPrecedenceArith2ArithOp,
	kNoisyTimeStampKeyParseHighPrecedenceBinaryBoolOp,
	kNoisyTimeStampKeyParseHighPrecedenceBinaryOp,
	kNoisyTimeStampKeyParseHighPrecedenceBoolSetOp,
	kNoisyTimeStampKeyParseIdentifierDefinitionTerminal,
	kNoisyTimeStampKeyParseIdentifierList,
	kNoisyTimeStampKeyParseIdentifierOrNil,
	kNoisyTimeStampKeyParseIdentifierOrNilList,
	kNoisyTimeStampKeyParseIdentifierUsageTerminal,
	kNoisyTimeStampKeyParseIdxInitList,
	kNoisyTimeStampKeyParseInitList,
	kNoisyTimeStampKeyParseIntParamOrConst,
	kNoisyTimeStampKeyParseIntegerType,
	kNoisyTimeStampKeyParseIterStatement,
	kNoisyTimeStampKeyParseLatencyTolerance,
	kNoisyTimeStampKeyParseListCastExpr,
	kNoisyTimeStampKeyParseListType,
	kNoisyTimeStampKeyParseLoadExpr,
	kNoisyTimeStampKeyParseLossTolerance,
	kNoisyTimeStampKeyParseLowPrecedenceArith2ArithOp,
	kNoisyTimeStampKeyParseLowPrecedenceBinaryBoolOp,
	kNoisyTimeStampKeyParseLowPrecedenceBinaryOp,
	kNoisyTimeStampKeyParseLowPrecedenceBoolSetOp,
	kNoisyTimeStampKeyParseMatchStatement,
	kNoisyTimeStampKeyParseMaxForExpr,
	kNoisyTimeStampKeyParseMinForExpr,
	kNoisyTimeStampKeyParseModuleDecl,
	kNoisyTimeStampKeyParseModuleDeclBody,
	kNoisyTimeStampKeyParseModuleTypenameDecl,
	kNoisyTimeStampKeyParseNamegenDeclaration,
	kNoisyTimeStampKeyParseNamegenDefinition,
	kNoisyTimeStampKeyParseNamegenInvokeShorthand,
	kNoisyTimeStampKeyParseNumericConst,
	kNoisyTimeStampKeyParseNumericType,
	kNoisyTimeStampKeyParseOperatorToleranceDecl,
	kNoisyTimeStampKeyParseOrderingHead,
	kNoisyTimeStampKeyParseParallelStatement,
	kNoisyTimeStampKeyParsePredArithExpr,
	kNoisyTimeStampKeyParsePredArithFactor,
	kNoisyTimeStampKeyParsePredArithTerm,
	kNoisyTimeStampKeyParsePredExpr,
	kNoisyTimeStampKeyParsePredFactor,
	kNoisyTimeStampKeyParsePredStmt,
	kNoisyTimeStampKeyParsePredStmtList,
	kNoisyTimeStampKeyParsePredTerm,
	kNoisyTimeStampKeyParsePredicateFnDecl,
	kNoisyTimeStampKeyParsePredicateFnDefn,
	kNoisyTimeStampKeyParseProbdefDecl,
	kNoisyTimeStampKeyParseProblemDefn,
	kNoisyTimeStampKeyParseProductForExpr,
	kNoisyTimeStampKeyParseProgram,
	kNoisyTimeStampKeyParseQuantifiedBoolTerm,
	kNoisyTimeStampKeyParseQuantifierOp,
	kNoisyTimeStampKeyParseQuantizeExpression,
	kNoisyTimeStampKeyParseRationalCastExpr,
	kNoisyTimeStampKeyParseRationalType,
	kNoisyTimeStampKeyParseReadTypeSignature,
	kNoisyTimeStampKeyParseRealParamOrConst,
	kNoisyTimeStampKeyParseRealType,
	kNoisyTimeStampKeyParseReturnSignature,
	kNoisyTimeStampKeyParseReturnStatement,
	kNoisyTimeStampKeyParseSampleExpression,
	kNoisyTimeStampKeyParseScopedPredStmtList,
	kNoisyTimeStampKeyParseScopedStatementList,
	kNoisyTimeStampKeyParseSequenceStatement,
	kNoisyTimeStampKeyParseSetCastExpr,
	kNoisyTimeStampKeyParseSetCmpOp,
	kNoisyTimeStampKeyParseSetCmpTerm,
	kNoisyTimeStampKeyParseSetExpr,
	kNoisyTimeStampKeyParseSetFactor,
	kNoisyTimeStampKeyParseSetHead,
	kNoisyTimeStampKeyParseSetTerm,
	kNoisyTimeStampKeyParseSetType,
	kNoisyTimeStampKeyParseSigfigDesignation,
	kNoisyTimeStampKeyParseSignalDesignation,
	kNoisyTimeStampKeyParseSignature,
	kNoisyTimeStampKeyParseStarInitList,
	kNoisyTimeStampKeyParseStatement,
	kNoisyTimeStampKeyParseStatementList,
	kNoisyTimeStampKeyParseStringParamOrConst,
	kNoisyTimeStampKeyParseSumForExpr,
	kNoisyTimeStampKeyParseSumProdMinMaxBody,
	kNoisyTimeStampKeyParseTerm,
	kNoisyTimeStampKeyParseTerminal,
	kNoisyTimeStampKeyParseTimeseriesDesignation,
	kNoisyTimeStampKeyParseTolerance,
	kNoisyTimeStampKeyParseTuple,
	kNoisyTimeStampKeyParseTupleType,
	kNoisyTimeStampKeyParseTupleValue,
	kNoisyTimeStampKeyParseTypeAnnoteDecl,
	kNoisyTimeStampKeyParseTypeAnnoteItem,
	kNoisyTimeStampKeyParseTypeAnnoteList,
	kNoisyTimeStampKeyParseTypeDecl,
	kNoisyTimeStampKeyParseTypeExpr,
	kNoisyTimeStampKeyParseTypeMaxExpr,
	kNoisyTimeStampKeyParseTypeMinExpr,
	kNoisyTimeStampKeyParseTypeName,
	kNoisyTimeStampKeyParseTypeParameterList,
	kNoisyTimeStampKeyParseUnaryBoolOp,
	kNoisyTimeStampKeyParseUnaryOp,
	kNoisyTimeStampKeyParseUnarySetOp,
	kNoisyTimeStampKeyParseUnitsArithExpr,
	kNoisyTimeStampKeyParseUnitsArithFactor,
	kNoisyTimeStampKeyParseUnitsArithTerm,
	kNoisyTimeStampKeyParseUnitsDesignation,
	kNoisyTimeStampKeyParseValfnSignature,
	kNoisyTimeStampKeyParseVarIntro,
	kNoisyTimeStampKeyParseVarIntroList,
	kNoisyTimeStampKeyParseVarTuple,
	kNoisyTimeStampKeyParseVectorType,
	kNoisyTimeStampKeyParseWriteTypeSignature,
	kNoisyTimeStampKeyParsenQualifiedIdentifier,


	/*
	 *	Parser local routines in noisy-parser.c.
	 */
	kNoisyTimeStampKeyParserSyntaxError,
	kNoisyTimeStampKeyParserSemanticError,
	kNoisyTimeStampKeyParserErrorRecovery,
	kNoisyTimeStampKeyParserModuleName2scope,
	kNoisyTimeStampKeyParserErrorUseBeforeDefinition,
	kNoisyTimeStampKeyParserErrorMultiDefinition,
	kNoisyTimeStampKeyParserPeekCheck,
	kNoisyTimeStampKeyLexPeekPrint,
	kNoisyTimeStampKeyParserDepthFirstWalk,
	kNoisyTimeStampKeyParserAddLeaf,
	kNoisyTimeStampKeyParserAddLeafWithChainingSeq,
	kNoisyTimeStampKeyParserAddToModuleScopes,
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
	 *	Public routines Protobuf backend
	 */
	kNoisyTimeStampKeyIrPassProtobufSymbotTableEmitter,
	kNoisyTimeStampKeyIrPassProtobufAstEmitter,
	kNoisyTimeStampKeyIrPassProtobufAstSerializeWalk,
	kNoisyTimeStampKeyIrPassProtobufSymbolTableSerializeWalk,


	/*
	 *	IR pass helpers. See issue #296.
	 */
	kNoisyTimeStampKeyIrPassHelperColorIr,
	kNoisyTimeStampKeyIrPassHelperColorSymbolTable,
	kNoisyTimeStampKeyIrPassHelperIrSize,
	kNoisyTimeStampKeyIrPassHelperSymbolTableSize,

	/*
	 *	Used to tag un-tracked time.
	 */
	kNoisyTimeStampKeyUnknown,


	/*
	 *	Code depends on this being last. We eschew using Noisy/Newton prefix as common-timeStamps.h assumes either Noisy or Newton header will be in effect
	 */
	kCommonTimeStampKeyMax,
} TimeStampKey;
