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

void			newtonParserSemanticError(State *  N, const char * format, ...);
void			newtonParserSyntaxError(State *  N, IrNodeType currentlyParsingProduction, IrNodeType expectedProductionOrToken);
void			newtonParserErrorRecovery(State *  N, IrNodeType expectedProductionOrToken) __attribute__((noreturn));

IrNode *		newtonParse(State *  N, Scope *  currentScope);
IrNode *		newtonParseFile(State *  N, Scope *  currentScope);
IrNode *		newtonParseRuleList(State *  N, Scope *  currentScope);
IrNode *		newtonParseRule(State *  N, Scope *  currentScope);
IrNode *		newtonParseInvariant(State *  N, Scope *  currentScope);
IrNode *		newtonParseParameter(State *  N, Scope *  currentScope, int parameterNumber);
IrNode *		newtonParseParameterTuple(State *  N, Scope *  currentScope);
IrNode *		newtonParseSubindex(State *  N, Scope *  currentScope);
IrNode *		newtonParseSubindexTuple(State *  N, Scope *  currentScope);
IrNode *		newtonParseConstraint(State *  N, Scope *  currentScope);
IrNode *		newtonParseBaseSignal(State *  N, Scope *  currentScope);
IrNode *		newtonParseName(State *  N, Scope *  currentScope);
IrNode *		newtonParseSymbol(State *  N, Scope *  currentScope);
IrNode *		newtonParseDerivation(State *  N, Scope *  currentScope);
IrNode *		newtonParseTerminal(State *  N, IrNodeType expectedType, Scope *  currentScope);
IrNode *		newtonParseIdentifier(State *  N, Scope *  currentScope);
IrNode *		newtonParseIdentifierUsageTerminal(State *  N, IrNodeType expectedType, Scope *  scope);
IrNode *		newtonParseIdentifierDefinitionTerminal(State *  N, IrNodeType  expectedType, Scope *  scope);
IrNode *		newtonParseConstant(State *  N, Scope *  currentScope);
IrNode *		newtonParseCompareOp(State *  N, Scope *  currentScope);
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
