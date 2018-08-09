/*
	Authored 2015-2018. Phillip Stanley-Marbell.

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
 *	Type signatures of nodes that can appear in a subtree representing a type. Note that it includes things such as kNoisyIrNodeType_PmoduleDeclBody.
 */
const char gNoisyTypeNodeSignatures[kNoisyIrNodeTypeMax] = {
								[kNoisyIrNodeType_PaccuracyTolerance]		= 'a',
								[kNoisyIrNodeType_PadtTypeDecl]			= 'b',
								[kNoisyIrNodeType_PanonAggregateType]		= 'c',
								[kNoisyIrNodeType_ParrayType]			= 'd',
								[kNoisyIrNodeType_PcomplexType]			= 'e',
								[kNoisyIrNodeType_PconstantDecl]		= 'f',
								[kNoisyIrNodeType_PfixedType]			= 'g',
								[kNoisyIrNodeType_PfunctionDecl]		= 'h',
								[kNoisyIrNodeType_PlatencyTolerance]		= 'i',
								[kNoisyIrNodeType_PlistType]			= 'j',
								[kNoisyIrNodeType_PlossTolerance]		= 'k',
								[kNoisyIrNodeType_PmoduleDecl]			= 'l',
								[kNoisyIrNodeType_PrationalType]		= 'm',
								[kNoisyIrNodeType_PtupleType]			= 'n',
								[kNoisyIrNodeType_PtypeDecl]			= 'o',
								[kNoisyIrNodeType_PtypeExpr]			= 'p',
								[kNoisyIrNodeType_PtypeName]			= 'q',
								[kNoisyIrNodeType_PtypeParameterList]		= 'r',
								[kNoisyIrNodeType_Tadt]				= 's',
								[kNoisyIrNodeType_Talpha]			= 't',
								[kNoisyIrNodeType_Tbool]			= 'u',
								[kNoisyIrNodeType_Tconst]			= 'v',
								[kNoisyIrNodeType_Tepsilon]			= 'w',
								[kNoisyIrNodeType_Tfixed]			= 'x',
								[kNoisyIrNodeType_Tfloat128]			= 'y',
								[kNoisyIrNodeType_Tfloat16]			= 'z',
								[kNoisyIrNodeType_Tfloat32]			= 'A',
								[kNoisyIrNodeType_Tfloat4]			= 'B',
								[kNoisyIrNodeType_Tfloat64]			= 'C',
								[kNoisyIrNodeType_Tfloat8]			= 'D',
								[kNoisyIrNodeType_Tfunction]			= 'E',
								[kNoisyIrNodeType_Tidentifier]			= 'F',
								[kNoisyIrNodeType_Tint128]			= 'G',
								[kNoisyIrNodeType_Tint16]			= 'H',
								[kNoisyIrNodeType_Tint32]			= 'I',
								[kNoisyIrNodeType_Tint4]			= 'J',
								[kNoisyIrNodeType_Tint64]			= 'K',
								[kNoisyIrNodeType_Tint8]			= 'L',
								[kNoisyIrNodeType_Tlist]			= 'M',
								[kNoisyIrNodeType_Tnat128]			= 'N',
								[kNoisyIrNodeType_Tnat16]			= 'O',
								[kNoisyIrNodeType_Tnat32]			= 'P',
								[kNoisyIrNodeType_Tnat4]			= 'Q',
								[kNoisyIrNodeType_Tnat64]			= 'R',
								[kNoisyIrNodeType_Tnat8]			= 'S',
								[kNoisyIrNodeType_TrealConst]			= 'T',
								[kNoisyIrNodeType_Tset]				= 'U',
								[kNoisyIrNodeType_Tstring]			= 'V',
								[kNoisyIrNodeType_Ttau]				= 'W',
								[kNoisyIrNodeType_Ttype]			= 'X',
								[kNoisyIrNodeType_PbasicType]			= '1',
								[kNoisyIrNodeType_PintegerType]			= '2',
								[kNoisyIrNodeType_PwriteTypeSignature]		= '3',
								[kNoisyIrNodeType_Psignature]			= '4',
								[kNoisyIrNodeType_PreadTypeSignature]		= '5',
								[kNoisyIrNodeType_PmoduleDeclBody]		= '6',
								[kNoisyIrNodeType_PmoduleTypeNameDecl]		= '7',
								[kNoisyIrNodeType_PidentifierList]		= '8',
								[kNoisyIrNodeType_PrealType]			= '9',
								[kNoisyIrNodeType_TintegerConst]		= '0',
								[kNoisyIrNodeType_Ptolerance]			= '#',
								[kNoisyIrNodeType_Xseq]				= ':',
};

