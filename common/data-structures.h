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
	kNoisyIrNodeType_Tnone,
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
	kNoisyIrNodeType_TprogtypeQualifier,
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
	kNoisyIrNodeType_Tvector,
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
	kNoisyIrNodeType_PvectorType,
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
	kNoisyIrNodeType_PtupleValue,
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
     * Newton related nodes
     */
	kNewtonIrNodeType_Tnil,
	kNewtonIrNodeType_Tnone,
    kNewtonIrNodeType_Tlt,
    kNewtonIrNodeType_Tle,
    kNewtonIrNodeType_Tgt,
    kNewtonIrNodeType_Tge,
    kNewtonIrNodeType_Tproportionality,
    kNewtonIrNodeType_Tequivalent,
    kNewtonIrNodeType_Tsemicolon,
    kNewtonIrNodeType_Tcolon,
    kNewtonIrNodeType_Tcomma,
    kNewtonIrNodeType_Tdot,
	kNewtonIrNodeType_Tdiv,
	kNewtonIrNodeType_Tmul,
	kNewtonIrNodeType_Tplus,
	kNewtonIrNodeType_Tminus,
    kNewtonIrNodeType_Texponent,
	kNewtonIrNodeType_Tequals,
    kNewtonIrNodeType_TintConst,
    kNewtonIrNodeType_TrealConst,	
	kNewtonIrNodeType_TstringConst,
	kNewtonIrNodeType_ZbadIdentifier,
	kNewtonIrNodeType_ZbadStringConst,
	kNewtonIrNodeType_Zepsilon,
	kNewtonIrNodeType_Zeof,
	kNewtonIrNodeType_Tcross,
	kNewtonIrNodeType_Tintegral,
	kNewtonIrNodeType_Tderivative,
	kNewtonIrNodeType_TSpanish,
	kNewtonIrNodeType_TEnglish,
	kNewtonIrNodeType_Tinvariant,
	kNewtonIrNodeType_Tconstant,
	kNewtonIrNodeType_Tsignal,
	kNewtonIrNodeType_Tderivation,
	kNewtonIrNodeType_Tsymbol,
	kNewtonIrNodeType_Tname,
	kNewtonIrNodeType_Pinteger,
	kNewtonIrNodeType_Tnumber,
	kNewtonIrNodeType_TrightBrace,
	kNewtonIrNodeType_TleftBrace,
	kNewtonIrNodeType_TrightParen,
	kNewtonIrNodeType_TleftParen,
	kNewtonIrNodeType_Tidentifier,
    kNewtonIrNodeType_PlanguageSetting,
	kNewtonIrNodeType_PcompareOp,
	kNewtonIrNodeType_PvectorOp,
	kNewtonIrNodeType_PhighPrecedenceBinaryOp,
	kNewtonIrNodeType_PmidPrecedenceBinaryOp,
	kNewtonIrNodeType_PlowPrecedenceBinaryOp,
	kNewtonIrNodeType_PunaryOp,
	kNewtonIrNodeType_PtimeOp,
	kNewtonIrNodeType_Pquantity,
	kNewtonIrNodeType_PquantityFactor,
	kNewtonIrNodeType_PquantityTerm,
	kNewtonIrNodeType_PquantityExpression,
	kNewtonIrNodeType_Pparameter,
	kNewtonIrNodeType_PparameterTuple,
	kNewtonIrNodeType_Pderivation,
	kNewtonIrNodeType_Psymbol,
	kNewtonIrNodeType_Pname,
	kNewtonIrNodeType_Pconstraint,
	kNewtonIrNodeType_PconstraintList,
	kNewtonIrNodeType_PbaseSignal,
	kNewtonIrNodeType_Pinvariant,
	kNewtonIrNodeType_Pconstant,
	kNewtonIrNodeType_Prule,
	kNewtonIrNodeType_PruleList,
	kNewtonIrNodeType_PnewtonFile,
	
    /*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyIrNodeTypeMax,
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
	kNoisyIrPassXXX					= (0 << 0),
	
	/*
	 *	Code depends on this bringing up the rear.
	 */
	kNoisyIrPassMax,
} NoisyIrPasses;



typedef enum
{
	kNoisyIrBackendDot				= (1 << 0),
	kNoisyIrBackendProtobuf				= (1 << 1),
	
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


typedef struct Scope	Scope;
typedef struct Symbol	Symbol;
typedef struct Token	Token;
typedef struct IrNode	IrNode;
typedef struct SourceInfo	SourceInfo;
typedef struct Dimension Dimension;
typedef struct Physics Physics;
typedef struct IntegralList IntegralList;
typedef struct Invariant Invariant;

struct Dimension
{
    char * identifier;
    char * abbreviation;

	
    Scope *		scope;
	
    SourceInfo *	sourceInfo;

    int primeNumber;

    Dimension * next;
};

struct Invariant
{
    char * identifier; // name of the physics quantity. of type kNoisyConfigType_Tidentifier

    
    Scope *		scope;
    SourceInfo *	sourceInfo;


    IrNode * parameterList; // this is just bunch of IrNode's in Xseq
    unsigned long long int id;

    IrNode * constraints;

    Invariant * next;
};

struct Physics
{
    char * identifier; // name of the physics quantity. of type kNoisyConfigType_Tidentifier
    unsigned long long int id;

    
    Scope *		scope;
    SourceInfo *	sourceInfo;

    bool isVector;
    Physics * vectorCounterpart; // non-NULL if a scalar AND counterpart defined in vectorScalarPairScope
    Physics * scalarCounterpart; // non-NULl if a vector AND counterpart defined in vectorScalarPairScope

    double value; /* for constants like Pi or gravitational acceleration */
    bool isConstant;

    /*
     * numeratorPrimeProduct and denominatorPrimeProduct == 1 means
     * the Physics is dimensionless. e.g. constants like Pi
     */
    Dimension * numeratorDimensions;
    int numberOfNumerators;
    int numeratorPrimeProduct;

    Dimension * denominatorDimensions;
    int numberOfDenominators;
    int denominatorPrimeProduct;

    char * dimensionAlias;
    char * dimensionAliasAbbreviation;

    Physics * definition;

    Physics * next;
};

struct IntegralList
{
    Physics * head;
    IntegralList * next;
};

struct IrNode
{
	IrNodeType		type;

	/*
	 *	Syntactic (AST) information.
	 */
	char *			tokenString;
  Token * token;
	SourceInfo	*	sourceInfo;
	IrNode *		irParent;
	IrNode *		irLeftChild;
	IrNode *		irRightChild;

	Symbol *		symbol;


  /*
   * Used for evaluating dimensions in expressions
   */
  Physics * physics;

  /*
   * only if this node belongs to a ParseNumericExpression subtree
   */
  double value; 

  /*
   * A parameter tuple of length n has ordering from zero to n - 1 
   */
  int parameterNumber;

  /*
   * When doing an API check of the invariant tree given a parameter tree,
   * the method looks up all instances of 
   */

  /*
	 *	Used for coloring the IR tree, e.g., during Graphviz/dot generation
	 */
	IrNodeColor	nodeColor;
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
	SourceInfo *	sourceInfo;
	
	Token *		prev;
	Token *		next;
};



struct Scope
{
	/*
	 *	For named scopes (at the moment, only Progtypes)
	 */
	char *			identifier;

	/*
	 *	Hierarchy. The firstChild is used to access its siblings via firstChild->next
	 */
	Scope *		parent;
	Scope *		firstChild;

	/*
	 *	Symbols in this scope. The list of symbols is accesed via firstSymbol->next
	 */
	Symbol *		firstSymbol;

    /*
     * each invariant scope will have its own list of parameters
     */
    IrNode * invariantParameterList; // this is just bunch of IrNode's in Xseq
    
    /*
     * For the config file, we only have one global scope that keeps track of all
     * dimensions ad physics quantities.
     */
    Dimension * firstDimension;
    Physics * firstPhysics;

	/*
	 *	Where in source scope begins and ends
	 */
	SourceInfo *	begin;
	SourceInfo *	end;

	/*
	 *	For chaining together scopes (currently only used for Progtype
	 *	scopes and for chaining together children).
	 */
	Scope *		next;
	Scope *		prev;

	/*
	 *	Used for coloring the IR tree, e.g., during Graphviz/dot generation
	 */
	IrNodeColor	nodeColor;
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
	Scope *		scope;

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
	TimeStamp *	timestamps;
	uint64_t		timestampCount;
	uint64_t		timestampSlots;


	/*
	 *	Track aggregate time spent in all routines, by incrementing
	 *	timeAggregates[timeAggregatesLastKey] by (now - timeAggregatesLastTimestamp)
	 */
	uint64_t *		timeAggregates;
	TimeStampKey	timeAggregatesLastKey;
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
	Scope *		progtypeScopes;


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
	Token *		tokenList;
	Token *		lastToken;
	

	/*
	 *	The root of the IR tree, and top scope
	 */
	IrNode *		noisyIrRoot;
	IrNode *		newtonIrRoot;
	Scope *		noisyIrTopScope;
  Scope *        newtonIrTopScope;

	/*
	 *	Output file name when emitting bytecode/protobuf
	 */
	char *			outputFilePath;

	NoisyMode		mode;
	uint64_t		verbosityLevel;
	uint64_t		dotDetailLevel;
	uint64_t		optimizationLevel;
	uint64_t		irPasses;
	uint64_t		irBackends;


	jmp_buf			jmpbuf;
	bool			jmpbufIsValid;
    
    /*
     * Global index of which prime numbers we have used for the dimension id's
     */
    int primeNumbersIndex;

  /*
   * When parsing invariant constraints, need to number the factors that correspond to the parameters passed in.
   * This is so that finding matching Parameter doesn't depend either the identifier passed, or the physics type.
   * That is a good idea because now we don't need to implicitly fill in the left identifier child of the parameter node.
   */
  int currentParameterNumber;

    /*
     * This is a group (linked list) of linked list of physics nodes
     */
    IntegralList * vectorIntegralLists;
    
    /*
     * This is a group (linked list) of linked list of physics nodes
     */
    IntegralList * scalarIntegralLists;

    Invariant * invariantList;

} State;


void				fatal(State *  C, const char *  msg) __attribute__((noreturn));
void				error(State *  C, const char *  msg);
void				timestampsInit(State *  C);
void				timeStampDumpTimeline(State *  C);
void				timeStampDumpResidencies(State *  C);
State *			init(NoisyMode mode);
void				dealloc(State *  C);
void				runPasses(State *  C);
uint64_t		checkRss(State *  C);
void				consolePrintBuffers(State *  C);
void				printToFile(State *  C, const char *  msg, const char *  fileName, NoisyPostFileWriteAction action);
void				renderDotInFile(State *  C, char *  pathName, char *  randomizedFileName);
void				checkCgiCompletion(State *  C, const char *  pathName, const char *  renderExtension);
