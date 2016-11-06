#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisyconfig-timeStamps.h"
#include "noisyconfig.h"


/*
 *	Some classes of characters used in Lexer
 */
const NoisyConfigIrNodeType gNoisyConfigReservedTokens[] = {
	
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

};

const char *    gReservedTokenDescriptions[kNoisyConfigIrNodeTypeMax] = {
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
};
