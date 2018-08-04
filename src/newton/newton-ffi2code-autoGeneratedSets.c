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
#include "common-errors.h"
#include "version.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"

/*
 *	Generated by running
 *
 *	ffi2code-darwin-EN ./newton.ffi >> newton-ffi2code-autoGeneratedSets.c
 *
 *	(with some subsequent manual edits). In particular, FIRST(EOF) should be set to all tokens (ending in TypeMax, not in T_Max).
 */

int gNewtonFirsts[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax]  = {
            [kNewtonIrNodeType_PcompareOp    ]            = {
                                                                 kNewtonIrNodeType_Tlt,
                                                                 kNewtonIrNodeType_Tle,
                                                                 kNewtonIrNodeType_Tgt,
                                                                 kNewtonIrNodeType_Tge,
                                                                 kNewtonIrNodeType_Tproportionality,
                                                                 kNewtonIrNodeType_Tequivalent,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PvectorOp     ]            = {
                                                                 kNewtonIrNodeType_Tdot,
                                                                 kNewtonIrNodeType_Tcross,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PhighPrecedenceBinaryOp]            = {kNewtonIrNodeType_Texponent, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_PmidPrecedenceBinaryOp]            = {
                                                                 kNewtonIrNodeType_Tmul,
                                                                 kNewtonIrNodeType_Tdiv,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PlowPrecedenceBinaryOp]            = {
                                                                 kNewtonIrNodeType_Tminus,
                                                                 kNewtonIrNodeType_Tplus,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PunaryOp      ]            = {kNewtonIrNodeType_Tminus, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_PtimeOp       ]            = {
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_Pquantity     ]            = {
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PquantityFactor]            = {
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeType_TleftParen,
                                                                 kNewtonIrNodeType_PquantityExpression,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PquantityTerm ]            = {
                                                                 kNewtonIrNodeType_Tminus,
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeType_Tdot,
                                                                 kNewtonIrNodeType_Tcross,
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeType_TleftParen,
                                                                 kNewtonIrNodeType_Tplus,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PquantityExpression]            = {
                                                                 kNewtonIrNodeType_Tminus,
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeType_Tdot,
                                                                 kNewtonIrNodeType_Tcross,
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeType_TleftParen,
                                                                 kNewtonIrNodeType_Tplus,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_Pparameter    ]            = {kNewtonIrNodeType_Tidentifier, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_PparameterTuple]            = {kNewtonIrNodeType_TleftParen, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Psubindex    ]            = {kNewtonIrNodeType_Tidentifier, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_PsubindexTuple]            = {kNewtonIrNodeType_TleftParen, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Pderivation   ]            = {kNewtonIrNodeType_Tderivation, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Psymbol       ]            = {kNewtonIrNodeType_Tsymbol, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Pname         ]            = {kNewtonIrNodeType_Tname, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Pconstraint   ]            = {
                                                                 kNewtonIrNodeType_Tminus,
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeType_Tdot,
                                                                 kNewtonIrNodeType_Tcross,
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeType_TleftParen,
                                                                 kNewtonIrNodeType_Tplus,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PconstraintList]            = {
                                                                 kNewtonIrNodeType_Tminus,
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeType_Tdot,
                                                                 kNewtonIrNodeType_Tcross,
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeType_TleftParen,
                                                                 kNewtonIrNodeType_Tplus,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PbaseSignal   ]            = {kNewtonIrNodeType_Tidentifier, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Pinvariant    ]            = {kNewtonIrNodeType_Tidentifier, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Pconstant     ]            = {kNewtonIrNodeType_Tidentifier, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Pstatement         ]       = {kNewtonIrNodeType_Tinclude, kNewtonIrNodeType_Tidentifier, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_PstatementList     ]       = {kNewtonIrNodeType_Tinclude, kNewtonIrNodeType_Tidentifier, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_PnewtonFile   ]            = {kNewtonIrNodeType_Tidentifier, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Zeof]                      = {
                                                                kNewtonIrNodeType_TEnglish,
                                                                kNewtonIrNodeType_TSpanish,
                                                                kNewtonIrNodeType_TatSign,
                                                                kNewtonIrNodeType_Tcolon,
                                                                kNewtonIrNodeType_Tcomma,
                                                                kNewtonIrNodeType_Tconstant,
                                                                kNewtonIrNodeType_Tcross,
                                                                kNewtonIrNodeType_Tderivation,
                                                                kNewtonIrNodeType_Tderivative,
                                                                kNewtonIrNodeType_Tdiv,
                                                                kNewtonIrNodeType_Tdot,
                                                                kNewtonIrNodeType_Tequals,
                                                                kNewtonIrNodeType_Tequivalent,
                                                                kNewtonIrNodeType_Texponent,
                                                                kNewtonIrNodeType_Tge,
                                                                kNewtonIrNodeType_Tgt,
                                                                kNewtonIrNodeType_Tidentifier,
                                                                kNewtonIrNodeType_TintConst,
                                                                kNewtonIrNodeType_Tintegral,
                                                                kNewtonIrNodeType_Tinvariant,
                                                                kNewtonIrNodeType_Tle,
                                                                kNewtonIrNodeType_TleftBrace,
                                                                kNewtonIrNodeType_TleftParen,
                                                                kNewtonIrNodeType_Tlt,
                                                                kNewtonIrNodeType_Tminus,
                                                                kNewtonIrNodeType_Tmul,
                                                                kNewtonIrNodeType_Tname,
                                                                kNewtonIrNodeType_Tnil,
                                                                kNewtonIrNodeType_Tnone,
                                                                kNewtonIrNodeType_Tnumber,
                                                                kNewtonIrNodeType_Tplus,
                                                                kNewtonIrNodeType_Tproportionality,
                                                                kNewtonIrNodeType_TrealConst,	
                                                                kNewtonIrNodeType_TrightBrace,
                                                                kNewtonIrNodeType_TrightParen,
                                                                kNewtonIrNodeType_Tsemicolon,
                                                                kNewtonIrNodeType_Tsignal,
                                                                kNewtonIrNodeType_TstringConst,
                                                                kNewtonIrNodeType_Tsymbol,
                                                                kNewtonIrNodeType_Tto,
                                                                kNewtonIrNodeTypeMax
                                                            }
 };

int gNewtonFollows[kCommonIrNodeTypeMax][kCommonIrNodeTypeMax]  = {
            [kNewtonIrNodeType_PcompareOp    ]            = {
                                                                 kNewtonIrNodeType_Tminus,
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeType_Tdot,
                                                                 kNewtonIrNodeType_Tcross,
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeType_TleftParen,
                                                                 kNewtonIrNodeType_Tplus,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PvectorOp     ]            = {kNewtonIrNodeType_TleftParen, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_PhighPrecedenceBinaryOp]            = {
                                                                 kNewtonIrNodeType_Tminus,
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeType_Tdot,
                                                                 kNewtonIrNodeType_Tcross,
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeType_TleftParen,
                                                                 kNewtonIrNodeType_Tplus,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PmidPrecedenceBinaryOp]            = {
                                                                 kNewtonIrNodeType_Tminus,
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeType_Tdot,
                                                                 kNewtonIrNodeType_Tcross,

                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeType_TleftParen,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PlowPrecedenceBinaryOp]            = {
                                                                 kNewtonIrNodeType_Tminus,
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeType_Tdot,
                                                                 kNewtonIrNodeType_Tcross,
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeType_TleftParen,
                                                                 kNewtonIrNodeType_Tplus,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PunaryOp      ]            = {
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeType_TleftParen,
                                                                 kNewtonIrNodeType_Tdot,
                                                                 kNewtonIrNodeType_Tcross,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PtimeOp       ]            = {
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_Pquantity     ]            = {
                                                                 kNewtonIrNodeType_Texponent,
                                                                 kNewtonIrNodeType_Tlt,
                                                                 kNewtonIrNodeType_Tle,
                                                                 kNewtonIrNodeType_Tgt,
                                                                 kNewtonIrNodeType_Tge,
                                                                 kNewtonIrNodeType_Tproportionality,
                                                                 kNewtonIrNodeType_Tequivalent,
                                                                 kNewtonIrNodeType_TrightBrace,
                                                                 kNewtonIrNodeType_Tcomma,
                                                                 kNewtonIrNodeType_TrightParen,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PquantityFactor]            = {
                                                                 kNewtonIrNodeType_Tmul,
                                                                 kNewtonIrNodeType_Tdiv,
                                                                 kNewtonIrNodeType_Tlt,
                                                                 kNewtonIrNodeType_Tle,
                                                                 kNewtonIrNodeType_Tgt,
                                                                 kNewtonIrNodeType_Tge,
                                                                 kNewtonIrNodeType_Tproportionality,
                                                                 kNewtonIrNodeType_Tequivalent,
                                                                 kNewtonIrNodeType_TrightBrace,
                                                                 kNewtonIrNodeType_Tcomma,
                                                                 kNewtonIrNodeType_TrightParen,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PquantityTerm ]            = {
                                                                 kNewtonIrNodeType_Tminus,
                                                                 kNewtonIrNodeType_Tplus,
                                                                 kNewtonIrNodeType_Tlt,
                                                                 kNewtonIrNodeType_Tle,
                                                                 kNewtonIrNodeType_Tgt,
                                                                 kNewtonIrNodeType_Tge,
                                                                 kNewtonIrNodeType_Tproportionality,
                                                                 kNewtonIrNodeType_Tequivalent,
                                                                 kNewtonIrNodeType_TrightBrace,
                                                                 kNewtonIrNodeType_Tcomma,
                                                                 kNewtonIrNodeType_TrightParen,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PquantityExpression]            = {
                                                                 kNewtonIrNodeType_Tlt,
                                                                 kNewtonIrNodeType_Tle,
                                                                 kNewtonIrNodeType_Tgt,
                                                                 kNewtonIrNodeType_Tge,
                                                                 kNewtonIrNodeType_Tproportionality,
                                                                 kNewtonIrNodeType_Tequivalent,
                                                                 kNewtonIrNodeType_TrightBrace,
                                                                 kNewtonIrNodeType_Tcomma,
                                                                 kNewtonIrNodeType_TrightParen,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_Pparameter    ]            = {
                                                                 kNewtonIrNodeType_Tcomma,
                                                                 kNewtonIrNodeType_TrightBrace,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_Psubindex    ]            = {
				kNewtonIrNodeType_TrightBrace,
				kNewtonIrNodeTypeMax
			},
            [kNewtonIrNodeType_PparameterTuple]            = {kNewtonIrNodeType_TleftBrace, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_PsubindexTuple]            = {kNewtonIrNodeType_TleftBrace, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Pderivation   ]            = {kNewtonIrNodeType_TrightBrace, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Psymbol       ]            = {kNewtonIrNodeType_Tderivation, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Pname         ]            = {kNewtonIrNodeType_Tsymbol, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Pconstraint   ]            = {
                                                                 kNewtonIrNodeType_TrightBrace,
                                                                 kNewtonIrNodeType_Tcomma,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PconstraintList]            = {kNewtonIrNodeType_TrightBrace, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_PbaseSignal   ]            = {kNewtonIrNodeType_Tidentifier, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Pinvariant    ]            = {kNewtonIrNodeType_TrightBrace, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Pconstant     ]            = {kNewtonIrNodeType_TrightBrace, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_Pstatement         ]            = {
                                                                 kNewtonIrNodeType_TrightBrace,
                                                                 kNewtonIrNodeType_Zeof,
                                                                 kNewtonIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PstatementList     ]            = {kNewtonIrNodeType_Zeof, kNewtonIrNodeTypeMax},
            [kNewtonIrNodeType_PnewtonFile   ]            = {kNewtonIrNodeType_Zeof, kNewtonIrNodeTypeMax},
};

char* gNewtonAstNodeStrings[kCommonIrNodeTypeMax]	= {
                                               [    kNewtonIrNodeType_PcompareOp]            = "kNewtonIrNodeType_PcompareOp",
                                               [     kNewtonIrNodeType_PvectorOp]            = "kNewtonIrNodeType_PvectorOp",
                                               [kNewtonIrNodeType_PhighPrecedenceBinaryOp]            = "kNewtonIrNodeType_PhighPrecedenceBinaryOp",
                                               [kNewtonIrNodeType_PmidPrecedenceBinaryOp]            = "kNewtonIrNodeType_PmidPrecedenceBinaryOp",
                                               [kNewtonIrNodeType_PlowPrecedenceBinaryOp]            = "kNewtonIrNodeType_PlowPrecedenceBinaryOp",
                                               [      kNewtonIrNodeType_PunaryOp]            = "kNewtonIrNodeType_PunaryOp",
                                               [       kNewtonIrNodeType_PtimeOp]            = "kNewtonIrNodeType_PtimeOp",
                                               [     kNewtonIrNodeType_Pquantity]            = "kNewtonIrNodeType_Pquantity",
                                               [kNewtonIrNodeType_PquantityFactor]            = "kNewtonIrNodeType_PquantityFactor",
                                               [ kNewtonIrNodeType_PquantityTerm]            = "kNewtonIrNodeType_PquantityTerm",
                                               [kNewtonIrNodeType_PquantityExpression]            = "kNewtonIrNodeType_PquantityExpression",
                                               [    kNewtonIrNodeType_Pparameter]            = "kNewtonIrNodeType_Pparameter",
                                               [kNewtonIrNodeType_PparameterTuple]            = "kNewtonIrNodeType_PparameterTuple",
                                               [   kNewtonIrNodeType_Pderivation]            = "kNewtonIrNodeType_Pderivation",
                                               [       kNewtonIrNodeType_Psymbol]            = "kNewtonIrNodeType_Psymbol",
                                               [         kNewtonIrNodeType_Pname]            = "kNewtonIrNodeType_Pname",
                                               [   kNewtonIrNodeType_Pconstraint]            = "kNewtonIrNodeType_Pconstraint",
                                               [kNewtonIrNodeType_PconstraintList]            = "kNewtonIrNodeType_PconstraintList",
                                               [   kNewtonIrNodeType_PbaseSignal]            = "kNewtonIrNodeType_PbaseSignal",
                                               [    kNewtonIrNodeType_Pinvariant]            = "kNewtonIrNodeType_Pinvariant",
                                               [     kNewtonIrNodeType_Pconstant]            = "kNewtonIrNodeType_Pconstant",
                                               [         kNewtonIrNodeType_Pstatement]            = "kNewtonIrNodeType_Pstatement",
                                               [     kNewtonIrNodeType_PstatementList]            = "kNewtonIrNodeType_PstatementList",
                                               [   kNewtonIrNodeType_PnewtonFile]            = "kNewtonIrNodeType_PnewtonFile",
                                               [          kNewtonIrNodeType_Tnil]            = "kNewtonIrNodeType_Tnil",
                                               [kNewtonIrNodeType_ZbadIdentifier]            = "kNewtonIrNodeType_ZbadIdentifier",
                                               [kNewtonIrNodeType_ZbadStringConst]            = "kNewtonIrNodeType_ZbadStringConst",
                                               [kNewtonIrNodeType_TstringConst]            = "kNewtonIrNodeType_TstringConst",
                                               [      kNewtonIrNodeType_Zepsilon]            = "kNewtonIrNodeType_Zepsilon",
                                               [          kNewtonIrNodeType_Zeof]            = "kNewtonIrNodeType_Zeof",
                                               [        kNewtonIrNodeType_Tcross]            = "kNewtonIrNodeType_Tcross",
                                               [          kNewtonIrNodeType_Tdot]            = "kNewtonIrNodeType_Tdot",
                                               [          kNewtonIrNodeType_Texponent]            = "kNewtonIrNodeType_Texponent",
                                               [     kNewtonIrNodeType_Tintegral]            = "kNewtonIrNodeType_Tintegral",
                                               [   kNewtonIrNodeType_Tderivative]            = "kNewtonIrNodeType_Tderivative",
                                               [kNoisyIrNodeType_Xseq]                       = "kNoisyIrNodeType_Xseq",
                                               [      kNewtonIrNodeType_TSpanish]            = "kNewtonIrNodeType_TSpanish",
                                               [      kNewtonIrNodeType_TEnglish]            = "kNewtonIrNodeType_TEnglish",
                                               [   kNewtonIrNodeType_Tsignal]            = "kNewtonIrNodeType_Tsignal",
                                               [    kNewtonIrNodeType_Tinvariant]            = "kNewtonIrNodeType_Tinvariant",
                                               [     kNewtonIrNodeType_Tconstant]            = "kNewtonIrNodeType_Tconstant",
                                               [   kNewtonIrNodeType_Tderivation]            = "kNewtonIrNodeType_Tderivation",
                                               [       kNewtonIrNodeType_Tsymbol]            = "kNewtonIrNodeType_Tsymbol",
                                               [       kNewtonIrNodeType_Tequals]            = "kNewtonIrNodeType_Tequals",
                                               [       kNewtonIrNodeType_Tnone]            = "kNewtonIrNodeType_Tnone",
                                               [       kNewtonIrNodeType_Tnumber]            = "kNewtonIrNodeType_Tnumber",
                                               [       kNewtonIrNodeType_Tplus]            = "kNewtonIrNodeType_Tplus",
                                               [       kNewtonIrNodeType_Tminus]            = "kNewtonIrNodeType_Tminus",
                                               [       kNewtonIrNodeType_Tmul]            = "kNewtonIrNodeType_Tmul",
                                               [       kNewtonIrNodeType_Tdiv]            = "kNewtonIrNodeType_Tdiv",
                                               [       kNewtonIrNodeType_Tge]            = "kNewtonIrNodeType_Tge",
                                               [       kNewtonIrNodeType_Tle]            = "kNewtonIrNodeType_Tle",
                                               [       kNewtonIrNodeType_Tgt]            = "kNewtonIrNodeType_Tgt",
                                               [       kNewtonIrNodeType_Tlt]            = "kNewtonIrNodeType_Tlt",
                                               [       kNewtonIrNodeType_Pinteger]            = "kNewtonIrNodeType_Pinteger",
                                               [       kNewtonIrNodeType_Tproportionality]            = "kNewtonIrNodeType_Tproportionality",
                                               [       kNewtonIrNodeType_Tequivalent]            = "kNewtonIrNodeType_Tequivalent",
                                               [         kNewtonIrNodeType_Tname]            = "kNewtonIrNodeType_Tname",
                                               [    kNewtonIrNodeType_TrightBrace]            = "kNewtonIrNodeType_TrightBrace",
                                               [     kNewtonIrNodeType_TleftBrace]            = "kNewtonIrNodeType_TleftBrace",
                                               [   kNewtonIrNodeType_TrightParen]            = "kNewtonIrNodeType_TrightParen",
                                               [    kNewtonIrNodeType_TleftParen]            = "kNewtonIrNodeType_TleftParen",
                                               [   kNewtonIrNodeType_Tidentifier]            = "kNewtonIrNodeType_Tidentifier",
                                   };
