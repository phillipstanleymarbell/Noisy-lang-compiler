
/*
	Authored 2016. Jonathan Lim.

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
#include "noisy-errors.h"
#include "version.h"
#include "noisy-timeStamps.h"
#include "noisy.h"
#include "newton-lexer.h"

extern int	gNewtonFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];
extern int	gNewtonFollows[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];
extern char *	gProductionStrings[];
extern char *	gTerminalStrings[];


/*
 *	NOTE: Unlike in our previous compilers (e.g., Crayon), we do
 *
 *		inFirst(NoisyIrNodeType productionOrToken)
 *
 *	with the token being checked implicit in the lexer state, rather than
 *
 *		inFirst(NoisyIrNodeType productionOrToken, NoisyIrNodeType token)
 */
bool
newtonInFirst(NoisyState *  N, NoisyIrNodeType productionOrToken)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyInFirst);

	NoisyToken *	token = newtonLexPeek(N, 1);

	if (productionOrToken > kNoisyIrNodeTypeMax)
	{
		noisyFatal(N, Esanity);
	}

	//flexprint(N->Fe, N->Fm, N->Fperr, "noisyInFirst(): productionOrToken=[%d], gProductionStrings[%d]=[%s],"
	//		"token->type=[%d], gTerminalStrings[%d]=[%s]...\n",
	//		productionOrToken, productionOrToken, gProductionStrings[productionOrToken],
	//		token->type, token->type, gTerminalStrings[token->type]);

	/*
	 *	NOTE: The arrays created by ffi2code have a kNoisyIrNodeTypeMax element at the end of each sub-array
	 */
	for (int i = 0; i < kNoisyIrNodeTypeMax && gNewtonFirsts[productionOrToken][i] != kNoisyIrNodeTypeMax; i++)
	{
		//flexprint(N->Fe, N->Fm, N->Fperr, "--->gNoisyFirsts[%d][%d] = %s...\n",
		//	productionOrToken, i, gTerminalStrings[gNoisyFirsts[productionOrToken][i]]);

		if (gNewtonFirsts[productionOrToken][i] == token->type)
		{
			return true;
		}
	}

	return false;
}


/*
 *	NOTE: Unlike in our previous compilers (e.g., Crayon), we do
 *
 *		inFollow(NoisyIrNodeType productionOrToken)
 *
 *	with the token being checked implicit in the lexer state, rather than
 *
 *		inFollow(NoisyIrNodeType productionOrToken, NoisyIrNodeType token)
 */
bool
newtonInFollow(NoisyState *  N, NoisyIrNodeType productionOrToken)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyInFollow);

	NoisyToken *	token = newtonLexPeek(N, 1);

	if (productionOrToken > kNoisyIrNodeTypeMax)
	{
		noisyFatal(N, Esanity);
	}

	/*
	 *	NOTE: The arrays created by ffi2code have a kNoisyIrNodeTypeMax element at the end of each sub-array
	 */
	for (int i = 0; i < kNoisyIrNodeTypeMax && gNewtonFollows[productionOrToken][i] != kNoisyIrNodeTypeMax; i++)
	{
		if (gNewtonFollows[productionOrToken][i] == token->type)
		{
			return true;
		}
	}

	return false;
}
