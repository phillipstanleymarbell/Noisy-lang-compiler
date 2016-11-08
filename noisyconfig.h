typedef enum
{
	kNoisyConfigVerbosityDebugLexer	=	(1 << 0),
	kNoisyConfigVerbosityDebugParser	= 	(1 << 1),
	kNoisyConfigVerbosityDebugAST		=	(1 << 2),
	kNoisyConfigVerbosityDebugFF		=	(1 << 3),
	
	/*
	 *	Code depends on this bringing up the rear.
	 */	
	kNoisyConfigVerbosityDebugMax,
} NoisyConfigVerbosityType;


typedef enum
{
	// T_XXX,
	kNoisyConfigIrNodeType_Tnil,
	kNoisyConfigIrNodeType_Tsemicolon,
	kNoisyConfigIrNodeType_TrightParen,
	kNoisyConfigIrNodeType_TleftParen,
	kNoisyConfigIrNodeType_TrightBrace,
	kNoisyConfigIrNodeType_TleftBrace,
	kNoisyConfigIrNodeType_TrightBrac,
	kNoisyConfigIrNodeType_TleftBrac,
	kNoisyConfigIrNodeType_Tequals,
	kNoisyConfigIrNodeType_Tcomma,
	kNoisyConfigIrNodeType_Tcross,
	kNoisyConfigIrNodeType_Tdot,
	kNoisyConfigIrNodeType_Tdiv,
	kNoisyConfigIrNodeType_Tmul,
	kNoisyConfigIrNodeType_Tplus,
	kNoisyConfigIrNodeType_Tminus,
	kNoisyConfigIrNodeType_TstringConst,
	kNoisyConfigIrNodeType_Tidentifier,
	kNoisyConfigIrNodeType_TvectorScalarPairs,
	kNoisyConfigIrNodeType_TscalarIntegrals,
	kNoisyConfigIrNodeType_TvectorIntegrals,
	kNoisyConfigIrNodeType_TdimensionAliases,
	kNoisyConfigIrNodeType_Tlaw,
	kNoisyConfigIrNodeType_TdimensionTypeNames,
	kNoisyConfigIrNodeType_PanonAggregateCastExpression,
	kNoisyIrNodeType_ParrayCastExpression,
	kNoisyConfigIrNodeType_PassignOp,
	kNoisyConfigIrNodeType_PvectorOp,
	kNoisyConfigIrNodeType_PhighPrecedenceBinaryOp,
	kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp,
	kNoisyConfigIrNodeType_PunaryOp,
	kNoisyConfigIrNodeType_Pfactor,
	kNoisyConfigIrNodeType_Pterm,
	kNoisyConfigIrNodeType_Pexpression,
	kNoisyConfigIrNodeType_PvectorScalarPairStatement,
	kNoisyConfigIrNodeType_PvectorIntegralList,
	kNoisyConfigIrNodeType_PscalarIntegralList,
	kNoisyConfigIrNodeType_PlawStatement,
	kNoisyConfigIrNodeType_PdimensionTypeNameStatement,
	kNoisyConfigIrNodeType_PdimensionAliasStatement,
	kNoisyConfigIrNodeType_PvectorScalarPairStatementList,
	kNoisyConfigIrNodeType_PvectorIntegralLists,
	kNoisyConfigIrNodeType_PscalarIntegralLists,
	kNoisyConfigIrNodeType_PlawStatementList,
	kNoisyConfigIrNodeType_PdimensionTypeNameStatementList,
	kNoisyConfigIrNodeType_PdimensionAliasStatementList,
	kNoisyConfigIrNodeType_PvectorScalarPairScope,
	kNoisyConfigIrNodeType_PscalarIntegralScope,
	kNoisyConfigIrNodeType_PvectorIntegralScope,
	kNoisyConfigIrNodeType_PdimensionAliasScope,
	kNoisyConfigIrNodeType_PlawScope,
	kNoisyConfigIrNodeType_PdimensionTypeNameScope,
	kNoisyConfigIrNodeType_PconfigFile,

	kNoisyConfigIrNodeType_Xseq,

    kNoisyConfigIrNodeType_ZbadIdentifier,
    kNoisyConfigIrNodeType_ZbadCharConst,
	kNoisyConfigIrNodeType_Zepsilon,
	kNoisyConfigIrNodeType_Zeof,
    kNoisyConfigIrNodeType_ZbadStringConst,

	kNoisyConfigIrNodeTypeMax
} NoisyConfigIrNodeType;


typedef enum
{
	kNoisyConfigVerbosityVerbose				= (1 << 0),
	kNoisyConfigVerbosityActionTrace			= (1 << 1),
	kNoisyConfigVerbosityCallTrace			= (1 << 2),
	kNoisyConfigVerbosityPostScanStreamCheck		= (1 << 3),
	kNoisyConfigVerbosityPreScanStreamCheck		= (1 << 4),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyConfigVerbosityMax,
} NoisyConfigVerbosity;



typedef enum
{
	kNoisyConfigIrPassXXX					= (0 << 0),
	
	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyConfigIrPassMax,
} NoisyConfigIrPasses;



typedef enum
{
	kNoisyConfigIrBackendDot				= (1 << 0),
	
	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyConfigIrBackendMax,
} NoisyConfigIrBackend;



typedef enum
{
	kNoisyConfigDotDetailLevelNoText			= (1 << 0),
	kNoisyConfigDotDetailLevelNoNilNodes			= (1 << 1),
	
	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyConfigDotDetailLevelMax,
} NoisyConfigDotDetailLevel;



typedef enum
{
	kNoisyConfigIrNodeColorDotBackendColoring		= (1 << 0),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyConfigIrNodeColor,
} NoisyConfigIrNodeColor;



typedef enum
{
	kNoisyConfigMaxBufferLength				= 8192,
	kNoisyConfigChunkBufferLength				= 8192,
	kNoisyConfigMaxErrorTokenCount			= 32,
	kNoisyConfigStreamchkWidth				= 32,
	kNoisyConfigMaxPrintBufferLength			= 8192,
	kNoisyConfigMaxTokenCharacters			= 32,
	kNoisyConfigMaxFilenameLength				= 128,
	kNoisyConfigCgiRandomDigits				= 10,
	kNoisyConfigRlimitCpuSeconds				= 5*60,			/*	5 mins	*/
	kNoisyConfigRlimitRssBytes				= 2*1024*1024*1024UL,	/*	2GB	*/
	kNoisyConfigProgressTimerSeconds			= 5,

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyConfigConstantMax,
} NoisyConfigConstant;



typedef enum
{
	kNoisyConfigModeDefault				= (0 << 0),
	kNoisyConfigModeCallTracing				= (1 << 0),
	kNoisyConfigModeCallStatistics			= (1 << 1),
	kNoisyConfigModeCGI					= (1 << 2),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyConfigModeMax
} NoisyConfigMode;



typedef enum
{
	kNoisyConfigPostFileWriteActionRenderDot		= (1 << 0),

	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyConfigPostFileWriteActionMax,
} NoisyConfigPostFileWriteAction;


typedef struct NoisyConfigScope	NoisyConfigScope;
typedef struct NoisyConfigSymbol	NoisyConfigSymbol;
typedef struct NoisyConfigToken	NoisyConfigToken;
typedef struct NoisyConfigIrNode	NoisyConfigIrNode;
typedef struct NoisyConfigSourceInfo	NoisyConfigSourceInfo;


struct NoisyConfigIrNode
{
	NoisyConfigIrNodeType		type;

	/*
	 *	Syntactic (AST) information.
	 */
	char *			tokenString;
	struct NoisyConfigSourceInfo	*	sourceInfo;
	struct NoisyConfigIrNode *		irParent;
	struct NoisyConfigIrNode *		irLeftChild;
	struct NoisyConfigIrNode *		irRightChild;

	struct NoisyConfigSymbol *		symbol;
    struct NoisyConfigScope * currentScope;

	/*
	 *	Used for coloring the IR tree, e.g., during Graphviz/dot generation
	 */
	NoisyConfigIrNodeColor	nodeColor;
};


struct NoisyConfigSourceInfo
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


struct NoisyConfigToken
{
	NoisyConfigIrNodeType		type;
	char *			identifier;
	uint64_t		integerConst;
	double			realConst;
	char *			stringConst;
	NoisyConfigSourceInfo *	sourceInfo;
	
	NoisyConfigToken *		prev;
	NoisyConfigToken *		next;
};



struct NoisyConfigScope
{
	/*
	 *	For named scopes (at the moment, only Progtypes)
	 */
	char *			identifier;

	/*
	 *	Hierarchy. The firstChild is used to access its siblings via firstChild->next
	 */
	struct NoisyConfigScope *		parent;
	struct NoisyConfigScope *		firstChild;

	/*
	 *	Symbols in this scope. The list of symbols is accesed via firstSymbol->next
	 */
	struct NoisyConfigSymbol *		firstSymbol;

	/*
	 *	Where in source scope begins and ends
	 */
	struct NoisyConfigSourceInfo *	begin;
	struct NoisyConfigSourceInfo *	end;

	/*
	 *	For chaining together scopes (currently only used for Progtype
	 *	scopes and for chaining together children).
	 */
	struct NoisyConfigScope *		next;
	struct NoisyConfigScope *		prev;

	/*
	 *	Used for coloring the IR tree, e.g., during Graphviz/dot generation
	 */
	NoisyConfigIrNodeColor	nodeColor;
};


struct NoisyConfigSymbol
{
	char *			identifier;

	/*
	 *	This field is duplicated in the AST node, since only
	 *	identifiers get into the symbol table:
	 */
	struct NoisyConfigSourceInfo *	sourceInfo;

	/*
	 *	Declaration, type definition, use, etc. (kNoisySymbolTypeXXX)
	 */
	int 			symbolType;

    NoisyConfigIrNodeType type;

	/*
	 *	Scope within which sym appears
	 */
	struct NoisyConfigScope *		scope;

	/*
	 *	If an identifier use, definition's Sym, if any
	 */
	struct NoisyConfigSymbol *		definition;

	/*
	 *	Subtree in AST that represents typeexpr
	 */
	struct NoisyConfigIrNode *		typeTree;

	/*
	 *	If an I_CONST, its value.
	 */
	int			intConst;
	double			realConst;
	char *			stringConst;
	
	/*
	 *	For chaining together sibling symbols in the same scope
	 */
	struct NoisyConfigSymbol *		next;
	struct NoisyConfigSymbol *		prev;
};


typedef struct
{
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


	/*
	 *	The output file of the last render. TODO: Not very happy
	 *	with this solution as it stands... (inherited from Sal/svm)
	 */
	char *			lastDotRender;


	/*
	 *	This is the name of the progtype that the file we're parsing implements
	 */
	char *			progtypeOfFile;

	/*
	 *	We keep a global handle on the list of progtype scopes, for easy reference.
	 *	In this use case, the node->identifier holds the scopes string name, and we
	 *	chain then using their prev/next fields.
	 */
	NoisyConfigScope *		progtypeScopes;


	/*
	 *	Lexer state
	 */
	char *			fileName;
	char *			lineBuffer;
	uint64_t		columnNumber;
	uint64_t		lineNumber;
	uint64_t		lineLength;
	char *			currentToken;
	uint64_t		currentTokenLength;
	struct NoisyConfigToken *		tokenList;
	struct NoisyConfigToken *		lastToken;
	

	/*
	 *	The root of the IR tree, and top scope
	 */
	struct NoisyConfigIrNode *		noisyConfigIrRoot;
	struct NoisyConfigScope *		noisyConfigIrTopScope;


	NoisyConfigMode		mode;
	uint64_t		verbosityLevel;
	uint64_t		dotDetailLevel;
	uint64_t		optimizationLevel;
	uint64_t		irPasses;
	uint64_t		irBackends;


	jmp_buf			jmpbuf;
	bool			jmpbufIsValid;
} NoisyConfigState;


void				noisyConfigFatal(NoisyConfigState *  C, const char *  msg) __attribute__((noreturn));
void				noisyConfigError(NoisyConfigState *  C, const char *  msg);
NoisyConfigState *	    noisyConfigInit(NoisyConfigMode mode);
void				noisyConfigDealloc(NoisyConfigState *  C);
void				noisyConfigRunPasses(NoisyConfigState *  C);
uint64_t			noisyConfigCheckRss(NoisyConfigState *  C);
void				noisyConfigConsolePrintBuffers(NoisyConfigState *  C);
void				noisyConfigPrintToFile(NoisyConfigState *  C, const char *  msg, const char *  fileName, NoisyConfigPostFileWriteAction action);
void				noisyConfigRenderDotInFile(NoisyConfigState *  C, char *  pathName, char *  randomizedFileName);
void				noisyConfigCheckCgiCompletion(NoisyConfigState *  C, const char *  pathName, const char *  renderExtension);
