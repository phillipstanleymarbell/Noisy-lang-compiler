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
void		noisyParserSyntaxError(State *  N, IrNodeType currentlyParsingProduction, IrNodeType expectedProductionOrToken);
void		noisyParserErrorRecovery(State *  N, IrNodeType expectedProductionOrToken) __attribute__((noreturn));


/*
 *	TODO / FUTURE:	Since we expose both the lexer and parser interface, 
 *	libNoisy could be used to build a code editor plugin that proactively
 *	flags syntax errors, offers code completion, etc.
 *
 *	The GitHub 'Atom' editor would be a good host organism.
 */
IrNode *	noisyParse(State *  N, Scope *  currentScope);
IrNode *	noisyParseProgram(State *  N, Scope *  currentScope);
IrNode *	noisyParseProgtypeDeclaration(State *  N, Scope *  scope);
IrNode *	noisyParseProgtypeBody(State *  N, Scope *  scope);
IrNode *	noisyParseProgtypeTypenameDeclaration(State *  N, Scope *  scope);
IrNode *	noisyParseConstantDeclaration(State *  N, Scope *  scope);
IrNode *	noisyParseTypeDeclaration(State *  N, Scope *  currentScope);
IrNode *	noisyParseAdtTypeDeclaration(State *  N, Scope *  scope);
IrNode *	noisyParseVectorType(State *  N, Scope *  scope);
IrNode *	noisyParseNamegenDeclaration(State *  N, Scope *  scope);
IrNode *	noisyParseIdentifierOrNil(State *  N, Scope *  currentScope);
IrNode *	noisyParseIdentifierOrNilList(State *  N, Scope *  currentScope);
IrNode *	noisyParseIdentifierList(State *  N, Scope *  currentScope);
IrNode *	noisyParseTypeExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseTypeName(State *  N, Scope *  scope);
IrNode *	noisyParseTolerance(State *  N, Scope *  currentScope);
IrNode *	noisyParseErrorMagnitudeTolerance(State *  N);
IrNode *	noisyParseLossTolerance(State *  N);
IrNode *	noisyParseLatencyTolerance(State *  N);
IrNode *	noisyParseBasicType(State *  N, Scope *  currentScope);
IrNode *	noisyParseRealType(State *  N, Scope *  currentScope);
IrNode *	noisyParseFixedType(State *  N);
IrNode *	noisyParseAnonAggregateType(State *  N, Scope *  currentScope);
IrNode *	noisyParseArrayType(State *  N, Scope *  currentScope);
IrNode *	noisyParseListType(State *  N, Scope *  currentScope);
IrNode *	noisyParseTupleType(State *  N, Scope *  currentScope);
IrNode *	noisyParseSetType(State *  N, Scope *  currentScope);
IrNode *	noisyParseInitList(State *  N, Scope *  scope);
IrNode *	noisyParseIdxInitList(State *  N, Scope *  scope);
IrNode *	noisyParseStarInitList(State *  N, Scope *  scope);
IrNode *	noisyParseElement(State *  N, Scope *  scope);
IrNode *	noisyParseNamegenDefinition(State *  N, Scope *  scope);
IrNode *	noisyParseScopedStatementList(State *  N, Scope *  scope);
IrNode *	noisyParseStatementList(State *  N, Scope *  currentScope);
IrNode *	noisyParseStatement(State *  N, Scope *  currentScope);
IrNode *	noisyParseAssignOp(State *  N);
IrNode *	noisyParseMatchStatement(State *  N, Scope *  scope);
IrNode *	noisyParseIterStatement(State *  N, Scope *  scope);
IrNode *	noisyParseGuardBody(State *  N, Scope *  currentScope);
IrNode *	noisyParseExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseListCastExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseSetCastExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseArrayCastExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseAnonAggregateCastExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseChanEventExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseChan2nameExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseVar2nameExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseName2chanExpression(State *  N, Scope *  currentScope);
IrNode *	noisyParseTerm(State *  N, Scope *  currentScope);
IrNode *	noisyParseFactor(State *  N, Scope *  currentScope);
IrNode *	noisyParseTupleValue(State *  N, Scope *  currentScope);
IrNode *	noisyParseFieldSelect(State *  N, Scope *  currentScope);
IrNode *	noisyParseHighPrecedenceBinaryOp(State *  N);
IrNode *	noisyParseLowPrecedenceBinaryOp(State *  N);
IrNode *	noisyParseCmpOp(State *  N);
IrNode *	noisyParseBooleanOp(State *  N);
IrNode *	noisyParseUnaryOp(State *  N);
IrNode *	noisyParseTerminal(State *  N, IrNodeType expectedType);
IrNode *	noisyParseIdentifierUsageTerminal(State *  N, IrNodeType expectedType, Scope *  scope);
IrNode *	noisyParseIdentifierDefinitionTerminal(State *  N, IrNodeType  expectedType, Scope *  scope);
