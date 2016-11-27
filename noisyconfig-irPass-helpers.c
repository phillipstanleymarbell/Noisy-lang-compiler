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
#include <string.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "version.h"
#include "noisyconfig.h"


void
noisyConfigIrPassHelperColorIr(NoisyConfigState *  N, NoisyConfigIrNode *  p, NoisyConfigIrNodeColor nodeColor, bool setNotClear, bool recurseFlag)
{
	if (p == NULL)
	{
		return;
	}

	if (p->irLeftChild == p || p->irRightChild == p)
	{
		noisyConfigFatal(N, "Immediate cycle in Ir, seen noisyConfigTimeStampKeyIrPassHelperColorIr()!!\n");
		return;
	}

	if (setNotClear)
	{
		p->nodeColor |= nodeColor;
	}
	else
	{
		p->nodeColor &= ~nodeColor;
	}

	if (recurseFlag)
	{
		noisyConfigIrPassHelperColorIr(N, p->irLeftChild, nodeColor, setNotClear, recurseFlag);
		noisyConfigIrPassHelperColorIr(N, p->irRightChild, nodeColor, setNotClear, recurseFlag);
	}

	return;
}


void
noisyConfigIrPassHelperColorSymbolTable(NoisyConfigState *  N, NoisyConfigScope *  p, NoisyConfigIrNodeColor nodeColor, bool setNotClear, bool recurseFlag)
{
	if (p == NULL)
	{
		return;
	}

	if (setNotClear)
	{
		p->nodeColor |= nodeColor;
	}
	else
	{
		p->nodeColor &= ~nodeColor;
	}

	if (recurseFlag)
	{
		NoisyConfigScope *	tmp = p->firstChild;
		while (tmp != NULL)
		{
			noisyConfigIrPassHelperColorSymbolTable(N, tmp, nodeColor, setNotClear, recurseFlag);
			tmp = tmp->next;
		}
	}
	noisyConfigIrPassHelperColorSymbolTable(N, p->next, nodeColor, setNotClear, recurseFlag);

	return;
}


uint64_t
noisyConfigIrPassHelperIrSize(NoisyConfigState *  N, NoisyConfigIrNode *  p)
{
	if (p == NULL)
	{
		return 0;
	}

	if (p->irLeftChild == p || p->irRightChild == p)
	{
		noisyConfigFatal(N, "Immediate cycle in AST, seen in noisyConfigIrPassHelperIrSize()!!\n");
		return 0;
	}

	return (1 + noisyConfigIrPassHelperIrSize(N, p->irLeftChild) + noisyConfigIrPassHelperIrSize(N, p->irRightChild));
}


uint64_t
noisyConfigIrPassHelperSymbolTableSize(NoisyConfigState *  N, NoisyConfigScope *  p)
{
	if (p == NULL)
	{
		return 0;
	}

	int		n = 0;
	NoisyConfigScope *	tmp = p->firstChild;
	while (tmp != NULL)
	{
		n += noisyConfigIrPassHelperSymbolTableSize(N, tmp);
		tmp = tmp->next;
	}

	return (1 + n + noisyConfigIrPassHelperSymbolTableSize(N, p->next));
}
