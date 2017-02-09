
void	checkTokenLength(NoisyState *  N, int  count);
char	cur(NoisyState *  N);
void		gobble(NoisyState *  N, int count);
void		done(NoisyState *  N, NoisyToken *  newToken);
bool		eqf(NoisyState *  N);
bool		isDecimal(NoisyState *  N, char *  string);
char *		stringAtLeft(NoisyState *  N, char *  string, char  character);
char *		stringAtRight(NoisyState *  N, char *  string, char  character);
bool		isDecimalSeparatedWithChar(NoisyState *  N, char *  string, char  character);
bool		isDecimalOrRealSeparatedWithChar(NoisyState *  N, char *  string, char  character);
bool		isRadixConst(NoisyState *  N, char *  string);
bool		isRealConst(NoisyState *  N, char *  string);
bool		isEngineeringRealConst(NoisyState *  N, char *  string);
uint64_t		stringToRadixConst(NoisyState *  N, char *  string);
double		stringToRealConst(NoisyState *  N, char *  string);
double		stringToEngineeringRealConst(NoisyState *  N, char *  string);


NoisySourceInfo *	noisyLexAllocateSourceInfo(	NoisyState *  N, char **  genealogy, 
							char *  fileName, uint64_t lineNumber,
							uint64_t columnNumber, uint64_t length);
NoisyToken *		noisyLexAllocateToken(		NoisyState *  N, NoisyIrNodeType type, 
							char *  identifier, uint64_t integerConst,
							double realConst, char * stringConst,
							NoisySourceInfo *  sourceInfo);
void			noisyLexPut(NoisyState *  N, NoisyToken *  newToken);
NoisyToken *		noisyLexGet(NoisyState *  N, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax]);
NoisyToken *		noisyLexPeek(NoisyState *  N, int lookAhead);

void			noisyLexPrintToken(NoisyState *  N, NoisyToken *  t, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax]);
void			noisyLexDebugPrintToken(NoisyState *  N, NoisyToken *  t, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax]);
void			noisyLexPeekPrint(NoisyState *  N, int maxTokens, int formatCharacters, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax]);
