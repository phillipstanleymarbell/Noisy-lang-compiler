#include <math.h>
#include <stdio.h>
#include <stdlib.h>
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
#include "noisyconfig.h"
#include "noisyconfig-parser.h"
#include "noisyconfig-lexer.h"


extern const char *	gReservedTokenDescriptions[];


static inline void	checkTokenLength(NoisyConfigState *  N, int  count);
static inline char	cur(NoisyConfigState *  N);
static void		gobble(NoisyConfigState *  N, int count);
static void		done(NoisyConfigState *  N, NoisyConfigToken *  newToken);
static void		checkComment(NoisyConfigState *  N);
static void		checkSingle(NoisyConfigState *  N, NoisyConfigIrNodeType tokenType);
static void		checkDoubleQuote(NoisyConfigState *  N);
static void		checkMinus(NoisyConfigState *  N);
static void		finishToken(NoisyConfigState *  N);
static bool		isOperatorOrSeparator(NoisyConfigState *  N, char c);




NoisyConfigSourceInfo *
noisyConfigLexAllocateSourceInfo(	NoisyConfigState *  N, char **  genealogy, char *  fileName,
				uint64_t lineNumber, uint64_t columnNumber, uint64_t length)
{
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
    uint64_t integerConst,
    double realConst, 
    char * stringConst,
	NoisyConfigSourceInfo *  sourceInfo
) {
	NoisyConfigToken *	newToken;

	newToken = (NoisyConfigToken *) calloc(1, sizeof(NoisyConfigToken));
	if (newToken == NULL)
	{
		noisyConfigFatal(N, Emalloc);
	}
	
	newToken->type		= type;
	newToken->identifier	= (identifier == NULL ? NULL : strdup(identifier));
	newToken->realConst	= realConst;
	newToken->integerConst= integerConst;
	newToken->stringConst	= (stringConst == NULL ? NULL : strdup(stringConst));
	newToken->sourceInfo	= sourceInfo;

	return newToken;
}



void
noisyConfigLexPut(NoisyConfigState *  N, NoisyConfigToken *  newToken)
{
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
noisyConfigLexGet(NoisyConfigState *  N)
{
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
						flexprint(N->Fe, N->Fm, N->Fperr, ">>>BUG/TODO: un-handled type %s in noisyConfigLexPeekPrint <<<", gReservedTokenDescriptions[tmp->type]);
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
                  // I DONT HAVE ANY OF THESE


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
					
                    case '{':
					{
						checkSingle(N, kNoisyConfigIrNodeType_TleftBrace);
						continue;
					}

					case '}':
					{
						checkSingle(N, kNoisyConfigIrNodeType_TrightBrace);
						continue;
					}
					
                    case ';':
					{
						checkSingle(N, kNoisyConfigIrNodeType_Tsemicolon);
						continue;
					}
                    
                    case '=':
					{
						/*
						 *	TODO/BUG: Is this right? Re-check. What about kNoisyIrNodeType_Tdot?
						 */
						checkSingle(N, kNoisyConfigIrNodeType_Tequals);
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
					case '"':
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
	                    flexprint(N->Fe, N->Fm, N->Fperr, "hello %c", cur(N));
                        noisyConfigConsolePrintBuffers(N);
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
	switch (t->type)
	{
        // TODO handle all the cases
		case kNoisyConfigIrNodeType_Tidentifier:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%20s\", ", t->identifier);
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
				//noisyConfigFatal(N, Esanity);
			}
		}
	}

	flexprint(N->Fe, N->Fm, N->Fperr, "source file: %16s, line %3d, pos %3d, length %3d\n",
		t->sourceInfo->fileName, t->sourceInfo->lineNumber, t->sourceInfo->columnNumber, t->sourceInfo->length);
}


void
noisyConfigLexPrintToken(NoisyConfigState *  N, NoisyConfigToken *  t)
{
	switch (t->type)
	{
		case kNoisyConfigIrNodeType_Tidentifier:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%s\"", t->identifier);
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
				//noisyConfigFatal(N, Esanity);
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
	if (N->currentTokenLength+count >= kNoisyConfigMaxBufferLength)
	{
		noisyConfigFatal(N, EtokenTooLong);
	}
}

static inline char
cur(NoisyConfigState *  N)
{
	return N->lineBuffer[N->columnNumber];
}


static void
gobble(NoisyConfigState *  N, int count)
{
	checkTokenLength(N, count);
	strncpy(N->currentToken, &N->lineBuffer[N->columnNumber], count);

	N->columnNumber += count;
}


static void
done(NoisyConfigState *  N, NoisyConfigToken *  newToken)
{
	newToken->sourceInfo = noisyConfigLexAllocateSourceInfo(N,	NULL				/*   genealogy 	*/,
								N->fileName			/*   fileName 	*/,
								N->lineNumber			/*   lineNumber */,
								N->columnNumber - N->currentTokenLength /* columnNumber */,
								N->currentTokenLength		/*   length 	*/);

	bzero(N->currentToken, kNoisyConfigMaxBufferLength);
	N->currentTokenLength = 0;
	noisyConfigLexPut(N, newToken);
}


static void
checkComment(NoisyConfigState *  N)
{
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
checkSingle(NoisyConfigState *  N, NoisyConfigIrNodeType tokenType)
{
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

	NoisyConfigToken *		newToken = noisyConfigLexAllocateToken(N,	tokenType /* type	*/,
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
checkDoubleQuote(NoisyConfigState *  N)
{
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
	if (strchr(&N->lineBuffer[N->columnNumber+1], '"') == NULL)
	{
		newToken = noisyConfigLexAllocateToken(N,	kNoisyConfigIrNodeType_ZbadStringConst	/* type		*/,
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
		while ((cur(N) != '"') && N->currentTokenLength < (kNoisyConfigMaxBufferLength - 1) && (N->columnNumber < N->lineLength))
		{
			checkTokenLength(N, 1);
			N->currentToken[N->currentTokenLength++] = N->lineBuffer[N->columnNumber++];
		}

		checkTokenLength(N, 1);
		N->currentToken[N->currentTokenLength++] = '\0';

		if (cur(N) != '"')
		{
			/*
			 *	We ran out of buffer space or reached end of lineBuffer
			 */
			noisyConfigFatal(N, EstringTooLongOrWithNewline);
		}
		else
		{
			/*
			 *	Move past the closing quote
			 */
			N->columnNumber++;
		}

		newToken = noisyConfigLexAllocateToken(N,	kNoisyConfigIrNodeType_TstringConst	/* type		*/,
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
	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

    gobble(N, 1);
    NoisyConfigIrNodeType type = kNoisyConfigIrNodeType_Tminus;

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
finishToken(NoisyConfigState *  N)
{
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

	for (int i = 0; i < kNoisyConfigIrNodeTypeMax; i++)
	{
		if ((gReservedTokenDescriptions[i] != NULL) && !strcmp(gReservedTokenDescriptions[i], N->currentToken))
		{
			NoisyConfigToken *	newToken = noisyConfigLexAllocateToken(N,	i	/* type		*/,
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
        // noisyconfig-lexer.c: finishToken: we don't use numbers in the config file. amirite
        noisyConfigFatal(N, Esanity);
		// makeNumericConst(N);
		return;
	}

	/*
	 *	Otherwise, it's an identifier.  It will not have illegal chars in it
	 *	since we would have halted the building of the token on seing them
	 *	and gotten called here.
	 */
	NoisyConfigToken *	newToken = noisyConfigLexAllocateToken(N,	kNoisyConfigIrNodeType_Tidentifier	/* type		*/,
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


static bool
isOperatorOrSeparator(NoisyConfigState *  N, char c)
{
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
