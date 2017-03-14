
void	checkTokenLength(State *  N, int  count);
char	cur(State *  N);
void		gobble(State *  N, int count);
void		done(State *  N, Token *  newToken);
bool		eqf(State *  N);
bool		isDecimal(State *  N, char *  string);
char *		stringAtLeft(State *  N, char *  string, char  character);
char *		stringAtRight(State *  N, char *  string, char  character);
bool		isDecimalSeparatedWithChar(State *  N, char *  string, char  character);
bool		isDecimalOrRealSeparatedWithChar(State *  N, char *  string, char  character);
bool		isRadixConst(State *  N, char *  string);
bool		isRealConst(State *  N, char *  string);
bool		isEngineeringRealConst(State *  N, char *  string);
uint64_t		stringToRadixConst(State *  N, char *  string);
double		stringToRealConst(State *  N, char *  string);
double		stringToEngineeringRealConst(State *  N, char *  string);


SourceInfo *	lexAllocateSourceInfo(	State *  N, char **  genealogy, 
							char *  fileName, uint64_t lineNumber,
							uint64_t columnNumber, uint64_t length);
Token *		lexAllocateToken(		State *  N, IrNodeType type, 
							char *  identifier, uint64_t integerConst,
							double realConst, char * stringConst,
							SourceInfo *  sourceInfo);
void			lexPut(State *  N, Token *  newToken);
Token *		lexGet(State *  N, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax]);
Token *		lexPeek(State *  N, int lookAhead);

void			lexPrintToken(State *  N, Token *  t, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax]);
void			lexDebugPrintToken(State *  N, Token *  t, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax]);
void			lexPeekPrint(State *  N, int maxTokens, int formatCharacters, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax]);
