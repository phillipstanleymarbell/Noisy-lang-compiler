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
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisy-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"



/*
 *	Type signatures of nodes that can appear in a subtree representing a type. TODO: This is incomplete given the language refresh.
 */
const char gNoisyTypeNodeSignatures[kNoisyIrNodeTypeMax] = {
								[kNoisyIrNodeType_PaccuracyTolerance]		= 'W',
								[kNoisyIrNodeType_PadtTypeDecl]			= 'A',
								[kNoisyIrNodeType_PanonAggregateType]		= 'G',
								[kNoisyIrNodeType_ParrayType]			= 'a',
								[kNoisyIrNodeType_PconstantDecl]		= 'L',
								[kNoisyIrNodeType_PfixedType]			= 'F',
								[kNoisyIrNodeType_PfunctionDecl]		= 'N',
								[kNoisyIrNodeType_PlatencyTolerance]		= 'Y',
								[kNoisyIrNodeType_PlistType]			= 'l',
								[kNoisyIrNodeType_PlossTolerance]		= 'X',
								[kNoisyIrNodeType_PtupleType]			= 'T',
								[kNoisyIrNodeType_PtypeDecl]			= 'P',
								[kNoisyIrNodeType_PtypeExpr]			= 'E',
								[kNoisyIrNodeType_PtypeName]			= 'M',
								[kNoisyIrNodeType_Tadt]				= 'd',
								[kNoisyIrNodeType_Talpha]			= 'h',
								[kNoisyIrNodeType_Tbool]			= 'o',
								[kNoisyIrNodeType_Tconst]			= 'c',
								[kNoisyIrNodeType_Tepsilon]			= 'e',
								[kNoisyIrNodeType_Tfixed]			= 'f',
								[kNoisyIrNodeType_Tfunction]			= 'g',
								[kNoisyIrNodeType_Tidentifier]			= 'n',
								[kNoisyIrNodeType_Tlist]			= 'l',
								[kNoisyIrNodeType_TrealConst]			= 'R',
								[kNoisyIrNodeType_Tset]				= 'z',
								[kNoisyIrNodeType_Tstring]			= 's',
								[kNoisyIrNodeType_Ttau]				= 't',
								[kNoisyIrNodeType_Ttype]			= 'p',
								[kNoisyIrNodeType_Xseq]				= ':',
};

