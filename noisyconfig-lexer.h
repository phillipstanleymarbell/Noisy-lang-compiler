NoisySourceInfo *	noisyConfigLexAllocateSourceInfo(	NoisyState *  N, char **  genealogy, 
							char *  fileName, uint64_t lineNumber,
							uint64_t columnNumber, uint64_t length);
NoisyToken *		noisyConfigLexAllocateToken(		NoisyState *  N, NoisyIrNodeType type, 
							char *  identifier, uint64_t integerConst,
							double realConst, char * stringConst,
							NoisySourceInfo *  sourceInfo);
void			noisyConfigLexPut(NoisyState *  N, NoisyToken *  newToken);
NoisyToken *		noisyConfigLexGet(NoisyState *  N);
NoisyToken *		noisyConfigLexPeek(NoisyState *  N, int lookAhead);
void			noisyConfigLexInit(NoisyState *  N, char *  fileName);
void			noisyConfigLexPrintToken(NoisyState *  N, NoisyToken *  t);
void			noisyConfigLexDebugPrintToken(NoisyState *  N, NoisyToken *  t);
void			noisyConfigLexPeekPrint(NoisyState *  N, int maxTokens, int formatCharacters);
