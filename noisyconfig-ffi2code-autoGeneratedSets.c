#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "version.h"
#include "noisy-timeStamps.h"
#include "noisy.h"

int    gNoisyConfigFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax]  = {
                                               [kNoisyConfigIrNodeType_PanonAggregateCastExpression]            = {kNoisyConfigIrNodeType_TleftBrac, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_ParrayCastExpression]            = {kNoisyConfigIrNodeType_TleftBrac, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PassignOp]            = {kNoisyConfigIrNodeType_Tequals, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorOp]            = {
                                                                                                    kNoisyConfigIrNodeType_Tdot,
                                                                                                    kNoisyConfigIrNodeType_Tcross,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp]            = {
                                                                                                    kNoisyConfigIrNodeType_Tmul,
                                                                                                    kNoisyConfigIrNodeType_Tdiv,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tplus,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PunaryOp ]            = {kNoisyConfigIrNodeType_Tminus, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_Pfactor  ]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tdot,
                                                                                                    kNoisyConfigIrNodeType_Tcross,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_Pterm    ]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tdot,
                                                                                                    kNoisyConfigIrNodeType_Tcross,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_Pexpression]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tplus,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TstringConst,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyConfigIrNodeType_Tdot,
                                                                                                    kNoisyConfigIrNodeType_Tcross,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PvectorScalarPairStatement]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PscalarIntegralList]            = {kNoisyConfigIrNodeType_TleftBrac, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorIntegralList]            = {kNoisyConfigIrNodeType_TleftBrac, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionAliasStatement]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PlawStatement]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameStatement]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorScalarPairStatementList]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PscalarIntegralLists]            = {kNoisyConfigIrNodeType_TleftBrac, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorIntegralLists]            = {kNoisyConfigIrNodeType_TleftBrac, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionAliasStatementList]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PlawStatementList]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameStatementList]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorScalarPairScope]            = {kNoisyConfigIrNodeType_TvectorScalarPairs, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PscalarIntegralScope]            = {kNoisyConfigIrNodeType_TscalarIntegrals, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorIntegralScope]            = {kNoisyConfigIrNodeType_TvectorIntegrals, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionAliasScope]            = {kNoisyConfigIrNodeType_TdimensionAliases, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PlawScope]            = {kNoisyConfigIrNodeType_Tlaw, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameScope]            = {kNoisyConfigIrNodeType_TdimensionTypeNames, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PconfigFile]            = {kNoisyConfigIrNodeType_TdimensionTypeNames, kNoisyIrNodeTypeMax}
                                    };

int    gNoisyConfigFollows[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax]  = {
                                               [kNoisyConfigIrNodeType_PanonAggregateCastExpression]            = {kNoisyConfigIrNodeType_TrightBrac, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_ParrayCastExpression]            = {kNoisyConfigIrNodeType_TrightBrac, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PassignOp]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tplus,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TstringConst,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyConfigIrNodeType_Tdot,
                                                                                                    kNoisyConfigIrNodeType_Tcross,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PvectorOp]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tdot,
                                                                                                    kNoisyConfigIrNodeType_Tcross,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tdot,
                                                                                                    kNoisyConfigIrNodeType_Tcross,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tdot,
                                                                                                    kNoisyConfigIrNodeType_Tcross,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PunaryOp ]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tdot,
                                                                                                    kNoisyConfigIrNodeType_Tcross,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_Pfactor  ]            = {
                                                                                                    kNoisyConfigIrNodeType_Tmul,
                                                                                                    kNoisyConfigIrNodeType_Tdiv,
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tplus,
                                                                                                    kNoisyConfigIrNodeType_TrightParen,
                                                                                                    kNoisyConfigIrNodeType_Tsemicolon,
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_Pterm    ]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tplus,
                                                                                                    kNoisyConfigIrNodeType_TrightParen,
                                                                                                    kNoisyConfigIrNodeType_Tsemicolon,
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_Pexpression]            = {
                                                                                                    kNoisyConfigIrNodeType_TrightParen,
                                                                                                    kNoisyConfigIrNodeType_Tsemicolon,
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PvectorScalarPairStatement]            = {
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PscalarIntegralList]            = {
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyConfigIrNodeType_TleftBrac,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PvectorIntegralList]            = {
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyConfigIrNodeType_TleftBrac,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PdimensionAliasStatement]            = {
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PlawStatement]            = {
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameStatement]            = {
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PvectorScalarPairStatementList]            = {kNoisyConfigIrNodeType_TrightBrace, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PscalarIntegralLists]            = {kNoisyConfigIrNodeType_TrightBrace, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorIntegralLists]            = {kNoisyConfigIrNodeType_TrightBrace, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionAliasStatementList]            = {kNoisyConfigIrNodeType_TrightBrace, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PlawStatementList]            = {kNoisyConfigIrNodeType_TrightBrace, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameStatementList]            = {kNoisyConfigIrNodeType_TrightBrace, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorScalarPairScope]            = {kNoisyConfigIrNodeType_Zeof, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PscalarIntegralScope]            = {kNoisyConfigIrNodeType_TvectorScalarPairs, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorIntegralScope]            = {kNoisyConfigIrNodeType_TscalarIntegrals, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionAliasScope]            = {kNoisyConfigIrNodeType_TvectorIntegrals, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PlawScope]            = {kNoisyConfigIrNodeType_TdimensionAliases, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameScope]            = {kNoisyConfigIrNodeType_Tlaw, kNoisyIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PconfigFile]            = {kNoisyConfigIrNodeType_Zeof, kNoisyIrNodeTypeMax},
                                               // [T_XXX                           ]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tnil     ]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_ZbadIdentifier]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_ZbadStringConst]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Zepsilon ]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Zeof     ]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tsemicolon]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TrightParen]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TleftParen]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TrightBrace]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TleftBrace]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TrightBrac]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TleftBrac]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tequals  ]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tcross   ]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tdot     ]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tdiv     ]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tmul     ]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tplus    ]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tminus   ]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TstringConst]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tidentifier]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TvectorScalarPairs]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TscalarIntegrals]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TvectorIntegrals]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TdimensionAliases]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tlaw     ]            = {T_XXX, kNoisyIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TdimensionTypeNames]            = {T_XXX, kNoisyIrNodeTypeMax}
                                   };

char	*gNoisyConfigAstNodeStrings[kNoisyIrNodeTypeMax]	= {
                                               [kNoisyConfigIrNodeType_PanonAggregateCastExpression]            = "kNoisyConfigIrNodeType_PanonAggregateCastExpression",
                                               [kNoisyConfigIrNodeType_ParrayCastExpression]            = "kNoisyConfigIrNodeType_ParrayCastExpression",
                                               [kNoisyConfigIrNodeType_PassignOp]            = "kNoisyConfigIrNodeType_PassignOp",
                                               [kNoisyConfigIrNodeType_PvectorOp]            = "kNoisyConfigIrNodeType_PvectorOp",
                                               [kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp]            = "kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp",
                                               [kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp]            = "kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp",
                                               [ kNoisyConfigIrNodeType_PunaryOp]            = "kNoisyConfigIrNodeType_PunaryOp",
                                               [  kNoisyConfigIrNodeType_Pfactor]            = "kNoisyConfigIrNodeType_Pfactor",
                                               [    kNoisyConfigIrNodeType_Pterm]            = "kNoisyConfigIrNodeType_Pterm",
                                               [kNoisyConfigIrNodeType_Pexpression]            = "kNoisyConfigIrNodeType_Pexpression",
                                               [kNoisyConfigIrNodeType_PvectorScalarPairStatement]            = "kNoisyConfigIrNodeType_PvectorScalarPairStatement",
                                               [kNoisyConfigIrNodeType_PscalarIntegralList]            = "kNoisyConfigIrNodeType_PscalarIntegralList",
                                               [kNoisyConfigIrNodeType_PvectorIntegralList]            = "kNoisyConfigIrNodeType_PvectorIntegralList",
                                               [kNoisyConfigIrNodeType_PdimensionAliasStatement]            = "kNoisyConfigIrNodeType_PdimensionAliasStatement",
                                               [kNoisyConfigIrNodeType_PlawStatement]            = "kNoisyConfigIrNodeType_PlawStatement",
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameStatement]            = "kNoisyConfigIrNodeType_PdimensionTypeNameStatement",
                                               [kNoisyConfigIrNodeType_PvectorScalarPairStatementList]            = "kNoisyConfigIrNodeType_PvectorScalarPairStatementList",
                                               [kNoisyConfigIrNodeType_PscalarIntegralLists]            = "kNoisyConfigIrNodeType_PscalarIntegralLists",
                                               [kNoisyConfigIrNodeType_PvectorIntegralLists]            = "kNoisyConfigIrNodeType_PvectorIntegralLists",
                                               [kNoisyConfigIrNodeType_PdimensionAliasStatementList]            = "kNoisyConfigIrNodeType_PdimensionAliasStatementList",
                                               [kNoisyConfigIrNodeType_PlawStatementList]            = "kNoisyConfigIrNodeType_PlawStatementList",
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameStatementList]            = "kNoisyConfigIrNodeType_PdimensionTypeNameStatementList",
                                               [kNoisyConfigIrNodeType_PvectorScalarPairScope]            = "kNoisyConfigIrNodeType_PvectorScalarPairScope",
                                               [kNoisyConfigIrNodeType_PscalarIntegralScope]            = "kNoisyConfigIrNodeType_PscalarIntegralScope",
                                               [kNoisyConfigIrNodeType_PvectorIntegralScope]            = "kNoisyConfigIrNodeType_PvectorIntegralScope",
                                               [kNoisyConfigIrNodeType_PdimensionAliasScope]            = "kNoisyConfigIrNodeType_PdimensionAliasScope",
                                               [kNoisyConfigIrNodeType_PlawScope]            = "kNoisyConfigIrNodeType_PlawScope",
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameScope]            = "kNoisyConfigIrNodeType_PdimensionTypeNameScope",
                                               [kNoisyConfigIrNodeType_PconfigFile]            = "kNoisyConfigIrNodeType_PconfigFile",
                                               // [                           T_XXX]            = "T_XXX",
                                               [     kNoisyConfigIrNodeType_Tnil]            = "kNoisyConfigIrNodeType_Tnil",
                                               [kNoisyConfigIrNodeType_ZbadIdentifier]            = "kNoisyConfigIrNodeType_ZbadIdentifier",
                                               [kNoisyConfigIrNodeType_ZbadStringConst]            = "kNoisyConfigIrNodeType_ZbadStringConst",
                                               [ kNoisyConfigIrNodeType_Zepsilon]            = "kNoisyConfigIrNodeType_Zepsilon",
                                               [     kNoisyConfigIrNodeType_Zeof]            = "kNoisyConfigIrNodeType_Zeof",
                                               [kNoisyConfigIrNodeType_Tsemicolon]            = "kNoisyConfigIrNodeType_Tsemicolon",
                                               [kNoisyConfigIrNodeType_TrightParen]            = "kNoisyConfigIrNodeType_TrightParen",
                                               [kNoisyConfigIrNodeType_TleftParen]            = "kNoisyConfigIrNodeType_TleftParen",
                                               [kNoisyConfigIrNodeType_TrightBrace]            = "kNoisyConfigIrNodeType_TrightBrace",
                                               [kNoisyConfigIrNodeType_TleftBrace]            = "kNoisyConfigIrNodeType_TleftBrace",
                                               [kNoisyConfigIrNodeType_TrightBrac]            = "kNoisyConfigIrNodeType_TrightBrac",
                                               [kNoisyConfigIrNodeType_TleftBrac]            = "kNoisyConfigIrNodeType_TleftBrac",
                                               [  kNoisyConfigIrNodeType_Tequals]            = "kNoisyConfigIrNodeType_Tequals",
                                               [   kNoisyConfigIrNodeType_Tcross]            = "kNoisyConfigIrNodeType_Tcross",
                                               [     kNoisyConfigIrNodeType_Tdot]            = "kNoisyConfigIrNodeType_Tdot",
                                               [     kNoisyConfigIrNodeType_Tdiv]            = "kNoisyConfigIrNodeType_Tdiv",
                                               [     kNoisyConfigIrNodeType_Tmul]            = "kNoisyConfigIrNodeType_Tmul",
                                               [    kNoisyConfigIrNodeType_Tplus]            = "kNoisyConfigIrNodeType_Tplus",
                                               [   kNoisyConfigIrNodeType_Tminus]            = "kNoisyConfigIrNodeType_Tminus",
                                               [kNoisyConfigIrNodeType_TstringConst]            = "kNoisyConfigIrNodeType_TstringConst",
                                               [kNoisyConfigIrNodeType_Tidentifier]            = "kNoisyConfigIrNodeType_Tidentifier",
                                               [kNoisyConfigIrNodeType_TvectorScalarPairs]            = "kNoisyConfigIrNodeType_TvectorScalarPairs",
                                               [kNoisyConfigIrNodeType_TscalarIntegrals]            = "kNoisyConfigIrNodeType_TscalarIntegrals",
                                               [kNoisyConfigIrNodeType_TvectorIntegrals]            = "kNoisyConfigIrNodeType_TvectorIntegrals",
                                               [kNoisyConfigIrNodeType_TdimensionAliases]            = "kNoisyConfigIrNodeType_TdimensionAliases",
                                               [     kNoisyConfigIrNodeType_Tlaw]            = "kNoisyConfigIrNodeType_Tlaw",
                                               [kNoisyConfigIrNodeType_TdimensionTypeNames]            = "kNoisyConfigIrNodeType_TdimensionTypeNames",
                                   };
