SourceInfo *	newtonLexAllocateSourceInfo(State *  N, char **  genealogy, 
							char *  fileName, uint64_t lineNumber,
							uint64_t columnNumber, uint64_t length);
Token *		newtonLexAllocateToken(	State *  N, IrNodeType type, 
							char *  identifier, uint64_t integerConst,
							double realConst, char * stringConst,
							SourceInfo *  sourceInfo);
void			newtonLexPut(State *  N, Token *  newToken);
Token *		newtonLexGet(State *  N);
Token *		newtonLexPeek(State *  N, int lookAhead);
void			newtonLexInit(State *  N, char *  fileName);
void			newtonLexPrintToken(State *  N, Token *  t);
void			newtonLexDebugPrintToken(State *  N, Token *  t);
void			newtonLexPeekPrint(State *  N, int maxTokens, int formatCharacters);

