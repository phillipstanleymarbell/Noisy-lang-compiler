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
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisy-timeStamps.h"
#include "noisy.h"


/*
 *	TODO: Places where we need the string form of a NoisyIrNodeType should also use gNoisyAstNodeStrings[], not
 *	gProductionStrings[] and gTerminalStrings[]. Get rid of gProductionStrings[] and gTokenStrings[]
 */

const char *	gTerminalStrings[kNoisyIrNodeTypeMax] = {
								[kNoisyIrNodeType_Tadt]			=	"NoisyIrNodeType_Tadt",
								[kNoisyIrNodeType_Talpha]		=	"NoisyIrNodeType_Talpha",
								[kNoisyIrNodeType_Tampersand]		=	"NoisyIrNodeType_Tampersand",
								[kNoisyIrNodeType_Tand]			=	"NoisyIrNodeType_Tand",
								[kNoisyIrNodeType_TandAs]		=	"NoisyIrNodeType_TandAs",
								[kNoisyIrNodeType_Tarray]		=	"NoisyIrNodeType_Tarray",
								[kNoisyIrNodeType_Tas]			=	"NoisyIrNodeType_Tas",
								[kNoisyIrNodeType_Tasterisk]		=	"NoisyIrNodeType_Tasterisk",
								[kNoisyIrNodeType_Tbang]		=	"NoisyIrNodeType_Tbang",
								[kNoisyIrNodeType_Tbool]		=	"NoisyIrNodeType_Tbool",
								[kNoisyIrNodeType_TboolConst]		=	"NoisyIrNodeType_TboolConst",
								[kNoisyIrNodeType_Tbyte]		=	"NoisyIrNodeType_Tbyte",
								[kNoisyIrNodeType_Tcaret]		=	"NoisyIrNodeType_Tcaret",
								[kNoisyIrNodeType_Tchan]		=	"NoisyIrNodeType_Tchan",
								[kNoisyIrNodeType_Tchan2name]		=	"NoisyIrNodeType_Tchan2name",
								[kNoisyIrNodeType_TchanWrite]		=	"NoisyIrNodeType_TchanWrite",
								[kNoisyIrNodeType_TcharConst]		=	"NoisyIrNodeType_TcharConst",
								[kNoisyIrNodeType_Tcolon]		=	"NoisyIrNodeType_Tcolon",
								[kNoisyIrNodeType_Tcomma]		=	"NoisyIrNodeType_Tcomma",
								[kNoisyIrNodeType_Tcons]		=	"NoisyIrNodeType_Tcons",
								[kNoisyIrNodeType_Tconst]		=	"NoisyIrNodeType_Tconst",
								[kNoisyIrNodeType_TdefineAs]		=	"NoisyIrNodeType_TdefineAs",
								[kNoisyIrNodeType_Tdec]			=	"NoisyIrNodeType_Tdec",
								[kNoisyIrNodeType_Tdiv]			=	"NoisyIrNodeType_Tdiv",
								[kNoisyIrNodeType_TdivAs]		=	"NoisyIrNodeType_TdivAs",
								[kNoisyIrNodeType_Tdot]			=	"NoisyIrNodeType_Tdot",
								[kNoisyIrNodeType_TdoubleQuote]		=	"NoisyIrNodeType_TdoubleQuote",
								[kNoisyIrNodeType_Tepsilon]		=	"NoisyIrNodeType_Tepsilon",
								[kNoisyIrNodeType_Teq]			=	"NoisyIrNodeType_Teq",
								[kNoisyIrNodeType_Terasures]		=	"NoisyIrNodeType_Terasures",
								[kNoisyIrNodeType_Terrors]		=	"NoisyIrNodeType_Terrors",
								[kNoisyIrNodeType_Tfalse]		=	"NoisyIrNodeType_Tfalse",
								[kNoisyIrNodeType_Tfixed]		=	"NoisyIrNodeType_Tfixed",
								[kNoisyIrNodeType_Tge]			=	"NoisyIrNodeType_Tge",
								[kNoisyIrNodeType_Tgets]		=	"NoisyIrNodeType_Tgets",
								[kNoisyIrNodeType_Tgoes]		=	"NoisyIrNodeType_Tgoes",
								[kNoisyIrNodeType_Tgt]			=	"NoisyIrNodeType_Tgt",
								[kNoisyIrNodeType_Thd]			=	"NoisyIrNodeType_Thd",
								[kNoisyIrNodeType_Tidentifier]		=	"NoisyIrNodeType_Tidentifier",
								[kNoisyIrNodeType_Tinc]			=	"NoisyIrNodeType_Tinc",
								[kNoisyIrNodeType_Tint]			=	"NoisyIrNodeType_Tint",
								[kNoisyIrNodeType_TintConst]		=	"NoisyIrNodeType_TintConst",
								[kNoisyIrNodeType_Titer]		=	"NoisyIrNodeType_Titer",
								[kNoisyIrNodeType_Tlatency]		=	"NoisyIrNodeType_Tlatency",
								[kNoisyIrNodeType_TleftBrac]		=	"NoisyIrNodeType_TleftBrac",
								[kNoisyIrNodeType_TleftBrace]		=	"NoisyIrNodeType_TleftBrace",
								[kNoisyIrNodeType_Tle]			=	"NoisyIrNodeType_Tle",
								[kNoisyIrNodeType_Tlen]			=	"NoisyIrNodeType_Tlen",
								[kNoisyIrNodeType_Tlist]		=	"NoisyIrNodeType_Tlist",
								[kNoisyIrNodeType_TleftParen]		=	"NoisyIrNodeType_TleftParen",
								[kNoisyIrNodeType_TleftShift]		=	"NoisyIrNodeType_TleftShift",
								[kNoisyIrNodeType_TleftShiftAs]		=	"NoisyIrNodeType_TleftShiftAs",
								[kNoisyIrNodeType_Tlt]			=	"NoisyIrNodeType_Tlt",
								[kNoisyIrNodeType_Tmatch]		=	"NoisyIrNodeType_Tmatch",
								[kNoisyIrNodeType_TmatchSeq]		=	"NoisyIrNodeType_TmatchSeq",
								[kNoisyIrNodeType_Tminus]		=	"NoisyIrNodeType_Tminus",
								[kNoisyIrNodeType_TmodAs]		=	"NoisyIrNodeType_TmodAs",
								[kNoisyIrNodeType_TmulAs]		=	"NoisyIrNodeType_TmulAs",
								[kNoisyIrNodeType_Tname2chan]		=	"NoisyIrNodeType_Tname2chan",
								[kNoisyIrNodeType_Tnamegen]		=	"NoisyIrNodeType_Tnamegen",
								[kNoisyIrNodeType_Tneq]			=	"NoisyIrNodeType_Tneq",
								[kNoisyIrNodeType_Tnil]			=	"NoisyIrNodeType_Tnil",
								[kNoisyIrNodeType_Tnybble]		=	"NoisyIrNodeType_Tnybble",
								[kNoisyIrNodeType_Tof]			=	"NoisyIrNodeType_Tof",
								[kNoisyIrNodeType_Tor]			=	"NoisyIrNodeType_Tor",
								[kNoisyIrNodeType_TorAs]		=	"NoisyIrNodeType_TorAs",
								[kNoisyIrNodeType_Tpercent]		=	"NoisyIrNodeType_Tpercent",
								[kNoisyIrNodeType_Tplus]		=	"NoisyIrNodeType_Tplus",
								[kNoisyIrNodeType_Tprogtype]		=	"NoisyIrNodeType_Tprogtype",
								[kNoisyIrNodeType_TprogtypeQualifier]	=	"kNoisyIrNodeType_TprogtypeQualifier",
								[kNoisyIrNodeType_TrightBrac]		=	"NoisyIrNodeType_TrightBrac",
								[kNoisyIrNodeType_TrightBrace]		=	"NoisyIrNodeType_TrightBrace",
								[kNoisyIrNodeType_Treal]		=	"NoisyIrNodeType_Treal",
								[kNoisyIrNodeType_TrealConst]		=	"NoisyIrNodeType_TrealConst",
								[kNoisyIrNodeType_TrightParen]		=	"NoisyIrNodeType_TrightParen",
								[kNoisyIrNodeType_TrightShift]		=	"NoisyIrNodeType_TrightShift",
								[kNoisyIrNodeType_TrightShiftAs]	=	"NoisyIrNodeType_TrightShiftAs",
								[kNoisyIrNodeType_Tsemicolon]		=	"NoisyIrNodeType_Tsemicolon",
								[kNoisyIrNodeType_Tset]			=	"NoisyIrNodeType_Tset",
								[kNoisyIrNodeType_TsingleQuote]		=	"NoisyIrNodeType_TsingleQuote",
								[kNoisyIrNodeType_TstringConst]		=	"NoisyIrNodeType_TstringConst",
								[kNoisyIrNodeType_Tstring]		=	"NoisyIrNodeType_Tstring",
								[kNoisyIrNodeType_Tstroke]		=	"NoisyIrNodeType_Tstroke",
								[kNoisyIrNodeType_TsubAs]		=	"NoisyIrNodeType_TsubAs",
								[kNoisyIrNodeType_Ttau]			=	"NoisyIrNodeType_Ttau",
								[kNoisyIrNodeType_Ttilde]		=	"NoisyIrNodeType_Ttilde",
								[kNoisyIrNodeType_Ttl]			=	"NoisyIrNodeType_Ttl",
								[kNoisyIrNodeType_Ttrue]		=	"NoisyIrNodeType_Ttrue",
								[kNoisyIrNodeType_Ttype]		=	"NoisyIrNodeType_Ttype",
								[kNoisyIrNodeType_Tvar2name]		=	"NoisyIrNodeType_Tvar2name",
								[kNoisyIrNodeType_TxorAs]		=	"NoisyIrNodeType_TxorAs",
								[kNoisyIrNodeType_ZbadCharConst]	=	"NoisyIrNodeType_ZbadCharConst",
								[kNoisyIrNodeType_ZbadStringConst]	=	"NoisyIrNodeType_ZbadStringConst",
								[kNoisyIrNodeType_ZbadIdentifier]	=	"NoisyIrNodeType_ZbadIdentifier",
								[kNoisyIrNodeType_Zepsilon]		=	"NoisyIrNodeType_Zepsilon",
								[kNoisyIrNodeType_Zeof]			=	"NoisyIrNodeType_Zeof",
							};


const char *	gReservedTokenDescriptions[kNoisyIrNodeTypeMax] = {
									[kNoisyIrNodeType_TaddAs]		=	"'+='",
									[kNoisyIrNodeType_Tadt]			=	"'adt'",
									[kNoisyIrNodeType_Talpha]		=	"'alpha'",
									//TODO: put the rest of the cases where we mean the literal string, in single quotes (but not, e.g., in case of "Bolean constant")
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



/*
 *	Some classes of characters used in Lexer
 */
const NoisyIrNodeType gNoisyReservedTokens[] = {
							kNoisyIrNodeType_Ttilde,
							kNoisyIrNodeType_Tbang,
							kNoisyIrNodeType_Tpercent,
							kNoisyIrNodeType_Tcaret,
							kNoisyIrNodeType_Tampersand,
							kNoisyIrNodeType_Tasterisk,
							kNoisyIrNodeType_TleftParen,
							kNoisyIrNodeType_TrightParen,
							kNoisyIrNodeType_Tminus,
							kNoisyIrNodeType_Tplus,
							kNoisyIrNodeType_Tas,
							kNoisyIrNodeType_Tdiv,
							kNoisyIrNodeType_Tgt,
							kNoisyIrNodeType_Tlt,
							kNoisyIrNodeType_Tsemicolon,
							kNoisyIrNodeType_Tcolon,
							kNoisyIrNodeType_TsingleQuote,
							kNoisyIrNodeType_TdoubleQuote,
							kNoisyIrNodeType_TleftBrace,
							kNoisyIrNodeType_TrightBrace,
							kNoisyIrNodeType_TleftBrac,
							kNoisyIrNodeType_TrightBrac,
							kNoisyIrNodeType_Tstroke,
							kNoisyIrNodeType_Tgets,
							kNoisyIrNodeType_Tdot,
							kNoisyIrNodeType_Tcomma,
							kNoisyIrNodeType_Tle,
							kNoisyIrNodeType_Tge,
							kNoisyIrNodeType_TxorAs,
							kNoisyIrNodeType_TorAs,
							kNoisyIrNodeType_TandAs,
							kNoisyIrNodeType_TmodAs,
							kNoisyIrNodeType_TdivAs,
							kNoisyIrNodeType_TmulAs,
							kNoisyIrNodeType_TsubAs,
							kNoisyIrNodeType_TaddAs,
							kNoisyIrNodeType_TdefineAs,
							kNoisyIrNodeType_Tneq,
							kNoisyIrNodeType_TrightShift,
							kNoisyIrNodeType_TrightShiftAs,
							kNoisyIrNodeType_TleftShift,
							kNoisyIrNodeType_TleftShiftAs,
							kNoisyIrNodeType_Tand,
							kNoisyIrNodeType_Tor,
							kNoisyIrNodeType_Tcons,
							kNoisyIrNodeType_Teq,
							kNoisyIrNodeType_Tinc,
							kNoisyIrNodeType_Tdec,
							kNoisyIrNodeType_TchanWrite,
							kNoisyIrNodeType_Tgoes,
							kNoisyIrNodeType_TprogtypeQualifier,
						};



