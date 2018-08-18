/*
	Authored 2015. Phillip Stanley-Marbell.
	Modified, 2016-2017, Jonathan Lim to add Newton hooks.

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
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

typedef enum
{
	kNoisyVerbosityDebugLexer	=	(1 << 0),
	kNoisyVerbosityDebugParser	= 	(1 << 1),
	kNoisyVerbosityDebugAST		=	(1 << 2),
	kNoisyVerbosityDebugFF		=	(1 << 3),
	
	/*
	 *	Code depends on this bringing up the rear.
	 */	
	kNoisyVerbosityDebugMax,
} NoisyVerbosityType;



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
	kNewtonIrNodeType_Tinclude,
	kNewtonIrNodeType_TEnglish,
	kNewtonIrNodeType_TatSign,
	kNewtonIrNodeType_Tcolon,
	kNewtonIrNodeType_Tcomma,
	kNewtonIrNodeType_Tconstant,
	kNewtonIrNodeType_Tcross,
	kNewtonIrNodeType_Tderivation,
	kNewtonIrNodeType_Tderivative,
	kNewtonIrNodeType_Tdiv,
	kNewtonIrNodeType_Tdimensionless,
	kNewtonIrNodeType_Tdot,
	kNewtonIrNodeType_Tequals,
	kNewtonIrNodeType_Tequivalent,
	kNewtonIrNodeType_Texponent,
	kNewtonIrNodeType_Tge,
	kNewtonIrNodeType_Tgt,
	kNewtonIrNodeType_Tidentifier,
	kNewtonIrNodeType_TintConst,
	kNewtonIrNodeType_Tintegral,
	kNewtonIrNodeType_Tinvariant,
	kNewtonIrNodeType_Tle,
	kNewtonIrNodeType_TleftBrace,
	kNewtonIrNodeType_TleftParen,
	kNewtonIrNodeType_Tlt,
	kNewtonIrNodeType_Tminus,
	kNewtonIrNodeType_Tmul,
	kNewtonIrNodeType_Tname,
	kNewtonIrNodeType_Tnil,
	kNewtonIrNodeType_Tnone,
	kNewtonIrNodeType_TnumericConst,
	kNewtonIrNodeType_Tplus,
	kNewtonIrNodeType_Tproportional,
	kNewtonIrNodeType_TrealConst,	
	kNewtonIrNodeType_TrightBrace,
	kNewtonIrNodeType_TrightParen,
	kNewtonIrNodeType_Tsemicolon,
	kNewtonIrNodeType_Tsignal,
	kNewtonIrNodeType_TstringConst,
	kNewtonIrNodeType_Tsymbol,
	kNewtonIrNodeType_Tto,

	/*
	 *	Code depends on this bringing up the rear for Newton Tokens.
	 */
	kNewtonIrNodeType_TMax,
	
	kNewtonIrNodeType_ZbadIdentifier,
	kNewtonIrNodeType_ZbadStringConst,
	kNewtonIrNodeType_Zeof,
	kNewtonIrNodeType_Zepsilon,

	
	/*
	 *	Newton grammar productions
	 */
	kNewtonIrNodeType_PMin,
	kNewtonIrNodeType_PbaseSignal,
	kNewtonIrNodeType_PcompareOp,
	kNewtonIrNodeType_Pconstant,
	kNewtonIrNodeType_Pconstraint,
	kNewtonIrNodeType_PconstraintList,
	kNewtonIrNodeType_Pderivation,
	kNewtonIrNodeType_PhighPrecedenceBinaryOp,
	kNewtonIrNodeType_Pinteger,
	kNewtonIrNodeType_Pinvariant,
	kNewtonIrNodeType_PlanguageSetting,
	kNewtonIrNodeType_PlowPrecedenceBinaryOp,
	kNewtonIrNodeType_PmidPrecedenceBinaryOp,
	kNewtonIrNodeType_Pname,
	kNewtonIrNodeType_PnewtonFile,
	kNewtonIrNodeType_Pparameter,
	kNewtonIrNodeType_PparameterTuple,
	kNewtonIrNodeType_Pquantity,
	kNewtonIrNodeType_PquantityExpression,
	kNewtonIrNodeType_PquantityFactor,
	kNewtonIrNodeType_PquantityStatement,
	kNewtonIrNodeType_PquantityTerm,
	kNewtonIrNodeType_Pstatement,
	kNewtonIrNodeType_PstatementList,
	kNewtonIrNodeType_Psubindex,
	kNewtonIrNodeType_PsubindexTuple,
	kNewtonIrNodeType_Psymbol,
	kNewtonIrNodeType_PtimeOp,
	kNewtonIrNodeType_PunaryOp,
	kNewtonIrNodeType_PvectorOp,

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
	kNoisyVerbosityVerbose				= (1 << 0),
	kNoisyVerbosityActionTrace			= (1 << 1),
	kNoisyVerbosityCallTrace			= (1 << 2),
	kNoisyVerbosityPostScanStreamCheck		= (1 << 3),
	kNoisyVerbosityPreScanStreamCheck		= (1 << 4),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyVerbosityMax,
} NoisyVerbosity;



typedef enum
{
	kNoisyIrDimensionMatrixPass			= (1 << 0),
	
	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyIrPassMax,
} NoisyIrPasses;



typedef enum
{
	kNoisyIrBackendDot				= (1 << 0),
	kNoisyIrBackendProtobuf				= (1 << 1),
	kNewtonIrBackendSmt				= (1 << 2),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyIrBackendMax,
} NoisyIrBackend;



typedef enum
{
	kNoisyDotDetailLevelNoText			= (1 << 0),
	kNoisyDotDetailLevelNoNilNodes			= (1 << 1),
	
	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyDotDetailLevelMax,
} NoisyDotDetailLevel;



typedef enum
{
	kNoisyIrNodeColorDotBackendColoring		= (1 << 0),
	kNoisyIrNodeColorProtobufBackendColoring	= (1 << 1),
	kNoisyIrNodeColorTreeTransformedColoring	= (1 << 2),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyIrNodeColor,
} IrNodeColor;



typedef enum
{
	kNoisyMaxBufferLength				= 8192,
	kNoisyChunkBufferLength				= 8192,
	kNoisyMaxErrorTokenCount			= 32,
	kNoisyStreamchkWidth				= 32,
	kNoisyMaxPrintBufferLength			= 8192,
	kNoisyMaxTokenCharacters			= 32,
	kNoisyMaxFilenameLength				= 128,
	kNoisyTimestampTimelineLength			= 4000000 /* Set to, e.g., 4000000 if we want to capture very long traces for debug; set to 1 otherwise */,
	kNoisyCgiRandomDigits				= 10,
	kNoisyRlimitCpuSeconds				= 5*60,			/*	5 mins	*/
	kNoisyRlimitRssBytes				= 2*1024*1024*1024UL,	/*	2GB	*/
	kNoisyProgressTimerSeconds			= 5,

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyConstantMax,
} Constant;



typedef enum
{
	kNoisyModeDefault				= (0 << 0),
	kNoisyModeCallTracing				= (1 << 0),
	kNoisyModeCallStatistics			= (1 << 1),
	kNoisyModeCGI					= (1 << 2),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyModeMax
} NoisyMode;



typedef enum
{
	kNoisyPostFileWriteActionRenderDot		= (1 << 0),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyPostFileWriteActionMax,
} NoisyPostFileWriteAction;


typedef struct Scope		Scope;
typedef struct Symbol		Symbol;
typedef struct Token		Token;
typedef struct IrNode		IrNode;
typedef struct SourceInfo	SourceInfo;
typedef struct Dimension	Dimension;
typedef struct Physics		Physics;
typedef struct IntegralList	IntegralList;
typedef struct Invariant	Invariant;

struct Dimension
{
	char *			name;
	char *			abbreviation;
	double			exponent;		//	Default value is 1 if exists
	Scope *			scope;
	SourceInfo *		sourceInfo;
	int			primeNumber;

	Dimension *		next;
};

struct Invariant
{
	char *			identifier;		//	Name of the physics quantity. of type kNoisyConfigType_Tidentifier
	Scope *			scope;
	SourceInfo *		sourceInfo;
	IrNode *		parameterList;		//	This is just bunch of IrNode's in Xseq
	unsigned long long int	id;
	IrNode *		constraints;
	
	Invariant *		next;
};

struct Physics
{
	char *			identifier;		//	Name of the physics quantity. of type kNoisyConfigType_Tidentifier
	unsigned long long int	id;
	int			subindex;		//	Index for further identification. e.g.) acceleration along x, y, z axes
	Scope *			scope;
	SourceInfo *		sourceInfo;
	bool			isVector;
	Physics *		vectorCounterpart;	//	Non-NULL if a scalar AND counterpart defined in vectorScalarPairScope
	Physics *		scalarCounterpart;	//	Non-NULl if a vector AND counterpart defined in vectorScalarPairScope
	double			value;			//	For constants like Pi or gravitational acceleration
	bool			isConstant;
	Dimension *		dimensions;
	char *			dimensionAlias;
	char *			dimensionAliasAbbreviation;
	Physics *		definition;

	Physics *		next;
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
	 *	Only if this node belongs to a ParseNumericExpression subtree
	 */
	double			value;

	int			subindexStart;
	int			subindexEnd;

	/*
	 *	A parameter tuple of length n has ordering from zero to n - 1
	 */
	int			parameterNumber;

	/*
	 * When doing an API check of the invariant tree given a parameter tree,
	 * the method looks up all instances of
	 */

	/*
	 *	Used for coloring the IR tree, e.g., during Graphviz/dot generation
	 */
	IrNodeColor		nodeColor;
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
	uint64_t		integerConst;
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
	IrNode *		invariantParameterList;		//	This is just bunch of IrNode's in Xseq

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
	int 			symbolType;

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

	NoisyMode		mode;
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

	/*
	 *	When parsing invariant constraints, need to number the factors that correspond to the parameters passed in.
	 *	This is so that finding matching Parameter doesn't depend either the identifier passed, or the physics type.
	 *	That is a good idea because now we don't need to implicitly fill in the left identifier child of the parameter node.
	 */
	int currentParameterNumber;

	/*
	 *	This is a group (linked list) of linked list of physics nodes
	 */
	IntegralList * vectorIntegralLists;

	/*
	 *	This is a group (linked list) of linked list of physics nodes
	 */
	IntegralList * scalarIntegralLists;
	Invariant * invariantList;
} State;


void		fatal(State *  C, const char *  msg) __attribute__((noreturn));
void		error(State *  C, const char *  msg);
void		timestampsInit(State *  C);
void		timeStampDumpTimeline(State *  C);
void		timeStampDumpResidencies(State *  C);
State *		init(NoisyMode mode);
void		dealloc(State *  C);
void		runPasses(State *  C);
uint64_t	checkRss(State *  C);
void		consolePrintBuffers(State *  C);
void		printToFile(State *  C, const char *  msg, const char *  fileName, NoisyPostFileWriteAction action);
void		renderDotInFile(State *  C, char *  pathName, char *  randomizedFileName);
void		checkCgiCompletion(State *  C, const char *  pathName, const char *  renderExtension);
