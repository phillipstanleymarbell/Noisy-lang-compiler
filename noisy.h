/*
	Authored 2015. Phillip Stanley-Marbell.

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



typedef enum
{
	kNoisyDebugLexer,
	kNoisyDebugParser,
	kNoisyDebugAST,
	kNoisyDebugFF,
	kNoisyDebugMax,
} NoisyDebugType;



typedef enum
{
	/*
	 *	Tokens
	 */
	kNoisyIrNodeType_TaddAs,
	kNoisyIrNodeType_Tadt,
	kNoisyIrNodeType_Talpha,
	kNoisyIrNodeType_Tampersand,
	kNoisyIrNodeType_Tand,
	kNoisyIrNodeType_TandAs,
	kNoisyIrNodeType_Tarray,
	kNoisyIrNodeType_Tas,
	kNoisyIrNodeType_Tasterisk,
	kNoisyIrNodeType_Tbang,
	kNoisyIrNodeType_Tbool,
	kNoisyIrNodeType_TboolConst,
	kNoisyIrNodeType_Tbyte,
	kNoisyIrNodeType_Tcaret,
	kNoisyIrNodeType_Tchan,
	kNoisyIrNodeType_Tchan2name,
	kNoisyIrNodeType_TchanWrite,
	kNoisyIrNodeType_TcharConst,
	kNoisyIrNodeType_Tcolon,
	kNoisyIrNodeType_Tcomma,
	kNoisyIrNodeType_Tcons,
	kNoisyIrNodeType_Tconst,
	kNoisyIrNodeType_TdefineAs,
	kNoisyIrNodeType_Tdec,
	kNoisyIrNodeType_Tdiv,
	kNoisyIrNodeType_TdivAs,
	kNoisyIrNodeType_Tdot,
	kNoisyIrNodeType_TdoubleQuote,
	kNoisyIrNodeType_Tepsilon,
	kNoisyIrNodeType_Teq,
	kNoisyIrNodeType_Terasures,
	kNoisyIrNodeType_Terrors,
	kNoisyIrNodeType_Tfalse,
	kNoisyIrNodeType_Tfixed,
	kNoisyIrNodeType_Tge,
	kNoisyIrNodeType_Tgets,
	kNoisyIrNodeType_Tgoes,
	kNoisyIrNodeType_Tgt,
	kNoisyIrNodeType_Thd,
	kNoisyIrNodeType_Tidentifier,
	kNoisyIrNodeType_Tinc,
	kNoisyIrNodeType_Tint,
	kNoisyIrNodeType_TintConst,
	kNoisyIrNodeType_Titer,
	kNoisyIrNodeType_Tlatency,
	kNoisyIrNodeType_TleftBrac,
	kNoisyIrNodeType_TleftBrace,
	kNoisyIrNodeType_Tle,
	kNoisyIrNodeType_Tlen,
	kNoisyIrNodeType_Tlist,
	kNoisyIrNodeType_TleftParen,
	kNoisyIrNodeType_TleftShift,
	kNoisyIrNodeType_TleftShiftAs,
	kNoisyIrNodeType_Tlt,
	kNoisyIrNodeType_Tmatch,
	kNoisyIrNodeType_TmatchSeq,
	kNoisyIrNodeType_Tminus,
	kNoisyIrNodeType_TmodAs,
	kNoisyIrNodeType_TmulAs,
	kNoisyIrNodeType_Tname2chan,
	kNoisyIrNodeType_Tnamegen,
	kNoisyIrNodeType_Tneq,
	kNoisyIrNodeType_Tnil,
	kNoisyIrNodeType_Tnybble,
	kNoisyIrNodeType_Tof,
	kNoisyIrNodeType_Tor,
	kNoisyIrNodeType_TorAs,
	kNoisyIrNodeType_Tpercent,
	kNoisyIrNodeType_Tplus,
	kNoisyIrNodeType_Tprogtype,
	kNoisyIrNodeType_TprogtypeEqual,
	kNoisyIrNodeType_TrightBrac,
	kNoisyIrNodeType_TrightBrace,
	kNoisyIrNodeType_Treal,
	kNoisyIrNodeType_TrealConst,
	kNoisyIrNodeType_TrightParen,
	kNoisyIrNodeType_TrightShift,
	kNoisyIrNodeType_TrightShiftAs,
	kNoisyIrNodeType_Tsemicolon,
	kNoisyIrNodeType_Tset,
	kNoisyIrNodeType_TsingleQuote,
	kNoisyIrNodeType_TstringConst,
	kNoisyIrNodeType_Tstring,
	kNoisyIrNodeType_Tstroke,
	kNoisyIrNodeType_TsubAs,
	kNoisyIrNodeType_Ttau,
	kNoisyIrNodeType_Ttilde,
	kNoisyIrNodeType_Ttl,
	kNoisyIrNodeType_Ttrue,
	kNoisyIrNodeType_Ttype,
	kNoisyIrNodeType_Tvar2name,
	kNoisyIrNodeType_TxorAs,

	kNoisyIrNodeType_ZbadCharConst,
	kNoisyIrNodeType_ZbadStringConst,
	kNoisyIrNodeType_ZbadIdentifier,


	/*
	 *	Epsilon and $ (i.e., EOF) need to be in same ordinal set as tokens
	 */
	kNoisyIrNodeType_Zepsilon,
	kNoisyIrNodeType_Zeof,


	kNoisyIrNodeType_TMax,


	/*
	 *	Grammar productions.
	 */
	kNoisyIrNodeType_Pcharacter,
	kNoisyIrNodeType_PreservedToken,
	kNoisyIrNodeType_PzeroNine,
	kNoisyIrNodeType_PoneNine,
	kNoisyIrNodeType_Pradix,
	kNoisyIrNodeType_PcharConst,
	kNoisyIrNodeType_PintConst,
	kNoisyIrNodeType_PboolConst,
	kNoisyIrNodeType_PdRealConst,
	kNoisyIrNodeType_PeRealConst,
	kNoisyIrNodeType_PrealConst,
	kNoisyIrNodeType_PstringConst,
	kNoisyIrNodeType_PidentifierChar,
	kNoisyIrNodeType_Pidentifier,
	kNoisyIrNodeType_Pprogram,
	kNoisyIrNodeType_PprogtypeDecl,
	kNoisyIrNodeType_PprogtypeBody,
	kNoisyIrNodeType_PprogtypeTypenameDecl,
	kNoisyIrNodeType_PconDecl,
	kNoisyIrNodeType_PtypeDecl,
	kNoisyIrNodeType_PadtTypeDecl,
	kNoisyIrNodeType_PnamegenDecl,
	kNoisyIrNodeType_PidentifierOrNil,
	kNoisyIrNodeType_PidentifierOrNilList,
	kNoisyIrNodeType_PidentifierList,
	kNoisyIrNodeType_PtypeExpression,
	kNoisyIrNodeType_Ptypename,
	kNoisyIrNodeType_Ptolerance,
	kNoisyIrNodeType_PerrorMagnitudeTolerance,
	kNoisyIrNodeType_PlossTolerance,
	kNoisyIrNodeType_PlatencyTolerance,
	kNoisyIrNodeType_PbasicType,
	kNoisyIrNodeType_PrealType,
	kNoisyIrNodeType_PfixedType,
	kNoisyIrNodeType_PanonAggregateType,
	kNoisyIrNodeType_ParrayType,
	kNoisyIrNodeType_PlistType,
	kNoisyIrNodeType_PtupleType,
	kNoisyIrNodeType_PsetType,
	kNoisyIrNodeType_PinitList,
	kNoisyIrNodeType_PindexedInitList,
	kNoisyIrNodeType_PstarInitList,
	kNoisyIrNodeType_Pelement,
	kNoisyIrNodeType_PnamegenDefinition,
	kNoisyIrNodeType_PscopedStatementList,
	kNoisyIrNodeType_PstatementList,
	kNoisyIrNodeType_Pstatement,
	kNoisyIrNodeType_PassignOp,
	kNoisyIrNodeType_PmatchStatement,
	kNoisyIrNodeType_PiterationStatement,
	kNoisyIrNodeType_PguardBody,
	kNoisyIrNodeType_Pexpression,
	kNoisyIrNodeType_PlistCastExpression,
	kNoisyIrNodeType_PsetCastExpression,
	kNoisyIrNodeType_ParrayCastExpression,
	kNoisyIrNodeType_PanonAggregateCastExpression,
	kNoisyIrNodeType_PchanEventExpression,
	kNoisyIrNodeType_Pchan2nameExpression,
	kNoisyIrNodeType_Pvar2nameExpression,
	kNoisyIrNodeType_Pname2chanExpression,
	kNoisyIrNodeType_Pterm,
	kNoisyIrNodeType_Pfactor,
	kNoisyIrNodeType_PfieldSelect,
	kNoisyIrNodeType_PhighPrecedenceBinaryOp,
	kNoisyIrNodeType_PlowPrecedenceBinaryOp,
	kNoisyIrNodeType_PcmpOp,
	kNoisyIrNodeType_PbooleanOp,
	kNoisyIrNodeType_PunaryOp,
	kNoisyIrNodeType_PMax,

	/*
	 *	This 'pre-node Op' node is inserted before members of a production sequence
	 */
	kNoisyIrNodeType_Xseq,

	kNoisyIrNodeTypeMax,
} NoisyIrNodeType;



/*
 *	Some classes of characters used in Lexer
 */
const NoisyIrNodeType gNoisyReservedTokens[] = {
							kNoisyIrNodeType_Ttilde,
							kNoisyIrNodeType_Tbang,
							kNoisyIrNodeType_Tpercent,
							kNoisyIrNodeType_Tcaret,
							kNoisyIrNodeType_Tampersand,
							kNoisyIrNodeType_Tasterisk,
							kNoisyIrNodeType_TleftParen,
							kNoisyIrNodeType_TrightParen,
							kNoisyIrNodeType_Tminus,
							kNoisyIrNodeType_Tplus,
							kNoisyIrNodeType_Tas,
							kNoisyIrNodeType_Tdiv,
							kNoisyIrNodeType_Tgt,
							kNoisyIrNodeType_Tlt,
							kNoisyIrNodeType_Tsemicolon,
							kNoisyIrNodeType_Tcolon,
							kNoisyIrNodeType_TsingleQuote,
							kNoisyIrNodeType_TdoubleQuote,
							kNoisyIrNodeType_TleftBrace,
							kNoisyIrNodeType_TrightBrace,
							kNoisyIrNodeType_TleftBrac,
							kNoisyIrNodeType_TrightBrac,
							kNoisyIrNodeType_Tstroke,
							kNoisyIrNodeType_Tgets,
							kNoisyIrNodeType_Tdot,
							kNoisyIrNodeType_Tcomma,
							kNoisyIrNodeType_Tle,
							kNoisyIrNodeType_Tge,
							kNoisyIrNodeType_TxorAs,
							kNoisyIrNodeType_TorAs,
							kNoisyIrNodeType_TandAs,
							kNoisyIrNodeType_TmodAs,
							kNoisyIrNodeType_TdivAs,
							kNoisyIrNodeType_TmulAs,
							kNoisyIrNodeType_TsubAs,
							kNoisyIrNodeType_TaddAs,
							kNoisyIrNodeType_TdefineAs,
							kNoisyIrNodeType_Tneq,
							kNoisyIrNodeType_TrightShift,
							kNoisyIrNodeType_TrightShiftAs,
							kNoisyIrNodeType_TleftShift,
							kNoisyIrNodeType_TleftShiftAs,
							kNoisyIrNodeType_Tand,
							kNoisyIrNodeType_Tor,
							kNoisyIrNodeType_Tcons,
							kNoisyIrNodeType_Teq,
							kNoisyIrNodeType_Tinc,
							kNoisyIrNodeType_Tdec,
							kNoisyIrNodeType_TchanWrite,
							kNoisyIrNodeType_Tgoes,
							kNoisyIrNodeType_TprogtypeEqual,
						};

