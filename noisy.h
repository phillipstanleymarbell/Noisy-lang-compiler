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
	
	/*
	 *	Code depends on this bringing up the rear.
	 */	
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
	kNoisyIrNodeType_PprogtypeDeclaration,
	kNoisyIrNodeType_PprogtypeBody,
	kNoisyIrNodeType_PprogtypeTypenameDeclaration,
	kNoisyIrNodeType_PconstantDeclaration,
	kNoisyIrNodeType_PtypeDeclaration,
	kNoisyIrNodeType_PadtTypeDeclaration,
	kNoisyIrNodeType_PnamegenDeclaration,
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

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyIrNodeTypeMax,
} NoisyIrNodeType;



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
	kNoisyIrPassXXX					= (0 << 0),
	
	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyIrPassMax,
} NoisyIrPasses;



typedef enum
{
	kNoisyIrBackendDot				= (1 << 0),
	
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
	
	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyIrNodeColor,
} NoisyIrNodeColor;



typedef enum
{
	kNoisyMaxBufferLength				= 8192,
	kNoisyChunkBufferLength				= 8192,
	kNoisyMaxErrorTokenCount			= 32,
	kNoisyStreamchkWidth				= 32,
	kNoisyMaxPrintBufferLength			= 8192,
	kNoisyMaxTokenCharacters			= 32,
	kNoisyMaxFilenameLength				= 128,
	kNoisyTimeStamptimelineLength			= 1 /* Set to, e.g., 4000000000 if we want to capture very long traces for debug */,
	kNoisyCgiRandomDigits				= 10,
	kNoisyRlimitCpuSeconds				= 5*60,			/*	5 mins	*/
	kNoisyRlimitRssBytes				= 2*1024*1024*1024UL,	/*	2GB	*/
	kNoisyProgressTimerSeconds			= 5,

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyConstantMax,
} NoisyConstant;



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


typedef struct NoisyIrNode NoisyIrNode;
struct NoisyIrNode
{
	NoisyIrNodeType		type;

	/*
	 *	Syntactic (AST) information.
	 */
	char *			tokenString;
	uint64_t		srcLineNumber;
	uint64_t		srcColumnNumber;
	NoisyIrNode *		irParent;
	NoisyIrNode *		irLeftChild;
	NoisyIrNode *		irRightChild;

	/*
	 *	Used for coloring the IR tree, e.g., during Graphviz/dot generation
	 */
	NoisyIrNodeColor	nodeColor;
};



typedef struct
{
	/*
	 *	Timestamps to track lifecycle
	 */
	uint64_t		initializationTimestamp;
	NoisyTimeStamp *	timestamps;
	uint64_t		timestampCount;
	uint64_t		timestampSlots;


	/*
	 *	Track aggregate time spent in all routines, by incrementing
	 *	timeAggregates[timeAggregatesLastKey] by (now - timeAggregatesLastTimestamp)
	 */
	uint64_t *		timeAggregates;
	NoisyTimeStampKey	timeAggregatesLastKey;
	uint64_t		timeAggregatesLastTimestamp;
	uint64_t		timeAggregateTotal;
	uint64_t *		callAggregates;
	uint64_t		callAggregateTotal;


	/*
	 *	Used to get error status from FlexLib routines
	 */
	FlexErrState *		Fe;

	/*
	 *	Tokenized input stream
	 */
	FlexIstream *		Fi;

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


	/*
	 *	The output file of the last render. TODO: Not very happy
	 *	with this solution as it stands... (inherited from Sal/svm)
	 */
	char *			lastDotRender;


	/*
	 *	This is the name of the progtype that the file we're parsing implements
	 */
	//const char	progtypeOfFile[];

	/*
	 *	We keep a global handle on the list of progtype scopes (list of (string, ref Scope)), for easy reference
	 */
	FlexTupleList *		progtypeScopes;


	/*
	 *	Lexer state
	 */
	const char *		fileName;
	const char *		lineBuffer;
	uint64_t		columnNumber;
	uint64_t		lineNumber;
	const char *		currentToken;
	FlexList *		tokenlist;
	

	/*
	 *	The root of the IR tree.
	 */
	NoisyIrNode *		noisyIrRoot;


	NoisyMode		mode;
	uint64_t		verbosityLevel;
	uint64_t		dotDetailLevel;
	uint64_t		optimizationLevel;
	uint64_t		irPasses;
	uint64_t		irBackends;


	jmp_buf			jmpbuf;
	bool			jmpbufIsValid;
} NoisyState;


void				noisyFatal(NoisyState *  C, const char *  msg) __attribute__((noreturn));
void				noisyError(NoisyState *  C, const char *  msg);
void				noisyTimestampsInit(NoisyState *  C);
void				noisyTimeStampDumpTimeline(NoisyState *  C);
void				noisyTimeStampDumpResidencies(NoisyState *  C);
NoisyState *			noisyInit(NoisyMode mode);
void				noisyDealloc(NoisyState *  C);
void				noisyRunPasses(NoisyState *  C);
uint64_t			noisyCheckRss(NoisyState *  C);
void				noisyConsolePrintBuffers(NoisyState *  C);
void				noisyPrintToFile(NoisyState *  C, const char *  msg, const char *  fileName, NoisyPostFileWriteAction action);
void				noisyRenderDotInFile(NoisyState *  C, char *  pathName, char *  randomizedFileName);
void				noisyCheckCgiCompletion(NoisyState *  C, const char *  pathName, const char *  renderExtension);
