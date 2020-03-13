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
									[kNewtonIrNodeType_Tcos]		= "cos",
									[kNewtonIrNodeType_Ttan]		= "tan",
									[kNewtonIrNodeType_Tcotan]		= "cotan",
									[kNewtonIrNodeType_Tsec]		= "sec",
									[kNewtonIrNodeType_Tcosec]		= "cosec",
									[kNewtonIrNodeType_Tarcsin]		= "arcsin",
									[kNewtonIrNodeType_Tarccos]		= "arccos",
									[kNewtonIrNodeType_Tarctan]		= "arctan",
									[kNewtonIrNodeType_Tarccotan]		= "arccotan",
									[kNewtonIrNodeType_Tarcsec]		= "arcsec",
									[kNewtonIrNodeType_Tarccosec]		= "arccosec",
									[kNewtonIrNodeType_Tsinh]		= "sinh",
									[kNewtonIrNodeType_Tcosh]		= "cosh",
									[kNewtonIrNodeType_Ttanh]		= "tanh",
									[kNewtonIrNodeType_Tcotanh]		= "cotanh",
									[kNewtonIrNodeType_Tsech]		= "sech",
									[kNewtonIrNodeType_Tcosech]		= "cosech",
									[kNewtonIrNodeType_Tarcsinh]		= "arcsinh",
									[kNewtonIrNodeType_Tarccosh]		= "arccosh",
									[kNewtonIrNodeType_Tarctanh]		= "arctanh",
									[kNewtonIrNodeType_Tarccotanh]		= "arccotanh",
									[kNewtonIrNodeType_Tarcsech]		= "arcsech",
									[kNewtonIrNodeType_Tarccosech]		= "arccosech",
									[kNewtonIrNodeType_Texp]		= "exp",
									[kNewtonIrNodeType_Tsqrt]		= "sqrt",
									[kNewtonIrNodeType_Tln]		= "ln",
									[kNewtonIrNodeType_Tlog10]		= "log10",
									[kNewtonIrNodeType_Tlog2]		= "log2",
									[kNewtonIrNodeType_TBernoulli]		= "BernoulliDistribution",
									[kNewtonIrNodeType_TBeta]		= "BetaDistribution",
									[kNewtonIrNodeType_TBetaBinomial]	= "BetaBinomialDistribution",
									[kNewtonIrNodeType_TBetaPrime]		= "BetaPrimeDistribution",
									[kNewtonIrNodeType_TBinomial]		= "BinomialDistribution",
									[kNewtonIrNodeType_TCauchy]		= "CauchyDistribution",
									[kNewtonIrNodeType_TDirichlet]		= "DirichletDistribution",
 									[kNewtonIrNodeType_TEnglish]		= "English",
									[kNewtonIrNodeType_TErlang]		= "ErlangDistribution",
									[kNewtonIrNodeType_TExponential]	= "ExponentialDistribution",
									[kNewtonIrNodeType_TExtremeValue]	= "ExtremeValueDistribution",
									[kNewtonIrNodeType_TF]			= "FDistribution",
									[kNewtonIrNodeType_TFermiDirac]		= "FermiDiracDistribution",
									[kNewtonIrNodeType_TFisherZ]		= "FisherZDistribution]",
									[kNewtonIrNodeType_TGamma]		= "GammaDistribution",
									[kNewtonIrNodeType_TGaussian]		= "GaussianDistribution",
									[kNewtonIrNodeType_TGibrat]		= "GibratDistribution",
									[kNewtonIrNodeType_TGumbel]		= "GumbelDistribution",
									[kNewtonIrNodeType_TLaplacian]		= "LaplacianDistribution",
									[kNewtonIrNodeType_TLogNormal]		= "LogNormalDistribution",
									[kNewtonIrNodeType_TLogSeries]		= "LogSeriesDistribution",
									[kNewtonIrNodeType_TLogitNormal]	= "LogitNormalDistribution",
									[kNewtonIrNodeType_TMaxwell]		= "MaxwellDistribution",
									[kNewtonIrNodeType_TMultinomial]	= "MultinomialDistribution",
									[kNewtonIrNodeType_TNegativeBinomial]	= "NegativeBinomialDistribution",
									[kNewtonIrNodeType_TPareto]		= "ParetoDistribution",
									[kNewtonIrNodeType_TPearsonIII]		= "PearsonIIIDistribution",
									[kNewtonIrNodeType_TPoisson]		= "PoissonDistribution",
									[kNewtonIrNodeType_TRayleigh]		= "RayleighDistribution",
									[kNewtonIrNodeType_TStudentT]		= "StudentTDistribution",
									[kNewtonIrNodeType_TStudentZ]		= "StudentZDistribution",
									[kNewtonIrNodeType_TWeibull]		= "WeibullDistribution",
									[kNewtonIrNodeType_TXi]			= "XiDistribution",
									[kNewtonIrNodeType_TXiSquared]		= "XiSquaredDistribution",
									[kNewtonIrNodeType_TUnconstrained]		= "UnconstrainedDistribution",
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
									[kNewtonIrNodeType_TdimensionallyMatchingProportional]	= "~",
									[kNewtonIrNodeType_Tdimensionless]	= "dimensionless",
									[kNewtonIrNodeType_Tdiv]		= "/",
									[kNewtonIrNodeType_TdotDot]		= "..",
									[kNewtonIrNodeType_Tdot]		= "dot",
									[kNewtonIrNodeType_Tequals]		= "==",
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
