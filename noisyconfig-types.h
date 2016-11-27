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
void        noisyConfigInferIdentifierTypeInDeclaration(NoisyConfigState * N, NoisyConfigScope * scope, NoisyConfigIrNode * node);
void        noisyConfigInferIdentifierTypeInStatement(NoisyConfigState * N, NoisyConfigScope * scope, NoisyConfigIrNode * node);
bool areSameTypes(NoisyConfigIrNodeType type1, NoisyConfigIrNodeType type2);
bool isTokenToIgnoreBinOp(NoisyConfigIrNodeType targetType);
NoisyConfigIrNode * lookupNodeInParents(NoisyConfigIrNode * node, NoisyConfigIrNodeType targetType);
NoisyConfigIrNode * lookupNodeInSubtree(NoisyConfigIrNode * node, NoisyConfigIrNodeType targetType);
NoisyConfigIrNodeType postOrderWalkBinOp(NoisyConfigState *N, NoisyConfigIrNode * node);
void checkPlus(NoisyConfigState * N, NoisyConfigIrNode * node);
void checkBinOps(NoisyConfigState * N, NoisyConfigIrNode * node);
void checkAllNodeTypes(NoisyConfigState * N, NoisyConfigIrNode * node);
bool isValidIdChar(char * string);
bool isNumber(char c);
bool isValidIdentifier(NoisyConfigState * N, NoisyConfigIrNode * node);
void noisyConfigIrPassTypeChecker(NoisyConfigState * N, NoisyConfigIrNode * irNode);
NoisyConfigIrNode *	noisyConfigTypeValidateIrSubtree(NoisyConfigState *  N, NoisyConfigIrNode *  subtree);
bool 		noisyConfigTypeEqualsSubtreeTypes(NoisyConfigState *  N, NoisyConfigIrNode *  subtreeA, NoisyConfigIrNode *  subtreeB);
char *	noisyConfigTypeMakeTypeSignature(NoisyConfigState *  N, NoisyConfigIrNode *  subtree);
