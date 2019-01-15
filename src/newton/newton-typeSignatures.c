/*
	Authored 2017. Jonathan Lim

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
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"


/*
 *	This is misusing the intent of isType(). See issue #320.
 */
const char	gNewtonTypeNodeSignatures[kNewtonIrNodeTypeMax] = {
									[	kNewtonIrNodeType_PcomparisonOperator	] = 'O',
									[	kNewtonIrNodeType_Pconstraint		] = '(',
									[	kNewtonIrNodeType_PconstraintList	] = ')',
									[	kNewtonIrNodeType_PhighPrecedenceBinaryOp]= 'Q',
									[	kNewtonIrNodeType_PhighPrecedenceOperator] = 'R',
									[	kNewtonIrNodeType_PlanguageSetting	] = 'N',
									[	kNewtonIrNodeType_PlowPrecedenceBinaryOp] = 'S',
									[	kNewtonIrNodeType_Pparameter		] = 'Z',
									[	kNewtonIrNodeType_PparameterTuple	] = ',',
									[	kNewtonIrNodeType_Pquantity		] = 'V',
									[	kNewtonIrNodeType_PquantityExpression	] = 'Y',
									[	kNewtonIrNodeType_PquantityFactor	] = 'W',
									[	kNewtonIrNodeType_PquantityTerm		] = 'X',
									[	kNewtonIrNodeType_PunaryOp		] = 'T',
									[	kNewtonIrNodeType_PvectorOp		] = 'P',
									[	kNewtonIrNodeType_TEnglish		] = 'z',
									[	kNewtonIrNodeType_Tcolon		] = 'j',
									[	kNewtonIrNodeType_Tcomma		] = 'k',
									[	kNewtonIrNodeType_Tconstant		] = 'B',
									[	kNewtonIrNodeType_Tcross		] = 'v',
									[	kNewtonIrNodeType_Tderivation		] = 'D',
									[	kNewtonIrNodeType_Tderivative		] = 'x',
									[	kNewtonIrNodeType_Tdimensionless	] = '0',
									[	kNewtonIrNodeType_Tdiv			] = 'm',
									[	kNewtonIrNodeType_Tdot			] = 'l',
									[	kNewtonIrNodeType_Tequals		] = 'r',
									[	kNewtonIrNodeType_Tge			] = 'f',
									[	kNewtonIrNodeType_Tgt			] = 'e',
									[	kNewtonIrNodeType_Tidentifier		] = 'M',
									[	kNewtonIrNodeType_TintegerConst		] = 's',
									[	kNewtonIrNodeType_Tintegral		] = 'w',
									[	kNewtonIrNodeType_Tinvariant		] = 'A',
									[	kNewtonIrNodeType_Tle			] = 'd',
									[	kNewtonIrNodeType_TleftBrace		] = 'J',
									[	kNewtonIrNodeType_TleftParen		] = 'L',
									[	kNewtonIrNodeType_Tlt			] = 'c',
									[	kNewtonIrNodeType_Tminus		] = 'p',
									[	kNewtonIrNodeType_Tmul			] = 'n',
									[	kNewtonIrNodeType_Tname			] = 'F',
									[	kNewtonIrNodeType_Tnil			] = 'a',
									[	kNewtonIrNodeType_Tnone			] = 'b',
									[	kNewtonIrNodeType_Tplus			] = 'o',
									[	kNewtonIrNodeType_TrealConst		] = 't',
									[	kNewtonIrNodeType_TrightBrace		] = 'I',
									[	kNewtonIrNodeType_TrightParen		] = 'K',
									[	kNewtonIrNodeType_Tsemicolon		] = 'i',
									[	kNewtonIrNodeType_Tsignal		] = 'C',
									[	kNewtonIrNodeType_TstringConst		] = 'u',
									[	kNewtonIrNodeType_Tsymbol		] = 'E',
									[	kNoisyIrNodeType_Xseq			] = ':',
								};

