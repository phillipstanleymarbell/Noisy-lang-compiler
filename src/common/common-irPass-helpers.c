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
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "noisy-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"


void
irPassHelperColorIr(State *  N, IrNode *  p, IrNodeColor nodeColor, bool setNotClear, bool recurseFlag)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyIrPassHelperColorIr);

	if (p == NULL)
	{
		return;
	}

	if (p->irLeftChild == p || p->irRightChild == p)
	{
		fatal(N, "Immediate cycle in Ir, seen noisyTimeStampKeyIrPassHelperColorIr()!!\n");
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
		irPassHelperColorIr(N, p->irLeftChild, nodeColor, setNotClear, recurseFlag);
		irPassHelperColorIr(N, p->irRightChild, nodeColor, setNotClear, recurseFlag);
	}

	return;
}


void
irPassHelperColorSymbolTable(State *  N, Scope *  p, IrNodeColor nodeColor, bool setNotClear, bool recurseFlag)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyIrPassHelperColorSymbolTable);

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
		Scope *	tmp = p->firstChild;
		while (tmp != NULL)
		{
			irPassHelperColorSymbolTable(N, tmp, nodeColor, setNotClear, recurseFlag);
			tmp = tmp->next;
		}
	}
	irPassHelperColorSymbolTable(N, p->next, nodeColor, setNotClear, recurseFlag);

	return;
}


uint64_t
irPassHelperIrSize(State *  N, IrNode *  p)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyIrPassHelperIrSize);

	if (p == NULL)
	{
		return 0;
	}

	if (p->irLeftChild == p || p->irRightChild == p)
	{
		fatal(N, "Immediate cycle in AST, seen in noisyIrPassHelperIrSize()!!\n");
		return 0;
	}

	return (1 + irPassHelperIrSize(N, p->irLeftChild) + irPassHelperIrSize(N, p->irRightChild));
}


uint64_t
irPassHelperSymbolTableSize(State *  N, Scope *  p)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyIrPassHelperSymbolTableSize);

	if (p == NULL)
	{
		return 0;
	}

	int		n = 0;
	Scope *	tmp = p->firstChild;
	while (tmp != NULL)
	{
		n += irPassHelperSymbolTableSize(N, tmp);
		tmp = tmp->next;
	}

	return (1 + n + irPassHelperSymbolTableSize(N, p->next));
}
