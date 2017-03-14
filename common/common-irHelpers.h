/*
    Authored 2015, Phillip Stanley-Marbell. Modified 2017, Jonathan Lim.

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

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTorS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTorS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TorT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

NoisyIrNode *	genNoisyIrNode(NoisyState *  N, NoisyIrNodeType type, NoisyIrNode *  irLeftChild, NoisyIrNode *  irRightChild, NoisySourceInfo *  sourceInfo);
void		errorUseBeforeDefinition(NoisyState *  N, const char *  identifier);
void		errorMultiDefinition(NoisyState *  N, NoisySymbol *  symbol);
NoisyIrNode *	depthFirstWalk(NoisyState *  N, NoisyIrNode *  node);
void		addLeaf(NoisyState *  N, NoisyIrNode *  parent, NoisyIrNode *  newNode);
void		addLeafWithChainingSeq(NoisyState *  N, NoisyIrNode *  parent, NoisyIrNode *  newNode);
void		addLeafWithChainingSeqNoLexer(NoisyState *  N, NoisyIrNode *  parent, NoisyIrNode *  newNode);
bool		peekCheck(NoisyState *  N, int lookAhead, NoisyIrNodeType expectedType);
NoisyIrNode* findNthIrNodeOfType(NoisyState * N, NoisyIrNode * root, NoisyIrNodeType expectedType, int nth);
NoisyIrNode* findNthIrNodeOfTypes(NoisyState * N, NoisyIrNode * root, NoisyIrNodeType productionOrToken, int firsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax], int nth);
NoisyIrNode* findNthIrNodeOfTypeHelper(NoisyState * N, NoisyIrNode * root, NoisyIrNodeType expectedType, int* nth);
NoisyIrNode* findNthIrNodeOfTypesHelper(NoisyState * N, NoisyIrNode * root, NoisyIrNodeType productionOrToken, int firsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax], int* nth);
