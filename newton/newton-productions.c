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
const char *	gProductionDescriptions[kNewtonIrNodeTypeMax] =
{
	[kNewtonIrNodeType_PbaseSignal]			= "base signal",
	[kNewtonIrNodeType_PcompareOp]			= "",
	[kNewtonIrNodeType_Pconstant]			= "",
	[kNewtonIrNodeType_Pconstraint]			= "",
	[kNewtonIrNodeType_PconstraintList]			= "",
	[kNewtonIrNodeType_Pderivation]			= "",
	[kNewtonIrNodeType_PhighPrecedenceBinaryOp]			= "",
	[kNewtonIrNodeType_Pinteger]			= "",
	[kNewtonIrNodeType_Pinvariant]			= "",
	[kNewtonIrNodeType_PlanguageSetting]			= "",
	[kNewtonIrNodeType_PlowPrecedenceBinaryOp]			= "",
	[kNewtonIrNodeType_PmidPrecedenceBinaryOp]			= "",
	[kNewtonIrNodeType_Pname]			= "",
	[kNewtonIrNodeType_PnewtonFile]			= "",
	[kNewtonIrNodeType_Pparameter]			= "",
	[kNewtonIrNodeType_PparameterTuple]			= "",
	[kNewtonIrNodeType_Pquantity]			= "",
	[kNewtonIrNodeType_PquantityExpression]			= "",
	[kNewtonIrNodeType_PquantityFactor]			= "",
	[kNewtonIrNodeType_PquantityStatement]			= "",
	[kNewtonIrNodeType_PquantityTerm]			= "",
	[kNewtonIrNodeType_Prule]			= "",
	[kNewtonIrNodeType_PruleList]			= "",
	[kNewtonIrNodeType_Psubindex]			= "",
	[kNewtonIrNodeType_PsubindexTuple]			= "",
	[kNewtonIrNodeType_Psymbol]			= "",
	[kNewtonIrNodeType_PtimeOp]			= "",
	[kNewtonIrNodeType_PunaryOp]			= "",
	[kNewtonIrNodeType_PvectorOp]			= "",
};


/*
 *	TODO: Places where we need the string form of a NoisyIrNodeType should also use gNoisyAstNodeStrings[], not
 *	gProductionStrings[] and gTerminalStrings[]. Get rid of gProductionStrings[] and gTerminalStrings[]
 */

const char *	gProductionStrings[kNewtonIrNodeTypeMax] =
{
	[kNewtonIrNodeType_PbaseSignal]			= "kNewtonIrNodeType_PbaseSignal",
	[kNewtonIrNodeType_PcompareOp]			= "kNewtonIrNodeType_PcompareOp",
	[kNewtonIrNodeType_Pconstant]			= "kNewtonIrNodeType_Pconstant",
	[kNewtonIrNodeType_Pconstraint]			= "",
	[kNewtonIrNodeType_PconstraintList]			= "",
	[kNewtonIrNodeType_Pderivation]			= "",
	[kNewtonIrNodeType_PhighPrecedenceBinaryOp]			= "",
	[kNewtonIrNodeType_Pinteger]			= "",
	[kNewtonIrNodeType_Pinvariant]			= "",
	[kNewtonIrNodeType_PlanguageSetting]			= "",
	[kNewtonIrNodeType_PlowPrecedenceBinaryOp]			= "",
	[kNewtonIrNodeType_PmidPrecedenceBinaryOp]			= "",
	[kNewtonIrNodeType_Pname]			= "",
	[kNewtonIrNodeType_PnewtonFile]			= "",
	[kNewtonIrNodeType_Pparameter]			= "",
	[kNewtonIrNodeType_PparameterTuple]			= "",
	[kNewtonIrNodeType_Pquantity]			= "",
	[kNewtonIrNodeType_PquantityExpression]			= "",
	[kNewtonIrNodeType_PquantityFactor]			= "",
	[kNewtonIrNodeType_PquantityStatement]			= "",
	[kNewtonIrNodeType_PquantityTerm]			= "",
	[kNewtonIrNodeType_Prule]			= "",
	[kNewtonIrNodeType_PruleList]			= "",
	[kNewtonIrNodeType_Psubindex]			= "",
	[kNewtonIrNodeType_PsubindexTuple]			= "",
	[kNewtonIrNodeType_Psymbol]			= "",
	[kNewtonIrNodeType_PtimeOp]			= "",
	[kNewtonIrNodeType_PunaryOp]			= "",
	[kNewtonIrNodeType_PvectorOp]			= "",
};
