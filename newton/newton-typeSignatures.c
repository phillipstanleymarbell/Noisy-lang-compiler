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

#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-timeStamps.h"
#include "data-structures.h"



/*
 *	I just included all the nodes, they shouldn't all be here though
 */
const char gNewtonTypeNodeSignatures[kNoisyIrNodeTypeMax] = {
[	kNewtonIrNodeType_Tnil] = 'a',
[	kNewtonIrNodeType_Tnone] = 'b',
[    kNewtonIrNodeType_Tlt] = 'c',
[    kNewtonIrNodeType_Tle] = 'd',
[    kNewtonIrNodeType_Tgt] = 'e',
[    kNewtonIrNodeType_Tge] = 'f',
[    kNewtonIrNodeType_Tproportionality] = 'g',
[    kNewtonIrNodeType_Tequivalent] = 'h',
[    kNewtonIrNodeType_Tsemicolon] = 'i',
[    kNewtonIrNodeType_Tcolon] = 'j',
[    kNewtonIrNodeType_Tcomma] = 'k',
[    kNewtonIrNodeType_Tdot] = 'l',
[	kNewtonIrNodeType_Tdiv] = 'm',
[	kNewtonIrNodeType_Tmul] = 'n',
[	kNewtonIrNodeType_Tplus] = 'o',
[	kNewtonIrNodeType_Tminus] = 'p',
[    kNewtonIrNodeType_Texponent] = 'q',
[	kNewtonIrNodeType_Tequals] = 'r',
[    kNewtonIrNodeType_TintConst] = 's',
[    kNewtonIrNodeType_TrealConst] = 't',
[	kNewtonIrNodeType_TstringConst] = 'u',
[	kNewtonIrNodeType_Tcross] = 'v',
[	kNewtonIrNodeType_Tintegral] = 'w',
[	kNewtonIrNodeType_Tderivative] = 'x',
[	kNewtonIrNodeType_TSpanish] = 'y',
[	kNewtonIrNodeType_TEnglish] = 'z',
[	kNewtonIrNodeType_Tinvariant] = 'A',
[	kNewtonIrNodeType_Tconstant] = 'B',
[	kNewtonIrNodeType_Tsignal] = 'C',
[	kNewtonIrNodeType_Tderivation] = 'D',
[	kNewtonIrNodeType_Tsymbol] = 'E',
[	kNewtonIrNodeType_Tname] = 'F',
[	kNewtonIrNodeType_Pinteger] = 'G',
[	kNewtonIrNodeType_Tnumber] = 'H',
[	kNewtonIrNodeType_TrightBrace] = 'I',
[	kNewtonIrNodeType_TleftBrace] = 'J',
[	kNewtonIrNodeType_TrightParen] = 'K',
[	kNewtonIrNodeType_TleftParen] = 'L',
[	kNewtonIrNodeType_Tidentifier] = 'M',
[    kNewtonIrNodeType_PlanguageSetting] = 'N',
[	kNewtonIrNodeType_PcompareOp] = 'O',
[	kNewtonIrNodeType_PvectorOp] = 'P',
[	kNewtonIrNodeType_PhighPrecedenceBinaryOp] = 'Q',
[	kNewtonIrNodeType_PmidPrecedenceBinaryOp] = 'R',
[	kNewtonIrNodeType_PlowPrecedenceBinaryOp] = 'S',
[	kNewtonIrNodeType_PunaryOp] = 'T',
[	kNewtonIrNodeType_PtimeOp] = 'U',
[	kNewtonIrNodeType_Pquantity] = 'V',
[	kNewtonIrNodeType_PquantityFactor] = 'W',
[	kNewtonIrNodeType_PquantityTerm] = 'X',
[	kNewtonIrNodeType_PquantityExpression] = 'Y',
[	kNewtonIrNodeType_Pparameter] = 'Z',
[	kNewtonIrNodeType_PparameterTuple] = ',',
[	kNewtonIrNodeType_Pderivation] = '.',
[	kNewtonIrNodeType_Psymbol] = '%',
[	kNewtonIrNodeType_Pname] = '*',
[	kNewtonIrNodeType_Pconstraint] = '(',
[	kNewtonIrNodeType_PconstraintList] = ')',
[	kNewtonIrNodeType_PbaseSignal] = '-',
[	kNewtonIrNodeType_Pinvariant] = '_',
[	kNewtonIrNodeType_Pconstant] = '!',
[	kNewtonIrNodeType_Prule] = '@',
[	kNewtonIrNodeType_PruleList] = '+',
[	kNewtonIrNodeType_PnewtonFile] = '/',
								[kNoisyIrNodeType_Xseq]				= ':',
};

