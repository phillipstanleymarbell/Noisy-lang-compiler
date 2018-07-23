/*
	Authored 2018. Phillip Stanley-Marbell.

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



/*
 *	Strings representing the various productions, for debugging and error reporting
 */
const char *	gProductionDescriptions[kCommonIrNodeTypeMax] =
{
	[kNewtonIrNodeType_PbaseSignal]			= "base signal",
	[kNewtonIrNodeType_PcompareOp]			= "compare operator",
	[kNewtonIrNodeType_Pconstant]			= "constant",
	[kNewtonIrNodeType_Pconstraint]			= "constraint",
	[kNewtonIrNodeType_PconstraintList]		= "constraint list",
	[kNewtonIrNodeType_Pderivation]			= "derivation",
	[kNewtonIrNodeType_PhighPrecedenceBinaryOp]	= "high-precedence binary operator",
	[kNewtonIrNodeType_Pinteger]			= "integer",
	[kNewtonIrNodeType_Pinvariant]			= "invariant",
	[kNewtonIrNodeType_PlanguageSetting]		= "language setting",
	[kNewtonIrNodeType_PlowPrecedenceBinaryOp]	= "low-precedence binary operator",
	[kNewtonIrNodeType_PmidPrecedenceBinaryOp]	= "mid-precedence (Ejonathan) binary operator",
	[kNewtonIrNodeType_Pname]			= "name",
	[kNewtonIrNodeType_PnewtonFile]			= "Newton description file",
	[kNewtonIrNodeType_Pparameter]			= "parameter",
	[kNewtonIrNodeType_PparameterTuple]		= "parameter tuple",
	[kNewtonIrNodeType_Pquantity]			= "quantity",
	[kNewtonIrNodeType_PquantityExpression]		= "quantity expression",
	[kNewtonIrNodeType_PquantityFactor]		= "quantity factor",
	[kNewtonIrNodeType_PquantityStatement]		= "quantity statement",
	[kNewtonIrNodeType_PquantityTerm]		= "quantity term",
	[kNewtonIrNodeType_Pstatement]			= "statement",
	[kNewtonIrNodeType_PstatementList]		= "statement list",
	[kNewtonIrNodeType_Psubindex]			= "sub index",
	[kNewtonIrNodeType_PsubindexTuple]		= "sub index tuple",
	[kNewtonIrNodeType_Psymbol]			= "symbol",
	[kNewtonIrNodeType_PtimeOp]			= "time operator",
	[kNewtonIrNodeType_PunaryOp]			= "unary operator",
	[kNewtonIrNodeType_PvectorOp]			= "vector operator",
};
