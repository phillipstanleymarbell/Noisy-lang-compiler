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
 *	This array is used both in the lexer for two purposes: (1) match keywords in order
 *	to return a kNewtonIrNodeType_Tabcd, as well as to print descriptions of keywords.
 */
const char *	gNoisyTokenDescriptions[kNoisyIrNodeTypeMax] = {
									/*
									 *	End keywords
									 */
									[kNoisyIrNodeType_TA]				=	"A",
									[kNoisyIrNodeType_TAmpere]			=	"ampere",
									[kNoisyIrNodeType_TK]				=	"K",
									[kNoisyIrNodeType_TKelvin]			=	"kelvin",
									[kNoisyIrNodeType_Tacceleration]		=	"acceleration",
									[kNoisyIrNodeType_Tadt]				=	"adt",
									[kNoisyIrNodeType_Talpha]			=	"alpha",
									[kNoisyIrNodeType_TandAssign]			=	"&=",
									[kNoisyIrNodeType_Tand]				=	"and",
									[kNoisyIrNodeType_Tanglerate]			=	"anglerate",
									[kNoisyIrNodeType_TarithmeticAnd]		=	"&",
									[kNoisyIrNodeType_Tarray]			=	"array",
									[kNoisyIrNodeType_Tarrow]			=	"->",
									[kNoisyIrNodeType_Tassign]			=	"=",
									[kNoisyIrNodeType_TasteriskAssign]		=	"*=",
									[kNoisyIrNodeType_Tasterisk]			=	"*",
									[kNoisyIrNodeType_Tat]				=	"@",
									[kNoisyIrNodeType_TbitwiseOr]			=	"|",
									[kNoisyIrNodeType_Tbool]			=	"bool",
									[kNoisyIrNodeType_Tcandela]			=	"candela",
									[kNoisyIrNodeType_Tcardinality]			=	"cardinality",
									[kNoisyIrNodeType_Tcd]				=	"cd",
									[kNoisyIrNodeType_Tcentralmoment]		=	"centralmoment",
									[kNoisyIrNodeType_TchannelOperatorAssign]	=	"<-=",
									[kNoisyIrNodeType_TchannelOperator]		=	"<-",
									[kNoisyIrNodeType_TcolonAssign]			=	":=",
									[kNoisyIrNodeType_TcolonColon]			=	"::",
									[kNoisyIrNodeType_Tcolon]			=	":",
									[kNoisyIrNodeType_Tcomma]			=	",",
									[kNoisyIrNodeType_Tcomplement]			=	"complement",
									[kNoisyIrNodeType_Tcomplex]			=	"complex",
									[kNoisyIrNodeType_Tconst]			=	"const",
									[kNoisyIrNodeType_Tcrossproduct]		=	"crossproduct",
									[kNoisyIrNodeType_Tcurrent]			=	"current",
									[kNoisyIrNodeType_Tdimensions]			=	"dimensions",
									[kNoisyIrNodeType_Tdistance]			=	"distance",
									[kNoisyIrNodeType_Tdistortions]			=	"distortions",
									[kNoisyIrNodeType_TdivideAssign]		=	"/=",
									[kNoisyIrNodeType_Tdivide]			=	"/",
									[kNoisyIrNodeType_Tdot]				=	".",
									[kNoisyIrNodeType_Tdotproduct]			=	"dotproduct",
									[kNoisyIrNodeType_Tepsilon]			=	"epsilon",
									[kNoisyIrNodeType_Tequals]			=	"==",
									[kNoisyIrNodeType_Terasures]			=	"erasures",
									[kNoisyIrNodeType_Texists]			=	"exists",
									[kNoisyIrNodeType_Tfalse]			=	"false",
									[kNoisyIrNodeType_Tfixed]			=	"fixed",
									[kNoisyIrNodeType_Tfloat128]			=	"float128",
									[kNoisyIrNodeType_Tfloat16]			=	"float16",
									[kNoisyIrNodeType_Tfloat32]			=	"float32",
									[kNoisyIrNodeType_Tfloat4]			=	"float4",
									[kNoisyIrNodeType_Tfloat64]			=	"float64",
									[kNoisyIrNodeType_Tfloat8]			=	"float8",
									[kNoisyIrNodeType_Tfor]				=	"for",
									[kNoisyIrNodeType_Tforall]			=	"forall",
									[kNoisyIrNodeType_Tfourier]			=	"fourier",
									[kNoisyIrNodeType_Tfrequencies]			=	"frequencies",
									[kNoisyIrNodeType_Tfrom]			=	"from",
									[kNoisyIrNodeType_Tfunction]			=	"function",
									[kNoisyIrNodeType_Tgiven]			=	"given",
									[kNoisyIrNodeType_TgreaterThanEqual]		=	">=",
									[kNoisyIrNodeType_TgreaterThan]			=	">",
									[kNoisyIrNodeType_Thead]			=	"head",
									[kNoisyIrNodeType_Thighpass]			=	"highpass",
									[kNoisyIrNodeType_Tiff]				=	"<=>",
									[kNoisyIrNodeType_Timplies]			=	"=>",
									[kNoisyIrNodeType_Tin]				=	"in",
									[kNoisyIrNodeType_Tint128]			=	"int128",
									[kNoisyIrNodeType_Tint16]			=	"int16",
									[kNoisyIrNodeType_Tint32]			=	"int32",
									[kNoisyIrNodeType_Tint4]			=	"int4",
									[kNoisyIrNodeType_Tint64]			=	"int64",
									[kNoisyIrNodeType_Tint8]			=	"int8",
									[kNoisyIrNodeType_TisPermutationOf]		=	">=<",
									[kNoisyIrNodeType_Titerate]			=	"iterate",
									[kNoisyIrNodeType_Tkg]				=	"kg",
									[kNoisyIrNodeType_Tkilogram]			=	"kilogram",
									[kNoisyIrNodeType_Tlatency]			=	"latency",
									[kNoisyIrNodeType_TleftBrace]			=	"{",
									[kNoisyIrNodeType_TleftBracket]			=	"[]",
									[kNoisyIrNodeType_TleftParens]			=	"(",
									[kNoisyIrNodeType_TleftShiftAssign]		=	"<<=",
									[kNoisyIrNodeType_TleftShift]			=	"<<",
									[kNoisyIrNodeType_Tlength]			=	"length",
									[kNoisyIrNodeType_TlessThanEqual]		=	"<=",
									[kNoisyIrNodeType_TlessThan]			=	"<",
									[kNoisyIrNodeType_Tlist]			=	"list",
									[kNoisyIrNodeType_Tload]			=	"load",
									[kNoisyIrNodeType_Tlog]				=	"log",
									[kNoisyIrNodeType_TlogicalAnd]			=	"&&",
									[kNoisyIrNodeType_TlogicalOr]			=	"||",
									[kNoisyIrNodeType_Tlowpass]			=	"lowpass",
									[kNoisyIrNodeType_Tluminosity]			=	"luminosity",
									[kNoisyIrNodeType_Tm]				=	"m",
									[kNoisyIrNodeType_Tmagneticfluxdensity]		=	"magneticfluxdensity",
									[kNoisyIrNodeType_Tmagnitudes]			=	"magnitudes",
									[kNoisyIrNodeType_Tmass]			=	"mass",
									[kNoisyIrNodeType_Tmatch]			=	"match",
									[kNoisyIrNodeType_Tmatchseq]			=	"matchseq",
									[kNoisyIrNodeType_Tmaterial]			=	"material",
									[kNoisyIrNodeType_Tmax]				=	"max",
									[kNoisyIrNodeType_Tmeasurement]			=	"measurement",
									[kNoisyIrNodeType_Tmeter]			=	"meter",
									[kNoisyIrNodeType_Tmin]				=	"min",
									[kNoisyIrNodeType_TminusAssign]			=	"-=",
									[kNoisyIrNodeType_TminusMinus]			=	"--",
									[kNoisyIrNodeType_Tminus]			=	"-",
									[kNoisyIrNodeType_Tmodule]			=	"module",
									[kNoisyIrNodeType_Tmole]			=	"mole",
									[kNoisyIrNodeType_Tnat128]			=	"nat128",
									[kNoisyIrNodeType_Tnat16]			=	"nat16",
									[kNoisyIrNodeType_Tnat32]			=	"nat32",
									[kNoisyIrNodeType_Tnat4]			=	"nat4",
									[kNoisyIrNodeType_Tnat64]			=	"nat64",
									[kNoisyIrNodeType_Tnat8]			=	"nat8",
									[kNoisyIrNodeType_Tnil]				=	"nil",
									[kNoisyIrNodeType_TnotEqual]			=	"!=",
									[kNoisyIrNodeType_Tnot]				=	"!",
									[kNoisyIrNodeType_Tnrt]				=	"nrt",
									[kNoisyIrNodeType_Tof]				=	"of",
									[kNoisyIrNodeType_Tomega]			=	"omega",
									[kNoisyIrNodeType_TorAssign]			=	"|=",
									[kNoisyIrNodeType_Tparallel]			=	"parallel",
									[kNoisyIrNodeType_Tpath]			=	"path",
									[kNoisyIrNodeType_TpercentAssign]		=	"%=",
									[kNoisyIrNodeType_Tpercent]			=	"%",
									[kNoisyIrNodeType_TplusAssign]			=	"+=",
									[kNoisyIrNodeType_TplusPlus]			=	"++",
									[kNoisyIrNodeType_Tplus]			=	"+",
									[kNoisyIrNodeType_Tpow]				=	"pow",
									[kNoisyIrNodeType_Tpowerset]			=	"powerset",
									[kNoisyIrNodeType_Tpredicate]			=	"predicate",
									[kNoisyIrNodeType_Tpressure]			=	"pressure",
									[kNoisyIrNodeType_Tprobdef]			=	"probdef",
									[kNoisyIrNodeType_Tproduct]			=	"product",
									[kNoisyIrNodeType_Tquantize]			=	"quantize",
									[kNoisyIrNodeType_Trational]			=	"rational",
									[kNoisyIrNodeType_Trelativehumidity]		=	"relativehumidity",
									[kNoisyIrNodeType_Treturn]			=	"return",
									[kNoisyIrNodeType_Treverse]			=	"reverse",
									[kNoisyIrNodeType_TrightBrace]			=	"}",
									[kNoisyIrNodeType_TrightBracket]		=	"]",
									[kNoisyIrNodeType_TrightParens]			=	")",
									[kNoisyIrNodeType_TrightShiftAssign]		=	">>=",
									[kNoisyIrNodeType_TrightShift]			=	">>",
									[kNoisyIrNodeType_Ts]				=	"s",
									[kNoisyIrNodeType_Tsample]			=	"sample",
									[kNoisyIrNodeType_Tsamples]			=	"samples",
									[kNoisyIrNodeType_Tsecond]			=	"second",
									[kNoisyIrNodeType_Tsemicolon]			=	";",
									[kNoisyIrNodeType_Tsequence]			=	"sequence",
									[kNoisyIrNodeType_TsetCross]			=	"><",
									[kNoisyIrNodeType_TsetIntersect]		=	"#",
									[kNoisyIrNodeType_Tset]				=	"set",
									[kNoisyIrNodeType_Tsigfigs]			=	"sigfigs",
									[kNoisyIrNodeType_Tsignal]			=	"signal",
									[kNoisyIrNodeType_Tsort]			=	"sort",
									[kNoisyIrNodeType_Tstring]			=	"string",
									[kNoisyIrNodeType_Tstrongdominates]		=	"strongdominates",
									[kNoisyIrNodeType_Tsum]				=	"sum",
									[kNoisyIrNodeType_Ttail]			=	"tail",
									[kNoisyIrNodeType_Ttailtip]			=	"tailtip",
									[kNoisyIrNodeType_Ttau]				=	"tau",
									[kNoisyIrNodeType_Ttderivative]			=	"tderivative",
									[kNoisyIrNodeType_Ttemperature]			=	"temperature",
									[kNoisyIrNodeType_Ttilde]			=	"~",
									[kNoisyIrNodeType_Ttime]			=	"time",
									[kNoisyIrNodeType_Ttimebase]			=	"timebase",
									[kNoisyIrNodeType_Ttimeseries]			=	"timeseries",
									[kNoisyIrNodeType_Ttintegral]			=	"tintegral",
									[kNoisyIrNodeType_Tto]				=	"to",
									[kNoisyIrNodeType_Ttrue]			=	"true",
									[kNoisyIrNodeType_Ttype]			=	"type",
									[kNoisyIrNodeType_Ttypeannote]			=	"typeannote",
									[kNoisyIrNodeType_Ttypemax]			=	"typemax",
									[kNoisyIrNodeType_Ttypemin]			=	"typemin",
									[kNoisyIrNodeType_Tuncertainty]			=	"uncertainty",
									[kNoisyIrNodeType_Tunits]			=	"units",
									[kNoisyIrNodeType_Tvalfn]			=	"valfn",
									[kNoisyIrNodeType_Tvector]			=	"vector",
									[kNoisyIrNodeType_Tweakdominates]		=	"weakdominates",
									[kNoisyIrNodeType_Twith]			=	"with",
									[kNoisyIrNodeType_TxorAssign]			=	"^=",
									[kNoisyIrNodeType_Txor]				=	"^",
									/*
									 *	End keywords
									 */


									/*
									 *	Begin tokens returned by lexer that are not literal keywords
									 */
									[kNoisyIrNodeType_TintegerConst]		=	"integer constant",
									[kNoisyIrNodeType_TcharConst]			=	"character constant",
									[kNoisyIrNodeType_TboolConst]			=	"Boolean constant (\"true\" or \"false\")",
									[kNoisyIrNodeType_Tidentifier]			=	"an identifier",
									[kNoisyIrNodeType_TstringConst]			=	"string constant",
									[kNoisyIrNodeType_TrealConst]			=	"real-valued constant",
									[kNoisyIrNodeType_TMax]				=	"maximum token value",
									[kNoisyIrNodeType_TMin]				=	"minimum token value",
									[kNoisyIrNodeType_ZbadCharConst]		=	"ill-formed character constant",
									[kNoisyIrNodeType_ZbadIdentifier]		=	"ill-formed identifier",
									[kNoisyIrNodeType_ZbadStringConst]		=	"ill-formed string constant",
									[kNoisyIrNodeType_Zeof]				=	"end of file",
									[kNoisyIrNodeType_Zepsilon]			=	"empty production",
									/*
									 *	End non-keyword tokens
									 */
};
