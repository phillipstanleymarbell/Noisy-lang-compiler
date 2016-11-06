#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisyconfig-errors.h"
#include "version.h"
#include "noisyconfig-timeStamps.h"
#include "noisyconfig.h"
#include "noisyconfig-parser.h"
#include "noisyconfig-lexer.h"


extern const char *	gTerminalStrings[];
extern const char *	gReservedTokenDescriptions[];


static inline void	checkTokenLength(NoisyConfigState *  N, int  count);
static inline char	cur(NoisyConfigState *  N);
static void		gobble(NoisyConfigState *  N, int count);
static void		done(NoisyConfigState *  N, NoisyConfigToken *  newToken);
static bool		eqf(NoisyConfigState *  N);
static void		checkComment(NoisyConfigState *  N);
static void		checkWeq(NoisyConfigState *  N, NoisyConfigIrNodeType type1, NoisyConfigIrNodeType type2);
static void		checkWeq3(NoisyConfigState *  N, NoisyConfigIrNodeType type1, NoisyConfigIrNodeType type2, char char2, NoisyConfigIrNodeType type3);
static void		checkSingle(NoisyConfigState *  N, NoisyConfigIrNodeType tokenType);
static void		checkDot(NoisyConfigState *  N);
static void		checkGt(NoisyConfigState *  N);
static void		checkLt(NoisyConfigState *  N);
static void		checkSingleQuote(NoisyConfigState *  N);
static void		checkDoubleQuote(NoisyConfigState *  N);
static void		checkMinus(NoisyConfigState *  N);
static void		finishToken(NoisyConfigState *  N);
static bool		isDecimal(NoisyConfigState *  N, char *  string);
static char *		stringAtLeft(NoisyConfigState *  N, char *  string, char  character);
static char *		stringAtRight(NoisyConfigState *  N, char *  string, char  character);
static bool		isRealConst(NoisyConfigState *  N, char *  string);
static bool		isEngineeringRealConst(NoisyConfigState *  N, char *  string);
static uint64_t		stringToRadixConst(NoisyConfigState *  N, char *  string);
static double		stringToRealConst(NoisyConfigState *  N, char *  string);
static bool		isOperatorOrSeparator(NoisyConfigState *  N, char c);




NoisyConfigSourceInfo *
noisyConfigLexAllocateSourceInfo(	NoisyConfigState *  N, char **  genealogy, char *  fileName,
				uint64_t lineNumber, uint64_t columnNumber, uint64_t length)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexAllocateSourceInfo);

	NoisyConfigSourceInfo *	newSourceInfo;

	newSourceInfo = (NoisyConfigSourceInfo *) calloc(1, sizeof(NoisyConfigSourceInfo));
	if (newSourceInfo == NULL)
	{
		noisyConfigFatal(N, Emalloc);
	}

	newSourceInfo->genealogy	= genealogy;
	newSourceInfo->fileName		= (fileName == NULL ? NULL : strdup(fileName));
	newSourceInfo->lineNumber	= lineNumber;
	newSourceInfo->columnNumber	= columnNumber;
	newSourceInfo->length		= length;

	return newSourceInfo;
}


NoisyConfigToken *
noisyConfigLexAllocateToken(	
    NoisyConfigState *  N, 
    NoisyConfigIrNodeType type, 
    char *  identifier,
    double realConst, 
    char * stringConst,
	NoisyConfigSourceInfo *  sourceInfo
) {
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexAllocateToken);

	NoisyConfigToken *	newToken;

	newToken = (NoisyConfigToken *) calloc(1, sizeof(NoisyConfigToken));
	if (newToken == NULL)
	{
		noisyFatal(N, Emalloc);
	}
	
	newToken->type		= type;
	newToken->identifier	= (identifier == NULL ? NULL : strdup(identifier));
	newToken->realConst	= realConst;
	newToken->stringConst	= (stringConst == NULL ? NULL : strdup(stringConst));
	newToken->sourceInfo	= sourceInfo;

	return newToken;
}



void
noisyConfigLexPut(NoisyConfigState *  N, NoisyConfigToken *  newToken)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexPut);

	if (newToken == NULL)
	{
		noisyConfigFatal(N, Esanity);
	}

	/*
	 *	If the token list is empty, we allocate and put on an EOF node, which we never remove.
	 */	
	if (N->tokenList == NULL)
	{
		N->lastToken = N->tokenList = newToken;
	}
	else
	{
		newToken->prev = N->lastToken;
		N->lastToken->next = newToken;
		N->lastToken = newToken;
	}
}


NoisyConfigToken *
noisyConfigLexGet(NoisyConfigConfigState *  N)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexGet);

	if (N->tokenList == NULL)
	{
		noisyConfigFatal(N, Esanity);
	}

	NoisyConfigToken *	t = N->tokenList;
	
	if (t->next != NULL)
	{
		N->tokenList = N->tokenList->next;
	}
	else if (t->type != kNoisyConfigIrNodeType_Zeof)
	{
		noisyConfigFatal(N, Esanity);
	}

	if (N->verbosityLevel & kNoisyConfigVerbosityDebugLexer)
	{
		noisyConfigLexDebugPrintToken(N, t);	
	}

	return t;
}


NoisyConfigToken *
noisyConfigLexPeek(NoisyConfigState *  N, int lookAhead)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexPeek);

	if (N->tokenList == NULL)
	{
		noisyConfigFatal(N, Esanity);
	}

	NoisyConfigToken *	tmp = N->tokenList;
	int 		which = 1;
	while ((tmp != NULL) && (which++ < lookAhead))
	{
		tmp = tmp->next;
	}

	return tmp;
}

void
noisyConfigLexPeekPrint(NoisyConfigState *  N, int maxTokens, int formatCharacters)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexPeekPrint);

	if (N->tokenList == NULL)
	{
		noisyConfigFatal(N, Esanity);
	}

	int		tripCharacters = 0, done = 0;
	NoisyConfigToken *	tmp = N->tokenList;

	flexprint(N->Fe, N->Fm, N->Fperr, "\t\tline %5d, token %3d\t", tmp->sourceInfo->lineNumber, tmp->sourceInfo->columnNumber);
	while (tmp != NULL)
	{
		if (maxTokens > 0 && (done++ > maxTokens))
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "...");
			break;
		}

		{
			/*
			 *	NOTE: We currently don't keep newlines and whitespace on the token list...
			 */
			switch (tmp->type)
			{
                // TODO filter cases
				case kNoisyConfigIrNodeType_Tidentifier:
				{
					flexprint(N->Fe, N->Fm, N->Fperr, "'%s' (identifier) ", tmp->identifier);

					/*
					 *	Account for the output string and the
					 *	two guarding "'" quotes.
					 */
					tripCharacters += strlen(tmp->identifier) + 14;

					break;
				}

				case kNoisyConfigIrNodeType_TintConst:
				{
					flexprint(N->Fe, N->Fm, N->Fperr, "'%llu' ", tmp->integerConst);

					char	dummy[64];
					tripCharacters += sprintf(dummy, "'%llu' ", tmp->integerConst);

					break;
				}

				case kNoisyConfigIrNodeType_TrealConst:
				{
					flexprint(N->Fe, N->Fm, N->Fperr, "'%f' ", tmp->realConst);

					char	dummy[64];
					tripCharacters += sprintf(dummy, "'%f' ", tmp->realConst);

					break;
				}

				case kNoisyConfigIrNodeType_TstringConst:
				{
					flexprint(N->Fe, N->Fm, N->Fperr, "'\"%s\"' ", tmp->stringConst);

					/*
					 *	Account for the output string and the
					 *	two guarding "'" quotes.
					 */
					tripCharacters += strlen(tmp->stringConst) + 2;

					break;
				}

				default:
				{
					if (gReservedTokenDescriptions[tmp->type] != NULL)
					{
						flexprint(N->Fe, N->Fm, N->Fperr, "%s ", gReservedTokenDescriptions[tmp->type]);

						/*
						 *	Account for the trailing space
						 */
						tripCharacters += strlen(gReservedTokenDescriptions[tmp->type]) + 1;

					}
					else
					{
						flexprint(N->Fe, N->Fm, N->Fperr, ">>>BUG/TODO: un-handled type [%d] in noisyConfigLexPeekPrint <<<", tmp->type);
						noisyConfigFatal(N, Esanity);
					}
				}
			}

			if ((tmp->next != NULL) && (tmp->sourceInfo->lineNumber != tmp->next->sourceInfo->lineNumber))
			{
				//flexprint(N->Fe, N->Fm, N->Fperr, "(newlines)");
				tripCharacters = 0;

				flexprint(N->Fe, N->Fm, N->Fperr, "\n\t\tline %5d\t\t", tmp->next->sourceInfo->lineNumber);
			}
			else if (tripCharacters >= formatCharacters)
			{
				tripCharacters = 0;
				flexprint(N->Fe, N->Fm, N->Fperr, "\n\t\t\t\t\t");
			}
		}

		tmp = tmp->next;
	}
	flexprint(N->Fe, N->Fm, N->Fperr, "\n");
}


void
noisyConfigLexInit(NoisyConfigState *  N, char *  fileName)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexInit);

	FILE *			filePointer;
	size_t			lineBufferSize;


	N->fileName 		= fileName;
	N->columnNumber		= 1;
	N->lineNumber		= 1;
	N->lineLength		= 0;


	filePointer = fopen(fileName, "r");
	if (filePointer == NULL)
	{
		noisyConfigFatal(N, Eopen);
	}


	/*
	 *	Notes:
	 *
	 *	(1)	The way we handle lexing in M and Noisy compilers does not use the
	 *		'stickies' as we do in our Yacc-based parsers.
	 *
	 *	(2)	We currently split up the input by '\n'-separated newline. This is
	 *		OK, since we also recognize '\r' as being a discardable whitespace.
	 *
	 *	(3)	In the CGI case, we dump the program to a temporary file (we do this
	 *		anyway to keep copies of inputs), and feed that file to the compiler.
	 */
	
	/*
	 *	The following two are needed in order for getline() to allocate the buffer itself
	 */
	N->lineBuffer = NULL;
	lineBufferSize = 0;

	while ((N->lineLength = getline(&(N->lineBuffer), &lineBufferSize, filePointer)) != -1)
	{
		N->columnNumber = 0;
		while (N->columnNumber < N->lineLength)
		{

			if (isOperatorOrSeparator(N, cur(N)))
			{
				switch (cur(N))
				{
					/*
					 *	These tokens may be paired with an equals sign or with another char (e.g., "::"),
					 *	but otherwise do not require additional special handling as in the case of ".".
					 *
					 *	We process the chars see so far as a finished token, then handle the following chars.
					 */
					case '&':
					{
						checkWeq3(N, kNoisyConfigIrNodeType_TandAs, kNoisyConfigIrNodeType_Tand, '&', kNoisyConfigIrNodeType_Tampersand);
						continue;
					}

					case '|':
					{
						checkWeq3(N, kNoisyConfigIrNodeType_TorAs, kNoisyConfigIrNodeType_Tor, '|', kNoisyConfigIrNodeType_Tstroke);
						continue;
					}

					case ':':
					{
						checkWeq3(N, kNoisyConfigIrNodeType_TdefineAs, kNoisyConfigIrNodeType_Tcons, ':', kNoisyConfigIrNodeType_Tcolon);
						continue;
					}

					case '=':
					{
						checkWeq3(N, kNoisyConfigIrNodeType_Teq, kNoisyConfigIrNodeType_Tgoes, '>', kNoisyConfigIrNodeType_Tas);
						continue;
					}

					case '+':
					{
						checkWeq3(N, kNoisyConfigIrNodeType_TaddAs, kNoisyConfigIrNodeType_Tinc, '+', kNoisyConfigIrNodeType_Tplus);
						continue;
					}


					/*
					 *	These tokens only occur alone.
					 *
					 *	We process the chars see so far as a finished token, then handle the following chars.
					 */
					case '(':
					{
						checkSingle(N, kNoisyConfigIrNodeType_TleftParen);
						continue;
					}

					case ')':
					{
						checkSingle(N, kNoisyConfigIrNodeType_TrightParen);
						continue;
					}
					
                    case ',':
					{
						/*
						 *	TODO/BUG: Is this right? Re-check. What about kNoisyIrNodeType_Tdot?
						 */
						checkSingle(N, kNoisyConfigIrNodeType_Tcomma);
						continue;
					}


					/*
					 *	These tokens require special handling beyond being paired with an equals,
					 *	being part of a number, or doubled-up (e.g., ">>", etc.).
					 */
					case '>':
					{
						checkGt(N);
						continue;
					}

					case '<':
					{
						checkLt(N);
						continue;
					}

					case '\'':
					{
						checkSingleQuote(N);
						continue;
					}

					case '\"':
					{
						checkDoubleQuote(N);
						continue;
					}

					case '-':
					{
						checkMinus(N);
						continue;
					}

					case '#':
					{
						checkComment(N);
						
						/*
						 *	Set columnNumber to lineLength so we stop chomping on this line
						 */
						N->columnNumber = N->lineLength;
						continue;
					}

					/*
					 *	Whitespace.
					 */
					case ' ' :
					case '\n':
					case '\r':
					case '\t':
					{
						N->columnNumber++; finishToken(N);
						continue;
					}

					default:
					{
						noisyConfigFatal(N, Esanity);
					}
				}
			}
			
			checkTokenLength(N, 1);
			N->currentToken[N->currentTokenLength++] = N->lineBuffer[N->columnNumber++];
		}
		N->lineNumber++;
	
		/*
		 *	In order for getline() to allocate the buffer itself on the next iteration...
		 */
		N->lineBuffer = NULL;
		lineBufferSize = 0;
	}

	fclose(filePointer);

	NoisyConfigSourceInfo *	eofSourceInfo = noisyConfigLexAllocateSourceInfo(N,	NULL /* genealogy */,
										N->fileName /* fileName */,
										N->lineNumber /* lineNumber */,
										N->columnNumber /* columnNumber */,
										0 /* length */);
										
	NoisyConfigToken *		eofToken = noisyConfigLexAllocateToken(N,	kNoisyConfigIrNodeType_Zeof /* type */,
									NULL /* identifier */,
									0 /* integerConst */,
									0.0 /* realConst */,
									NULL /* stringConst */,
									eofSourceInfo /* sourceInfo */);
	noisyConfigLexPut(N, eofToken);

	if (N->verbosityLevel & kNoisyConfigVerbosityDebugLexer)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Done lexing...\n");
		
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\n");
		NoisyConfigToken *	p = N->tokenList;
		while (p != NULL)
		{
			noisyConfigLexDebugPrintToken(N, p);
			p = p->next;
		}
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\n");
	}


	return;
}


void
noisyConfigLexDebugPrintToken(NoisyConfigState *  N, NoisyConfigToken *  t)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexDebugPrintToken);

	flexprint(N->Fe, N->Fm, N->Fperr, "Token %30s: ", gTerminalStrings[t->type]);

	switch (t->type)
	{
        // TODO handle all the cases
		case kNoisyConfigIrNodeType_Tidentifier:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%20s\", ", t->identifier);
			break;
		}

		case kNoisyConfigIrNodeType_TintConst:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%20d\", ", t->integerConst);
			break;
		}

		case kNoisyConfigIrNodeType_TrealConst:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%20f\", ", t->realConst);
			break;
		}

		case kNoisyConfigIrNodeType_TstringConst:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%20s\", ", t->stringConst);
			break;
		}

		default:
		{
			if (gReservedTokenDescriptions[t->type] != NULL)
			{
				flexprint(N->Fe, N->Fm, N->Fperr, "%22s, ", gReservedTokenDescriptions[t->type]);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fperr, ">>>BUG: unhandled type [%d] in noisyConfigLexDebugPrintToken <<<", t->type);
				//noisyFatal(N, Esanity);
			}
		}
	}

	flexprint(N->Fe, N->Fm, N->Fperr, "source file: %16s, line %3d, pos %3d, length %3d\n",
		t->sourceInfo->fileName, t->sourceInfo->lineNumber, t->sourceInfo->columnNumber, t->sourceInfo->length);
}


void
noisyConfigLexPrintToken(NoisyConfigConfigState *  N, NoisyConfigToken *  t)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexPrintToken);

	switch (t->type)
	{
		case kNoisyConfigIrNodeType_Tidentifier:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%s\"", t->identifier);
			break;
		}

		case kNoisyConfigIrNodeType_TintConst:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%d\"", t->integerConst);
			break;
		}

		case kNoisyConfigIrNodeType_TstringConst:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%s\"", t->stringConst);
			break;
		}

		default:
		{
			if (gReservedTokenDescriptions[t->type] != NULL)
			{
				flexprint(N->Fe, N->Fm, N->Fperr, "%s", gReservedTokenDescriptions[t->type]);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fperr, ">>>BUG: unhandled type [%d] in noisyLexPrintToken <<<", t->type);
				//noisyFatal(N, Esanity);
			}
		}
	}
}



/*
 *	Local non-exposed routines.
 */



static inline void
checkTokenLength(NoisyConfigState *  N, int  count)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerCheckTokenLength);

	if (N->currentTokenLength+count >= kNoisyConfigMaxBufferLength)
	{
		noisyConfigFatal(N, EtokenTooLong);
	}
}

static inline char
cur(NoisyConfigState *  N)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerCur);

	return N->lineBuffer[N->columnNumber];
}


static void
gobble(NoisyConfigState *  N, int count)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerGobble);

	checkTokenLength(N, count);
	strncpy(N->currentToken, &N->lineBuffer[N->columnNumber], count);

	N->columnNumber += count;
}


static void
done(NoisyConfigState *  N, NoisyConfigToken *  newToken)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerDone);

	newToken->sourceInfo = noisyLexAllocateSourceInfo(N,	NULL				/*   genealogy 	*/,
								N->fileName			/*   fileName 	*/,
								N->lineNumber			/*   lineNumber */,
								N->columnNumber - N->currentTokenLength /* columnNumber */,
								N->currentTokenLength		/*   length 	*/);

	bzero(N->currentToken, kNoisyConfigMaxBufferLength);
	N->currentTokenLength = 0;
	noisyLexPut(N, newToken);
}


static bool
eqf(NoisyConfigState *  N)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerEqf);

	return (N->lineLength >= 2 && N->lineBuffer[N->columnNumber+1] == '=');
}


static void
checkComment(NoisyConfigState *  N)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerCheckComment);

	/*
	 *	Gobble any extant chars
	 */
	finishToken(N);

	/*
	 *	NOTE: currentToken is allocated only once, but lineBuffer is allocated each 
	 *	time a line is read via getline().
	 */
	free(N->lineBuffer);
}


static void
checkWeq(NoisyConfigState *  N, NoisyConfigIrNodeType type1, NoisyConfigIrNodeType type2)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerCheckWeq);

	NoisyConfigIrNodeType		type;

	/*
	 *	Gobble any extant chars
	 */
	finishToken(N);

	if (eqf(N))
	{
		gobble(N, 2);
		type = type1;
	}
	else
	{
		gobble(N, 1);
		type = type2;
	}

	NoisyConfigToken *		newToken = noisyConfigLexAllocateToken(N,	type	/* type		*/,
									NULL	/* identifier	*/,
									0	/* integerConst	*/,
									0.0	/* realConst	*/,
									NULL	/* stringConst	*/,
									NULL	/* sourceInfo	*/);

	/*
	 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
	 */
	done(N, newToken);
}


static void
checkWeq3(NoisyConfigState *  N, NoisyConfigIrNodeType type1, NoisyConfigIrNodeType type2, char char2, NoisyConfigIrNodeType type3)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerCheckWeq3);

	NoisyConfigIrNodeType		type;

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	if (eqf(N))
	{
		gobble(N, 2);
		type = type1;
	}
	else if (N->lineLength >= 2 && N->lineBuffer[N->columnNumber+1] == char2)
	{
		gobble(N, 2);
		type = type2;
	}
	else
	{
		gobble(N, 1);
		type = type3;
	}

	NoisyConfigToken *		newToken = noisyLexAllocateToken(N,	type	/* type		*/,
									NULL	/* identifier	*/,
									0	/* integerConst	*/,
									0.0	/* realConst	*/,
									NULL	/* stringConst	*/,
									NULL	/* sourceInfo	*/);

	/*
	 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
	 */
	done(N, newToken);
}


static void
checkSingle(NoisyConfigState *  N, NoisyConfigIrNodeType tokenType)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerCheckSingle);

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	gobble(N, 1);

	if (N->verbosityLevel & kNoisyConfigVerbosityDebugLexer)
	{
		//flexprint(N->Fe, N->Fm, N->Fperr, "checkSingle(), tokenType = %d\n", tokenType);
//fprintf(stderr, "checkSingle(), tokenType = %d\n", tokenType);
	}

	NoisyConfigToken *		newToken = noisyLexAllocateToken(N,	tokenType /* type	*/,
									NULL	/* identifier	*/,
									0	/* integerConst	*/,
									0.0	/* realConst	*/,
									NULL	/* stringConst	*/,
									NULL	/* sourceInfo	*/);

	/*
	 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
	 */
	done(N, newToken);
}


static void
checkDot(NoisyConfigState *  N)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerCheckDot);

	/*
	 *	If token thus far is	"0" | onenine {zeronine}	then
	 *	don't gobble; continue building token.  However, something like
	 *	"5zyyg".
	 */
	if (isDecimal(N, N->currentToken))
	{
		checkTokenLength(N, 1);
		N->currentToken[N->currentTokenLength++] = N->lineBuffer[N->columnNumber++];

		return;
	}

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	gobble(N, 1);

	NoisyConfigToken *		newToken = noisyLexAllocateToken(N,	kNoisyConfigIrNodeType_Tdot /* type	*/,
									NULL	/* identifier	*/,
									0	/* integerConst	*/,
									0.0	/* realConst	*/,
									NULL	/* stringConst	*/,
									NULL	/* sourceInfo	*/);

	/*
	 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
	 */
	done(N, newToken);
}


static void
checkGt(NoisyConfigState *  N)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerCheckGt);

	NoisyConfigIrNodeType		type;

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	if (eqf(N))
	{
		gobble(N, 2);
		type = kNoisyConfigIrNodeType_Tge;
	}
	else if (N->lineLength >= 3 &&
		N->lineBuffer[N->columnNumber+1] == '>' &&
		N->lineBuffer[N->columnNumber+2] == '=')
	{
		gobble(N, 3);
		type = kNoisyConfigIrNodeType_TrightShiftAs;
	}
	else if (N->lineLength >= 2 &&
		N->lineBuffer[N->columnNumber+1] == '>')
	{
		gobble(N, 2);
		type = kNoisyConfigIrNodeType_TrightShift;
	}
	else
	{
		gobble(N, 1);
		type = kNoisyConfigIrNodeType_Tgt;
	}

	NoisyConfigToken *		newToken = noisyLexAllocateToken(N,	type	/* type		*/,
									NULL	/* identifier	*/,
									0	/* integerConst	*/,
									0.0	/* realConst	*/,
									NULL	/* stringConst	*/,
									NULL	/* sourceInfo	*/);

	/*
	 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
	 */
	done(N, newToken);
}


static void
checkLt(NoisyConfigState *  N)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerCheckLt);

	NoisyConfigIrNodeType		type;

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	if (eqf(N))
	{
		gobble(N, 2);
		type = kNoisyConfigIrNodeType_Tle;
	}
	else if (N->lineLength >= 3 &&
		N->lineBuffer[N->columnNumber+1] == '<' &&
		N->lineBuffer[N->columnNumber+2] == '=')
	{
		gobble(N, 3);
		type = kNoisyConfigIrNodeType_TleftShiftAs;
	}
	else if (N->lineLength >= 3 &&
		N->lineBuffer[N->columnNumber+1] == '-' &&
		N->lineBuffer[N->columnNumber+2] == '=')
	{
		gobble(N, 3);
		type = kNoisyConfigIrNodeType_TchanWrite;
	}
	else if (N->lineLength >= 2 &&
		N->lineBuffer[N->columnNumber+1] == '<')
	{
		gobble(N, 2);
		type = kNoisyConfigIrNodeType_TleftShift;
	}
	else if (N->lineLength >= 2 &&
		N->lineBuffer[N->columnNumber+1] == '-')
	{
		gobble(N, 2);
		type = kNoisyConfigIrNodeType_Tgets;
	}
	else
	{
		gobble(N, 1);
		type = kNoisyConfigIrNodeType_Tlt;
	}

	NoisyConfigToken *		newToken = noisyLexAllocateToken(N,	type	/* type		*/,
									NULL	/* identifier	*/,
									0	/* integerConst	*/,
									0.0	/* realConst	*/,
									NULL	/* stringConst	*/,
									NULL	/* sourceInfo	*/);

	/*
	 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
	 */
	done(N, newToken);
}


static void
checkSingleQuote(NoisyConfigState *  N)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerCheckSingleQuote);

	/*
	 *	TODO/BUG: we do not handle escaped squotes in a charconst
	 */
	NoisyConfigIrNodeType		type;
	char			quotedChar;


	/*
	 *	Gobble any extant chars
	 */
	finishToken(N);


	if ((N->lineLength >= 3) && (N->lineBuffer[N->columnNumber+2] == '\''))
	{
		type = kNoisyConfigIrNodeType_TcharConst;
		quotedChar = N->lineBuffer[N->columnNumber+1];
		gobble(N, 3);
	}
	else
	{
		/*
		 *	BUG/TODO: This could be improved:
		 *
		 *	If we see >1 char in-between single quotes, create a
		 *	kNoisyConfigIrNodeType_ZbadCharConst token using the first
		 *	character after the quote as the token character constant.
		 */
		type = kNoisyConfigIrNodeType_ZbadCharConst;
		quotedChar = N->lineBuffer[N->columnNumber+1];
		gobble(N, 1);
	}

	NoisyConfigToken *		newToken = noisyLexAllocateToken(N,	type	/* type		*/,
									NULL	/* identifier	*/,
									quotedChar	/* integerConst	*/,
									0.0	/* realConst	*/,
									NULL	/* stringConst	*/,
									NULL	/* sourceInfo	*/);

	/*
	 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
	 */
	done(N, newToken);
}


static void
checkDoubleQuote(NoisyConfigState *  N)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerCheckDoubleQuote);

	/*
	 *	TODO/BUG: we do not handle escaped dquotes in a strconst
	 */
	NoisyConfigToken *	newToken;


	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	/*
	 *	String constants cannot contain an un-escaped newline;  the current
	 *	N->lineBuffer must contain the closing quote, else we flag this as a
	 *	bad string constant (kNoisyConfigIrNodeType_ZbadStringConst)
	 */
	if (strchr(&N->lineBuffer[N->columnNumber+1], '\"') == NULL)
	{
		newToken = noisyLexAllocateToken(N,	kNoisyConfigIrNodeType_ZbadStringConst	/* type		*/,
							NULL					/* identifier	*/,
							0					/* integerConst	*/,
							0.0					/* realConst	*/,
							&N->lineBuffer[N->columnNumber]		/* stringConst	*/,
							NULL					/* sourceInfo	*/);
	}
	else
	{
		N->columnNumber++;

		/*
		 *	NOTE: N->currentToken is pre-allocated to be kNoisyConfigMaxBufferLength characters long.
		 */
		while ((cur(N) != '\"') && N->currentTokenLength < (kNoisyConfigMaxBufferLength - 1) && (N->columnNumber < N->lineLength))
		{
			checkTokenLength(N, 1);
			N->currentToken[N->currentTokenLength++] = N->lineBuffer[N->columnNumber++];
		}

		checkTokenLength(N, 1);
		N->currentToken[N->currentTokenLength++] = '\0';

		if (cur(N) != '\"')
		{
			/*
			 *	We ran out of buffer space or reached end of lineBuffer
			 */
			noisyFatal(N, EstringTooLongOrWithNewline);
		}
		else
		{
			/*
			 *	Move past the closing quote
			 */
			N->columnNumber++;
		}

		newToken = noisyLexAllocateToken(N,	kNoisyConfigIrNodeType_TstringConst	/* type		*/,
							NULL				/* identifier	*/,
							0				/* integerConst	*/,
							0.0				/* realConst	*/,
							N->currentToken			/* stringConst	*/,
							NULL				/* sourceInfo	*/);
	}

	/*
	 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
	 */
	done(N, newToken);
}


static void
checkMinus(NoisyConfigState *  N)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerCheckMinus);

	NoisyConfigIrNodeType		type;

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	if (eqf(N))
	{
		gobble(N, 2);
		type = kNoisyConfigIrNodeType_TsubAs;
	}
	else if (N->lineLength >= 2 && N->lineBuffer[N->columnNumber+1] == '-')
	{
		gobble(N, 2);
		type = kNoisyConfigIrNodeType_Tdec;
	}
	else if (N->lineLength >= 2 && N->lineBuffer[N->columnNumber+1] == '>')
	{
		gobble(N, 2);
		type = kNoisyConfigIrNodeType_TprogtypeQualifier;
	}
	else
	{
		gobble(N, 1);
		type = kNoisyConfigIrNodeType_Tminus;
	}

	NoisyConfigToken *		newToken = noisyLexAllocateToken(N,	type	/* type		*/,
									NULL	/* identifier	*/,
									0	/* integerConst	*/,
									0.0	/* realConst	*/,
									NULL	/* stringConst	*/,
									NULL	/* sourceInfo	*/);

	/*
	 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
	 */
	done(N, newToken);
}

static void
finishToken(NoisyConfigState *  N)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerFinishToken);

	if (N->verbosityLevel & kNoisyConfigVerbosityDebugLexer)
	{
		//flexprint(N->Fe, N->Fm, N->Fperr, "in finishToken(), N->currentToken = [%s]\n", N->currentToken);
fprintf(stderr, "in finishToken(), N->currentToken = [%s]\n", N->currentToken);
	}

	/*
	 *	Called when we've eaten (zero or more) chars which are
	 *	not operators or separators.  Whitespace isn't actually
	 *	added to N->currentToken, but rather we get called here.
	 */
	if (N->currentTokenLength == 0)
	{
		return;
	}

	/*
	 *	'true' and 'false' are reserved indentifiers, but are constants. Eat them first.
	 */
	if (!strcmp(N->currentToken, "true") || !strcmp(N->currentToken, "false"))
	{
		/*
		 *	Booleans are kept in the 'integerConst' field of the token.
		 */
		NoisyConfigToken *	newToken = noisyLexAllocateToken(N,	kNoisyConfigIrNodeType_TboolConst	/*	type		*/,
									NULL				/*	identifier	*/,
									!strcmp(N->currentToken, "true")/*	integerConst	*/,
									0.0	/* realConst	*/,
									NULL	/* stringConst	*/,
									NULL	/* sourceInfo	*/);

		/*
		 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
		 */
		done(N, newToken);

		return;
	}

	for (int i = 0; i < kNoisyConfigIrNodeTypeMax; i++)
	{
		if ((gReservedTokenDescriptions[i] != NULL) && !strcmp(gReservedTokenDescriptions[i], N->currentToken))
		{
			NoisyConfigToken *	newToken = noisyLexAllocateToken(N,	i	/* type		*/,
										NULL	/* identifier	*/,
										0	/* integerConst	*/,
										0.0	/* realConst	*/,
										NULL	/* stringConst	*/,
										NULL	/* sourceInfo	*/);

			/*
			 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
			 */
			done(N, newToken);

			return;
		}
	}

	if (N->currentToken[0] >= '0' && N->currentToken[0] <= '9')
	{
		makeNumericConst(N);
		return;
	}

	/*
	 *	Otherwise, it's an identifier.  It will not have illegal chars in it
	 *	since we would have halted the building of the token on seing them
	 *	and gotten called here.
	 */
	NoisyConfigToken *	newToken = noisyLexAllocateToken(N,	kNoisyConfigIrNodeType_Tidentifier	/* type		*/,
								N->currentToken			/* identifier	*/,
								0	/* integerConst	*/,
								0.0	/* realConst	*/,
								NULL	/* stringConst	*/,
								NULL	/* sourceInfo	*/);

	/*
	 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
	 */
	done(N, newToken);
}

static void
makeNumericConst(NoisyConfigState *  N)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerMakeNumericConst);

//fprintf(stderr, "in makeNumericConst(), N->currentToken = [%s]\n", N->currentToken);

	if (N->currentTokenLength == 0)
	{
		noisyFatal(N, EruntTokenInNumericConst);
	}

	/*
	 *	If it begins with a onenine or a zeronine (see regular grammar),
	 *	check if it's an intconst or realconst tokens which begin with 
	 *	numbers but do not make valid consts, make a "illegal identifier"
	 *	token kNoisyConfigIrNodeType_ZbadIdentifier
	 */

	/*
	 *	There are only two valid cases with leading zero: "0" and "0.XXX".
	 *	You cannot have 0XXX or even 0rXXX
	 */
	if (N->currentToken[0] == '0')
	{
		if (N->currentTokenLength == 1)
		{
			NoisyConfigToken *	newToken = noisyLexAllocateToken(N,	kNoisyConfigIrNodeType_TintConst	/* type		*/,
										NULL	/* identifier	*/,
										0	/* integerConst	*/,
										0.0	/* realConst	*/,
										NULL	/* stringConst	*/,
										NULL	/* sourceInfo	*/);

			/*
			 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
			 */
			done(N, newToken);

			return;
		}
		else if (N->currentToken[1] != '.')
		{
			NoisyConfigToken *	newToken = noisyLexAllocateToken(N,	kNoisyConfigIrNodeType_ZbadIdentifier	/* type		*/,
										N->currentToken	/* identifier	*/,
										0	/* integerConst	*/,
										0.0	/* realConst	*/,
										NULL	/* stringConst	*/,
										NULL	/* sourceInfo	*/);
			

			/*
			 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
			 */
			done(N, newToken);

			return;
		}
	}

	/*
	 *	First (before checking if it has a '.'), check if it has the form XXX 'e|E' YYY, XXX can be decimal or real, YYY must be decimal.
	 *
	 *	(TODO: this implies that 'e' and 'E' bind tighter than '.'. We should record this and discuss in the language manual.)
	 */
	if (isEngineeringRealConst(N, N->currentToken))
	{
		NoisyConfigToken *	newToken = noisyLexAllocateToken(N,	kNoisyConfigIrNodeType_TrealConst	/* type		*/,
									NULL				/* identifier	*/,
									0				/* integerConst	*/,
									stringToEngineeringRealConst(N, N->currentToken) /* realConst	*/,
									NULL				/* stringConst	*/,
									NULL				/* sourceInfo	*/);

		/*
		 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
		 */
		done(N, newToken);

		return;
	}

	/*
	 *	Otherwise, check if it has the form XXX '.' YYY, XXX and YYY must be decimals.
	 */
	if (isRealConst(N, N->currentToken))
	{
		NoisyConfigToken *	newToken = noisyLexAllocateToken(N,	kNoisyConfigIrNodeType_TrealConst	/* type		*/,
									NULL				/* identifier	*/,
									0				/* integerConst	*/,
									stringToRealConst(N, N->currentToken)	/* realConst	*/,
									NULL				/* stringConst	*/,
									NULL				/* sourceInfo	*/);

		/*
		 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
		 */
		done(N, newToken);

		return;
	}

	/*
	 *	Has the form XXX 'r' YYY, XXX must be decimal, YYY must be in base XXX.
	 */
	if (isRadixConst(N, N->currentToken))
	{
		NoisyConfigToken *	newToken = noisyLexAllocateToken(N,	kNoisyConfigIrNodeType_TintConst	/* type		*/,
									NULL				/* identifier	*/,
									stringToRadixConst(N, N->currentToken)	/* integerConst	*/,
									0				/* realConst	*/,
									NULL				/* stringConst	*/,
									NULL				/* sourceInfo	*/);

		/*
		 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
		 */
		done(N, newToken);

		return;
	}

	/*
	 *	At this point, if it is a non-decimal, it must be illegal.
	 */
	if (!isDecimal(N, N->currentToken))
	{
		NoisyConfigToken *	newToken = noisyLexAllocateToken(N,	kNoisyConfigIrNodeType_ZbadIdentifier	/* type		*/,
									N->currentToken			/* identifier	*/,
									0				/* integerConst	*/,
									0				/* realConst	*/,
									NULL				/* stringConst	*/,
									NULL				/* sourceInfo	*/);

		/*
		 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
		 */
		done(N, newToken);

		return;
	}

	/*
	 *	It's a decimal.
	 */
	char		tmp;
	char *		ep = &tmp;
	uint64_t 	decimalValue = strtoul(N->currentToken, &ep, 0);
	if (*ep == '\0')
	{
		NoisyConfigToken *	newToken = noisyLexAllocateToken(N,	kNoisyConfigIrNodeType_TintConst	/* type		*/,
									NULL				/* identifier	*/,
									decimalValue			/* integerConst	*/,
									0				/* realConst	*/,
									NULL				/* stringConst	*/,
									NULL				/* sourceInfo	*/);

		/*
		 *	done() sets the N->currentTokenLength to zero and bzero's the N->currentToken buffer.
		 */
		done(N, newToken);
	}
	else
	{
		noisyFatal(N, Esanity);
	}
}


static bool
isDecimal(NoisyConfigState *  N, char *  string)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerIsDecimal);

	if (string == NULL)
	{
		return false;
	}

	size_t	stringLength = strlen(string);
	for (int i = 0; i < stringLength; i++)
	{
		if (string[i] < '0' || string[i] > '9')
		{
			return false;
		}
	}

	return (string != NULL);
}


static char *
stringAtLeft(NoisyConfigState *  N, char *  string, char character)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerStringAtLeft);

	if (string == NULL)
	{
		return string;
	}

	/*
	 *	NOTE: stringAtLeft (but not stringAtRight) makes
	 *	a copy, because it will modify 'string' by inserting
	 *	a '\0' at the position of 'character' and this has
	 *	to be freed by caller.
	 */
	char *	left = strdup(string);
	char *	right = strchr(string, character);

	if (right == NULL)
	{
		return left;
	}

	/*
	 *	right - string. Since left is a strdup, its address could be anywhere...
	 */
	left[right - string] = '\0';

	return left;
}

static char *
stringAtRight(NoisyConfigState *  N, char *  string, char character)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerStringAtRight);

	if (string == NULL)
	{
		return string;
	}

	char *	right = strchr(string, character);

	if (right == NULL)
	{
		return NULL;
	}

	return &right[1];
}


static bool
isDecimalSeparatedWithChar(NoisyConfigState *  N, char *  string, char  character)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerIsDecimalSeparatedWithChar);

//fprintf(stderr, "isDecimalSeparatedWithChar, string = [%s]\n", string);

	if (string == NULL)
	{
		return false;
	}

	if (!strchr(string, character))
	{
		return false;
	}

	char *	left = stringAtLeft(N, string, character);
	char *	right = stringAtRight(N, string, character);
	bool	result = isDecimal(N, left) && isDecimal(N, right);

//fprintf(stderr, "isDecimalSeparatedWithChar. stringAtLeft(N, [%s], [%c]) = [%s]\n", string, character, left);
//fprintf(stderr, "isDecimalSeparatedWithChar. stringAtRight(N, [%s], [%c]) = [%s]\n", string, character, right);

	/*
	 *	stringAtLeft() makes a copy, which needs to be freed.
	 *	(stringAtRight on the other hand does not need to make
	 *	a copy, and doesn't).
	 */
	free(left);


	return result;
}

static bool
isDecimalOrRealSeparatedWithChar(NoisyConfigState *  N, char *  string, char  character)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerIsDecimalSeparatedWithChar);

	if (string == NULL)
	{
		return false;
	}

	if (!strchr(string, character))
	{
		return false;
	}

	char *	left = stringAtLeft(N, string, character);
	char *	right = stringAtRight(N, string, character);
	bool	result = (isDecimal(N, left)  || isRealConst(N, left))
			&&
			(isDecimal(N, right) || isRealConst(N, right));

//fprintf(stderr, "stringAtLeft(N, [%s], [%c]) = [%s]\n", string, character, left);
//fprintf(stderr, "stringAtRight(N, [%s], [%c]) = [%s]\n", string, character, right);

	/*
	 *	stringAtLeft() makes a copy, which needs to be freed.
	 *	(stringAtRight on the other hand does not need to make
	 *	a copy, and doesn't).
	 */
	free(left);


	return result;
}


static bool
isRadixConst(NoisyConfigState *  N, char *  string)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerIsRadixConst);

	if (string == NULL || !strchr(string, 'r'))
	{
		return false;
	}

//fprintf(stderr, "isRadixConst(N, %s) = %d\n", string, isDecimalSeparatedWithChar(N, string, 'r'));
	return isDecimalSeparatedWithChar(N, string, 'r');
}


static bool
isRealConst(NoisyConfigState *  N, char *  string)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerIsRealConst);

	if (string == NULL || !strchr(string, '.'))
	{
		return false;
	}

//fprintf(stderr, "isRealConst, string = [%s]\n", string);
//fprintf(stderr, "isRealConst(N, %s) = %d\n", string, isDecimalSeparatedWithChar(N, string, '.'));
	return isDecimalSeparatedWithChar(N, string, '.');
}


static bool
isEngineeringRealConst(NoisyConfigState *  N, char *  string)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerIsEngineeringRealConst);

	if (string == NULL || (!strchr(string, 'e') && !strchr(string, 'E')))
	{
		return false;
	}

//fprintf(stderr, "isEngineeringRealConst(N, %s)...\n", string);
	return (isDecimalOrRealSeparatedWithChar(N, string, 'e') || isDecimalOrRealSeparatedWithChar(N, string, 'E'));
}


static uint64_t
stringToRadixConst(NoisyConfigState *  N, char *  string)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerStringToRadixConst);

	char		tmp;
	char *		ep = &tmp;
	char *		left;
	char *		right;
	int		rightLength;
	uint64_t	base, value, p;


	left		= stringAtLeft(N, string, 'r');
	right		= stringAtRight(N, string, 'r');
	rightLength	= strlen(right);

	base = strtoul(left, &ep, 0);
	if (*ep != '\0')
	{
		/*
		 *	BUG/TODO: We should make sure that errorRecovery uses setjmp to eject
		 *	us out of here.
		 */
		noisyFatal(N, Esanity);

		/* Not reached */
	}

	value = 0;

	for (int i = 0; i < rightLength; i++)
	{
		char	digitChar;
		char	digitValue;

		if (i == 0)
		{
			p = 1;
		}
		else
		{
			p = base;
			for (int j = 0; j < i-1; j++)
			{
				p *= base;
			}
		}

		/*
		 *	NoisyConfig supports up to base 36 (e.g., 36rZZZ), which is the most
		 *	human friendly range. We could in principle support, e.g., base64,
		 *	but that would lead to value strings that would unecessarily
		 *	complicate the lexer and prser (e.g., "37r{{{").
		 */
		digitChar = right[rightLength - 1 - i];
		if (digitChar >= '0' && digitChar <= '9')
		{
			digitValue = digitChar - '0';
		}
		else if (digitChar >= 'A' && digitChar <= 'Z')
		{
			digitValue = digitChar - 'A' + 10;
		}
		else if (digitChar >= 'a' && digitChar <= 'z')
		{
			digitValue = digitChar - 'a' + 10;
		}
		else
		{
			noisyFatal(N, Esanity);
		}

		value += p * digitValue;
	}

	/*
	 *	stringAtLeft() makes a copy, which needs to be freed.
	 *	(stringAtRight on the other hand does not need to make
	 *	a copy, and doesn't).
	 */
	free(left);


	return value;
}


static double
stringToRealConst(NoisyConfigState *  N, char *  string)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerStringToRealConst);

	char		tmp;
	char *		ep = &tmp;
	char *		left;
	char *		right;
	int		rightLength;
	uint64_t	integerPart, fractionalPart;


	left		= stringAtLeft(N, string, '.');
	right		= stringAtRight(N, string, '.');
	if (right != NULL)
	{
		rightLength	= strlen(right);
	}

	integerPart = strtoul(left, &ep, 0);
	if (*ep != '\0')
	{
		noisyFatal(N, Esanity);

		/* Not reached */
	}

	
	if (right == NULL)
	{
		/*
		 *	stringAtLeft() makes a copy, which needs to be freed.
		 *	(stringAtRight on the other hand does not need to make
		 *	a copy, and doesn't).
		 */
		free(left);

		return (double)integerPart;
	}
	fractionalPart	= strtoul(right, &ep, 0);
	if (*ep != '\0')
	{
		noisyFatal(N, Esanity);

		/* Not reached */
	}

	/*
	 *	stringAtLeft() makes a copy, which needs to be freed.
	 *	(stringAtRight on the other hand does not need to make
	 *	a copy, and doesn't).
	 */
	free(left);


	return (double)integerPart + ((double)fractionalPart/pow(10.0, rightLength));
}


static double
stringToEngineeringRealConst(NoisyConfigState *  N, char *  string)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerStringToEngineeringRealConst);

	char		engineeringChar;
	char *		left;
	char *		right;
	double		mantissa, exponent;


	if (strchr(string, 'e'))
	{
		engineeringChar = 'e';
	}
	else
	{
		engineeringChar = 'E';
	}

	left		= stringAtLeft(N, string, engineeringChar);
	right		= stringAtRight(N, string, engineeringChar);

	mantissa 	= stringToRealConst(N, left);
	exponent 	= stringToRealConst(N, right);

	/*
	 *	stringAtLeft() makes a copy, which needs to be freed.
	 *	(stringAtRight on the other hand does not need to make
	 *	a copy, and doesn't).
	 */
	free(left);


	return (mantissa * pow(10.0, exponent));
}


static bool
isOperatorOrSeparator(NoisyConfigState *  N, char c)
{
	NoisyConfigTimeStampTraceMacro(kNoisyConfigTimeStampKeyLexerIsOperatorOrSeparator);

	/*
	 *	Unlike in our Yacc-driven compielers, we don't use a "stickies" array
	 */
	switch (c)
	{
		case '~':
		case '!':
		case '%':
		case '^':
		case '&':
		case '*':
		case '(':
		case ')':
		case '-':
		case '+':
		case '=':
		case '/':
		case '>':
		case '<':
		case ';':
		case ':':
		case '\'':
		case '\"':
		case '{':
		case '}':
		case '[':
		case ']':
		case '|':
		case ',':
		case '.':
		case ' ':
		case '\n':
		case '\r':
		case '\t':
		case '#':
		{
			return true;
		}
	}

	return false;
}
