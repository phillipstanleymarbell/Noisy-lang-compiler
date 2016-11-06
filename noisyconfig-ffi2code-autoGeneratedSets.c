#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisyconfig-errors.h"
#include "version.h"
#include "noisyconfig.h"


int    gNoisyConfigFirsts[kNoisyConfigIrNodeTypeMax][kNoisyConfigIrNodeTypeMax]  = {
                                               [kNoisyConfigIrNodeType_PanonAggregateCastExpression]            = {kNoisyConfigIrNodeType_TleftBrac, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyIrNodeType_ParrayCastExpression]            = {kNoisyConfigIrNodeType_TleftBrac, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PassignOp]            = {kNoisyConfigIrNodeType_Tequals, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorOp]            = {
                                                                                                    kNoisyConfigIrNodeType_Tdot,
                                                                                                    kNoisyConfigIrNodeType_Tcross,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp]            = {
                                                                                                    kNoisyConfigIrNodeType_Tmul,
                                                                                                    kNoisyConfigIrNodeType_Tdiv,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tplus,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PunaryOp ]            = {kNoisyConfigIrNodeType_Tminus, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_Pfactor  ]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tplus,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_Pterm    ]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_Pexpression]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tplus,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TstringConst,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyConfigIrNodeType_Tdot,
                                                                                                    kNoisyConfigIrNodeType_Tcross,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PvectorScalarPairStatement]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PintegralList]            = {kNoisyConfigIrNodeType_TleftBrac, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PlawStatement]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameStatement]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorScalarPairStatementList]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PintegralLists]            = {kNoisyConfigIrNodeType_TleftBrac, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PlawStatementList]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameStatementList]            = {kNoisyConfigIrNodeType_Tidentifier, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorScalarPairScope]            = {kNoisyConfigIrNodeType_TvectorScalarPairs, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PscalarIntegralScope]            = {kNoisyConfigIrNodeType_TscalarIntegrals, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorIntegralScope]            = {kNoisyConfigIrNodeType_TvectorIntegrals, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionAliasScope]            = {kNoisyConfigIrNodeType_TdimensionAliases, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PlawScope]            = {kNoisyConfigIrNodeType_Tlaw, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameScope]            = {kNoisyConfigIrNodeType_TdimensionTypeNames, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PconfigFile]            = {kNoisyConfigIrNodeType_TdimensionTypeNames, kNoisyConfigIrNodeTypeMax}
                                    };

int   gNoisyConfigFollows[kNoisyConfigIrNodeTypeMax][kNoisyConfigIrNodeTypeMax]  = {
                                               [kNoisyConfigIrNodeType_PanonAggregateCastExpression]            = {kNoisyConfigIrNodeType_TrightBrac, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyIrNodeType_ParrayCastExpression]            = {kNoisyConfigIrNodeType_TrightBrac, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PassignOp]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tplus,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TstringConst,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyConfigIrNodeType_Tdot,
                                                                                                    kNoisyConfigIrNodeType_Tcross,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PvectorOp]            = {kNoisyConfigIrNodeType_TleftParen, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tplus,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PunaryOp ]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeType_TleftParen,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_Pfactor  ]            = {
                                                                                                    kNoisyConfigIrNodeType_Tmul,
                                                                                                    kNoisyConfigIrNodeType_Tdiv,
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tplus,
                                                                                                    kNoisyConfigIrNodeType_TrightParen,
                                                                                                    kNoisyConfigIrNodeType_Tsemicolon,
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_Pterm    ]            = {
                                                                                                    kNoisyConfigIrNodeType_Tminus,
                                                                                                    kNoisyConfigIrNodeType_Tplus,
                                                                                                    kNoisyConfigIrNodeType_TrightParen,
                                                                                                    kNoisyConfigIrNodeType_Tsemicolon,
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_Pexpression]            = {
                                                                                                    kNoisyConfigIrNodeType_TrightParen,
                                                                                                    kNoisyConfigIrNodeType_Tsemicolon,
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PvectorScalarPairStatement]            = {
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PintegralList]            = {
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyConfigIrNodeType_TleftBrac,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PlawStatement]            = {
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameStatement]            = {
                                                                                                    kNoisyConfigIrNodeType_TrightBrace,
                                                                                                    kNoisyConfigIrNodeType_Tidentifier,
                                                                                                    kNoisyConfigIrNodeTypeMax
                                                                                               },
                                               [kNoisyConfigIrNodeType_PvectorScalarPairStatementList]            = {kNoisyConfigIrNodeType_TrightBrace, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PintegralLists]            = {kNoisyConfigIrNodeType_TrightBrace, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PlawStatementList]            = {kNoisyConfigIrNodeType_TrightBrace, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameStatementList]            = {kNoisyConfigIrNodeType_TrightBrace, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorScalarPairScope]            = {kNoisyConfigIrNodeType_Zeof, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PscalarIntegralScope]            = {kNoisyConfigIrNodeType_TvectorScalarPairs, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PvectorIntegralScope]            = {kNoisyConfigIrNodeType_TscalarIntegrals, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionAliasScope]            = {kNoisyConfigIrNodeType_TvectorIntegrals, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PlawScope]            = {kNoisyConfigIrNodeType_TdimensionAliases, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameScope]            = {kNoisyConfigIrNodeType_Tlaw, kNoisyConfigIrNodeTypeMax},
                                               [kNoisyConfigIrNodeType_PconfigFile]            = {kNoisyConfigIrNodeType_Zeof, kNoisyConfigIrNodeTypeMax},
                                               
                                               // [T_XXX                           ]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tnil     ]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Zepsilon ]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Zeof     ]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tsemicolon]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TrightParen]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TleftParen]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TrightBrace]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TleftBrace]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TrightBrac]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TleftBrac]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tequals  ]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tcross   ]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tdot     ]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tdiv     ]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tmul     ]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tplus    ]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tminus   ]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TstringConst]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tidentifier]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TvectorScalarPairs]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TscalarIntegrals]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TvectorIntegrals]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TdimensionAliases]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_Tlaw     ]            = {T_XXX, kNoisyConfigIrNodeTypeMax},
                                               // [kNoisyConfigIrNodeType_TdimensionTypeNames]            = {T_XXX, kNoisyConfigIrNodeTypeMax}
                                   };

char	*gNoisyConfigAstNodeStrings[kNoisyConfigIrNodeTypeMax]	= {
                                               [kNoisyConfigIrNodeType_PanonAggregateCastExpression]            = "kNoisyConfigIrNodeType_PanonAggregateCastExpression",
                                               [kNoisyIrNodeType_ParrayCastExpression]            = "kNoisyIrNodeType_ParrayCastExpression",
                                               [kNoisyConfigIrNodeType_PassignOp]            = "kNoisyConfigIrNodeType_PassignOp",
                                               [kNoisyConfigIrNodeType_PvectorOp]            = "kNoisyConfigIrNodeType_PvectorOp",
                                               [kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp]            = "kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp",
                                               [kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp]            = "kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp",
                                               [ kNoisyConfigIrNodeType_PunaryOp]            = "kNoisyConfigIrNodeType_PunaryOp",
                                               [  kNoisyConfigIrNodeType_Pfactor]            = "kNoisyConfigIrNodeType_Pfactor",
                                               [    kNoisyConfigIrNodeType_Pterm]            = "kNoisyConfigIrNodeType_Pterm",
                                               [kNoisyConfigIrNodeType_Pexpression]            = "kNoisyConfigIrNodeType_Pexpression",
                                               [kNoisyConfigIrNodeType_PvectorScalarPairStatement]            = "kNoisyConfigIrNodeType_PvectorScalarPairStatement",
                                               [kNoisyConfigIrNodeType_PintegralList]            = "kNoisyConfigIrNodeType_PintegralList",
                                               [kNoisyConfigIrNodeType_PlawStatement]            = "kNoisyConfigIrNodeType_PlawStatement",
                                               [kNoisyConfigIrNodeType_PdimensionTypeNameStatement]            = "kNoisyConfigIrNodeType_PdimensionTypeNameStatement",
                                               [kNoisyConfigIrNodeType_PvectorScalarPairStatementList]            = "kNoisyConfigIrNodeType_PvectorScalarPairStatementList",
                                               [kNoisyConfigIrNodeType_PintegralLists]            = "kNoisyConfigIrNodeType_PintegralLists",
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
