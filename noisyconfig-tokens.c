#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisy-timeStamps.h"
#include "noisy.h"


/*
 *	Some classes of characters used in Lexer
 */
const NoisyIrNodeType gNoisyConfigReservedTokens[] = {
	
    kNoisyConfigIrNodeType_Tsemicolon,
	kNoisyConfigIrNodeType_TrightParen,
	kNoisyConfigIrNodeType_TleftParen,
	kNoisyConfigIrNodeType_TrightBrace,
	kNoisyConfigIrNodeType_TleftBrace,
	kNoisyConfigIrNodeType_TrightBrac,
	kNoisyConfigIrNodeType_TleftBrac,
	kNoisyConfigIrNodeType_Tequals,
	kNoisyConfigIrNodeType_Tcomma,
	kNoisyConfigIrNodeType_Tcross,
	kNoisyConfigIrNodeType_Tdot,
	kNoisyConfigIrNodeType_Tdiv,
	kNoisyConfigIrNodeType_Tmul,
	kNoisyConfigIrNodeType_Tplus,
	kNoisyConfigIrNodeType_Tminus,
	kNoisyConfigIrNodeType_TvectorScalarPairs,
	kNoisyConfigIrNodeType_TscalarIntegrals,
	kNoisyConfigIrNodeType_TvectorIntegrals,
	kNoisyConfigIrNodeType_TdimensionAliases,
	kNoisyConfigIrNodeType_Tlaw,
	kNoisyConfigIrNodeType_TdimensionTypeNames,
	
	kNoisyConfigIrNodeType_ZbadStringConst,
	kNoisyConfigIrNodeType_ZbadIdentifier,
	kNoisyConfigIrNodeType_Zepsilon,
	kNoisyConfigIrNodeType_Zeof
};

const char *	gReservedConfigTokenDescriptions[kNoisyIrNodeTypeMax] = {
    [kNoisyConfigIrNodeType_Tsemicolon] = ";",
	[kNoisyConfigIrNodeType_TrightParen] = ")",
	[kNoisyConfigIrNodeType_TleftParen] = "(",
	[kNoisyConfigIrNodeType_TrightBrace] = "}",
	[kNoisyConfigIrNodeType_TleftBrace] = "{",
	[kNoisyConfigIrNodeType_TrightBrac] = "]",
	[kNoisyConfigIrNodeType_TleftBrac] = "[",
	[kNoisyConfigIrNodeType_Tequals] = "=",
	[kNoisyConfigIrNodeType_Tcomma] = ",",
	[kNoisyConfigIrNodeType_Tcross] = "cross",
	[kNoisyConfigIrNodeType_Tdot] = "dot",
	[kNoisyConfigIrNodeType_Tdiv] = "/",
	[kNoisyConfigIrNodeType_Tmul] = "*",
	[kNoisyConfigIrNodeType_Tplus] = "+",
	[kNoisyConfigIrNodeType_Tminus] = "-",
	[kNoisyConfigIrNodeType_TvectorScalarPairs] = "vectorScalarPairs",
	[kNoisyConfigIrNodeType_TscalarIntegrals] = "scalarIntegrals",
	[kNoisyConfigIrNodeType_TvectorIntegrals] = "vectorIntegrals",
	[kNoisyConfigIrNodeType_TdimensionAliases] = "dimensionAliases",
	[kNoisyConfigIrNodeType_Tlaw] = "law",
	[kNoisyConfigIrNodeType_TdimensionTypeNames] = "dimensionTypeNames",
	
    [kNoisyConfigIrNodeType_ZbadStringConst]  =   "NoisyConfigIrNodeType_ZbadStringConst",
    [kNoisyConfigIrNodeType_ZbadIdentifier]   =   "NoisyConfigIrNodeType_ZbadIdentifier",
    [kNoisyConfigIrNodeType_Zepsilon]     =   "NoisyConfigIrNodeType_Zepsilon",
    [kNoisyConfigIrNodeType_Zeof]         =   "NoisyConfigIrNodeType_Zeof"
						};
