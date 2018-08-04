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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "noisy-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"



const char *	TimeStampKeyStrings[kCommonTimeStampKeyMax] =
{
	/*
	 *	Generated from body of kNoisyTimeStampKey enum by piping through
	 *
	 *		grep 'kNoisy' | grep -v kNoisyTimeStampKeyMax| awk -F',' '{print "\t["$1"]\t\t\""$1"\","}'
	 */
	[kNoisyTimeStampKeyCheckCgiCompletion]		"	kNoisyTimeStampKeyCheckCgiCompletion",
	[kNoisyTimeStampKeyCheckRss]		"	kNoisyTimeStampKeyCheckRss",
	[kNoisyTimeStampKeyConsolePrintBuffers]		"	kNoisyTimeStampKeyConsolePrintBuffers",
	[kNoisyTimeStampKeyError]		"	kNoisyTimeStampKeyError",
	[kNoisyTimeStampKeyFatal]		"	kNoisyTimeStampKeyFatal",
	[kNoisyTimeStampKeyGenNoisyIrNode]		"	kNoisyTimeStampKeyGenNoisyIrNode",
	[kNoisyTimeStampKeyInFirst]		"	kNoisyTimeStampKeyInFirst",
	[kNoisyTimeStampKeyInFollow]		"	kNoisyTimeStampKeyInFollow",
	[kNoisyTimeStampKeyIrPassAstDotPrintWalk]		"	kNoisyTimeStampKeyIrPassAstDotPrintWalk",
	[kNoisyTimeStampKeyIrPassDotAstDotFmt]		"	kNoisyTimeStampKeyIrPassDotAstDotFmt",
	[kNoisyTimeStampKeyIrPassDotBackend]		"	kNoisyTimeStampKeyIrPassDotBackend",
	[kNoisyTimeStampKeyIrPassDotIsType]		"	kNoisyTimeStampKeyIrPassDotIsType",
	[kNoisyTimeStampKeyIrPassDotScope2Id2]		"	kNoisyTimeStampKeyIrPassDotScope2Id2",
	[kNoisyTimeStampKeyIrPassDotScope2Id]		"	kNoisyTimeStampKeyIrPassDotScope2Id",
	[kNoisyTimeStampKeyIrPassDotSymbol2Id]		"	kNoisyTimeStampKeyIrPassDotSymbol2Id",
	[kNoisyTimeStampKeyIrPassDotSymbotTableDotFmt]		"	kNoisyTimeStampKeyIrPassDotSymbotTableDotFmt",
	[kNoisyTimeStampKeyIrPassHelperColorIr]		"	kNoisyTimeStampKeyIrPassHelperColorIr",
	[kNoisyTimeStampKeyIrPassHelperColorSymbolTable]		"	kNoisyTimeStampKeyIrPassHelperColorSymbolTable",
	[kNoisyTimeStampKeyIrPassHelperIrSize]		"	kNoisyTimeStampKeyIrPassHelperIrSize",
	[kNoisyTimeStampKeyIrPassHelperSymbolTableSize]		"	kNoisyTimeStampKeyIrPassHelperSymbolTableSize",
	[kNoisyTimeStampKeyIrPassProtobufAstEmitter]		"	kNoisyTimeStampKeyIrPassProtobufAstEmitter",
	[kNoisyTimeStampKeyIrPassProtobufAstSerializeWalk]		"	kNoisyTimeStampKeyIrPassProtobufAstSerializeWalk",
	[kNoisyTimeStampKeyIrPassProtobufSymbolTableSerializeWalk]		"	kNoisyTimeStampKeyIrPassProtobufSymbolTableSerializeWalk",
	[kNoisyTimeStampKeyIrPassProtobufSymbotTableEmitter]		"	kNoisyTimeStampKeyIrPassProtobufSymbotTableEmitter",
	[kNoisyTimeStampKeyIrPassSymbolTableDotPrintWalk]		"	kNoisyTimeStampKeyIrPassSymbolTableDotPrintWalk",
	[kNoisyTimeStampKeyLexAllocateSourceInfo]		"	kNoisyTimeStampKeyLexAllocateSourceInfo",
	[kNoisyTimeStampKeyLexAllocateToken]		"	kNoisyTimeStampKeyLexAllocateToken",
	[kNoisyTimeStampKeyLexDebugPrintToken]		"	kNoisyTimeStampKeyLexDebugPrintToken",
	[kNoisyTimeStampKeyLexGet]		"	kNoisyTimeStampKeyLexGet",
	[kNoisyTimeStampKeyLexInit]		"	kNoisyTimeStampKeyLexInit",
	[kNoisyTimeStampKeyLexPeekPrint]		"	kNoisyTimeStampKeyLexPeekPrint",
	[kNoisyTimeStampKeyLexPeek]		"	kNoisyTimeStampKeyLexPeek",
	[kNoisyTimeStampKeyLexPrintToken]		"	kNoisyTimeStampKeyLexPrintToken",
	[kNoisyTimeStampKeyLexPut]		"	kNoisyTimeStampKeyLexPut",
	[kNoisyTimeStampKeyLexerCheckComment]		"	kNoisyTimeStampKeyLexerCheckComment",
	[kNoisyTimeStampKeyLexerCheckDot]		"	kNoisyTimeStampKeyLexerCheckDot",
	[kNoisyTimeStampKeyLexerCheckDoubleQuote]		"	kNoisyTimeStampKeyLexerCheckDoubleQuote",
	[kNoisyTimeStampKeyLexerCheckGt]		"	kNoisyTimeStampKeyLexerCheckGt",
	[kNoisyTimeStampKeyLexerCheckLt]		"	kNoisyTimeStampKeyLexerCheckLt",
	[kNoisyTimeStampKeyLexerCheckMinus]		"	kNoisyTimeStampKeyLexerCheckMinus",
	[kNoisyTimeStampKeyLexerCheckSingleQuote]		"	kNoisyTimeStampKeyLexerCheckSingleQuote",
	[kNoisyTimeStampKeyLexerCheckSingle]		"	kNoisyTimeStampKeyLexerCheckSingle",
	[kNoisyTimeStampKeyLexerCheckTokenLength]		"	kNoisyTimeStampKeyLexerCheckTokenLength",
	[kNoisyTimeStampKeyLexerCheckWeq3]		"	kNoisyTimeStampKeyLexerCheckWeq3",
	[kNoisyTimeStampKeyLexerCheckWeq]		"	kNoisyTimeStampKeyLexerCheckWeq",
	[kNoisyTimeStampKeyLexerCur]		"	kNoisyTimeStampKeyLexerCur",
	[kNoisyTimeStampKeyLexerDone]		"	kNoisyTimeStampKeyLexerDone",
	[kNoisyTimeStampKeyLexerEqf]		"	kNoisyTimeStampKeyLexerEqf",
	[kNoisyTimeStampKeyLexerFinishToken]		"	kNoisyTimeStampKeyLexerFinishToken",
	[kNoisyTimeStampKeyLexerGobble]		"	kNoisyTimeStampKeyLexerGobble",
	[kNoisyTimeStampKeyLexerIsDecimalSeparatedWithChar]		"	kNoisyTimeStampKeyLexerIsDecimalSeparatedWithChar",
	[kNoisyTimeStampKeyLexerIsDecimal]		"	kNoisyTimeStampKeyLexerIsDecimal",
	[kNoisyTimeStampKeyLexerIsEngineeringRealConst]		"	kNoisyTimeStampKeyLexerIsEngineeringRealConst",
	[kNoisyTimeStampKeyLexerIsOperatorOrSeparator]		"	kNoisyTimeStampKeyLexerIsOperatorOrSeparator",
	[kNoisyTimeStampKeyLexerIsRadixConst]		"	kNoisyTimeStampKeyLexerIsRadixConst",
	[kNoisyTimeStampKeyLexerIsRealConst]		"	kNoisyTimeStampKeyLexerIsRealConst",
	[kNoisyTimeStampKeyLexerMakeNumericConst]		"	kNoisyTimeStampKeyLexerMakeNumericConst",
	[kNoisyTimeStampKeyLexerStringAtLeft]		"	kNoisyTimeStampKeyLexerStringAtLeft",
	[kNoisyTimeStampKeyLexerStringAtRight]		"	kNoisyTimeStampKeyLexerStringAtRight",
	[kNoisyTimeStampKeyLexerStringToEngineeringRealConst]		"	kNoisyTimeStampKeyLexerStringToEngineeringRealConst",
	[kNoisyTimeStampKeyLexerStringToRadixConst]		"	kNoisyTimeStampKeyLexerStringToRadixConst",
	[kNoisyTimeStampKeyLexerStringToRealConst]		"	kNoisyTimeStampKeyLexerStringToRealConst",
	[kNoisyTimeStampKeyNoisyInit]		"	kNoisyTimeStampKeyNoisyInit",
	[kNoisyTimeStampKeyParseAccuracyTolerance]	"kNoisyTimeStampKeyParseAccuracyTolerance",
	[kNoisyTimeStampKeyParseAdtTypeDecl]		"	kNoisyTimeStampKeyParseAdtTypeDecl",
	[kNoisyTimeStampKeyParseAnonAggrCastExpr]		"	kNoisyTimeStampKeyParseAnonAggrCastExpr",
	[kNoisyTimeStampKeyParseAnonAggregateType]		"	kNoisyTimeStampKeyParseAnonAggregateType",
	[kNoisyTimeStampKeyParseArith2BoolOp]		"	kNoisyTimeStampKeyParseArith2BoolOp",
	[kNoisyTimeStampKeyParseArithConst]		"	kNoisyTimeStampKeyParseArithConst",
	[kNoisyTimeStampKeyParseArrayCastExpr]		"	kNoisyTimeStampKeyParseArrayCastExpr",
	[kNoisyTimeStampKeyParseArrayType]		"	kNoisyTimeStampKeyParseArrayType",
	[kNoisyTimeStampKeyParseAssignOp]		"	kNoisyTimeStampKeyParseAssignOp",
	[kNoisyTimeStampKeyParseAssignmentStatement]		"	kNoisyTimeStampKeyParseAssignmentStatement",
	[kNoisyTimeStampKeyParseBaseConst]		"	kNoisyTimeStampKeyParseBaseConst",
	[kNoisyTimeStampKeyParseBasicSignalDimension]		"	kNoisyTimeStampKeyParseBasicSignalDimension",
	[kNoisyTimeStampKeyParseBasicSignalUnits]		"	kNoisyTimeStampKeyParseBasicSignalUnits",
	[kNoisyTimeStampKeyParseBasicSignal]		"	kNoisyTimeStampKeyParseBasicSignal",
	[kNoisyTimeStampKeyParseBasicType]		"	kNoisyTimeStampKeyParseBasicType",
	[kNoisyTimeStampKeyParseChanEventExpr]		"	kNoisyTimeStampKeyParseChanEventExpr",
	[kNoisyTimeStampKeyParseCmpOp]		"	kNoisyTimeStampKeyParseCmpOp",
	[kNoisyTimeStampKeyParseComplexCastExpr]		"	kNoisyTimeStampKeyParseComplexCastExpr",
	[kNoisyTimeStampKeyParseComplexType]		"	kNoisyTimeStampKeyParseComplexType",
	[kNoisyTimeStampKeyParseConstSetExpr]		"	kNoisyTimeStampKeyParseConstSetExpr",
	[kNoisyTimeStampKeyParseConstantDecl]		"	kNoisyTimeStampKeyParseConstantDecl",
	[kNoisyTimeStampKeyParseDimensionArithExpr]		"	kNoisyTimeStampKeyParseDimensionArithExpr",
	[kNoisyTimeStampKeyParseDimensionArithFactor]		"	kNoisyTimeStampKeyParseDimensionArithFactor",
	[kNoisyTimeStampKeyParseDimensionArithTerm]	"kNoisyTimeStampKeyParseDimensionArithTerm",
	[kNoisyTimeStampKeyParseDimensionsDesignation]		"	kNoisyTimeStampKeyParseDimensionsDesignation",
	[kNoisyTimeStampKeyParseElement]		"	kNoisyTimeStampKeyParseElement",
	[kNoisyTimeStampKeyParseErrorMagnitudeTolerance]		"	kNoisyTimeStampKeyParseErrorMagnitudeTolerance",
	[kNoisyTimeStampKeyParseExpression]		"	kNoisyTimeStampKeyParseExpression",
	[kNoisyTimeStampKeyParseFactor]		"	kNoisyTimeStampKeyParseFactor",
	[kNoisyTimeStampKeyParseFieldSelect]		"	kNoisyTimeStampKeyParseFieldSelect",
	[kNoisyTimeStampKeyParseFixedType]		"	kNoisyTimeStampKeyParseFixedType",
	[kNoisyTimeStampKeyParseFunctionDecl]		"	kNoisyTimeStampKeyParseFunctionDecl",
	[kNoisyTimeStampKeyParseGuardedExpressionList]		"	kNoisyTimeStampKeyParseGuardedExpressionList",
	[kNoisyTimeStampKeyParseGuardedStatementList]		"	kNoisyTimeStampKeyParseGuardedStatementList",
	[kNoisyTimeStampKeyParseHighPrecedenceArith2ArithOp]		"	kNoisyTimeStampKeyParseHighPrecedenceArith2ArithOp",
	[kNoisyTimeStampKeyParseHighPrecedenceBinaryBoolOp]		"	kNoisyTimeStampKeyParseHighPrecedenceBinaryBoolOp",
	[kNoisyTimeStampKeyParseHighPrecedenceBinaryOp]		"	kNoisyTimeStampKeyParseHighPrecedenceBinaryOp",
	[kNoisyTimeStampKeyParseHighPrecedenceBoolSetOp]		"	kNoisyTimeStampKeyParseHighPrecedenceBoolSetOp",
	[kNoisyTimeStampKeyParseIdentifierDefinitionTerminal]		"	kNoisyTimeStampKeyParseIdentifierDefinitionTerminal",
	[kNoisyTimeStampKeyParseIdentifierList]		"	kNoisyTimeStampKeyParseIdentifierList",
	[kNoisyTimeStampKeyParseIdentifierOrNilList]		"	kNoisyTimeStampKeyParseIdentifierOrNilList",
	[kNoisyTimeStampKeyParseIdentifierOrNil]		"	kNoisyTimeStampKeyParseIdentifierOrNil",
	[kNoisyTimeStampKeyParseIdentifierUsageTerminal]		"	kNoisyTimeStampKeyParseIdentifierUsageTerminal",
	[kNoisyTimeStampKeyParseIdxInitList]		"	kNoisyTimeStampKeyParseIdxInitList",
	[kNoisyTimeStampKeyParseInitList]		"	kNoisyTimeStampKeyParseInitList",
	[kNoisyTimeStampKeyParseIntParamOrConst]		"	kNoisyTimeStampKeyParseIntParamOrConst",
	[kNoisyTimeStampKeyParseIntegerType]		"	kNoisyTimeStampKeyParseIntegerType",
	[kNoisyTimeStampKeyParseIterStatement]		"	kNoisyTimeStampKeyParseIterStatement",
	[kNoisyTimeStampKeyParseLatencyTolerance]		"	kNoisyTimeStampKeyParseLatencyTolerance",
	[kNoisyTimeStampKeyParseListCastExpr]		"	kNoisyTimeStampKeyParseListCastExpr",
	[kNoisyTimeStampKeyParseListType]		"	kNoisyTimeStampKeyParseListType",
	[kNoisyTimeStampKeyParseLoadExpr]		"	kNoisyTimeStampKeyParseLoadExpr",
	[kNoisyTimeStampKeyParseLossTolerance]		"	kNoisyTimeStampKeyParseLossTolerance",
	[kNoisyTimeStampKeyParseLowPrecedenceArith2ArithOp]		"	kNoisyTimeStampKeyParseLowPrecedenceArith2ArithOp",
	[kNoisyTimeStampKeyParseLowPrecedenceBinaryBoolOp]		"	kNoisyTimeStampKeyParseLowPrecedenceBinaryBoolOp",
	[kNoisyTimeStampKeyParseLowPrecedenceBinaryOp]		"	kNoisyTimeStampKeyParseLowPrecedenceBinaryOp",
	[kNoisyTimeStampKeyParseLowPrecedenceBoolSetOp]		"	kNoisyTimeStampKeyParseLowPrecedenceBoolSetOp",
	[kNoisyTimeStampKeyParseMatchStatement]		"	kNoisyTimeStampKeyParseMatchStatement",
	[kNoisyTimeStampKeyParseMaxOverExpr]	"kNoisyTimeStampKeyParseMaxOverExpr",
	[kNoisyTimeStampKeyParseMinOverExpr]	"kNoisyTimeStampKeyParseMinOverExpr",
	[kNoisyTimeStampKeyParseModuleDeclBody]		"	kNoisyTimeStampKeyParseModuleDeclBody",
	[kNoisyTimeStampKeyParseModuleDecl]		"	kNoisyTimeStampKeyParseModuleDecl",
	[kNoisyTimeStampKeyParseNamegenDeclaration]		"	kNoisyTimeStampKeyParseNamegenDeclaration",
	[kNoisyTimeStampKeyParseNamegenDefinition]		"	kNoisyTimeStampKeyParseNamegenDefinition",
	[kNoisyTimeStampKeyParseNamegenInvokeShorthand]		"	kNoisyTimeStampKeyParseNamegenInvokeShorthand",
	[kNoisyTimeStampKeyParseNumericConst]		"	kNoisyTimeStampKeyParseNumericConst",
	[kNoisyTimeStampKeyParseNumericType]		"	kNoisyTimeStampKeyParseNumericType",
	[kNoisyTimeStampKeyParseOperatorToleranceDecl]		"	kNoisyTimeStampKeyParseOperatorToleranceDecl",
	[kNoisyTimeStampKeyParseOrderingHead]		"	kNoisyTimeStampKeyParseOrderingHead",
	[kNoisyTimeStampKeyParseParallelStatement]		"	kNoisyTimeStampKeyParseParallelStatement",
	[kNoisyTimeStampKeyParsePredArithExpr]		"	kNoisyTimeStampKeyParsePredArithExpr",
	[kNoisyTimeStampKeyParsePredArithFactor]		"	kNoisyTimeStampKeyParsePredArithFactor",
	[kNoisyTimeStampKeyParsePredArithTerm]		"	kNoisyTimeStampKeyParsePredArithTerm",
	[kNoisyTimeStampKeyParsePredExpr]		"	kNoisyTimeStampKeyParsePredExpr",
	[kNoisyTimeStampKeyParsePredFactor]		"	kNoisyTimeStampKeyParsePredFactor",
	[kNoisyTimeStampKeyParsePredStmtList]		"	kNoisyTimeStampKeyParsePredStmtList",
	[kNoisyTimeStampKeyParsePredStmt]		"	kNoisyTimeStampKeyParsePredStmt",
	[kNoisyTimeStampKeyParsePredTerm]		"	kNoisyTimeStampKeyParsePredTerm",
	[kNoisyTimeStampKeyParsePredicateFnDecl]	"kNoisyTimeStampKeyParsePredicateFnDecl",
	[kNoisyTimeStampKeyParsePredicateFnDefn]		"	kNoisyTimeStampKeyParsePredicateFnDefn",
	[kNoisyTimeStampKeyParseProbdefDecl]		"	kNoisyTimeStampKeyParseProbdefDecl",
	[kNoisyTimeStampKeyParseProblemDefn]		"	kNoisyTimeStampKeyParseProblemDefn",
	[kNoisyTimeStampKeyParseProductOverExpr]		"	kNoisyTimeStampKeyParseProductOverExpr",
	[kNoisyTimeStampKeyParseProgram]		"	kNoisyTimeStampKeyParseProgram",
	[kNoisyTimeStampKeyParseModuleTypenameDecl]		"	kNoisyTimeStampKeyParseModuleTypenameDecl",
	[kNoisyTimeStampKeyParseQuantifiedBoolTerm]		"	kNoisyTimeStampKeyParseQuantifiedBoolTerm",
	[kNoisyTimeStampKeyParseQuantifierOp]		"	kNoisyTimeStampKeyParseQuantifierOp",
	[kNoisyTimeStampKeyParseQuantizeExpression]		"	kNoisyTimeStampKeyParseQuantizeExpression",
	[kNoisyTimeStampKeyParseRationalCastExpr]		"	kNoisyTimeStampKeyParseRationalCastExpr",
	[kNoisyTimeStampKeyParseRationalType]		"	kNoisyTimeStampKeyParseRationalType",
	[kNoisyTimeStampKeyParseReadTypeSignature]		"	kNoisyTimeStampKeyParseReadTypeSignature",
	[kNoisyTimeStampKeyParseRealParamOrConst]		"	kNoisyTimeStampKeyParseRealParamOrConst",
	[kNoisyTimeStampKeyParseRealType]		"	kNoisyTimeStampKeyParseRealType",
	[kNoisyTimeStampKeyParseReturnSignature]		"	kNoisyTimeStampKeyParseReturnSignature",
	[kNoisyTimeStampKeyParseReturnStatement]		"	kNoisyTimeStampKeyParseReturnStatement",
	[kNoisyTimeStampKeyParseSampleExpression]		"	kNoisyTimeStampKeyParseSampleExpression",
	[kNoisyTimeStampKeyParseScopedPredStmtList]		"	kNoisyTimeStampKeyParseScopedPredStmtList",
	[kNoisyTimeStampKeyParseScopedStatementList]		"	kNoisyTimeStampKeyParseScopedStatementList",
	[kNoisyTimeStampKeyParseSequenceStatement]		"	kNoisyTimeStampKeyParseSequenceStatement",
	[kNoisyTimeStampKeyParseSetCastExpr]		"	kNoisyTimeStampKeyParseSetCastExpr",
	[kNoisyTimeStampKeyParseSetCmpOp]		"	kNoisyTimeStampKeyParseSetCmpOp",
	[kNoisyTimeStampKeyParseSetCmpTerm]		"	kNoisyTimeStampKeyParseSetCmpTerm",
	[kNoisyTimeStampKeyParseSetExpr]		"	kNoisyTimeStampKeyParseSetExpr",
	[kNoisyTimeStampKeyParseSetFactor]		"	kNoisyTimeStampKeyParseSetFactor",
	[kNoisyTimeStampKeyParseSetHead]		"	kNoisyTimeStampKeyParseSetHead",
	[kNoisyTimeStampKeyParseSetTerm]		"	kNoisyTimeStampKeyParseSetTerm",
	[kNoisyTimeStampKeyParseSetType]		"	kNoisyTimeStampKeyParseSetType",
	[kNoisyTimeStampKeyParseSigfigDesignation]		"	kNoisyTimeStampKeyParseSigfigDesignation",
	[kNoisyTimeStampKeyParseSignalDesignation]		"	kNoisyTimeStampKeyParseSignalDesignation",
	[kNoisyTimeStampKeyParseSignature]		"	kNoisyTimeStampKeyParseSignature",
	[kNoisyTimeStampKeyParseStarInitList]		"	kNoisyTimeStampKeyParseStarInitList",
	[kNoisyTimeStampKeyParseStatementList]		"	kNoisyTimeStampKeyParseStatementList",
	[kNoisyTimeStampKeyParseStatement]		"	kNoisyTimeStampKeyParseStatement",
	[kNoisyTimeStampKeyParseStringParamOrConst]		"	kNoisyTimeStampKeyParseStringParamOrConst",
	[kNoisyTimeStampKeyParseSumOverExpr]		"	kNoisyTimeStampKeyParseSumOverExpr",
	[kNoisyTimeStampKeyParseSumProdMinMaxBody]		"	kNoisyTimeStampKeyParseSumProdMinMaxBody",
	[kNoisyTimeStampKeyParseTerm]		"	kNoisyTimeStampKeyParseTerm",
	[kNoisyTimeStampKeyParseTerminal]		"	kNoisyTimeStampKeyParseTerminal",
	[kNoisyTimeStampKeyParseTimeseriesDesignation]		"	kNoisyTimeStampKeyParseTimeseriesDesignation",
	[kNoisyTimeStampKeyParseTolerance]		"	kNoisyTimeStampKeyParseTolerance",
	[kNoisyTimeStampKeyParseTupleType]		"	kNoisyTimeStampKeyParseTupleType",
	[kNoisyTimeStampKeyParseTupleValue]		"	kNoisyTimeStampKeyParseTupleValue",
	[kNoisyTimeStampKeyParseTuple]		"	kNoisyTimeStampKeyParseTuple",
	[kNoisyTimeStampKeyParseTypeAnnoteDecl]		"	kNoisyTimeStampKeyParseTypeAnnoteDecl",
	[kNoisyTimeStampKeyParseTypeAnnoteItem]		"	kNoisyTimeStampKeyParseTypeAnnoteItem",
	[kNoisyTimeStampKeyParseTypeAnnoteList]		"	kNoisyTimeStampKeyParseTypeAnnoteList",
	[kNoisyTimeStampKeyParseTypeDecl]		"	kNoisyTimeStampKeyParseTypeDecl",
	[kNoisyTimeStampKeyParseTypeExpr]		"	kNoisyTimeStampKeyParseTypeExpr",
	[kNoisyTimeStampKeyParseTypeMaxExpr]		"	kNoisyTimeStampKeyParseTypeMaxExpr",
	[kNoisyTimeStampKeyParseTypeMinExpr]		"	kNoisyTimeStampKeyParseTypeMinExpr",
	[kNoisyTimeStampKeyParseTypeName]		"	kNoisyTimeStampKeyParseTypeName",
	[kNoisyTimeStampKeyParseTypeParameterList]		"	kNoisyTimeStampKeyParseTypeParameterList",
	[kNoisyTimeStampKeyParseUnaryBoolOp]		"	kNoisyTimeStampKeyParseUnaryBoolOp",
	[kNoisyTimeStampKeyParseUnaryOp]		"	kNoisyTimeStampKeyParseUnaryOp",
	[kNoisyTimeStampKeyParseUnarySetOp]		"	kNoisyTimeStampKeyParseUnarySetOp",
	[kNoisyTimeStampKeyParseUnitsArithExpr]		"	kNoisyTimeStampKeyParseUnitsArithExpr",
	[kNoisyTimeStampKeyParseUnitsArithFactor]		"	kNoisyTimeStampKeyParseUnitsArithFactor",
	[kNoisyTimeStampKeyParseUnitsArithTerm]		"	kNoisyTimeStampKeyParseUnitsArithTerm",
	[kNoisyTimeStampKeyParseUnitsDesignation]		"	kNoisyTimeStampKeyParseUnitsDesignation",
	[kNoisyTimeStampKeyParseValfnSignature]		"	kNoisyTimeStampKeyParseValfnSignature",
	[kNoisyTimeStampKeyParseVarIntroList]		"	kNoisyTimeStampKeyParseVarIntroList",
	[kNoisyTimeStampKeyParseVarIntro]		"	kNoisyTimeStampKeyParseVarIntro",
	[kNoisyTimeStampKeyParseVarTuple]		"	kNoisyTimeStampKeyParseVarTuple",
	[kNoisyTimeStampKeyParseVectorType]		"	kNoisyTimeStampKeyParseVectorType",
	[kNoisyTimeStampKeyParseWriteTypeSignature]		"	kNoisyTimeStampKeyParseWriteTypeSignature",
	[kNoisyTimeStampKeyParse]		"	kNoisyTimeStampKeyParse",
	[kNoisyTimeStampKeyParserAddLeafWithChainingSeq]		"	kNoisyTimeStampKeyParserAddLeafWithChainingSeq",
	[kNoisyTimeStampKeyParserAddLeaf]		"	kNoisyTimeStampKeyParserAddLeaf",
	[kNoisyTimeStampKeyParserAddToModuleScopes]		"	kNoisyTimeStampKeyParserAddToModuleScopes",
	[kNoisyTimeStampKeyParserAssignTypes]		"	kNoisyTimeStampKeyParserAssignTypes",
	[kNoisyTimeStampKeyParserDepthFirstWalk]		"	kNoisyTimeStampKeyParserDepthFirstWalk",
	[kNoisyTimeStampKeyParserErrorMultiDefinition]		"	kNoisyTimeStampKeyParserErrorMultiDefinition",
	[kNoisyTimeStampKeyParserErrorRecovery]		"	kNoisyTimeStampKeyParserErrorRecovery",
	[kNoisyTimeStampKeyParserErrorUseBeforeDefinition]		"	kNoisyTimeStampKeyParserErrorUseBeforeDefinition",
	[kNoisyTimeStampKeyParserModuleName2scope]		"	kNoisyTimeStampKeyParserModuleName2scope",
	[kNoisyTimeStampKeyParserPeekCheck]		"	kNoisyTimeStampKeyParserPeekCheck",
	[kNoisyTimeStampKeyParserSemanticError]		"	kNoisyTimeStampKeyParserSemanticError",
	[kNoisyTimeStampKeyParserSyntaxError]		"	kNoisyTimeStampKeyParserSyntaxError",
	[kNoisyTimeStampKeyPrintToFile]		"	kNoisyTimeStampKeyPrintToFile",
	[kNoisyTimeStampKeyRenderDotInFile]		"	kNoisyTimeStampKeyRenderDotInFile",
	[kNoisyTimeStampKeyRunPasses]		"	kNoisyTimeStampKeyRunPasses",
	[kNoisyTimeStampKeySymbolTableAddOrLookupSymbolForToken]		"	kNoisyTimeStampKeySymbolTableAddOrLookupSymbolForToken",
	[kNoisyTimeStampKeySymbolTableAllocScope]		"	kNoisyTimeStampKeySymbolTableAllocScope",
	[kNoisyTimeStampKeySymbolTableCloseScope]		"	kNoisyTimeStampKeySymbolTableCloseScope",
	[kNoisyTimeStampKeySymbolTableOpenScope]		"	kNoisyTimeStampKeySymbolTableOpenScope",
	[kNoisyTimeStampKeySymbolTableSymbolForIdentifier]		"	kNoisyTimeStampKeySymbolTableSymbolForIdentifier",
	[kNoisyTimeStampKeyTypeEqualsSubtreeTypes]		"	kNoisyTimeStampKeyTypeEqualsSubtreeTypes",
	[kNoisyTimeStampKeyTypeMakeTypeSignature]		"	kNoisyTimeStampKeyTypeMakeTypeSignature",
	[kNoisyTimeStampKeyTypeValidateIrSubtree]		"	kNoisyTimeStampKeyTypeValidateIrSubtree",
	[kNoisyTimeStampKeyUnknown]		"	kNoisyTimeStampKeyUnknown",
};
