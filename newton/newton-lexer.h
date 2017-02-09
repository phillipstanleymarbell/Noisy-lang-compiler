NoisySourceInfo *	newtonLexAllocateSourceInfo(	NoisyState *  N, char **  genealogy, 
							char *  fileName, uint64_t lineNumber,
							uint64_t columnNumber, uint64_t length);
NoisyToken *		newtonLexAllocateToken(		NoisyState *  N, NoisyIrNodeType type, 
							char *  identifier, uint64_t integerConst,
							double realConst, char * stringConst,
							NoisySourceInfo *  sourceInfo);
void			newtonLexPut(NoisyState *  N, NoisyToken *  newToken);
NoisyToken *		newtonLexGet(NoisyState *  N);
NoisyToken *		newtonLexPeek(NoisyState *  N, int lookAhead);
void			newtonLexInit(NoisyState *  N, char *  fileName);
void			newtonLexPrintToken(NoisyState *  N, NoisyToken *  t);
void			newtonLexDebugPrintToken(NoisyState *  N, NoisyToken *  t);
void			newtonLexPeekPrint(NoisyState *  N, int maxTokens, int formatCharacters);

