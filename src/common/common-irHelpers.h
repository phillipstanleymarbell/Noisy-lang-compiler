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

IrNode *	genIrNode(State *  N, IrNodeType type, IrNode *  irLeftChild, IrNode *  irRightChild, SourceInfo *  sourceInfo);
void		errorUseBeforeDefinition(State *  N, const char *  identifier);
void		errorMultiDefinition(State *  N, Symbol *  symbol);
IrNode *	depthFirstWalk(State *  N, IrNode *  node);
void		addLeaf(State *  N, IrNode *  parent, IrNode *  newNode);
void		addLeafWithChainingSeq(State *  N, IrNode *  parent, IrNode *  newNode);
void		addLeafWithChainingSeqNoLexer(State *  N, IrNode *  parent, IrNode *  newNode);
bool		peekCheck(State *  N, int lookAhead, IrNodeType expectedType);
IrNode *	findNthIrNodeOfType(State *  N, IrNode *  root, IrNodeType expectedType, int nth);
IrNode *	findNthIrNodeOfTypes(State *  N, IrNode *  root, IrNodeType productionOrToken, int firsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax], int nth);
IrNode *	findNthIrNodeOfTypeHelper(State *  N, IrNode *  root, IrNodeType expectedType, int *  nth);
IrNode *	findNthIrNodeOfTypesHelper(State *  N, IrNode *  root, IrNodeType productionOrToken, int firsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax], int *  nth);
