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



void		noisyParserSemanticError(NoisyState *  N, const char * format, ...);
void		noisyParserSyntaxError(NoisyState *  N, NoisyIrNodeType currentlyParsingProduction, NoisyIrNodeType expectedProductionOrToken);
void		noisyParserErrorRecovery(NoisyState *  N, NoisyIrNodeType expectedProductionOrToken) __attribute__((noreturn));


/*
 *	TODO / FUTURE:	Since we expose both the lexer and parser interface, 
 *	libNoisy could be used to build a code editor plugin that proactively
 *	flags syntax errors, offers code completion, etc.
 *
 *	The GitHub 'Atom' editor would be a good host organism.
 */
NoisyIrNode *	noisyParse(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseProgram(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseProgtypeDeclaration(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseProgtypeBody(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseProgtypeTypenameDeclaration(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseConstantDeclaration(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseTypeDeclaration(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseAdtTypeDeclaration(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseVectorType(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseNamegenDeclaration(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseIdentifierOrNil(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseIdentifierOrNilList(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseIdentifierList(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseTypeExpression(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseTypeName(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseTolerance(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseErrorMagnitudeTolerance(NoisyState *  N);
NoisyIrNode *	noisyParseLossTolerance(NoisyState *  N);
NoisyIrNode *	noisyParseLatencyTolerance(NoisyState *  N);
NoisyIrNode *	noisyParseBasicType(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseRealType(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseFixedType(NoisyState *  N);
NoisyIrNode *	noisyParseAnonAggregateType(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseArrayType(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseListType(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseTupleType(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseSetType(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseInitList(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseIdxInitList(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseStarInitList(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseElement(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseNamegenDefinition(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseScopedStatementList(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseStatementList(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseStatement(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseAssignOp(NoisyState *  N);
NoisyIrNode *	noisyParseMatchStatement(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseIterStatement(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode *	noisyParseGuardBody(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseExpression(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseListCastExpression(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseSetCastExpression(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseArrayCastExpression(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseAnonAggregateCastExpression(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseChanEventExpression(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseChan2nameExpression(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseVar2nameExpression(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseName2chanExpression(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseTerm(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseFactor(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseTupleValue(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseFieldSelect(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode *	noisyParseHighPrecedenceBinaryOp(NoisyState *  N);
NoisyIrNode *	noisyParseLowPrecedenceBinaryOp(NoisyState *  N);
NoisyIrNode *	noisyParseCmpOp(NoisyState *  N);
NoisyIrNode *	noisyParseBooleanOp(NoisyState *  N);
NoisyIrNode *	noisyParseUnaryOp(NoisyState *  N);
NoisyIrNode *	noisyParseTerminal(NoisyState *  N, NoisyIrNodeType expectedType);
NoisyIrNode *	noisyParseIdentifierUsageTerminal(NoisyState *  N, NoisyIrNodeType expectedType, NoisyScope *  scope);
NoisyIrNode *	noisyParseIdentifierDefinitionTerminal(NoisyState *  N, NoisyIrNodeType  expectedType, NoisyScope *  scope);
