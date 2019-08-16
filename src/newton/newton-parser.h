/*
	Authored 2017. Jonathan Lim. Modified 2018, Phillip Stanley-Marbell.

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

void			newtonParserSemanticError(State *  N, IrNodeType currentlyParsingTokenOrProduction, char *  details);
void			newtonParserSyntaxError(State *  N, IrNodeType currentlyParsingTokenOrProduction, IrNodeType expectedProductionOrToken, int firstOrFollowsArray[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax]);
void			newtonParserErrorRecovery(State *  N, IrNodeType expectedProductionOrToken) __attribute__((noreturn));

IrNode *		newtonParse(State *  N, Scope *  currentScope);
IrNode *		newtonParseAccuracyStatement(State * N, Scope *  currentScope);
IrNode *		newtonParseArithmeticCommand(State * N, Scope *  currentScope);
IrNode *		newtonParseBaseSignal(State *  N, Scope *  currentScope);
IrNode *		newtonParseCompareOp(State *  N, Scope *  currentScope);
IrNode *		newtonParseConstant(State *  N, Scope *  currentScope);
IrNode *		newtonParseConstant(State * N, Scope *  currentScope);
IrNode *		newtonParseConstraint(State *  N, Scope *  currentScope);
IrNode *		newtonParseConstraintList(State * N, Scope *  currentScope);
IrNode *		newtonParseDelayCommand(State * N, Scope *  currentScope);
IrNode *		newtonParseDerivation(State *  N, Scope *  currentScope);
IrNode *		newtonParseDerivation(State * N, Scope *  currentScope);
IrNode *		newtonParseDistribution(State * N, Scope *  currentScope);
IrNode *		newtonParseDistributionFactor(State * N, Scope *  currentScope);
IrNode *		newtonParseErasureValueStatement(State * N, Scope *  currentScope);
IrNode *		newtonParseExponentiationOperator(State *  N, Scope *  currentScope);
IrNode *		newtonParseExpression(State * N, Scope *  currentScope);
IrNode *		newtonParseFactor(State * N, Scope *  currentScope);
IrNode *		newtonParseFile(State *  N, Scope *  currentScope);
IrNode *		newtonParseHighPrecedenceBinaryOp(State *  N, Scope *  currentScope);
IrNode *		newtonParseHighPrecedenceQuantityOperator(State *  N, Scope *  currentScope);
IrNode *		newtonParseHighPrecedenceOperator(State *  N, Scope *  currentScope);
IrNode *		newtonParseIdentifier(State *  N, Scope *  currentScope);
IrNode *		newtonParseIdentifierDefinitionTerminal(State *  N, IrNodeType  expectedType, Scope *  scope);
IrNode *		newtonParseIdentifierUsageTerminal(State *  N, IrNodeType expectedType, Scope *  scope);
IrNode *		newtonParseInteger(State *  N, Scope *  currentScope);
IrNode *		newtonParseInteger(State * N, Scope *  currentScope);
IrNode *		newtonParseInvariant(State *  N, Scope *  currentScope);
IrNode *		newtonParseInvariant(State * N, Scope *  currentScope);
IrNode *		newtonParseLanguageSetting(State * N, Scope *  currentScope);
IrNode *		newtonParseLowPrecedenceBinaryOp(State *  N, Scope *  currentScope);
IrNode *		newtonParseLowPrecedenceOperator(State * N, Scope *  currentScope);
IrNode *		newtonParseName(State *  N, Scope *  currentScope);
IrNode *		newtonParseNumericConst(State *  N, Scope *  currentScope);
IrNode *		newtonParseNumericConstTuple(State * N, Scope *  currentScope);
IrNode *		newtonParseNumericConstTupleList(State * N, Scope *  currentScope);
IrNode *		newtonParseNumericExpression(State *  N, Scope *  currentScope);
IrNode *		newtonParseNumericFactor(State *  N, Scope *  currentScope);
IrNode *		newtonParseNumericTerm(State *  N, Scope *  currentScope);
IrNode *		newtonParseParameter(State *  N, Scope *  currentScope, int parameterNumber);
IrNode *		newtonParseParameter(State * N, Scope *  currentScope, int parameterNumber);
IrNode *		newtonParseParameterTuple(State *  N, Scope *  currentScope);
IrNode *		newtonParseParameterTuple(State * N, Scope *  currentScope);
IrNode *		newtonParseParameterValueList(State * N, Scope *  currentScope);
IrNode *		newtonParsePrecisionStatement(State * N, Scope *  currentScope);
IrNode *		newtonParseQuantity(State * N, Scope *  currentScope);
IrNode *		newtonParseQuantityExpression(State *  N, Scope *  currentScope);
IrNode *		newtonParseQuantityExpression(State * N, Scope *  currentScope);
IrNode *		newtonParseQuantityFactor(State *  N, Scope *  currentScope);
IrNode *		newtonParseQuantityFactor(State * N, Scope *  currentScope);
IrNode *		newtonParseQuantityTerm(State *  N, Scope *  currentScope);
IrNode *		newtonParseQuantityTerm(State * N, Scope *  currentScope);
IrNode *		newtonParseRangeStatement(State * N, Scope *  currentScope);
IrNode *		newtonParseReadRegisterCommand(State * N, Scope *  currentScope);
IrNode *		newtonParseRule(State *  N, Scope *  currentScope);
IrNode *		newtonParseRuleList(State *  N, Scope *  currentScope);
IrNode *		newtonParseSensorDefinition(State * N, Scope *  currentScope);
IrNode *		newtonParseSensorInterfaceCommand(State * N, Scope *  currentScope);
IrNode *		newtonParseSensorInterfaceCommandList(State * N, Scope *  currentScope);
IrNode *		newtonParseSensorInterfaceStatement(State * N, Scope *  currentScope);
IrNode *		newtonParseSensorInterfaceType(State *  N, Scope *  currentScope);
IrNode *		newtonParseSensorProperty(State * N, Scope *  currentScope);
IrNode *		newtonParseSensorPropertyList(State * N, Scope *  currentScope);
IrNode *		newtonParseSubindex(State *  N, Scope *  currentScope);
IrNode *		newtonParseSubindexTuple(State *  N, Scope *  currentScope);
IrNode *		newtonParseSubindexTuple(State * N, Scope *  currentScope);
IrNode *		newtonParseSymbol(State *  N, Scope *  currentScope);
IrNode *		newtonParseSymbol(State * N, Scope *  currentScope);
IrNode *		newtonParseTerm(State * N, Scope *  currentScope);
IrNode *		newtonParseTerminal(State *  N, IrNodeType expectedType, Scope *  currentScope);
IrNode *		newtonParseTerminal(State *  N, IrNodeType expectedType, Scope *  currentScope);
IrNode *		newtonParseFunctionalOperator(State * N, Scope *  currentScope);
IrNode *		newtonParseUnaryOp(State *  N, Scope *  currentScope);
IrNode *		newtonParseUnaryOp(State *  N, Scope *  currentScope);
IrNode *		newtonParseUncertaintyStatement(State * N, Scope *  currentScope);
IrNode *		newtonParseUnit(State * N, Scope *  currentScope);
IrNode *		newtonParseUnitExpression(State * N, Scope *  currentScope);
IrNode *		newtonParseUnitFactor(State * N, Scope *  currentScope);
IrNode *		newtonParseUnitTerm(State * N, Scope *  currentScope);
IrNode *		newtonParseNumericExpression(State *  N, Scope *  currentScope);
IrNode *		newtonParseNumericTerm(State *  N, Scope *  currentScope);
IrNode *		newtonParseNumericFactor(State *  N, Scope *  currentScope);
IrNode *		newtonParseVectorOp(State * N, Scope *  currentScope);
IrNode *		newtonParseWriteRegisterCommand(State * N, Scope *  currentScope);


bool			newtonIsDimensionless(Physics *  physics);
void 			newtonParseResetPhysicsWithCorrectSubindex(
				State *  N,
				IrNode *  node,
				Scope *  scope,
				char *  identifier,
				int subindex);
unsigned long long int	newtonGetInvariantIdByParameters(State *  N, IrNode *  parameterTreeRoot, unsigned long long int invariantId);
int			newtonGetPhysicsId(State *  N, Physics *  physics);
Physics *		newtonParseGetPhysicsByBoundIdentifier(State *  N, IrNode *  root, char *  boundVariableIdentifier);
IrNode *		newtonParseFindNodeByPhysicsId(State *  N, IrNode *  root, int physicsId);
IrNode *		newtonParseFindParameterByTokenString(State *  N, IrNode *  parameterTreeRoot, char *  tokenString);
IrNode *		newtonParseFindNodeByParameterNumberAndSubindex(State *  N, IrNode *  root, int parameterNumber, int subindex);
