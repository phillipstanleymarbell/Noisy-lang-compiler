/*
	Authored 2015. Phillip Stanley-Marbell.
	Modified, 2016-2017, Jonathan Lim to add Newton hooks.
	Modified, 2019, Kiseki Hirakawa

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

#include <llvm-c/Core.h>

typedef enum
{
	kCommonVerbosityDebugLexer	=	(1 << 0),
	kCommonVerbosityDebugParser	=	(1 << 1),
	kCommonVerbosityDebugAST	=	(1 << 2),
	kCommonVerbosityDebugFF		=	(1 << 3),
	
	/*
	 *	Code depends on this bringing up the rear.
	 */	
	kCommonVerbosityDebugMax,
} VerbosityType;



typedef enum
{
	/*
	 *	Noisy Tokens
	 */
	kNoisyIrNodeType_TMin,
	kNoisyIrNodeType_TA,
	kNoisyIrNodeType_TAmpere,
	kNoisyIrNodeType_TK,
	kNoisyIrNodeType_TKelvin,
	kNoisyIrNodeType_Tacceleration,
	kNoisyIrNodeType_Tadt,
	kNoisyIrNodeType_Talpha,
	kNoisyIrNodeType_Tand,
	kNoisyIrNodeType_TandAssign,
	kNoisyIrNodeType_Tanglerate,
	kNoisyIrNodeType_TarithmeticAnd,
	kNoisyIrNodeType_Tarray,
	kNoisyIrNodeType_Tarrow,
	kNoisyIrNodeType_Tassign,
	kNoisyIrNodeType_Tasterisk,
	kNoisyIrNodeType_TasteriskAssign,
	kNoisyIrNodeType_Tat,
	kNoisyIrNodeType_TbitwiseOr,
	kNoisyIrNodeType_Tbool,
	kNoisyIrNodeType_TboolConst,
	kNoisyIrNodeType_Tcandela,
	kNoisyIrNodeType_Tcardinality,
	kNoisyIrNodeType_Tcd,
	kNoisyIrNodeType_Tcentralmoment,
	kNoisyIrNodeType_TchannelOperator,
	kNoisyIrNodeType_TchannelOperatorAssign,
	kNoisyIrNodeType_TcharConst,
	kNoisyIrNodeType_Tcolon,
	kNoisyIrNodeType_TcolonAssign,
	kNoisyIrNodeType_TcolonColon,
	kNoisyIrNodeType_Tcomma,
	kNoisyIrNodeType_Tcomplement,
	kNoisyIrNodeType_Tcomplex,
	kNoisyIrNodeType_Tconst,
	kNoisyIrNodeType_Tcrossproduct,
	kNoisyIrNodeType_Tcurrent,
	kNoisyIrNodeType_Tdimensions,
	kNoisyIrNodeType_Tdistance,
	kNoisyIrNodeType_Tdistortions,
	kNoisyIrNodeType_Tdivide,
	kNoisyIrNodeType_TdivideAssign,
	kNoisyIrNodeType_Tdot,
	kNoisyIrNodeType_Tdotproduct,
	kNoisyIrNodeType_Tepsilon,
	kNoisyIrNodeType_Tequals,
	kNoisyIrNodeType_Terasures,
	kNoisyIrNodeType_Texists,
	kNoisyIrNodeType_Tfalse,
	kNoisyIrNodeType_Tfixed,
	kNoisyIrNodeType_Tfloat128,
	kNoisyIrNodeType_Tfloat16,
	kNoisyIrNodeType_Tfloat32,
	kNoisyIrNodeType_Tfloat4,
	kNoisyIrNodeType_Tfloat64,
	kNoisyIrNodeType_Tfloat8,
	kNoisyIrNodeType_Tfor,
	kNoisyIrNodeType_Tforall,
	kNoisyIrNodeType_Tfourier,
	kNoisyIrNodeType_Tfrequencies,
	kNoisyIrNodeType_Tfrom,
	kNoisyIrNodeType_Tfunction,
	kNoisyIrNodeType_Tgiven,
	kNoisyIrNodeType_TgreaterThan,
	kNoisyIrNodeType_TgreaterThanEqual,
	kNoisyIrNodeType_Thead,
	kNoisyIrNodeType_Thighpass,
	kNoisyIrNodeType_Tidentifier,
	kNoisyIrNodeType_Tiff,
	kNoisyIrNodeType_Timplies,
	kNoisyIrNodeType_Tin,
	kNoisyIrNodeType_Tint128,
	kNoisyIrNodeType_Tint16,
	kNoisyIrNodeType_Tint32,
	kNoisyIrNodeType_Tint4,
	kNoisyIrNodeType_Tint64,
	kNoisyIrNodeType_Tint8,
	kNoisyIrNodeType_TintegerConst,
	kNoisyIrNodeType_TisPermutationOf,
	kNoisyIrNodeType_Titerate,
	kNoisyIrNodeType_Tkg,
	kNoisyIrNodeType_Tkilogram,
	kNoisyIrNodeType_Tlatency,
	kNoisyIrNodeType_TleftBrace,
	kNoisyIrNodeType_TleftBracket,
	kNoisyIrNodeType_TleftParens,
	kNoisyIrNodeType_TleftShift,
	kNoisyIrNodeType_TleftShiftAssign,
	kNoisyIrNodeType_Tlength,
	kNoisyIrNodeType_TlessThan,
	kNoisyIrNodeType_TlessThanEqual,
	kNoisyIrNodeType_Tlist,
	kNoisyIrNodeType_Tload,
	kNoisyIrNodeType_Tlog,
	kNoisyIrNodeType_TlogicalAnd,
	kNoisyIrNodeType_TlogicalOr,
	kNoisyIrNodeType_Tlowpass,
	kNoisyIrNodeType_Tluminosity,
	kNoisyIrNodeType_Tm,
	kNoisyIrNodeType_Tmagneticfluxdensity,
	kNoisyIrNodeType_Tmagnitudes,
	kNoisyIrNodeType_Tmass,
	kNoisyIrNodeType_Tmatch,
	kNoisyIrNodeType_Tmatchseq,
	kNoisyIrNodeType_Tmaterial,
	kNoisyIrNodeType_Tmax,
	kNoisyIrNodeType_Tmeasurement,
	kNoisyIrNodeType_Tmeter,
	kNoisyIrNodeType_Tmin,
	kNoisyIrNodeType_Tminus,
	kNoisyIrNodeType_TminusAssign,
	kNoisyIrNodeType_TminusMinus,
	kNoisyIrNodeType_Tmodule,
	kNoisyIrNodeType_Tmole,
	kNoisyIrNodeType_Tnat128,
	kNoisyIrNodeType_Tnat16,
	kNoisyIrNodeType_Tnat32,
	kNoisyIrNodeType_Tnat4,
	kNoisyIrNodeType_Tnat64,
	kNoisyIrNodeType_Tnat8,
	kNoisyIrNodeType_Tnil,
	kNoisyIrNodeType_Tnot,
	kNoisyIrNodeType_TnotEqual,
	kNoisyIrNodeType_Tnrt,
	kNoisyIrNodeType_Tof,
	kNoisyIrNodeType_Tomega,
	kNoisyIrNodeType_TorAssign,
	kNoisyIrNodeType_Tparallel,
	kNoisyIrNodeType_Tpath,
	kNoisyIrNodeType_Tpercent,
	kNoisyIrNodeType_TpercentAssign,
	kNoisyIrNodeType_Tplus,
	kNoisyIrNodeType_TplusAssign,
	kNoisyIrNodeType_TplusPlus,
	kNoisyIrNodeType_Tpow,
	kNoisyIrNodeType_Tpowerset,
	kNoisyIrNodeType_Tpredicate,
	kNoisyIrNodeType_Tpressure,
	kNoisyIrNodeType_Tprobdef,
	kNoisyIrNodeType_Tproduct,
	kNoisyIrNodeType_Tquantize,
	kNoisyIrNodeType_Trational,
	kNoisyIrNodeType_Trawmem,
	kNoisyIrNodeType_TrealConst,
	kNoisyIrNodeType_Trelativehumidity,
	kNoisyIrNodeType_Treturn,
	kNoisyIrNodeType_Treverse,
	kNoisyIrNodeType_TrightBrace,
	kNoisyIrNodeType_TrightBracket,
	kNoisyIrNodeType_TrightParens,
	kNoisyIrNodeType_TrightShift,
	kNoisyIrNodeType_TrightShiftAssign,
	kNoisyIrNodeType_Ts,
	kNoisyIrNodeType_Tsample,
	kNoisyIrNodeType_Tsamples,
	kNoisyIrNodeType_Tsecond,
	kNoisyIrNodeType_Tsemicolon,
	kNoisyIrNodeType_Tsequence,
	kNoisyIrNodeType_Tset,
	kNoisyIrNodeType_TsetCross,
	kNoisyIrNodeType_TsetIntersect,
	kNoisyIrNodeType_Tsigfigs,
	kNoisyIrNodeType_Tsignal,
	kNoisyIrNodeType_Tsort,
	kNoisyIrNodeType_Tstring,
	kNoisyIrNodeType_TstringConst,
	kNoisyIrNodeType_Tstrongdominates,
	kNoisyIrNodeType_Tsum,
	kNoisyIrNodeType_Ttail,
	kNoisyIrNodeType_Ttailtip,
	kNoisyIrNodeType_Ttau,
	kNoisyIrNodeType_Ttderivative,
	kNoisyIrNodeType_Ttemperature,
	kNoisyIrNodeType_Tinterrupt,
	kNoisyIrNodeType_Texception,
	kNoisyIrNodeType_Ttilde,
	kNoisyIrNodeType_Ttime,
	kNoisyIrNodeType_Ttimebase,
	kNoisyIrNodeType_Ttimeseries,
	kNoisyIrNodeType_Ttintegral,
	kNoisyIrNodeType_Tto,
	kNoisyIrNodeType_Ttrue,
	kNoisyIrNodeType_Ttype,
	kNoisyIrNodeType_Ttypeannote,
	kNoisyIrNodeType_Ttypemax,
	kNoisyIrNodeType_Ttypemin,
	kNoisyIrNodeType_Tuncertainty,
	kNoisyIrNodeType_Tunits,
	kNoisyIrNodeType_Tvalfn,
	kNoisyIrNodeType_Tvector,
	kNoisyIrNodeType_Tweakdominates,
	kNoisyIrNodeType_Twith,
	kNoisyIrNodeType_Txor,
	kNoisyIrNodeType_TxorAssign,


	/*
	 *	Code depends on this bringing up the rear for Noisy tokens.
	 */
	kNoisyIrNodeType_TMax,

	kNoisyIrNodeType_ZbadCharConst,
	kNoisyIrNodeType_ZbadStringConst,
	kNoisyIrNodeType_ZbadIdentifier,

	/*
	 *	Epsilon and $ (i.e., EOF) need to be in same ordinal set as tokens
	 */
	kNoisyIrNodeType_Zepsilon,
	kNoisyIrNodeType_Zeof,




	/*
	 *	Noisy grammar productions.
	 */
	kNoisyIrNodeType_PMin,
	kNoisyIrNodeType_PaccuracyTolerance,
	kNoisyIrNodeType_PadtTypeDecl,
	kNoisyIrNodeType_PanonAggrCastExpr,
	kNoisyIrNodeType_PanonAggregateType,
	kNoisyIrNodeType_Parith2BoolOp,
	kNoisyIrNodeType_ParithParamOrConst,
	kNoisyIrNodeType_ParrayCastExpr,
	kNoisyIrNodeType_ParrayType,
	kNoisyIrNodeType_PassignOp,
	kNoisyIrNodeType_PassignmentStatement,
	kNoisyIrNodeType_PbaseConst,
	kNoisyIrNodeType_PbasicSignal,
	kNoisyIrNodeType_PbasicSignalDimension,
	kNoisyIrNodeType_PbasicSignalUnits,
	kNoisyIrNodeType_PbasicType,
	kNoisyIrNodeType_PchanEventExpr,
	kNoisyIrNodeType_PcmpOp,
	kNoisyIrNodeType_PcomplexCastExpr,
	kNoisyIrNodeType_PcomplexType,
	kNoisyIrNodeType_PconstSetExpr,
	kNoisyIrNodeType_PconstantDecl,
	kNoisyIrNodeType_PdimensionArithExpr,
	kNoisyIrNodeType_PdimensionArithFactor,
	kNoisyIrNodeType_PdimensionArithTerm,
	kNoisyIrNodeType_PdimensionsDesignation,
	kNoisyIrNodeType_Pelement,
	kNoisyIrNodeType_Pexpression,
	kNoisyIrNodeType_Pfactor,
	kNoisyIrNodeType_PfieldSelect,
	kNoisyIrNodeType_PfixedType,
	kNoisyIrNodeType_PfunctionDecl,
	kNoisyIrNodeType_PfunctionDefn,
	kNoisyIrNodeType_PguardedExpressionList,
	kNoisyIrNodeType_PguardedStatementList,
	kNoisyIrNodeType_PhighPrecedenceArith2ArithOp,
	kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp,
	kNoisyIrNodeType_PhighPrecedenceBinaryOp,
	kNoisyIrNodeType_PhighPrecedenceBoolSetOp,
	kNoisyIrNodeType_PidentifierList,
	kNoisyIrNodeType_PidentifierOrNil,
	kNoisyIrNodeType_PidentifierOrNilList,
	kNoisyIrNodeType_PidxInitList,
	kNoisyIrNodeType_PinitList,
	kNoisyIrNodeType_PintParamOrConst,
	kNoisyIrNodeType_PintegerType,
	kNoisyIrNodeType_PiterateStatement,
	kNoisyIrNodeType_PlatencyTolerance,
	kNoisyIrNodeType_PlistCastExpr,
	kNoisyIrNodeType_PlistType,
	kNoisyIrNodeType_PloadExpr,
	kNoisyIrNodeType_PlossTolerance,
	kNoisyIrNodeType_PlowPrecedenceArith2ArithOp,
	kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp,
	kNoisyIrNodeType_PlowPrecedenceBinaryOp,
	kNoisyIrNodeType_PlowPrecedenceBoolSetOp,
	kNoisyIrNodeType_PmatchStatement,
	kNoisyIrNodeType_PmaxForExpr,
	kNoisyIrNodeType_PminForExpr,
	kNoisyIrNodeType_PmoduleDecl,
	kNoisyIrNodeType_PmoduleDeclBody,
	kNoisyIrNodeType_PmoduleTypeNameDecl,
	kNoisyIrNodeType_PnamegenInvokeShorthand,
	kNoisyIrNodeType_PnumericConst,
	kNoisyIrNodeType_PnumericType,
	kNoisyIrNodeType_PoperatorToleranceDecl,
	kNoisyIrNodeType_PorderingHead,
	kNoisyIrNodeType_PparallelStatement,
	kNoisyIrNodeType_PpredArithExpr,
	kNoisyIrNodeType_PpredArithFactor,
	kNoisyIrNodeType_PpredArithTerm,
	kNoisyIrNodeType_PpredExpr,
	kNoisyIrNodeType_PpredFactor,
	kNoisyIrNodeType_PpredStmt,
	kNoisyIrNodeType_PpredStmtList,
	kNoisyIrNodeType_PpredTerm,
	kNoisyIrNodeType_PpredicateFnDecl,
	kNoisyIrNodeType_PpredicateFnDefn,
	kNoisyIrNodeType_PprobdefDecl,
	kNoisyIrNodeType_PproblemDefn,
	kNoisyIrNodeType_PproductForExpr,
	kNoisyIrNodeType_Pprogram,
	kNoisyIrNodeType_PqualifiedIdentifier,
	kNoisyIrNodeType_PquantifiedBoolTerm,
	kNoisyIrNodeType_PquantifierOp,
	kNoisyIrNodeType_PquantizeExpression,
	kNoisyIrNodeType_PrationalCastExpr,
	kNoisyIrNodeType_PrationalType,
	kNoisyIrNodeType_PreadTypeSignature,
	kNoisyIrNodeType_PrealParamOrConst,
	kNoisyIrNodeType_PrealType,
	kNoisyIrNodeType_PreturnSignature,
	kNoisyIrNodeType_PreturnStatement,
	kNoisyIrNodeType_PsampleExpression,
	kNoisyIrNodeType_PscopedPredStmtList,
	kNoisyIrNodeType_PscopedStatementList,
	kNoisyIrNodeType_PsequenceStatement,
	kNoisyIrNodeType_PsetCastExpr,
	kNoisyIrNodeType_PsetCmpOp,
	kNoisyIrNodeType_PsetCmpTerm,
	kNoisyIrNodeType_PsetExpr,
	kNoisyIrNodeType_PsetFactor,
	kNoisyIrNodeType_PsetHead,
	kNoisyIrNodeType_PsetTerm,
	kNoisyIrNodeType_PsetType,
	kNoisyIrNodeType_PsigfigDesignation,
	kNoisyIrNodeType_PsignalDesignation,
	kNoisyIrNodeType_Psignature,
	kNoisyIrNodeType_PstarInitList,
	kNoisyIrNodeType_Pstatement,
	kNoisyIrNodeType_PstatementList,
	kNoisyIrNodeType_PstringParamOrConst,
	kNoisyIrNodeType_PsumForExpr,
	kNoisyIrNodeType_PsumProdMinMaxBody,
	kNoisyIrNodeType_Pterm,
	kNoisyIrNodeType_PtimeseriesDesignation,
	kNoisyIrNodeType_Ptolerance,
	kNoisyIrNodeType_Ptuple,
	kNoisyIrNodeType_PtupleType,
	kNoisyIrNodeType_PtupleValue,
	kNoisyIrNodeType_PtypeAnnoteDecl,
	kNoisyIrNodeType_PtypeAnnoteItem,
	kNoisyIrNodeType_PtypeAnnoteList,
	kNoisyIrNodeType_PtypeDecl,
	kNoisyIrNodeType_PtypeExpr,
	kNoisyIrNodeType_PtypeMaxExpr,
	kNoisyIrNodeType_PtypeMinExpr,
	kNoisyIrNodeType_PtypeName,
	kNoisyIrNodeType_PtypeParameterList,
	kNoisyIrNodeType_PunaryBoolOp,
	kNoisyIrNodeType_PunaryOp,
	kNoisyIrNodeType_PunarySetOp,
	kNoisyIrNodeType_PunitsArithExpr,
	kNoisyIrNodeType_PunitsArithFactor,
	kNoisyIrNodeType_PunitsArithTerm,
	kNoisyIrNodeType_PunitsDesignation,
	kNoisyIrNodeType_PvalfnSignature,
	kNoisyIrNodeType_PvarIntro,
	kNoisyIrNodeType_PvarIntroList,
	kNoisyIrNodeType_PvarTuple,
	kNoisyIrNodeType_PvectorTypeDecl,
	kNoisyIrNodeType_PwriteTypeSignature,

	/*
	 *	Code depends on this bringing up the rear for Noisy productions.
	 */
	kNoisyIrNodeType_PMax,

	/*
	 *	This 'pre-node Op' node is inserted before members of a production sequence
	 */
	kNoisyIrNodeType_Xseq,

	/*
	 *	Code depends on this bringing up the rear for Noisy.
	 */
	kNoisyIrNodeTypeMax,


	/*
	 *	Newton tokens
	 */
	kNewtonIrNodeType_TMin,	
	/*
	 *	From auto-generated sets:
	 */
	kNewtonIrNodeType_Tidentifier,
	kNewtonIrNodeType_TstringConst,
	kNewtonIrNodeType_TrealConst,
	kNewtonIrNodeType_TintegerConst,
	kNewtonIrNodeType_Totherwise,
	kNewtonIrNodeType_Tcase,
	kNewtonIrNodeType_Tpiecewise,
	kNewtonIrNodeType_TrightArrow,
	kNewtonIrNodeType_Tpercent,
	kNewtonIrNodeType_TbitwiseOr,
	kNewtonIrNodeType_TrightBracket,
	kNewtonIrNodeType_TleftBracket,
	kNewtonIrNodeType_Tinclude,
	kNewtonIrNodeType_Tassign,
	kNewtonIrNodeType_Tdef,
	kNewtonIrNodeType_TminusMinus,
	kNewtonIrNodeType_TplusPlus,
	kNewtonIrNodeType_TdotDot,
	kNewtonIrNodeType_Tdimensionless,
	kNewtonIrNodeType_Tbits,
	kNewtonIrNodeType_Twrite,
	kNewtonIrNodeType_Tuncertainty,
	kNewtonIrNodeType_Tto,
	kNewtonIrNodeType_TStudentT,
	kNewtonIrNodeType_Tsymbol,
	kNewtonIrNodeType_Tspi,
	kNewtonIrNodeType_Tsignal,
	kNewtonIrNodeType_Tsensor,
	kNewtonIrNodeType_Tsemicolon,
	kNewtonIrNodeType_TrightShift,
	kNewtonIrNodeType_TrightParen,
	kNewtonIrNodeType_TrightBrace,
	kNewtonIrNodeType_Trelated,
	kNewtonIrNodeType_Tread,
	kNewtonIrNodeType_Trange,
	kNewtonIrNodeType_TdimensionallyAgnosticProportional,
	kNewtonIrNodeType_Tprecision,
	kNewtonIrNodeType_Tplus,
	kNewtonIrNodeType_Tnone,
	kNewtonIrNodeType_Tnil,
	kNewtonIrNodeType_Tname,
	kNewtonIrNodeType_Tmutualinf,
	kNewtonIrNodeType_Tmul,
	kNewtonIrNodeType_Tminus,
	kNewtonIrNodeType_TLaplacian,
	kNewtonIrNodeType_Tlt,
	kNewtonIrNodeType_TleftShift,
	kNewtonIrNodeType_TleftParen,
	kNewtonIrNodeType_TleftBrace,
	kNewtonIrNodeType_Tle,
	kNewtonIrNodeType_Tinvariant,
	kNewtonIrNodeType_Tinterface,
	kNewtonIrNodeType_Tintegral,
	kNewtonIrNodeType_Ti2c,
	kNewtonIrNodeType_Tgt,
	kNewtonIrNodeType_Tge,
	kNewtonIrNodeType_Tlog2,
	kNewtonIrNodeType_Tlog10,
	kNewtonIrNodeType_Tln,
	kNewtonIrNodeType_Tsqrt,
	kNewtonIrNodeType_Texp,
	kNewtonIrNodeType_Tarccosech,
	kNewtonIrNodeType_Tarcsech,
	kNewtonIrNodeType_Tarccotanh,
	kNewtonIrNodeType_Tarctanh,
	kNewtonIrNodeType_Tarccosh,
	kNewtonIrNodeType_Tarcsinh,
	kNewtonIrNodeType_Tcosech,
	kNewtonIrNodeType_Tsech,
	kNewtonIrNodeType_Tcotanh,
	kNewtonIrNodeType_Ttanh,
	kNewtonIrNodeType_Tcosh,
	kNewtonIrNodeType_Tsinh,
	kNewtonIrNodeType_Tarccosec,
	kNewtonIrNodeType_Tarcsec,
	kNewtonIrNodeType_Tarccotan,
	kNewtonIrNodeType_Tarctan,
	kNewtonIrNodeType_Tarccos,
	kNewtonIrNodeType_Tarcsin,
	kNewtonIrNodeType_Tcosec,
	kNewtonIrNodeType_Tsec,
	kNewtonIrNodeType_Tcotan,
	kNewtonIrNodeType_Ttan,
	kNewtonIrNodeType_Tcos,
	kNewtonIrNodeType_Tsin,
	kNewtonIrNodeType_TUnconstrained,
	kNewtonIrNodeType_TXiSquared,
	kNewtonIrNodeType_TXi,
	kNewtonIrNodeType_TF,
	kNewtonIrNodeType_TExtremeValue,
	kNewtonIrNodeType_TPearsonIII,
	kNewtonIrNodeType_TGibrat,
	kNewtonIrNodeType_TRayleigh,
	kNewtonIrNodeType_TGumbel,
	kNewtonIrNodeType_TLogSeries,
	kNewtonIrNodeType_TFisherZ,
	kNewtonIrNodeType_TFermiDirac,
	kNewtonIrNodeType_TMaxwell,
	kNewtonIrNodeType_TErlang,
	kNewtonIrNodeType_TWeibull,
	kNewtonIrNodeType_TStudentZ,
	kNewtonIrNodeType_TBetaPrime,
	kNewtonIrNodeType_TPareto,
	kNewtonIrNodeType_TLogNormal,
	kNewtonIrNodeType_TCauchy,
	kNewtonIrNodeType_TDirichlet,
	kNewtonIrNodeType_TLogitNormal,
	kNewtonIrNodeType_TBeta,
	kNewtonIrNodeType_TMultinomial,
	kNewtonIrNodeType_TGamma,
	kNewtonIrNodeType_TExponential,
	kNewtonIrNodeType_TBetaBinomial,
	kNewtonIrNodeType_TNegativeBinomial,
	kNewtonIrNodeType_TPoisson,
	kNewtonIrNodeType_TBinomial,
	kNewtonIrNodeType_TBernoulli,
	kNewtonIrNodeType_TGaussian,
	kNewtonIrNodeType_Texponentiation,
	kNewtonIrNodeType_TerasureToken,
	kNewtonIrNodeType_TdimensionallyMatchingProportional,
	kNewtonIrNodeType_Tequals,
	kNewtonIrNodeType_Tdot,
	kNewtonIrNodeType_Tdiv,
	kNewtonIrNodeType_Tderivative,
	kNewtonIrNodeType_Tderivation,
	kNewtonIrNodeType_Tdelay,
	kNewtonIrNodeType_Tcross,
	kNewtonIrNodeType_Tconstant,
	kNewtonIrNodeType_Tcomma,
	kNewtonIrNodeType_Tcolon,
	kNewtonIrNodeType_TatSign,
	kNewtonIrNodeType_Tanalog,
	kNewtonIrNodeType_Taccuracy,
	kNewtonIrNodeType_TEnglish,

	kNewtonIrNodeType_ZbadIdentifier,
	kNewtonIrNodeType_ZbadStringConst,
	kNewtonIrNodeType_Zeof,
	kNewtonIrNodeType_Zepsilon,

	/*
	 *	Code depends on this bringing up the rear for Newton Tokens.
	 */
	kNewtonIrNodeType_TMax,



	/*
	 *	Newton grammar productions
	 */
	kNewtonIrNodeType_PMin,
	/*
	 *	From auto-generated sets:
	 */
	kNewtonIrNodeType_PunaryOp,
	kNewtonIrNodeType_PlowPrecedenceBinaryOp,
	kNewtonIrNodeType_PhighPrecedenceBinaryOp,
	kNewtonIrNodeType_Ptranscendental,
	kNewtonIrNodeType_Pdistribution,
	kNewtonIrNodeType_PparameterValueList,
	kNewtonIrNodeType_PdistributionFactor,
	kNewtonIrNodeType_Pfactor,
	kNewtonIrNodeType_Pterm,
	kNewtonIrNodeType_Pexpression,
	kNewtonIrNodeType_PnumericConstTuple,
	kNewtonIrNodeType_PnumericConstTupleList,
	kNewtonIrNodeType_PprecisionStatement,
	kNewtonIrNodeType_PaccuracyStatement,
	kNewtonIrNodeType_PerasureValueStatement,
	kNewtonIrNodeType_PuncertaintyStatement,
	kNewtonIrNodeType_PrangeStatement,
	kNewtonIrNodeType_ParithmeticCommand,
	kNewtonIrNodeType_PdelayCommand,
	kNewtonIrNodeType_PwriteRegisterCommand,
	kNewtonIrNodeType_PreadRegisterCommand,
	kNewtonIrNodeType_PsensorInterfaceCommand,
	kNewtonIrNodeType_PsensorInterfaceCommandList,
	kNewtonIrNodeType_PsensorInterfaceType,
	kNewtonIrNodeType_PsensorInterfaceStatement,
	kNewtonIrNodeType_PsensorProperty,
	kNewtonIrNodeType_PsensorPropertyList,
	kNewtonIrNodeType_PnumericFactor,
	kNewtonIrNodeType_PnumericTerm,
	kNewtonIrNodeType_PnumericExpression,
	kNewtonIrNodeType_Punit,
	kNewtonIrNodeType_PunitFactor,
	kNewtonIrNodeType_PunitTerm,
	kNewtonIrNodeType_PunitExpression,
	kNewtonIrNodeType_PlanguageSetting,
	kNewtonIrNodeType_PcomparisonOperator,
	kNewtonIrNodeType_PfunctionalOperator,
	kNewtonIrNodeType_PvectorOp,
	kNewtonIrNodeType_PexponentiationOperator,
	kNewtonIrNodeType_PhighPrecedenceOperator,
	kNewtonIrNodeType_PhighPrecedenceQuantityOperator,
	kNewtonIrNodeType_PlowPrecedenceOperator,
	kNewtonIrNodeType_Pquantity,
	kNewtonIrNodeType_PquantityFactor,
	kNewtonIrNodeType_PquantityTerm,
	kNewtonIrNodeType_PquantityExpression,
	kNewtonIrNodeType_PcaseStatement,
	kNewtonIrNodeType_PcaseStatementList,
	kNewtonIrNodeType_PpiecewiseConstraint,
	kNewtonIrNodeType_PcallParameterTuple,
	kNewtonIrNodeType_Pconstraint,
	kNewtonIrNodeType_PconstraintList,
	kNewtonIrNodeType_Pparameter,
	kNewtonIrNodeType_PparameterTuple,
	kNewtonIrNodeType_PsubdimensionTuple,
	kNewtonIrNodeType_PderivationStatement,
	kNewtonIrNodeType_PsymbolStatement,
	kNewtonIrNodeType_PsensorStatement,
	kNewtonIrNodeType_PsignalUncertaintyStatement,
	kNewtonIrNodeType_PnameStatement,
	kNewtonIrNodeType_PsensorDefinition,
	kNewtonIrNodeType_PbaseSignalDefinition,
	kNewtonIrNodeType_PinvariantDefinition,
	kNewtonIrNodeType_PconstantDefinition,
	kNewtonIrNodeType_Prule,
	kNewtonIrNodeType_PruleList,
	kNewtonIrNodeType_PnewtonDescription,
	kNewtonIrNodeType_PnumericConst,

	/*
	 *	Code depends on this bringing up the rear for Newton Productions.
	 */
	kNewtonIrNodeType_PMax,


	/*
	 *	Code depends on this bringing up the rear for Newton.
	 */
	kNewtonIrNodeTypeMax,

	/*
	 *	Code depends on this bringing up the rear for both Newton and Noisy.
	 */
	kCommonIrNodeTypeMax
} IrNodeType;



typedef enum
{
	kCommonVerbosityVerbose				= (1 << 0),
	kCommonVerbosityActionTrace			= (1 << 1),
	kCommonVerbosityCallTrace			= (1 << 2),
	kCommonVerbosityPostScanStreamCheck		= (1 << 3),
	kCommonVerbosityPreScanStreamCheck		= (1 << 4),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kCommonVerbosityMax,
} CommonVerbosity;



typedef enum
{
	kNewtonIrPassDimensionalMatrixAnnotation		= (1 << 0),
	kNewtonIrPassDimensionalMatrixPiGroups			= (1 << 1),
	kNewtonIrPassDimensionalMatrixKernelRowCanonicalization	= (1 << 2),
	kNewtonIrPassDimensionalMatrixPiGroupSorted		= (1 << 3),
	kNewtonIrPassDimensionalMatrixPiGroupsWeedOutDuplicates	= (1 << 4),
	kNewtonIrPassDimensionalMatrixKernelPrinter		= (1 << 5),
	kNewtonIrPassDimensionalMatrixConvertToList		= (1 << 6),
	kNewtonIrPassDimensionalMatrixAnnotationByBody          = (1 << 7),
	kNewtonIrPassDimensionalMatrixKernelPrinterFromBody	= (1 << 8),
	KNewtonIrPassDimensionalMatrixConstantPi		= (1 << 9),
	kNewtonIrPassInvariantSignalAnnotation			= (1 << 10),

	kNewtonIrPassPiGroupsSignalAnnotation			= (1 << 11),
	kNewtonIrPassLLVMIRDimensionCheck								= (1 << 12),
	kNewtonIrPassSensorsDisable				= (1 << 13),
	kNewtonIrPassLLVMIRLivenessAnalysis				= (1 << 14),
	kNewtonirPassLLVMIROptimizeByRange				= (1 << 15),
	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNewtonIrPassMax,
} NewtonIrPasses;



typedef enum
{
	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyIrPassMax,
} NoisyIrPasses;



typedef enum
{
	kNewtonIrBackendDot				= (1 << 0),
	kNewtonIrBackendProtobuf			= (1 << 1),
	kNewtonIrBackendSmt				= (1 << 2),
	kNewtonIrBackendC				= (1 << 3),
	kNewtonIrBackendRTL				= (1 << 4),
	kNewtonIrBackendTargetParam		= (1 << 5),

	/*
	 *	The LaTeX backend isn't a true backend per se, but rather
	 *	the flag enables dumping the LaTeX / KaTeX when the kernel
	 *	dumping is happening.
	 */
	kNewtonIrBackendLatex				= (1 << 6),
	kNewtonIrBackendEstimatorSynthesis	= (1 << 7),
	kNewtonIrBackendIpsa				= (1 << 8),

	kNewtonIrBackendSignalTypedefHeader	= (1 << 9),
	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNewtonIrBackendMax,
} NewtonIrBackends;


typedef enum
{
	kNoisyIrBackendDot				= (1 << 0),
	kNoisyIrBackendProtobuf				= (1 << 1),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyIrBackendMax,
} NoisyIrBackends;

typedef enum
{
	kCommonDotDetailLevelNoText			= (1 << 0),
	kCommonDotDetailLevelNoNilNodes			= (1 << 1),
	
	/*
	 *	Code depends on this bringing up the rear.
	 */
	kCommonDotDetailLevelMax,
} DetailLevel;



typedef enum
{
	kCommonIrNodeColorDotBackendColoring		= (1 << 0),
	kCommonIrNodeColorProtobufBackendColoring	= (1 << 1),
	kCommonIrNodeColorTreeTransformedColoring	= (1 << 2),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kCommonIrNodeColor,
} IrNodeColor;



typedef enum
{
	kCommonMaxBufferLength				= 65536,
	kCommonChunkBufferLength			= 8192,
	kCommonMaxErrorTokenCount			= 32,
	kCommonStreamchkWidth				= 32,
	kCommonMaxPrintBufferLength			= 8192,
	kCommonMaxTokenCharacters			= 32,
	kCommonMaxFilenameLength			= 128,
	kCommonTimestampTimelineLength			= 4000000 /* Set to, e.g., 4000000 if we want to capture very long traces for debug; set to 1 otherwise */,
	kCommonCgiRandomDigits				= 10,
	kCommonRlimitCpuSeconds				= 5*60,			/*	5 mins	*/
	kCommonRlimitRssBytes				= 2*1024*1024*1024UL,	/*	2GB	*/
	kCommonProgressTimerSeconds			= 5,

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kCommonConstantMax,
} Constant;



typedef enum
{
	kCommonModeDefault				= (0 << 0),
	kCommonModeCallTracing				= (1 << 0),
	kCommonModeCallStatistics			= (1 << 1),
	kCommonModeCGI					= (1 << 2),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kCommonModeMax
} CommonMode;



typedef enum
{
	kCommonPostFileWriteActionRenderDot		= (1 << 0),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kCommonPostFileWriteActionMax,
} PostFileWriteAction;

typedef enum
{
        noisyInitType,
        noisyBool,
        noisyInt4,
        noisyInt8,
        noisyInt16,
        noisyInt32,
        noisyInt64,
        noisyInt128,
        noisyIntegerConstType,
        noisyNat4,
        noisyNat8,
        noisyNat16,
        noisyNat32,
        noisyNat64,
        noisyNat128,
        noisyFloat16,
        noisyFloat32,
        noisyFloat64,
        noisyFloat128,
        noisyRealConstType,
        noisyString,
        noisyArrayType,
        noisyArithType,
        noisyNilType,
	noisyNamegenType,
        noisyTypeError
} NoisyBasicType;



typedef struct Scope		Scope;
typedef struct Symbol		Symbol;
typedef struct Token		Token;
typedef struct IrNode		IrNode;
typedef struct SourceInfo	SourceInfo;
typedef struct Dimension	Dimension;
typedef struct Physics		Physics;
typedef struct IntegralList	IntegralList;
typedef struct Invariant	Invariant;
typedef struct Signal		Signal;
<<<<<<< HEAD
typedef struct Sensor		Sensor;
typedef struct Modality		Modality;
=======
typedef struct NoisyType	NoisyType;

struct NoisyType
{
        NoisyBasicType basicType;
        int dimensions;
        NoisyBasicType arrayType;
	Symbol * functionDefinition;
        int sizeOfDimension[128];
};



>>>>>>> 1aa380097... Move noisyType to the common data-structure

struct Dimension
{
	char *			name;
	char *			abbreviation;
	double			exponent;			//	Default value is 1 if exists
	Scope *			scope;
	SourceInfo *		sourceInfo;
	int			primeNumber;

	Dimension *		next;
};

struct Invariant
{
	char *			identifier;			//	Name of the physics quantity, of type Tidentifier
	Scope *			scope;
	SourceInfo *		sourceInfo;
	IrNode *		parameterList;			//	This is just bunch of IrNode's in Xseq
	uint64_t		id;
	IrNode *		constraints;
	double * 		dimensionalMatrix;		//	Dimensional matrix
	int 			dimensionalMatrixRowCount;	//	Number of dimensional matrix rows		
	int 			dimensionalMatrixColumnCount;	//	Number of dimensional matrix columns 
	char **			dimensionalMatrixRowLabels;	//	Labels of dimensional matrix rows		
	char **			dimensionalMatrixColumnLabels;	//	Labels of dimensional matrix columns 
	double ***		nullSpace;			//	Initial null space and parameter used in kernel printer
	double ***		nullSpaceWithoutDuplicates;	//	Duplicate kernels are taken out
	double ***		nullSpaceRowReordered;		//	Reorders the rows of the kernels lexicographically
	double ***		nullSpaceCanonicallyReordered;	//	Canonically reordered
	char ***		canonicallyReorderedLabels;	//	Debugging use
	int			kernelColumnCount;
	int			numberOfUniqueKernels;		//	Saves the unique kernel count
	int			numberOfTotalKernels;		//	Saves the total kernels before canonicalisation
	int *			permutedIndexArrayPointer;	//	Saves the permutation indeces
	int ** 			numberOfConstPiArray;		//	Saves the number of constant Pi in each kernel

	Invariant *		next;
};

struct Signal {
	IrNode *		baseNode;				//	The baseSignalDefinition IrNode.
	char *			identifier;				//	The signal identifier.
	char *			invariantExpressionIdentifier;	//Identifier used in invariant expressions.
	int			axis;					//	The axis of the multi axis signal that the signal corresponds to. Default value is zero.
	char *			sensorIdentifier;		//	Identifier of the sensor associated to a signal.
	int			physicalGroupNumber;	//  Conveys information about the physical origin of the signal. (e.g. The I2C bus number of a sensor connected to Ipsa).
	int			dimensionIndex;			//	Conveys information about the dimension of the signal. Currently used for storing the dimension index for Ipsa.
	Signal *		relatedSignalList;		//	List of signals that should be co-sampled with this signal.
	Signal *		relatedSignalListNext;	//	Move to the next element of the relatedSignalList.
	Signal *		relatedSignalListPrev;	//	Move to the previous element of the relatedSignalList.
};

struct Physics
{
	char *			identifier;			//	Name of the physics quantity. of type Tidentifier
	uint64_t		id;
	int			subindex;			//	Index for further identification. e.g.) acceleration along x, y, z axes
	Scope *			scope;
	Scope *			uncertaintyScope;
	SourceInfo *		sourceInfo;
	bool			isVector;
	Physics *		vectorCounterpart;		//	Non-NULL if a scalar AND counterpart defined in vectorScalarPairScope
	Physics *		scalarCounterpart;		//	Non-NULl if a vector AND counterpart defined in vectorScalarPairScope
	double			value;				//	For constants like Pi or gravitational acceleration
	bool			isConstant;
	Dimension *		dimensions;
	char *			dimensionAlias;
	char *			dimensionAliasAbbreviation;
	Physics *		definition;

	Physics *		next;
};

typedef enum {
	kNewtonSensorInterfaceTypeI2C,
	kNewtonSensorInterfaceTypeSPI,
	kNewtonSensorInterfaceTypeAnalog,
	kNewtonSensorInterfaceTypeUART,

	kNewtonSensorInterfaceTypeMax
} SensorInterfaceType;

struct Modality {
	char *		identifier;		/* Modality name, e.g, "bmx055xAcceleration" */
	Signal *	signal;			/* Signal type to follow */
	Physics *	_physics;		/* Temporary field */
	double		rangeLowerBound;
	double		rangeUpperBound;

	int		precisionBits;
	double		precisionCost;

	double		accuracy;
	double		accuracyCost;
	// Signal *	accuracySignal;
	
	SensorInterfaceType	interfaceType;	/* WiP */
	/* Missing register address for modality */
	uint64_t	registerAddress;

	Modality *	next;
	// Modality *	prev;
};

struct Sensor {
	IrNode *	baseNode;	/* Pointer to AST node of definition */
	char *		identifier;	/* Definition identifier */
	Modality *	modalityList;	/* List of sensor modalities */
	uint16_t	erasureToken;

	Sensor *	next;
	// Sensor *	prev;
};

struct IntegralList
{
	Physics *		head;
	IntegralList *		next;
};

struct IrNode
{
	IrNodeType		type;

	/*
	 *	Syntactic (AST) information.
	 */
	char *			tokenString;
	Token *			token;
	SourceInfo *		sourceInfo;
	IrNode *		irParent;
	IrNode *		irLeftChild;
	IrNode *		irRightChild;

	Symbol *		symbol;

	/*
	 *	Used for evaluating dimensions in expressions
	 */
	Physics *		physics;

	/*
	 *	Used for connecting invariant parameters to signals.
	 */
	Signal *		signal;

	/*
	*	Used for type checking invariant parameters during invariant call.
	*/
	Invariant *		invariant;
	
	/*
	 *	Only if this node belongs to a ParseNumericExpression subtree
	 */
	double			value;
	int				integerValue;

	int			subindexStart;
	int			subindexEnd;

	/*
	 *	A parameter tuple of length n has ordering from zero to n - 1
	 */
	int			parameterNumber;

	/*
	 *	Used for coloring the IR tree, e.g., during Graphviz/dot generation
	 */
	IrNodeColor		nodeColor;

	/*
		Used to keep track of whether the node was visited or not
	 */
	bool			isVisited;   			
};


struct SourceInfo
{
	/*
	 *	Not yet used; for when we implement includes, this will be
	 *	the 'genealogy' of includes leading to this token.
	 */
	char **			genealogy;
	
	char *			fileName;
	uint64_t		lineNumber;
	uint64_t		columnNumber;
	uint64_t		length;
};


struct Token
{
	IrNodeType		type;
	char *			identifier;
	int64_t			integerConst;
	double			realConst;
	char *			stringConst;
	SourceInfo *		sourceInfo;
	
	Token *			prev;
	Token *			next;
};



struct Scope
{
	/*
	 *	For named scopes (at the moment, only Progtypes)
	 */
	char *			identifier;

	int			currentSubindex;

	/*
	 *	Hierarchy. The firstChild is used to access its siblings via firstChild->next
	 */
	Scope *			parent;
	Scope *			firstChild;

	/*
	 *	Symbols in this scope. The list of symbols is accesed via firstSymbol->next
	 */
	Symbol *		firstSymbol;

	/*
	 *	Each invariant scope will have its own list of parameters
	 */
	IrNode *		scopeParameterList;		//	This is just bunch of IrNode's in Xseq

	/*
	 *	For the config file, we only have one global scope that keeps track of all
	 *	dimensions ad physics quantities.
	 */
	Dimension *		firstDimension;
	Physics *		firstPhysics;

	/*
	 *	Where in source scope begins and ends
	 */
	SourceInfo *		begin;
	SourceInfo *		end;

	/*
	 *	For chaining together scopes (currently only used for Progtype
	 *	scopes and for chaining together children).
	 */
	Scope *			next;
	Scope *			prev;

	/*
	 *	Used for coloring the IR tree, e.g., during Graphviz/dot generation
	 */
	IrNodeColor		nodeColor;
};

typedef enum
{
	kNoisySymbolTypeTypeError,
	kNoisySymbolTypeProgtype,
	kNoisySymbolTypeConstantDeclaration,
	kNoisySymbolTypeTypeDeclaration,
	kNoisySymbolTypeNamegenDeclaration,
	kNoisySymbolTypeVariableDeclaration,
	kNoisySymbolTypeNamegenDefinition,
	kNoisySymbolTypeUse,
	kNoisySymbolTypeModule,
	kNoisySymbolTypeModuleParameter,
	kNoisySymbolTypeParameter,
	kNoisySymbolTypeReturnParameter,

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisySymbolTypeMax,
} NoisySymbolType;


struct Symbol
{
	char *			identifier;

	/*
	 *	This field is duplicated in the AST node, since only
	 *	identifiers get into the symbol table:
	 */
	SourceInfo *	sourceInfo;

	/*
	 *	Declaration, type definition, use, etc. (kNoisySymbolTypeXXX)
	 */
	NoisySymbolType 	symbolType;
	NoisyType		noisyType;

	/*
	*	The IrNode where function definition starts. Used for loading functions.
	*/
	IrNode *		functionDefinition;

	/*
	*	Number of parameters. Used only for functions and Noisy
	*	code generation.
	*/
	int			parameterNum;
	bool			isTypeComplete;
	int			paramPosition;
	LLVMValueRef		llvmPointer;

	/*
	 *	Scope within which sym appears
	 */
	Scope *			scope;

	/*
	 *	If an identifier use, definition's Sym, if any
	 */
	Symbol *		definition;

	/*
	 *	Subtree in AST that represents typeexpr
	 */
	IrNode *		typeTree;

	/*
	 *	If an I_CONST, its value.
	 */
	int			intConst;
	double			realConst;
	char *			stringConst;
	
	/*
	 *	For chaining together sibling symbols in the same scope
	 */
	Symbol *		next;
	Symbol *		prev;
};


typedef struct
{
	/*
	 *	Timestamps to track lifecycle
	 */
	uint64_t		initializationTimestamp;
	TimeStamp *		timestamps;
	uint64_t		timestampCount;
	uint64_t		timestampSlots;

	/*
	 *	Track aggregate time spent in all routines, by incrementing
	 *	timeAggregates[timeAggregatesLastKey] by (now - timeAggregatesLastTimestamp)
	 */
	uint64_t *		timeAggregates;
	TimeStampKey		timeAggregatesLastKey;
	uint64_t		timeAggregatesLastTimestamp;
	uint64_t		timeAggregateTotal;
	uint64_t *		callAggregates;
	uint64_t		callAggregateTotal;

	/*
	 *	Used to get error status from FlexLib routines
	 */
	FlexErrState *		Fe;

	/*
	 *	State for the portable/monitoring allocator (FlexM)
	 */
	FlexMstate *		Fm;

	/*
	 *	State for portable/buffering print routines (FlexP)
	 *	We have one buffer for informational messages, another
	 *	for errors and warnings.
	 */
	FlexPrintBuf *		Fperr;
	FlexPrintBuf *		Fpinfo;
	FlexPrintBuf *		Fpsmt2;
	FlexPrintBuf *		Fpc;
<<<<<<< HEAD
	FlexPrintBuf *		Fph;
=======
	FlexPrintBuf *		Fpg;
>>>>>>> 56645ef6f... Add Fpg buffer to the state for the code generation
	FlexPrintBuf *		Fprtl;
	FlexPrintBuf *		Fpmathjax;
	FlexPrintBuf *		Fpipsa;

	/*
	 *	The output file of the last render. TODO: Not very happy
	 *	with this solution as it stands... (inherited from Sal/svm)
	 */
	char *			lastDotRender;


	/*
	 *	This is the name of the module that the file we're parsing implements
	 */
	char *			moduleOfFile;

	/*
	 *	This is the target parameter for the targetParam backend
	 */
	char *			targetParam;

	/*
	 *	This is the invariant where the target parameter appears only once (for the targetParam backend)
	 */
	int 			targetParamLocatedKernel;

	/*
	 *	This is data type that a signal will be typedef'ed to
	 *	in the signal typedef generation backend
	 */
	char *		signalTypedefDatatype;

	/*
	 *	We keep a global handle on the list of module scopes, for easy reference.
	 *	In this use case, the node->identifier holds the scopes string name, and we
	 *	chain then using their prev/next fields.
	 */
	Scope *		moduleScopes;

	/*
	 *	Lexer state
	 */
	FILE *			filePointer;
	char *			fileName;
	char *			lineBuffer;
	uint64_t		columnNumber;
	uint64_t		lineNumber;
	uint64_t		lineLength;
	char *			currentToken;
	uint64_t		currentTokenLength;
	Token *			tokenList;
	Token *			lastToken;

	/*
	 *	The root of the IR tree, and top scope
	 */
	IrNode *		noisyIrRoot;
	IrNode *		newtonIrRoot;
	Scope *			noisyIrTopScope;
	Scope *			newtonIrTopScope;

	/*
	 *	Output file name when emitting bytecode/protobuf
	 */
	char *			outputFilePath;
	char *			outputSmtFilePath;
	char *			outputCFilePath;
	char *			outputSignalTypedefHeaderFilePath;
	char *			outputRTLFilePath;
	char *			outputEstimatorSynthesisFilePath;
	char *			outputIpsaFilePath;
	
	/*
	 *	Invariant identifiers specified for State Estimator Synthesis
	 */
	char *			estimatorProcessModel;
	char *			estimatorMeasurementModel;
	bool			autodiff;
	
	/*
	 *	LLVM IR input file
	 */
	char *			llvmIR;
	
	/*
	 *	Variables for storing lists of identifiers attached
	 *	to a physical group number.
	 */
	char *			physicalGroup1;
	char *			physicalGroup2;

	/*
	 *	Variables to keep track of the kernel number and pi number
	 *	specified by the user for Pi Groups Signal Annotation.
	 */
	int			kernelNumber;
	int			piNumber;
	bool			enableKernelSelect;
	bool			enablePiSelect;

	CommonMode		mode;
	uint64_t		verbosityLevel;
	uint64_t		dotDetailLevel;
	uint64_t		optimizationLevel;
	uint64_t		irPasses;
	uint64_t		irBackends;


	jmp_buf			jmpbuf;
	bool			jmpbufIsValid;

	/*
	 *	Global index of which prime numbers we have used for the dimension id's
	 */
	int		primeNumbersIndex;
	Invariant *	invariantList;
	Sensor *	sensorList;
} State;


void		fatal(State *  C, const char *  msg) __attribute__((noreturn));
void		error(State *  C, const char *  msg);
void		timestampsInit(State *  C);
void		timeStampDumpTimeline(State *  C);
void		timeStampDumpResidencies(State *  C);
State *		init(CommonMode mode);
void		dealloc(State *  C);
void		runPasses(State *  C);
uint64_t	checkRss(State *  C);
void		consolePrintBuffers(State *  C);
void		printToFile(State *  C, const char *  msg, const char *  fileName, PostFileWriteAction action);
void		renderDotInFile(State *  C, char *  pathName, char *  randomizedFileName);
void		checkCgiCompletion(State *  C, const char *  pathName, const char *  renderExtension);
