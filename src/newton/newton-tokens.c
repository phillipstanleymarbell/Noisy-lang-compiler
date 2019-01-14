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
 *	This array is used both in the lexer for two purposes: (1) match keywords in order
 *	to return a kNewtonIrNodeType_Tabcd, as well as to print descriptions of keywords.
 */
const char *	gNewtonTokenDescriptions[kCommonIrNodeTypeMax] = {
									/*
									 *	Begin keywords
									 */
									[kNewtonIrNodeType_TBernoulli]		= "Bernoulli",
									[kNewtonIrNodeType_TBeta]		= "Beta",
									[kNewtonIrNodeType_TBetaBinomial]	= "BetaBinomial",
									[kNewtonIrNodeType_TBetaPrime]		= "BetaPrime",
									[kNewtonIrNodeType_TBinomial]		= "Binomial",
									[kNewtonIrNodeType_TCauchy]		= "Cauchy",
									[kNewtonIrNodeType_TDirichlet]		= "Dirichlet",
 									[kNewtonIrNodeType_TEnglish]		= "English",
									[kNewtonIrNodeType_TErlang]		= "Erlang",
									[kNewtonIrNodeType_TExponential]	= "Exponential",
									[kNewtonIrNodeType_TExtremeValue]	= "ExtremeValue",
									[kNewtonIrNodeType_TF]			= "F",
									[kNewtonIrNodeType_TFermiDirac]		= "FermiDirac",
									[kNewtonIrNodeType_TFisherZ]		= "FisherZ]",
									[kNewtonIrNodeType_TGamma]		= "Gamma",
									[kNewtonIrNodeType_TGaussian]		= "Gaussian",
									[kNewtonIrNodeType_TGibrat]		= "Gibrat",
									[kNewtonIrNodeType_TGumbel]		= "Gumbel",
									[kNewtonIrNodeType_TLaplacian]		= "Laplacian",
									[kNewtonIrNodeType_TLogNormal]		= "LogNormal",
									[kNewtonIrNodeType_TLogSeries]		= "LogSeries",
									[kNewtonIrNodeType_TLogitNormal]	= "LogitNormal",
									[kNewtonIrNodeType_TMaxwell]		= "Maxwell",
									[kNewtonIrNodeType_TMultinomial]	= "Multinomial",
									[kNewtonIrNodeType_TNegativeBinomial]	= "NegativeBinomial",
									[kNewtonIrNodeType_TPareto]		= "Pareto",
									[kNewtonIrNodeType_TPearsonIII]		= "PearsonIII",
									[kNewtonIrNodeType_TPoisson]		= "Poisson",
									[kNewtonIrNodeType_TRayleigh]		= "Rayleigh",
									[kNewtonIrNodeType_TStudentT]		= "StudentT",
									[kNewtonIrNodeType_TStudentZ]		= "StudentZ",
									[kNewtonIrNodeType_TWeibull]		= "Weibull",
									[kNewtonIrNodeType_TXi]			= "Xi",
									[kNewtonIrNodeType_TXiSquared]		= "XiSquared",
									[kNewtonIrNodeType_Taccuracy]		= "accuracy",
									[kNewtonIrNodeType_Tanalog]		= "analog",
									[kNewtonIrNodeType_Tassign]		= "=",
									[kNewtonIrNodeType_TatSign]		= "@",
									[kNewtonIrNodeType_Tbits]		= "bits",
									[kNewtonIrNodeType_Tcolon]		= ":",
									[kNewtonIrNodeType_Tcomma]		= ",",
									[kNewtonIrNodeType_Tconstant]		= "constant",
									[kNewtonIrNodeType_Tcross]		= "cross",
									[kNewtonIrNodeType_Tdelay]		= "delay",
									[kNewtonIrNodeType_Tderivation]		= "derivation",
									[kNewtonIrNodeType_Tderivative]		= "derivative",
									[kNewtonIrNodeType_TdimensionallyAgnosticProportional]	= "o<",
									[kNewtonIrNodeType_TdimensionallyMatchingProportional]		= "~",
									[kNewtonIrNodeType_Tdimensionless]	= "dimensionless",
									[kNewtonIrNodeType_Tdiv]		= "/",
									[kNewtonIrNodeType_TdotDot]		= "..",
									[kNewtonIrNodeType_Tdot]		= "dot",
									[kNewtonIrNodeType_Tequals]		= "==",
									[kNewtonIrNodeType_Tequivalent]		= "~",
									[kNewtonIrNodeType_TerasureToken]	= "erasureToken",
									[kNewtonIrNodeType_Texponentiation]	= "**",
									[kNewtonIrNodeType_Tge]			= ">=",
									[kNewtonIrNodeType_Tgt]			= ">",
									[kNewtonIrNodeType_Ti2c]		= "i2c",
									[kNewtonIrNodeType_Tinclude]		= "include",
									[kNewtonIrNodeType_Tintegral]		= "integral",
									[kNewtonIrNodeType_Tinterface]		= "interface",
									[kNewtonIrNodeType_Tinvariant]		= "invariant",
									[kNewtonIrNodeType_Tle]			= "<=",
									[kNewtonIrNodeType_TleftBrace]		= "{",
									[kNewtonIrNodeType_TleftParen]		= "(",
									[kNewtonIrNodeType_TleftShift]		= "<<",
									[kNewtonIrNodeType_Tlt]			= "<",
									[kNewtonIrNodeType_Tminus]		= "-",
									[kNewtonIrNodeType_TminusMinus]		= "--",
									[kNewtonIrNodeType_Tmul]		= "*",
									[kNewtonIrNodeType_Tmutualinf]		= "><",
									[kNewtonIrNodeType_Tname]		= "name",
									[kNewtonIrNodeType_Tnil]		= "nil",
									[kNewtonIrNodeType_Tnone]		= "none",
									[kNewtonIrNodeType_Tplus]		= "+",
									[kNewtonIrNodeType_TplusPlus]		= "++",
									[kNewtonIrNodeType_Tprecision]		= "precision",
									[kNewtonIrNodeType_Trange]		= "range",
									[kNewtonIrNodeType_Tread]		= "read",
									[kNewtonIrNodeType_Trelated]		= "<->",
									[kNewtonIrNodeType_TrightBrace]		= "}",
									[kNewtonIrNodeType_TrightParen]		= ")",
									[kNewtonIrNodeType_TrightShift]		= ">>",
									[kNewtonIrNodeType_Tsemicolon]		= ";",
									[kNewtonIrNodeType_Tsensor]		= "sensor",
									[kNewtonIrNodeType_Tsignal]		= "signal",
									[kNewtonIrNodeType_Tspi]		= "spi",
									[kNewtonIrNodeType_Tsymbol]		= "symbol",
									[kNewtonIrNodeType_Tto]			= "to",
									[kNewtonIrNodeType_Tuncertainty]	= "uncertainty",
									[kNewtonIrNodeType_Twrite]		= "write",
									/*
									 *	End keywords
									 */


									/*
									 *	Begin tokens returned by lexer that are not literal keywords
									 */
									[kNewtonIrNodeType_Tidentifier]		= "identifier",
									[kNewtonIrNodeType_TnumericConst]	= "numeric constant",
									[kNewtonIrNodeType_TintegerConst]	= "integer constant",
									[kNewtonIrNodeType_TrealConst]		= "real-valued constant",
									[kNewtonIrNodeType_TstringConst]	= "string constant",
									[kNewtonIrNodeType_ZbadIdentifier]	= "bad identifier",
									[kNewtonIrNodeType_ZbadStringConst]	= "bad string const",
									[kNewtonIrNodeType_Zeof]		= "end of file",
									[kNewtonIrNodeType_Zepsilon]		= "epsilon",
									/*
									 *	End non-keyword tokens
									 */
								};
