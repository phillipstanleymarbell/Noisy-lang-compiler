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
#include "noisy-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"



/*
 *	TODO:	Even though we would like to put the tokens in quotes, we cannot, 
 *	because we also use the array for checking to see if a token is a token.
 *	Create a separate array for the latter
 */
const char *	gNoisyTokenDescriptions[kNoisyIrNodeTypeMax] = {
									[kNoisyIrNodeType_TaddAs]		=	"+=",
									[kNoisyIrNodeType_Tadt]			=	"adt",
									[kNoisyIrNodeType_Talpha]		=	"alpha",
									[kNoisyIrNodeType_Tampersand]		=	"&",
									[kNoisyIrNodeType_Tand]			=	"&&",
									[kNoisyIrNodeType_TandAs]		=	"&=",
									[kNoisyIrNodeType_Tarray]		=	"array",
									[kNoisyIrNodeType_Tas]			=	"=",
									[kNoisyIrNodeType_Tasterisk]		=	"*",
									[kNoisyIrNodeType_Tbang]		=	"!",
									[kNoisyIrNodeType_Tbool]		=	"bool",
									[kNoisyIrNodeType_TboolConst]		=	"Boolean constant",
									[kNoisyIrNodeType_Tbyte]		=	"byte",
									[kNoisyIrNodeType_Tcaret]		=	"^",
									[kNoisyIrNodeType_Tchan]		=	"chan",
									[kNoisyIrNodeType_Tchan2name]		=	"chan2name",
									[kNoisyIrNodeType_TchanWrite]		=	"<-=",
									[kNoisyIrNodeType_TcharConst]		=	"character constant",
									[kNoisyIrNodeType_Tcolon]		=	":",
									[kNoisyIrNodeType_Tcomma]		=	",",
									[kNoisyIrNodeType_Tcons]		=	"::",
									[kNoisyIrNodeType_Tconst]		=	"const",
									[kNoisyIrNodeType_TdefineAs]		=	":=",
									[kNoisyIrNodeType_Tdec]			=	"--",
									[kNoisyIrNodeType_Tdiv]			=	"/",
									[kNoisyIrNodeType_TdivAs]		=	"/=",
									[kNoisyIrNodeType_Tdot]			=	".",
									[kNoisyIrNodeType_TdoubleQuote]		=	"\"",
									[kNoisyIrNodeType_Tepsilon]		=	"epsilon",
									[kNoisyIrNodeType_Teq]			=	"==",
									[kNoisyIrNodeType_Terasures]		=	"erasures",
									[kNoisyIrNodeType_Terrors]		=	"errors",
									[kNoisyIrNodeType_Tfalse]		=	"false",
									[kNoisyIrNodeType_Tfixed]		=	"fixed",
									[kNoisyIrNodeType_Tge]			=	">=",
									[kNoisyIrNodeType_Tgets]		=	"<-",
									[kNoisyIrNodeType_Tgoes]		=	"=>",
									[kNoisyIrNodeType_Tgt]			=	">",
									[kNoisyIrNodeType_Thd]			=	"hd",
									[kNoisyIrNodeType_Tidentifier]		=	"identifier",
									[kNoisyIrNodeType_Tinc]			=	"++",
									[kNoisyIrNodeType_Tint]			=	"int",
									[kNoisyIrNodeType_TintConst]		=	"integer constant",
									[kNoisyIrNodeType_Titer]		=	"iter",
									[kNoisyIrNodeType_Tlatency]		=	"latency",
									[kNoisyIrNodeType_TleftBrac]		=	"[",
									[kNoisyIrNodeType_TleftBrace]		=	"{",
									[kNoisyIrNodeType_Tle]			=	"<=",
									[kNoisyIrNodeType_Tlen]			=	"len",
									[kNoisyIrNodeType_Tlist]		=	"list",
									[kNoisyIrNodeType_TleftParen]		=	"(",
									[kNoisyIrNodeType_TleftShift]		=	"<<",
									[kNoisyIrNodeType_TleftShiftAs]		=	"<<=",
									[kNoisyIrNodeType_Tlt]			=	"<",
									[kNoisyIrNodeType_Tmatch]		=	"match",
									[kNoisyIrNodeType_TmatchSeq]		=	"matchseq",
									[kNoisyIrNodeType_Tminus]		=	"-",
									[kNoisyIrNodeType_TmodAs]		=	"%=",
									[kNoisyIrNodeType_TmulAs]		=	"*=",
									[kNoisyIrNodeType_Tname2chan]		=	"name2chan",
									[kNoisyIrNodeType_Tnamegen]		=	"namegen",
									[kNoisyIrNodeType_Tneq]			=	"!=",
									[kNoisyIrNodeType_Tnil]			=	"nil",
									[kNoisyIrNodeType_Tnybble]		=	"nybble",
									[kNoisyIrNodeType_Tof]			=	"of",
									[kNoisyIrNodeType_Tor]			=	"||",
									[kNoisyIrNodeType_TorAs]		=	"|=",
									[kNoisyIrNodeType_Tpercent]		=	"%",
									[kNoisyIrNodeType_Tplus]		=	"+",
									[kNoisyIrNodeType_Tprogtype]		=	"progtype",
									[kNoisyIrNodeType_TprogtypeQualifier]	=	"->",
									[kNoisyIrNodeType_TrightBrac]		=	"]",
									[kNoisyIrNodeType_TrightBrace]		=	"}",
									[kNoisyIrNodeType_Treal]		=	"real",
									[kNoisyIrNodeType_TrealConst]		=	"real-valued constant",
									[kNoisyIrNodeType_TrightParen]		=	")",
									[kNoisyIrNodeType_TrightShift]		=	">>",
									[kNoisyIrNodeType_TrightShiftAs]	=	">>=",
									[kNoisyIrNodeType_Tsemicolon]		=	";",
									[kNoisyIrNodeType_Tset]			=	"set",
									[kNoisyIrNodeType_TsingleQuote]		=	"'",
									[kNoisyIrNodeType_TstringConst]		=	"string constant",
									[kNoisyIrNodeType_Tstring]		=	"string",
									[kNoisyIrNodeType_Tstroke]		=	"|",
									[kNoisyIrNodeType_TsubAs]		=	"-=",
									[kNoisyIrNodeType_Ttau]			=	"tau",
									[kNoisyIrNodeType_Ttilde]		=	"~",
									[kNoisyIrNodeType_Ttl]			=	"tl",
									[kNoisyIrNodeType_Ttrue]		=	"true",
									[kNoisyIrNodeType_Ttype]		=	"type",
									[kNoisyIrNodeType_Tvar2name]		=	"var2name",
									[kNoisyIrNodeType_TxorAs]		=	"^=",
									[kNoisyIrNodeType_ZbadCharConst]	=	"ill-formed character constant",
									[kNoisyIrNodeType_ZbadIdentifier]	=	"ill-formed identifier",
									[kNoisyIrNodeType_ZbadStringConst]	=	"ill-formed string constant",
									[kNoisyIrNodeType_Zeof]			=	"end of file",
									[kNoisyIrNodeType_Zepsilon]		=	"empty production",
						};
