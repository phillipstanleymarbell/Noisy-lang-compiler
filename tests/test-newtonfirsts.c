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
