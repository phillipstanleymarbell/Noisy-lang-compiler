/*
	Authored 2015. Jonathan Lim.

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

NoisyIrNode * noisyConfigParse(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode * noisyConfigParseConfigFile(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode * noisyConfigParseDimensionTypeNameScope(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseDimensionTypeNameStatementList(NoisyState * N, NoisyScope * scope);
NoisyIrNode * noisyConfigParseDimensionTypeNameStatement(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode * noisyConfigParseLawScope(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseLawStatementList(NoisyState * N, NoisyScope * scope);
NoisyIrNode * noisyConfigParseLawStatement(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode * noisyConfigParseExpression(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode * noisyConfigParseTerm(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode * noisyConfigParseFactor(NoisyState * N, NoisyScope * currentScope);

NoisyIrNode * noisyConfigParseDimensionAliasScope(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseDimensionAliasStatementList(NoisyState * N, NoisyScope * scope);
NoisyIrNode * noisyConfigParseDimensionAliasStatement(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode * noisyConfigParseVectorIntegralScope(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseVectorIntegralLists(NoisyState * N, NoisyScope * scope);
NoisyIrNode * noisyConfigParseVectorIntegralList(NoisyState * N, NoisyScope * scope);

NoisyIrNode * noisyConfigParseScalarIntegralScope(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseScalarIntegralLists(NoisyState * N, NoisyScope * scope);
NoisyIrNode * noisyConfigParseScalarIntegralList(NoisyState * N, NoisyScope * scope);

NoisyIrNode * noisyConfigParseVectorScalarPairScope(NoisyState *  N, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseVectorScalarPairStatementList(NoisyState * N, NoisyScope * scope);
NoisyIrNode * noisyConfigParseVectorScalarPairStatement(NoisyState *  N, NoisyScope *  currentScope);

NoisyIrNode * noisyConfigParseLowPrecedenceBinaryOp(NoisyState *  N, NoisyScope * currentScope);
NoisyIrNode * noisyConfigParseHighPrecedenceBinaryOp(NoisyState *  N, NoisyScope * currentScope);
NoisyIrNode * noisyConfigParseUnaryOp(NoisyState *  N, NoisyScope * currentScope);
NoisyIrNode * noisyConfigParseVectorOp(NoisyState *  N, NoisyScope * currentScope);
NoisyIrNode * noisyConfigParseAssignOp(NoisyState *  N, NoisyScope * currentScope);


NoisyIrNode * noisyConfigParseTerminal(NoisyState *  N, NoisyIrNodeType expectedType, NoisyScope * currentScope);
NoisyIrNode * noisyConfigParseIdentifier(NoisyState *  N, NoisyScope *  currentScope);
NoisyIrNode * noisyConfigParseIdentifierDefinitionTerminal(NoisyState *  N, NoisyIrNodeType  expectedType, NoisyScope *  scope);
NoisyIrNode * noisyConfigParseIdentifierUsageTerminal(NoisyState *  N, NoisyIrNodeType expectedType, NoisyScope *  scope);
