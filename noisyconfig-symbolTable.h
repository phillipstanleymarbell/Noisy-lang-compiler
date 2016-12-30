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
#include <string.h>


Dimension * noisyConfigDimensionTableAddDimensionForToken(NoisyState *  N, NoisyScope *  scope, struct NoisyToken *  token);
Dimension * noisyConfigDimensionTableDimensionForIdentifier(NoisyState *  N, NoisyScope *  scope, const char *  identifier);

Physics * noisyConfigPhysicsTableAddPhysicsForToken(NoisyState *  N, NoisyScope *  scope, struct NoisyToken *  token);
Physics * noisyConfigPhysicsTablePhysicsForIdentifier(NoisyState *  N, NoisyScope *  scope, const char *  identifier);

Physics* copyPhysicsNode(Physics* list);
Physics* getTailPhysics(Physics* list);

IntegralList* getTailIntegralList(IntegralList* list);

int countNumberTime(Dimension* head);

void noisyConfigPhysicsAddNumeratorDimension(NoisyState * N, Physics * physics, Dimension * numerator); 
void noisyConfigPhysicsAddDenominatorDimension(NoisyState * N, Physics * physics, Dimension * denominator);

void noisyConfigPhysicsCopyNumeratorDimensions(NoisyState * N, Physics * dest, Physics * source);
void noisyConfigPhysicsCopyDenominatorDimensions(NoisyState * N, Physics * dest, Physics * source);
void noisyConfigPhysicsCopyNumeratorToDenominatorDimensions(NoisyState * N, Physics * dest, Physics * source);
void noisyConfigPhysicsCopyDenominatorToNumeratorDimensions(NoisyState * N, Physics * dest, Physics * source);

NoisyScope *	noisyConfigSymbolTableAllocScope(NoisyState *  N);
NoisySymbol *	noisyConfigSymbolTableAddOrLookupSymbolForToken(NoisyState *  N, struct NoisyScope *  scope, struct NoisyToken *  token);
NoisySymbol *	noisyConfigSymbolTableSymbolForIdentifier(NoisyState *  N, struct NoisyScope *  scope, const char *  identifier);
NoisyScope *	noisyConfigSymbolTableOpenScope(NoisyState *  N, struct NoisyScope *  scope, struct NoisyIrNode *  subtree);
void		        noisyConfigSymbolTableCloseScope(NoisyState *  N, struct NoisyScope *  scope, struct NoisyIrNode *  subtree);
