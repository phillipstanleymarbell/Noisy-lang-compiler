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


typedef struct
{
	/*
	 *	Hierarchy
	 */
	NoisyScope *		parent;
	NoisyScope **		children;

	/*
	 *	Symbols in this scope
	 */
	NoisySymbol **		symbols;

	/*
	 *	Where in source scope begins and ends
	 */
	NoisyLexerSrc *		begin;
	NoisyLexerSrc *		end;
} NoisyScope;


typedef struct
{
	const char *		identifier;

	/*
	 *	This field is duplicated in the AST node, since only
	 *	identifiers get into the symbol table:
	 */
	NoisyLexerSrc *		sourceInfo;

	/*
	 *	Declaration, type definition, use, etc. (kNoisySymbolTypeXXX)
	 */
	int 			symbolType;

	/*
	 *	Scope within which sym appears
	 */
	NoisyScope *		scope;

	/*
	 *	If an identifier use, definition's Sym, if any
	 */
	NoisySym *		definition;

	/*
	 *	Subtree in AST that represents typeexpr
	 */
	NoisyIrNode *		typeTree;

	/*
	 *	If an I_CONST, its value.
	 */
	int			intConst;
	double			realConst;
	const char *		stringConst;
} NoisySymbol;


NoisySymbol *	noisySymbolTableSymbolForIdentifier(NoisyScope *  scope, NoisyLexerToken *  token);
NoisyScope *	noisySymbolTableOpenScope(NoisyScope *  scope, NoisyIrNode *  subtree);
void		noisySymbolTableCloseScope(NoisyScope *  scope, NoisyIrNode *  subtree);
NoisySym *	noisySymbolTableLookup(NoisyScope *  scope, const char *  identifier);


typedef enum
{
	kNoisySymbolTypeTypeError,
	kNoisySymbolTypeProgtype,
	kNoisySymbolTypeConstantDeclaration,
	kNoisySymbolTypeTypeDeclaration,
	kNoisySymbolTypeNamegenDeclaration,
	kNoisySymbolTypeVariableDeclaration,
	kNoisySymbolTypeNamegenDefinition,
	kNoisySymbolTypeUse,

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisySymbolTypeMax,
} NoisySymbolType;
