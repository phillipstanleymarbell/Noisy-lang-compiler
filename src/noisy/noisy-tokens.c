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
 *	Strings representing the tokens, for debugging and error reporting. TODO: this needs to be cleaned up next in finishing up the language refresh.
 */
const char *	gNoisyTokenDescriptions[kNoisyIrNodeTypeMax] = {
	[kNoisyIrNodeType_TA]			= "",
	[kNoisyIrNodeType_TAmpere]		= "",
	[kNoisyIrNodeType_TK]			= "",
	[kNoisyIrNodeType_TKelvin]		= "",
	[kNoisyIrNodeType_TMax]			= "",
	[kNoisyIrNodeType_TMin]			= "",
	[kNoisyIrNodeType_Tacceleration]	= "",
	[kNoisyIrNodeType_Tadt]			=	"adt",
	[kNoisyIrNodeType_Talpha]		=	"alpha",
	[kNoisyIrNodeType_TandAssign]			= "&=",
	[kNoisyIrNodeType_Tand]			=	"&&",
	[kNoisyIrNodeType_Tanglerate]			= "",
	[kNoisyIrNodeType_TarithmeticAnd]			= "",
	[kNoisyIrNodeType_Tarray]		=	"array",
	[kNoisyIrNodeType_Tarrow]			= "",
	[kNoisyIrNodeType_Tassign]			= "",
	[kNoisyIrNodeType_TasteriskAssign]			= "",
	[kNoisyIrNodeType_Tasterisk]		=	"*",
	[kNoisyIrNodeType_Tat]			= "",
	[kNoisyIrNodeType_TbitwiseOr]			= "",
	[kNoisyIrNodeType_TboolConst]			= "Boolean constant (\"true\" or \"false\")",
	[kNoisyIrNodeType_Tbool]		=	"bool",
	[kNoisyIrNodeType_Tcandela]			= "",
	[kNoisyIrNodeType_Tcardinality]			= "",
	[kNoisyIrNodeType_Tcd]			= "",
	[kNoisyIrNodeType_Tcentralmoment]		= "",
	[kNoisyIrNodeType_TchannelOperatorAssign]			= "",
	[kNoisyIrNodeType_TchannelOperator]			= "",
	[kNoisyIrNodeType_TcolonAssign]			= "",
	[kNoisyIrNodeType_TcolonColon]			= "",
	[kNoisyIrNodeType_Tcolon]		=	":",
	[kNoisyIrNodeType_Tcomma]		=	",",
	[kNoisyIrNodeType_Tcomplex]			= "",
	[kNoisyIrNodeType_Tconst]		=	"const",
	[kNoisyIrNodeType_TcharConst]		=	"",
	[kNoisyIrNodeType_Tcrossproduct]			= "",
	[kNoisyIrNodeType_Tcurrent]			= "",
	[kNoisyIrNodeType_Tdimensions]			= "",
	[kNoisyIrNodeType_Tdistance]			= "",
	[kNoisyIrNodeType_Tdistortions]			= "",
	[kNoisyIrNodeType_TdivideAssign]			= "",
	[kNoisyIrNodeType_Tdivide]			= "",
	[kNoisyIrNodeType_Tdot]			=	".",
	[kNoisyIrNodeType_Tdotproduct]			= "",
	[kNoisyIrNodeType_Tepsilon]		=	"epsilon",
	[kNoisyIrNodeType_Tequals]			= "",
	[kNoisyIrNodeType_Terasures]		=	"erasures",
	[kNoisyIrNodeType_Texists]			= "",
	[kNoisyIrNodeType_Tfalse]		=	"false",
	[kNoisyIrNodeType_Tfixed]		=	"fixed",
	[kNoisyIrNodeType_Tfloat128]			= "",
	[kNoisyIrNodeType_Tfloat16]			= "",
	[kNoisyIrNodeType_Tfloat32]			= "",
	[kNoisyIrNodeType_Tfloat4]			= "",
	[kNoisyIrNodeType_Tfloat64]			= "",
	[kNoisyIrNodeType_Tfloat8]			= "",
	[kNoisyIrNodeType_Tforall]			= "",
	[kNoisyIrNodeType_Tfourier]			= "",
	[kNoisyIrNodeType_Tfrequencies]			= "",
	[kNoisyIrNodeType_Tfunction]			= "",
	[kNoisyIrNodeType_Tgiven]			= "",
	[kNoisyIrNodeType_TgreaterThanEqual]			= "",
	[kNoisyIrNodeType_TgreaterThan]			= "",
	[kNoisyIrNodeType_Thead]			= "",
	[kNoisyIrNodeType_Thighpass]			= "",
	[kNoisyIrNodeType_Tidentifier]		=	"identifier",
	[kNoisyIrNodeType_Tiff]			= "",
	[kNoisyIrNodeType_Timplies]			= "",
	[kNoisyIrNodeType_Tin]			= "",
	[kNoisyIrNodeType_Tint128]			= "",
	[kNoisyIrNodeType_Tint16]			= "",
	[kNoisyIrNodeType_Tint32]			= "",
	[kNoisyIrNodeType_Tint4]			= "",
	[kNoisyIrNodeType_Tint64]			= "",
	[kNoisyIrNodeType_Tint8]			= "",
	[kNoisyIrNodeType_TintegerConst]			= "",
	[kNoisyIrNodeType_TisPermutationOf]			= "",
	[kNoisyIrNodeType_Titerate]			= "",
	[kNoisyIrNodeType_Tkg]			= "",
	[kNoisyIrNodeType_Tkilogram]			= "",
	[kNoisyIrNodeType_Tlatency]		=	"latency",
	[kNoisyIrNodeType_TleftBrace]		=	"{",
	[kNoisyIrNodeType_TleftBracket]			= "",
	[kNoisyIrNodeType_TleftParens]			= "",
	[kNoisyIrNodeType_TleftShiftAssign]			= "",
	[kNoisyIrNodeType_TleftShift]		=	"<<",
	[kNoisyIrNodeType_Tlength]			= "",
	[kNoisyIrNodeType_TlessThanEqual]			= "",
	[kNoisyIrNodeType_TlessThan]			= "",
	[kNoisyIrNodeType_Tlist]		=	"list",
	[kNoisyIrNodeType_Tload]			= "",
	[kNoisyIrNodeType_TlogicalAnd]			= "",
	[kNoisyIrNodeType_TlogicalOr]			= "",
	[kNoisyIrNodeType_Tlowpass]			= "",
	[kNoisyIrNodeType_Tluminosity]			= "",
	[kNoisyIrNodeType_Tm]			= "",
	[kNoisyIrNodeType_Tmagneticfluxdensity]			= "",
	[kNoisyIrNodeType_Tmagnitudes]			= "",
	[kNoisyIrNodeType_Tmass]			= "",
	[kNoisyIrNodeType_Tmatch]		=	"match",
	[kNoisyIrNodeType_Tmatchseq]			= "",
	[kNoisyIrNodeType_Tmaterial]			= "",
	[kNoisyIrNodeType_Tmeasurement]			= "",
	[kNoisyIrNodeType_Tmeter]			= "",
	[kNoisyIrNodeType_TminusAssign]			= "",
	[kNoisyIrNodeType_TminusMinus]			= "",
	[kNoisyIrNodeType_Tminus]		=	"-",
	[kNoisyIrNodeType_Tmodule]			= "",
	[kNoisyIrNodeType_Tmole]			= "",
	[kNoisyIrNodeType_Tnat128]			= "",
	[kNoisyIrNodeType_Tnat16]			= "",
	[kNoisyIrNodeType_Tnat32]			= "",
	[kNoisyIrNodeType_Tnat4]			= "",
	[kNoisyIrNodeType_Tnat64]			= "",
	[kNoisyIrNodeType_Tnat8]			= "",
	[kNoisyIrNodeType_Tnil]			=	"nil",
	[kNoisyIrNodeType_TnotEqual]			= "",
	[kNoisyIrNodeType_Tnot]			= "",
	[kNoisyIrNodeType_Tof]			=	"of",
	[kNoisyIrNodeType_Tomega]			= "",
	[kNoisyIrNodeType_TorAssign]			= "",
	[kNoisyIrNodeType_Tparallel]			= "",
	[kNoisyIrNodeType_Tpath]			= "",
	[kNoisyIrNodeType_TpercentAssign]			= "",
	[kNoisyIrNodeType_Tpercent]		=	"%",
	[kNoisyIrNodeType_TplusAssign]			= "",
	[kNoisyIrNodeType_TplusPlus]			= "",
	[kNoisyIrNodeType_Tplus]		=	"+",
	[kNoisyIrNodeType_Tpredicate]			= "",
	[kNoisyIrNodeType_Tpressure]			= "",
	[kNoisyIrNodeType_Tprobdef]			= "",
	[kNoisyIrNodeType_Tquantize]			= "",
	[kNoisyIrNodeType_Trational]			= "",
	[kNoisyIrNodeType_TrealConst]		=	"real-valued constant",
	[kNoisyIrNodeType_Trelativehumidity]			= "",
	[kNoisyIrNodeType_Treturn]			= "",
	[kNoisyIrNodeType_Treverse]			= "",
	[kNoisyIrNodeType_TrightBrace]		=	"}",
	[kNoisyIrNodeType_TrightBracket]			= "",
	[kNoisyIrNodeType_TrightParens]			= "",
	[kNoisyIrNodeType_TrightShiftAssign]			= "",
	[kNoisyIrNodeType_TrightShift]		=	">>",
	[kNoisyIrNodeType_Ts]			= "",
	[kNoisyIrNodeType_Tsample]			= "",
	[kNoisyIrNodeType_Tsamples]			= "",
	[kNoisyIrNodeType_Tsecond]			= "",
	[kNoisyIrNodeType_Tsemicolon]		=	";",
	[kNoisyIrNodeType_Tsequence]			= "",
	[kNoisyIrNodeType_TsetCross]			= "",
	[kNoisyIrNodeType_TsetIntersect]			= "",
	[kNoisyIrNodeType_Tset]			=	"set",
	[kNoisyIrNodeType_Tsigfigs]			= "",
	[kNoisyIrNodeType_Tsignal]			= "",
	[kNoisyIrNodeType_Tsort]			= "",
	[kNoisyIrNodeType_TstringConst]		=	"string constant",
	[kNoisyIrNodeType_Tstring]		=	"string",
	[kNoisyIrNodeType_Ttail]			= "",
	[kNoisyIrNodeType_Ttailtip]			= "",
	[kNoisyIrNodeType_Ttau]			=	"tau",
	[kNoisyIrNodeType_Ttderivative]			= "",
	[kNoisyIrNodeType_Ttemperature]			= "",
	[kNoisyIrNodeType_Ttilde]		=	"~",
	[kNoisyIrNodeType_Ttime]			= "",
	[kNoisyIrNodeType_Ttimebase]			= "",
	[kNoisyIrNodeType_Ttimeseries]			= "",
	[kNoisyIrNodeType_Ttintegral]			= "",
	[kNoisyIrNodeType_Ttrue]		=	"true",
	[kNoisyIrNodeType_Ttype]		=	"type",
	[kNoisyIrNodeType_Ttypeannote]			= "",
	[kNoisyIrNodeType_Ttypemax]			= "",
	[kNoisyIrNodeType_Ttypemin]			= "",
	[kNoisyIrNodeType_Tunits]			= "",
	[kNoisyIrNodeType_Tvalfn]		=	"",
	[kNoisyIrNodeType_Tvector]		=	"vector",
	[kNoisyIrNodeType_TxorAssign]		=	"^=",
	[kNoisyIrNodeType_Txor]			=	"^",
	[kNoisyIrNodeType_ZbadCharConst]	=	"ill-formed character constant",
	[kNoisyIrNodeType_ZbadIdentifier]	=	"ill-formed identifier",
	[kNoisyIrNodeType_ZbadStringConst]	=	"ill-formed string constant",
	[kNoisyIrNodeType_Zeof]			=	"end of file",
	[kNoisyIrNodeType_Zepsilon]		=	"empty production",

};
