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
#include "common-timeStamps.h"
#include "data-structures.h"


int gNewtonFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax]  = {
            [kNewtonIrNodeType_PcompareOp    ]            = {
                                                                 kNewtonIrNodeType_Tlt,
                                                                 kNewtonIrNodeType_Tle,
                                                                 kNewtonIrNodeType_Tgt,
                                                                 kNewtonIrNodeType_Tge,
                                                                 kNewtonIrNodeType_Tproportionality,
                                                                 kNewtonIrNodeType_Tequivalent,
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PvectorOp     ]            = {
                                                                 kNewtonIrNodeType_Tdot,
                                                                 kNewtonIrNodeType_Tcross,
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PhighPrecedenceBinaryOp]            = {kNewtonIrNodeType_Texponent, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_PmidPrecedenceBinaryOp]            = {
                                                                 kNewtonIrNodeType_Tmul,
                                                                 kNewtonIrNodeType_Tdiv,
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PlowPrecedenceBinaryOp]            = {
                                                                 kNewtonIrNodeType_Tminus,
                                                                 kNewtonIrNodeType_Tplus,
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PunaryOp      ]            = {kNewtonIrNodeType_Tminus, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_PtimeOp       ]            = {
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_Pquantity     ]            = {
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PquantityFactor]            = {
                                                                 kNewtonIrNodeType_Tminus,
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeType_Tdot,
                                                                 kNewtonIrNodeType_Tcross,
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeType_TleftParen,
                                                                 kNoisyIrNodeTypeMax
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
                                                                 kNoisyIrNodeTypeMax
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
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_Pparameter    ]            = {kNewtonIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_PparameterTuple]            = {kNewtonIrNodeType_TleftParen, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_Pderivation   ]            = {kNewtonIrNodeType_Tderivation, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_Psymbol       ]            = {kNewtonIrNodeType_Tsymbol, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_Pname         ]            = {kNewtonIrNodeType_Tname, kNoisyIrNodeTypeMax},
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
                                                                 kNoisyIrNodeTypeMax
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
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PbaseSignal   ]            = {kNewtonIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_Pinvariant    ]            = {kNewtonIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_Pconstant     ]            = {kNewtonIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_Prule         ]            = {kNewtonIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_PruleList     ]            = {kNewtonIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_PnewtonFile   ]            = {kNewtonIrNodeType_Tidentifier, kNoisyIrNodeTypeMax}
 };

int gNewtonFollows[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax]  = {
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
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PvectorOp     ]            = {kNewtonIrNodeType_TleftParen, kNoisyIrNodeTypeMax},
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
                                                                 kNoisyIrNodeTypeMax
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
                                                                 kNoisyIrNodeTypeMax
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
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PunaryOp      ]            = {
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeType_TleftParen,
                                                                 kNewtonIrNodeType_Tdot,
                                                                 kNewtonIrNodeType_Tcross,
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PtimeOp       ]            = {
                                                                 kNewtonIrNodeType_Tderivative,
                                                                 kNewtonIrNodeType_Tintegral,
                                                                 kNewtonIrNodeType_Tnumber,
                                                                 kNewtonIrNodeType_Tidentifier,
                                                                 kNoisyIrNodeTypeMax
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
                                                                 kNoisyIrNodeTypeMax
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
                                                                 kNoisyIrNodeTypeMax
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
                                                                 kNoisyIrNodeTypeMax
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
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_Pparameter    ]            = {
                                                                 kNewtonIrNodeType_Tcomma,
                                                                 kNewtonIrNodeType_TrightBrace,
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PparameterTuple]            = {kNewtonIrNodeType_TleftBrace, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_Pderivation   ]            = {kNewtonIrNodeType_TrightBrace, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_Psymbol       ]            = {kNewtonIrNodeType_Tderivation, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_Pname         ]            = {kNewtonIrNodeType_Tsymbol, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_Pconstraint   ]            = {
                                                                 kNewtonIrNodeType_TrightBrace,
                                                                 kNewtonIrNodeType_Tcomma,
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PconstraintList]            = {kNewtonIrNodeType_TrightBrace, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_PbaseSignal   ]            = {kNewtonIrNodeType_TrightBrace, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_Pinvariant    ]            = {kNewtonIrNodeType_TrightBrace, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_Pconstant     ]            = {kNewtonIrNodeType_TrightBrace, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_Prule         ]            = {
                                                                 kNewtonIrNodeType_TrightBrace,
                                                                 kNewtonIrNodeType_Zeof,
                                                                 kNoisyIrNodeTypeMax
                                                            },
            [kNewtonIrNodeType_PruleList     ]            = {kNewtonIrNodeType_Zeof, kNoisyIrNodeTypeMax},
            [kNewtonIrNodeType_PnewtonFile   ]            = {kNewtonIrNodeType_Zeof, kNoisyIrNodeTypeMax},
            // [T_XXX                           ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Tnil          ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_ZbadIdentifier]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_ZbadStringConst]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Zepsilon      ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Zeof          ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Tcross        ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Tdot          ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Tintegral     ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Tderivative   ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_TSpanish      ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_TEnglish      ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Tsignal   ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Tinvariant    ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Tconstant     ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Tderivation   ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Tsymbol       ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Tname         ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_TrightBrace    ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_TleftBrace     ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_TrightParen   ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_TleftParen    ]            = {T_XXX, kNoisyIrNodeTypeMax},
            // [kNewtonIrNodeType_Tidentifier   ]            = {T_XXX, kNoisyIrNodeTypeMax}
};

char* gNewtonAstNodeStrings[kNoisyIrNodeTypeMax]	= {
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
                                               [         kNewtonIrNodeType_Prule]            = "kNewtonIrNodeType_Prule",
                                               [     kNewtonIrNodeType_PruleList]            = "kNewtonIrNodeType_PruleList",
                                               [   kNewtonIrNodeType_PnewtonFile]            = "kNewtonIrNodeType_PnewtonFile",
                                               //[                           T_XXX]            = "T_XXX",
                                               [          kNewtonIrNodeType_Tnil]            = "kNewtonIrNodeType_Tnil",
                                               [kNewtonIrNodeType_ZbadIdentifier]            = "kNewtonIrNodeType_ZbadIdentifier",
                                               [kNewtonIrNodeType_ZbadStringConst]            = "kNewtonIrNodeType_ZbadStringConst",
                                               [      kNewtonIrNodeType_Zepsilon]            = "kNewtonIrNodeType_Zepsilon",
                                               [          kNewtonIrNodeType_Zeof]            = "kNewtonIrNodeType_Zeof",
                                               [        kNewtonIrNodeType_Tcross]            = "kNewtonIrNodeType_Tcross",
                                               [          kNewtonIrNodeType_Tdot]            = "kNewtonIrNodeType_Tdot",
                                               [     kNewtonIrNodeType_Tintegral]            = "kNewtonIrNodeType_Tintegral",
                                               [   kNewtonIrNodeType_Tderivative]            = "kNewtonIrNodeType_Tderivative",
                                               [      kNewtonIrNodeType_TSpanish]            = "kNewtonIrNodeType_TSpanish",
                                               [      kNewtonIrNodeType_TEnglish]            = "kNewtonIrNodeType_TEnglish",
                                               [   kNewtonIrNodeType_Tsignal]            = "kNewtonIrNodeType_Tsignal",
                                               [    kNewtonIrNodeType_Tinvariant]            = "kNewtonIrNodeType_Tinvariant",
                                               [     kNewtonIrNodeType_Tconstant]            = "kNewtonIrNodeType_Tconstant",
                                               [   kNewtonIrNodeType_Tderivation]            = "kNewtonIrNodeType_Tderivation",
                                               [       kNewtonIrNodeType_Tsymbol]            = "kNewtonIrNodeType_Tsymbol",
                                               [       kNewtonIrNodeType_Tequals]            = "kNewtonIrNodeType_Tequals",
                                               [       kNewtonIrNodeType_Tnone]            = "kNewtonIrNodeType_Tnone",
                                               [         kNewtonIrNodeType_Tname]            = "kNewtonIrNodeType_Tname",
                                               [    kNewtonIrNodeType_TrightBrace]            = "kNewtonIrNodeType_TrightBrace",
                                               [     kNewtonIrNodeType_TleftBrace]            = "kNewtonIrNodeType_TleftBrace",
                                               [   kNewtonIrNodeType_TrightParen]            = "kNewtonIrNodeType_TrightParen",
                                               [    kNewtonIrNodeType_TleftParen]            = "kNewtonIrNodeType_TleftParen",
                                               [   kNewtonIrNodeType_Tidentifier]            = "kNewtonIrNodeType_Tidentifier",
                                   };
