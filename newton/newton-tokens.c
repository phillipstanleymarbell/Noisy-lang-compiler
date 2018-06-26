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


const char *	gNewtonTokenDescriptions[kCommonIrNodeTypeMax] = {
									[kNewtonIrNodeType_TEnglish]		= "English",
									[kNewtonIrNodeType_TSpanish]		= "Spanish",
									[kNewtonIrNodeType_TatSign]		= "@",
									[kNewtonIrNodeType_Tcolon]		= ":",
									[kNewtonIrNodeType_Tcomma]		= ",",
									[kNewtonIrNodeType_Tconstant]		= "constant",
									[kNewtonIrNodeType_Tcross]		= "cross",
									[kNewtonIrNodeType_Tderivation]		= "derivation",
									[kNewtonIrNodeType_Tderivative]		= "derivative",
									[kNewtonIrNodeType_Tdiv]		= "/",
									[kNewtonIrNodeType_Tdot]		= "dot",
									[kNewtonIrNodeType_Tequals]		= "=",
									[kNewtonIrNodeType_Tequivalent]		= "~",
									[kNewtonIrNodeType_Texponent]		= "**",
									[kNewtonIrNodeType_Tge]			= ">=",
									[kNewtonIrNodeType_Tgt]			= ">",
									[kNewtonIrNodeType_Tidentifier]		= "identifier",
									[kNewtonIrNodeType_TintConst]		= "integer constant",
									[kNewtonIrNodeType_Tintegral]		= "integral",
									[kNewtonIrNodeType_Tinvariant]		= "invariant",
									[kNewtonIrNodeType_Tle]			= "<=",
									[kNewtonIrNodeType_TleftBrace]		= "{",
									[kNewtonIrNodeType_TleftParen]		= "(",
									[kNewtonIrNodeType_Tlt]			= "<",
									[kNewtonIrNodeType_Tminus]		= "-",
									[kNewtonIrNodeType_Tmul]		= "*",
									[kNewtonIrNodeType_Tname]		= "name",
									[kNewtonIrNodeType_Tnil]		= "nil",
									[kNewtonIrNodeType_Tnone]		= "none",
									[kNewtonIrNodeType_Tnumber]		= "number",
									[kNewtonIrNodeType_Tplus]		= "+",
									[kNewtonIrNodeType_Tproportionality]	= "@<",
									[kNewtonIrNodeType_TrealConst]		= "real-valued constant",
									[kNewtonIrNodeType_TrightBrace]		= "}",
									[kNewtonIrNodeType_TrightParen]		= ")",
									[kNewtonIrNodeType_Tsemicolon]		= ";",
									[kNewtonIrNodeType_Tsignal]		= "signal",
									[kNewtonIrNodeType_TstringConst]	= "string constant",
									[kNewtonIrNodeType_Tsymbol]		= "symbol",
									[kNewtonIrNodeType_Tto]			= "to",
									[kNewtonIrNodeType_ZbadIdentifier]	= "bad identifier",
									[kNewtonIrNodeType_ZbadStringConst]	= "bad string const",
									[kNewtonIrNodeType_Zeof]		= "end of file",
									[kNewtonIrNodeType_Zepsilon]		= "epsilon",
								};
