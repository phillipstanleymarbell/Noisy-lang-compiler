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
 *	LibFlex's FlexList and FlexTuple etc. require user to tag type with their own enums.
 */
typedef enum
{
	kNoisyFlexListTypeString,
	kNoisyFlexListTypeNoisyScopePointer,
} NoisyFlexListType;



#define noisyValidFlexTupleCheckMacro(x)	(((x)->siblings != NULL) && ((x)->siblings->hd != NULL))



void		noisyParserSemanticError(State *  N, IrNodeType currentlyParsingProduction, char *  details);
void		noisyParserSyntaxError(State *  N, IrNodeType currentlyParsingTokenOrProduction, IrNodeType expectedProductionOrToken, int firstOrFollowsArray[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax]);
void		noisyParserErrorRecovery(State *  N, IrNodeType expectedProductionOrToken) __attribute__((noreturn));

IrNode *	noisyParse(State *  N, Scope *  currentScope);
IrNode *	noisyParseAccuracyTolerance(State *  N);
IrNode *	noisyParseAdtTypeDecl(State *  N, Scope *  scope);
IrNode *	noisyParseAnonAggrCastExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseAnonAggregateType(State *  N, Scope *  currentScope);
IrNode *	noisyParseArith2BoolOp(State *  N, Scope *  currentScope);
IrNode *	noisyParseArithConst(State *  N, Scope *  currentScope);
IrNode *	noisyParseArrayCastExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseArrayType(State *  N, Scope *  currentScope);
IrNode *	noisyParseAssignOp(State *  N);
IrNode *	noisyParseAssignmentStatement(State *  N, Scope *  currentScope);
IrNode *	noisyParseBaseConst(State *  N, Scope *  currentScope);
IrNode *	noisyParseBasicSignal(State *  N, Scope *  currentScope);
IrNode *	noisyParseBasicSignalDimension(State *  N, Scope *  currentScope);
IrNode *	noisyParseBasicSignalUnits(State *  N, Scope *  currentScope);
IrNode *	noisyParseBasicType(State *  N, Scope *  currentScope);
IrNode *	noisyParseChanEventExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseCmpOp(State *  N);
IrNode *	noisyParseComplexCastExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseComplexType(State *  N, Scope *  currentScope);
IrNode *	noisyParseConstSetExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseConstantDecl(State *  N, Scope *  scope);
IrNode *	noisyParseDimensionArithExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseDimensionArithFactor(State *  N, Scope *  currentScope);
IrNode *	noisyParseDimensionsDesignation(State *  N, Scope *  currentScope);
IrNode *	noisyParseElement(State *  N, Scope *  scope);
IrNode *	noisyParseExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseFactor(State *  N, Scope *  currentScope);
IrNode *	noisyParseFieldSelect(State *  N, Scope *  currentScope);
IrNode *	noisyParseFixedType(State *  N);
IrNode *	noisyParseFunctionDecl(State *  N, Scope *  scope);
IrNode *	noisyParseFunctionDefn(State *  N, Scope *  scope);
IrNode *	noisyParseGuardedExpressionList(State *  N, Scope *  currentScope);
IrNode *	noisyParseGuardedStatementList(State *  N, Scope *  currentScope);
IrNode *	noisyParseHighPrecedenceArith2ArithOp(State *  N, Scope *  currentScope);
IrNode *	noisyParseHighPrecedenceBinaryBoolOp(State *  N);
IrNode *	noisyParseHighPrecedenceBinaryOp(State *  N);
IrNode *	noisyParseHighPrecedenceBoolSetOp(State *  N, Scope *  currentScope);
IrNode *	noisyParseIdentifierDefinitionTerminal(State *  N, Scope *  scope);
IrNode *	noisyParseIdentifierList(State *  N, Scope *  currentScope);
IrNode *	noisyParseIdentifierOrNil(State *  N, Scope *  currentScope, bool isDefinition);
IrNode *	noisyParseIdentifierOrNilList(State *  N, Scope *  currentScope, bool isDefinition);
IrNode *	noisyParseIdentifierUsageTerminal(State *  N, Scope *  scope);
IrNode *	noisyParseIdxInitList(State *  N, Scope *  scope);
IrNode *	noisyParseInitList(State *  N, Scope *  scope);
IrNode *	noisyParseIntParamOrConst(State *  N, Scope *  currentScope);
IrNode *	noisyParseIntegerType(State *  N, Scope *  currentScope);
IrNode *	noisyParseIterStatement(State *  N, Scope *  scope);
IrNode *	noisyParseLatencyTolerance(State *  N);
IrNode *	noisyParseListCastExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseListType(State *  N, Scope *  currentScope);
IrNode *	noisyParseLoadExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseLossTolerance(State *  N);
IrNode *	noisyParseLowPrecedenceArith2ArithOp(State *  N, Scope *  currentScope);
IrNode *	noisyParseLowPrecedenceBinaryBoolOp(State *  N);
IrNode *	noisyParseLowPrecedenceBinaryOp(State *  N);
IrNode *	noisyParseLowPrecedenceBoolSetOp(State *  N, Scope *  currentScope);
IrNode *	noisyParseMatchStatement(State *  N, Scope *  scope);
IrNode *	noisyParseMaxForExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseMinForExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseModuleDecl(State *  N, Scope *  scope);
IrNode *	noisyParseModuleDeclBody(State *  N, Scope *  scope);
IrNode *	noisyParseModuleTypeNameDecl(State *  N, Scope *  scope);
IrNode *	noisyParseNamegenInvokeShorthand(State *  N, Scope *  currentScope);
IrNode *	noisyParseNumericConst(State *  N, Scope *  currentScope);
IrNode *	noisyParseNumericType(State *  N, Scope *  currentScope);
IrNode *	noisyParseOperatorToleranceDecl(State *  N, Scope *  currentScope);
IrNode *	noisyParseOrderingHead(State *  N, Scope *  currentScope);
IrNode *	noisyParseParallelStatement(State *  N, Scope *  currentScope);
IrNode *	noisyParsePredArithExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParsePredArithFactor(State *  N, Scope *  currentScope);
IrNode *	noisyParsePredArithTerm(State *  N, Scope *  currentScope);
IrNode *	noisyParsePredExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParsePredFactor(State *  N, Scope *  currentScope);
IrNode *	noisyParsePredStmt(State *  N, Scope *  currentScope);
IrNode *	noisyParsePredStmtList(State *  N, Scope *  currentScope);
IrNode *	noisyParsePredTerm(State *  N, Scope *  currentScope);
IrNode *	noisyParsePredicateFnDecl(State *  N, Scope *  scope);
IrNode *	noisyParsePredicateFnDefn(State *  N, Scope *  currentScope);
IrNode *	noisyParseProbdefDecl(State *  N, Scope *  scope);
IrNode *	noisyParseProblemDefn(State *  N, Scope *  currentScope);
IrNode *	noisyParseProductForExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseProgram(State *  N, Scope *  currentScope);
IrNode *	noisyParseQualifiedIdentifier(State *  N, Scope *  currentScope, bool isDefinition);
IrNode *	noisyParseQuantifiedBoolTerm(State *  N, Scope *  currentScope);
IrNode *	noisyParseQuantifierOp(State *  N, Scope *  currentScope);
IrNode *	noisyParseQuantizeExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseRationalCastExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseRationalType(State *  N, Scope *  currentScope);
IrNode *	noisyParseReadTypeSignature(State *  N, Scope *  scope);
IrNode *	noisyParseRealParamOrConst(State *  N, Scope *  currentScope);
IrNode *	noisyParseRealType(State *  N, Scope *  currentScope);
IrNode *	noisyParseReturnSignature(State *  N, Scope *  currentScope);
IrNode *	noisyParseReturnStatement(State *  N, Scope *  currentScope);
IrNode *	noisyParseSampleExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseScopedPredStmtList(State *  N, Scope *  currentScope);
IrNode *	noisyParseScopedStatementList(State *  N, Scope *  scope);
IrNode *	noisyParseSequenceStatement(State *  N, Scope *  currentScope);
IrNode *	noisyParseSetCastExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseSetCmpOp(State *  N, Scope *  currentScope);
IrNode *	noisyParseSetCmpTerm(State *  N, Scope *  currentScope);
IrNode *	noisyParseSetExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseSetFactor(State *  N, Scope *  currentScope);
IrNode *	noisyParseSetHead(State *  N, Scope *  currentScope);
IrNode *	noisyParseSetTerm(State *  N, Scope *  currentScope);
IrNode *	noisyParseSetType(State *  N, Scope *  currentScope);
IrNode *	noisyParseSigfigDesignation(State *  N, Scope *  currentScope);
IrNode *	noisyParseSignalDesignation(State *  N, Scope *  currentScope);
IrNode *	noisyParseSignature(State *  N, Scope *  currentScope, bool isReturn);
IrNode *	noisyParseStarInitList(State *  N, Scope *  scope);
IrNode *	noisyParseStatement(State *  N, Scope *  currentScope);
IrNode *	noisyParseStatementList(State *  N, Scope *  currentScope);
IrNode *	noisyParseStringParamOrConst(State *  N, Scope *  currentScope);
IrNode *	noisyParseSumForExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseSumProdMinMaxBody(State *  N, Scope *  currentScope);
IrNode *	noisyParseTerm(State *  N, Scope *  currentScope);
IrNode *	noisyParseTerminal(State *  N, IrNodeType expectedType);
IrNode *	noisyParseTimeseriesDesignation(State *  N, Scope *  currentScope);
IrNode *	noisyParseTolerance(State *  N, Scope *  currentScope);
IrNode *	noisyParseTuple(State *  N, Scope *  currentScope);
IrNode *	noisyParseTupleType(State *  N, Scope *  currentScope);
IrNode *	noisyParseTupleValue(State *  N, Scope *  currentScope);
IrNode *	noisyParseTypeAnnoteDecl(State *  N, Scope *  currentScope);
IrNode *	noisyParseTypeAnnoteItem(State *  N, Scope *  currentScope);
IrNode *	noisyParseTypeAnnoteList(State *  N, Scope *  currentScope);
IrNode *	noisyParseTypeDecl(State *  N, Scope *  currentScope);
IrNode *	noisyParseTypeExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseTypeMaxExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseTypeMinExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseTypeName(State *  N, Scope *  scope);
IrNode *	noisyParseTypeParameterList(State *  N, Scope *  currentScope);
IrNode *	noisyParseUnaryBoolOp(State *  N);
IrNode *	noisyParseUnaryOp(State *  N);
IrNode *	noisyParseUnarySetOp(State *  N, Scope *  currentScope);
IrNode *	noisyParseUnitsArithExpr(State *  N, Scope *  currentScope);
IrNode *	noisyParseUnitsArithFactor(State *  N, Scope *  currentScope);
IrNode *	noisyParseUnitsArithTerm(State *  N, Scope *  currentScope);
IrNode *	noisyParseUnitsDesignation(State *  N, Scope *  currentScope);
IrNode *	noisyParseValfnSignature(State *  N, Scope *  scope);
IrNode *	noisyParseVarIntro(State *  N, Scope *  currentScope);
IrNode *	noisyParseVarIntroList(State *  N, Scope *  currentScope);
IrNode *	noisyParseVarTuple(State *  N, Scope *  currentScope);
IrNode *	noisyParseVectorTypeDecl(State *  N, Scope *  currentScope);
IrNode *	noisyParseWriteTypeSignature(State *  N, Scope *  scope);
IrNode *	noisyParseIdentifierTerminalIgnoreSymtab(State *  N);
