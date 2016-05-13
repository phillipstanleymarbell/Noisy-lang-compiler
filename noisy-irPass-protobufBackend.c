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

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/time.h>
#include <string.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisy-errors.h"
#include "version.h"
#include "noisy-timeStamps.h"
#include "noisy.h"
#include "noisy-parser.h"
#include "noisy-lexer.h"
#include "noisy-irPass-helpers.h"
#include "noisy-irPass-protobufBackend.h"
#include "noisy-types.h"
#include "noisy.pb-c.h"



void
noisyIrPassProtobufBackend(NoisyState *  N)
{
	/*
	 *	Temporarily color the graph, so we can know
	 *	which nodes have been visited, in case when
	 *	the graph is not a tree.
	 */
	noisyIrPassHelperColorIr(N, N->noisyIrRoot, kNoisyIrNodeColorProtobufBackendColoring, true/* set */, true/* recurse flag */);
	noisyIrPassHelperColorSymbolTable(N, N->noisyIrTopScope, kNoisyIrNodeColorProtobufBackendColoring, true/* set */, true/* recurse flag */);

	noisyIrPassProtobufAstSerializeWalk(N, N->noisyIrRoot);
	noisyIrPassProtobufSymbolTableSerializeWalk(N, N->noisyIrTopScope);
}


void
noisyIrPassProtobufSymbolTableNodeEmitter(NoisyState *  N, NoisyScope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyIrPassProtobufSymbotTableEmitter);

}


void
noisyIrPassProtobufAstNodeEmitter(NoisyState *  N, NoisyIrNode *  irNode)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyIrPassProtobufAstEmitter);

	Noisy__NoisyIrNode	node = NOISY__NOISY_IR_NODE__INIT;
	void *			buffer;
	unsigned		bufferLength;

	node.type = irNode->type;
	bufferLength = noisy__noisy_ir_node__get_packed_size(&node);

	buffer = malloc(bufferLength);
	noisy__noisy_ir_node__pack(&node, buffer);

	fprintf(stderr,"Writing %d serialized bytes\n", bufferLength);
	fwrite(buffer, bufferLength, 1, stdout);

	free(buffer);
}


void
noisyIrPassProtobufAstSerializeWalk(NoisyState *  N, NoisyIrNode *  irNode)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyIrPassProtobufAstSerializeWalk);

	if (irNode == NULL)
	{
		return;
	}

    checkAllNodeTypes(N, irNode); // type checking

	if (L(irNode) == irNode || R(irNode) == irNode)
	{
		noisyFatal(N, "Immediate cycle in Ir, seen noisyIrPassProtobufAstSerializeWalk()!!\n");
	}

	/*
	 *	We walk tree in postorder.
	 */
	noisyIrPassProtobufAstSerializeWalk(N, L(irNode));
	noisyIrPassProtobufAstSerializeWalk(N, R(irNode));

	/*
	 *	Only process nodes once.
	 */
	if (irNode->nodeColor & kNoisyIrNodeColorProtobufBackendColoring)
	{
		noisyIrPassProtobufAstNodeEmitter(N, irNode);
		irNode->nodeColor &= ~kNoisyIrNodeColorProtobufBackendColoring;
	}
}


void
noisyIrPassProtobufSymbolTableSerializeWalk(NoisyState *  N, NoisyScope *  scope)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyIrPassProtobufSymbolTableSerializeWalk);

	if (scope == NULL)
	{
		return;
	}

	NoisyScope *	tmp = scope->firstChild;
	while (tmp != NULL)
	{
		noisyIrPassProtobufSymbolTableSerializeWalk(N, tmp);
		tmp = tmp->next;
	}

	/*
	 *	Only process nodes once.
	 */
	if (scope->nodeColor & kNoisyIrNodeColorProtobufBackendColoring)
	{
		noisyIrPassProtobufSymbolTableNodeEmitter(N, scope);
		scope->nodeColor &= ~kNoisyIrNodeColorProtobufBackendColoring;
	}

	noisyIrPassProtobufSymbolTableSerializeWalk(N, scope->next);
}
