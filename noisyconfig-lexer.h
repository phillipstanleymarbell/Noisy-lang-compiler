NoisyConfigSourceInfo *	noisyConfigLexAllocateSourceInfo(	NoisyConfigState *  N, char **  genealogy, 
							char *  fileName, uint64_t lineNumber,
							uint64_t columnNumber, uint64_t length);
NoisyConfigToken *		noisyConfigLexAllocateToken(		NoisyConfigState *  N, NoisyConfigIrNodeType type, 
							char *  identifier, uint64_t integerConst,
							double realConst, char * stringConst,
							NoisyConfigSourceInfo *  sourceInfo);
void			noisyConfigLexPut(NoisyConfigState *  N, NoisyConfigToken *  newToken);
NoisyConfigToken *		noisyConfigLexGet(NoisyConfigState *  N);
NoisyConfigToken *		noisyConfigLexPeek(NoisyConfigState *  N, int lookAhead);
void			noisyConfigLexInit(NoisyConfigState *  N, char *  fileName);
void			noisyConfigLexPrintToken(NoisyConfigState *  N, NoisyConfigToken *  t);
void			noisyConfigLexDebugPrintToken(NoisyConfigState *  N, NoisyConfigToken *  t);
void			noisyConfigLexPeekPrint(NoisyConfigState *  N, int maxTokens, int formatCharacters);
