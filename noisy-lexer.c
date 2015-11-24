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
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

#include <math.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisy-errors.h"
#include "version.h"
#include "noisy-timeStamps.h"
#include "noisy.h"
#include "noisy-parser.h"
#include "noisy-lexer.h"


extern const char *	gTerminalStrings[];
extern const char *	gReservedTokenDescriptions[];


static inline void	checkTokenLength(NoisyState *  N, int  count);
static inline char	cur(NoisyState *  N);
static void		gobble(NoisyState *  N, int count);
static void		done(NoisyState *  N, NoisyToken *  newToken);
static bool		eqf(NoisyState *  N);
static void		checkComment(NoisyState *  N);
static void		checkWeq(NoisyState *  N, NoisyIrNodeType type1, NoisyIrNodeType type2);
static void		checkWeq3(NoisyState *  N, NoisyIrNodeType type1, NoisyIrNodeType type2, char char2, NoisyIrNodeType type3);
static void		checkSingle(NoisyState *  N, NoisyIrNodeType tokenType);
static void		checkDot(NoisyState *  N);
static void		checkGt(NoisyState *  N);
static void		checkLt(NoisyState *  N);
static void		checkSingleQuote(NoisyState *  N);
static void		checkDoubleQuote(NoisyState *  N);
static void		checkMinus(NoisyState *  N);
static void		finishToken(NoisyState *  N);
static void		makeNumericConst(NoisyState *  N);
static bool		isDecimal(NoisyState *  N, char *  string);
static char *		stringAtLeft(NoisyState *  N, char *  string, char  character);
static char *		stringAtRight(NoisyState *  N, char *  string, char  character);
static bool		isDecimalSeparatedWithChar(NoisyState *  N, char *  string, char  character);
static bool		isDecimalOrRealSeparatedWithChar(NoisyState *  N, char *  string, char  character);
static bool		isRadixConst(NoisyState *  N, char *  string);
static bool		isRealConst(NoisyState *  N, char *  string);
static bool		isEngineeringRealConst(NoisyState *  N, char *  string);
static uint64_t		stringToRadixConst(NoisyState *  N, char *  string);
static double		stringToRealConst(NoisyState *  N, char *  string);
static double		stringToEngineeringRealConst(NoisyState *  N, char *  string);
static bool		isOperatorOrSeparator(NoisyState *  N, char c);




NoisySourceInfo *
noisyLexAllocateSourceInfo(	NoisyState *  N, char **  genealogy, char *  fileName,
				uint64_t lineNumber, uint64_t columnNumber, uint64_t length)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexAllocateSourceInfo);

	NoisySourceInfo *	newSourceInfo;

	newSourceInfo = (NoisySourceInfo *) calloc(1, sizeof(NoisySourceInfo));
	if (newSourceInfo == NULL)
	{
		noisyFatal(N, Emalloc);
	}

	newSourceInfo->genealogy	= genealogy;
	newSourceInfo->fileName		= (fileName == NULL ? NULL : strdup(fileName));
	newSourceInfo->lineNumber	= lineNumber;
	newSourceInfo->columnNumber	= columnNumber;
	newSourceInfo->length		= length;

	return newSourceInfo;
}


NoisyToken *
noisyLexAllocateToken(	NoisyState *  N, NoisyIrNodeType type, char *  identifier,
			uint64_t integerConst, double realConst, char * stringConst,
			NoisySourceInfo *  sourceInfo)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexAllocateToken);

	NoisyToken *	newToken;

	newToken = (NoisyToken *) calloc(1, sizeof(NoisyToken));
	if (newToken == NULL)
	{
		noisyFatal(N, Emalloc);
	}
	
	newToken->type		= type;
	newToken->identifier	= (identifier == NULL ? NULL : strdup(identifier));
	newToken->integerConst	= integerConst;
	newToken->realConst	= realConst;
	newToken->stringConst	= (stringConst == NULL ? NULL : strdup(stringConst));
	newToken->sourceInfo	= sourceInfo;

	return newToken;
}



void
noisyLexPut(NoisyState *  N, NoisyToken *  newToken)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexPut);

	if (newToken == NULL)
	{
		noisyFatal(N, Esanity);
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


NoisyToken *
noisyLexGet(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexGet);

	if (N->tokenList == NULL)
	{
		noisyFatal(N, Esanity);
	}

	NoisyToken *	t = N->tokenList;
	
	if (t->next != NULL)
	{
		N->tokenList = N->tokenList->next;
	}
	else if (t->type != kNoisyIrNodeType_Zeof)
	{
		noisyFatal(N, Esanity);
	}

	if (N->verbosityLevel & kNoisyVerbosityDebugLexer)
	{
		noisyLexPrintToken(N, t);	
	}

	return t;
}


NoisyToken *
noisyLexPeek(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexPeek);

	if (N->tokenList == NULL)
	{
		noisyFatal(N, Esanity);
	}

	return N->tokenList;
}


void
noisyLexInit(NoisyState *  N, char *  fileName)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexInit);

	FILE *			filePointer;
	size_t			lineBufferSize;


	N->fileName 		= fileName;
	N->columnNumber		= 0;
	N->lineNumber		= 0;
	N->lineLength		= 0;


	filePointer = fopen(fileName, "r");
	if (filePointer == NULL)
	{
		noisyFatal(N, Eopen);
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

			if (N->verbosityLevel & kNoisyVerbosityDebugLexer)
			{
				//flexprint(N->Fe, N->Fm, N->Fperr, "N->lineBuffer[%llu] = [%c]\n",
				//		N->columnNumber, N->lineBuffer[N->columnNumber]);
//fprintf(stderr, "N->lineBuffer[%llu] = [%c]\n", N->columnNumber, N->lineBuffer[N->columnNumber]);
			}

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
						checkWeq3(N, kNoisyIrNodeType_TandAs, kNoisyIrNodeType_Tand, '&', kNoisyIrNodeType_Tampersand);
						continue;
					}

					case '|':
					{
						checkWeq3(N, kNoisyIrNodeType_TorAs, kNoisyIrNodeType_Tor, '|', kNoisyIrNodeType_Tstroke);
						continue;
					}

					case ':':
					{
						checkWeq3(N, kNoisyIrNodeType_TdefineAs, kNoisyIrNodeType_Tcons, ':', kNoisyIrNodeType_Tcolon);
						continue;
					}

					case '=':
					{
						checkWeq3(N, kNoisyIrNodeType_Teq, kNoisyIrNodeType_Tgoes, '>', kNoisyIrNodeType_Tas);
						continue;
					}

					case '+':
					{
						checkWeq3(N, kNoisyIrNodeType_TaddAs, kNoisyIrNodeType_Tinc, '+', kNoisyIrNodeType_Tplus);
						continue;
					}


					/*
					 *	These tokens may be paired with an equals sign.
					 *
					 *	We process the chars see so far as a finished token, then handle the following chars.
					 */
					case '!':
					{
						checkWeq(N, kNoisyIrNodeType_Tneq, kNoisyIrNodeType_Tbang);
						continue;
					}

					case '%':
					{
						checkWeq(N, kNoisyIrNodeType_TmodAs, kNoisyIrNodeType_Tpercent);
						continue;
					}

					case '^':
					{
						checkWeq(N, kNoisyIrNodeType_TxorAs, kNoisyIrNodeType_Tcaret);
						continue;
					}

					case '*':
					{
						checkWeq(N, kNoisyIrNodeType_TmulAs, kNoisyIrNodeType_Tasterisk);
						continue;
					}

					case '/':
					{
						checkWeq(N, kNoisyIrNodeType_TdivAs, kNoisyIrNodeType_Tdiv);
						continue;
					}


					/*
					 *	These tokens only occur alone.
					 *
					 *	We process the chars see so far as a finished token, then handle the following chars.
					 */
					case '~':
					{
						checkSingle(N, kNoisyIrNodeType_Ttilde);
						continue;
					}

					case '(':
					{
						checkSingle(N, kNoisyIrNodeType_TleftParen);
						continue;
					}

					case ')':
					{
						checkSingle(N, kNoisyIrNodeType_TrightParen);
						continue;
					}

					case ';':
					{
						checkSingle(N, kNoisyIrNodeType_Tsemicolon);
						continue;
					}

					case '{':
					{
						checkSingle(N, kNoisyIrNodeType_TleftBrace);
						continue;
					}

					case '}':
					{
						checkSingle(N, kNoisyIrNodeType_TrightBrace);
						continue;
					}

					case '[':
					{
						checkSingle(N, kNoisyIrNodeType_TleftBrac);
						continue;
					}

					case ']':
					{
						checkSingle(N, kNoisyIrNodeType_TrightBrac);
						continue;
					}

					case ',':
					{
						/*
						 *	TODO/BUG: Is this right? Re-check. What about kNoisyIrNodeType_Tdot?
						 */
						checkSingle(N, kNoisyIrNodeType_Tcomma);
						continue;
					}


					/*
					 *	These tokens require special handling beyond being paired with an equals,
					 *	being part of a number, or doubled-up (e.g., ">>", etc.).
					 */
					case '.':
					{
						checkDot(N);
						continue;
					}

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
						noisyFatal(N, Esanity);
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

	NoisySourceInfo *	eofSourceInfo = noisyLexAllocateSourceInfo(N,	NULL /* genealogy */,
										N->fileName /* fileName */,
										N->lineNumber /* lineNumber */,
										N->columnNumber /* columnNumber */,
										0 /* length */);
										
	NoisyToken *		eofToken = noisyLexAllocateToken(N,	kNoisyIrNodeType_Zeof /* type */,
									NULL /* identifier */,
									0 /* integerConst */,
									0.0 /* realConst */,
									NULL /* stringConst */,
									eofSourceInfo /* sourceInfo */);
	noisyLexPut(N, eofToken);

	if (N->verbosityLevel & kNoisyVerbosityDebugLexer)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Done lexing...\n");
		
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\n");
		NoisyToken *	p = N->tokenList;
		while (p != NULL)
		{
			noisyLexPrintToken(N, p);
			p = p->next;
		}
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\n");
	}


	return;
}


void
noisyLexPrintToken(NoisyState *  N, NoisyToken *  t)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexPrintToken);

	flexprint(N->Fe, N->Fm, N->Fperr, "Token %30s: ", gTerminalStrings[t->type]);

	switch (t->type)
	{
		case kNoisyIrNodeType_Tidentifier:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%20s\", ", t->identifier);
			break;
		}

		case kNoisyIrNodeType_TintConst:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%20d\", ", t->integerConst);
			break;
		}

		case kNoisyIrNodeType_TrealConst:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%20f\", ", t->realConst);
			break;
		}

		case kNoisyIrNodeType_TstringConst:
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
				flexprint(N->Fe, N->Fm, N->Fperr, ">>>BUG: unhandled type [%d] in noisyLexPrintToken <<<", t->type);
				//noisyFatal(N, Esanity);
			}
		}
	}

	flexprint(N->Fe, N->Fm, N->Fperr, "source file: %16s, line %3d, pos %3d, length %3d\n",
		t->sourceInfo->fileName, t->sourceInfo->lineNumber, t->sourceInfo->columnNumber, t->sourceInfo->length);
}



/*
 *	Local non-exposed routines.
 */



static inline void
checkTokenLength(NoisyState *  N, int  count)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCheckTokenLength);

	if (N->currentTokenLength+count >= kNoisyMaxBufferLength)
	{
		noisyFatal(N, EtokenTooLong);
	}
}

static inline char
cur(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCur);

	return N->lineBuffer[N->columnNumber];
}


static void
gobble(NoisyState *  N, int count)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerGobble);

	checkTokenLength(N, count);
	strncpy(N->currentToken, &N->lineBuffer[N->columnNumber], count);

	if (N->verbosityLevel & kNoisyVerbosityDebugLexer)
	{
		//flexprint(N->Fe, N->Fm, N->Fperr, "gobble, N->currentToken = \"%s\"\n", N->currentToken);
//fprintf(stderr, "gobble, N->currentToken = \"%s\"\n", N->currentToken);
	}

	N->columnNumber += count;
}


static void
done(NoisyState *  N, NoisyToken *  newToken)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerDone);

	newToken->sourceInfo = noisyLexAllocateSourceInfo(N,	NULL				/*   genealogy 	*/,
								N->fileName			/*   fileName 	*/,
								N->lineNumber			/*   lineNumber */,
								N->columnNumber - N->currentTokenLength /* columnNumber */,
								N->currentTokenLength		/*   length 	*/);

	bzero(N->currentToken, kNoisyMaxBufferLength);
	N->currentTokenLength = 0;
	noisyLexPut(N, newToken);
}


static bool
eqf(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerEqf);

	return (N->lineLength >= 2 && N->lineBuffer[N->columnNumber+1] == '=');
}


static void
checkComment(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCheckComment);

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
checkWeq(NoisyState *  N, NoisyIrNodeType type1, NoisyIrNodeType type2)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCheckWeq);

	NoisyIrNodeType		type;

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

	NoisyToken *		newToken = noisyLexAllocateToken(N,	type	/* type		*/,
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
checkWeq3(NoisyState *  N, NoisyIrNodeType type1, NoisyIrNodeType type2, char char2, NoisyIrNodeType type3)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCheckWeq3);

	NoisyIrNodeType		type;

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

	NoisyToken *		newToken = noisyLexAllocateToken(N,	type	/* type		*/,
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
checkSingle(NoisyState *  N, NoisyIrNodeType tokenType)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCheckSingle);

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	gobble(N, 1);

	if (N->verbosityLevel & kNoisyVerbosityDebugLexer)
	{
		//flexprint(N->Fe, N->Fm, N->Fperr, "checkSingle(), tokenType = %d\n", tokenType);
//fprintf(stderr, "checkSingle(), tokenType = %d\n", tokenType);
	}

	NoisyToken *		newToken = noisyLexAllocateToken(N,	tokenType /* type	*/,
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
checkDot(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCheckDot);

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

	NoisyToken *		newToken = noisyLexAllocateToken(N,	kNoisyIrNodeType_Tdot /* type	*/,
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
checkGt(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCheckGt);

	NoisyIrNodeType		type;

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	if (eqf(N))
	{
		gobble(N, 2);
		type = kNoisyIrNodeType_Tge;
	}
	else if (N->lineLength >= 3 &&
		N->lineBuffer[N->columnNumber+1] == '>' &&
		N->lineBuffer[N->columnNumber+2] == '=')
	{
		gobble(N, 3);
		type = kNoisyIrNodeType_TrightShiftAs;
	}
	else if (N->lineLength >= 2 &&
		N->lineBuffer[N->columnNumber+1] == '>')
	{
		gobble(N, 2);
		type = kNoisyIrNodeType_TrightShift;
	}
	else
	{
		gobble(N, 1);
		type = kNoisyIrNodeType_Tgt;
	}

	NoisyToken *		newToken = noisyLexAllocateToken(N,	type	/* type		*/,
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
checkLt(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCheckLt);

	NoisyIrNodeType		type;

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	if (eqf(N))
	{
		gobble(N, 2);
		type = kNoisyIrNodeType_Tle;
	}
	else if (N->lineLength >= 3 &&
		N->lineBuffer[N->columnNumber+1] == '<' &&
		N->lineBuffer[N->columnNumber+2] == '=')
	{
		gobble(N, 3);
		type = kNoisyIrNodeType_TleftShiftAs;
	}
	else if (N->lineLength >= 3 &&
		N->lineBuffer[N->columnNumber+1] == '-' &&
		N->lineBuffer[N->columnNumber+2] == '=')
	{
		gobble(N, 3);
		type = kNoisyIrNodeType_TchanWrite;
	}
	else if (N->lineLength >= 2 &&
		N->lineBuffer[N->columnNumber+1] == '<')
	{
		gobble(N, 2);
		type = kNoisyIrNodeType_TleftShift;
	}
	else if (N->lineLength >= 2 &&
		N->lineBuffer[N->columnNumber+1] == '-')
	{
		gobble(N, 2);
		type = kNoisyIrNodeType_Tgets;
	}
	else
	{
		gobble(N, 1);
		type = kNoisyIrNodeType_Tlt;
	}

	NoisyToken *		newToken = noisyLexAllocateToken(N,	type	/* type		*/,
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
checkSingleQuote(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCheckSingleQuote);

	/*
	 *	TODO/BUG: we do not handle escaped squotes in a charconst
	 */
	NoisyIrNodeType		type;
	char			quotedChar;


	/*
	 *	Gobble any extant chars
	 */
	finishToken(N);


	if ((N->lineLength >= 3) && (N->lineBuffer[N->columnNumber+2] == '\''))
	{
		type = kNoisyIrNodeType_TcharConst;
		quotedChar = N->lineBuffer[N->columnNumber+1];
		gobble(N, 3);
	}
	else
	{
		/*
		 *	BUG/TODO: This could be improved:
		 *
		 *	If we see >1 char in-between single quotes, create a
		 *	kNoisyIrNodeType_ZbadCharConst token using the first
		 *	character after the quote as the token character constant.
		 */
		type = kNoisyIrNodeType_ZbadCharConst;
		quotedChar = N->lineBuffer[N->columnNumber+1];
		gobble(N, 1);
	}

	NoisyToken *		newToken = noisyLexAllocateToken(N,	type	/* type		*/,
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
checkDoubleQuote(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCheckDoubleQuote);

	/*
	 *	TODO/BUG: we do not handle escaped dquotes in a strconst
	 */
	NoisyToken *	newToken;


	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	/*
	 *	String constants cannot contain an un-escaped newline;  the current
	 *	N->lineBuffer must contain the closing quote, else we flag this as a
	 *	bad string constant (kNoisyIrNodeType_ZbadStringConst)
	 */
	if (strchr(&N->lineBuffer[N->columnNumber+1], '\"') == NULL)
	{
		newToken = noisyLexAllocateToken(N,	kNoisyIrNodeType_ZbadStringConst	/* type		*/,
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
		 *	NOTE: N->currentToken is pre-allocated to be kNoisyMaxBufferLength characters long.
		 */
		while ((cur(N) != '\"') && N->currentTokenLength < (kNoisyMaxBufferLength - 1) && (N->columnNumber < N->lineLength))
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

		newToken = noisyLexAllocateToken(N,	kNoisyIrNodeType_TstringConst	/* type		*/,
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
checkMinus(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCheckMinus);

	NoisyIrNodeType		type;

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	if (eqf(N))
	{
		gobble(N, 2);
		type = kNoisyIrNodeType_TsubAs;
	}
	else if (N->lineLength >= 2 && N->lineBuffer[N->columnNumber+1] == '-')
	{
		gobble(N, 2);
		type = kNoisyIrNodeType_Tdec;
	}
	else if (N->lineLength >= 2 && N->lineBuffer[N->columnNumber+1] == '>')
	{
		gobble(N, 2);
		type = kNoisyIrNodeType_TprogtypeQualifier;
	}
	else
	{
		gobble(N, 1);
		type = kNoisyIrNodeType_Tminus;
	}

	NoisyToken *		newToken = noisyLexAllocateToken(N,	type	/* type		*/,
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
finishToken(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerFinishToken);

	if (N->verbosityLevel & kNoisyVerbosityDebugLexer)
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
		NoisyToken *	newToken = noisyLexAllocateToken(N,	kNoisyIrNodeType_TboolConst	/*	type		*/,
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

	for (int i = 0; i < kNoisyIrNodeTypeMax; i++)
	{
		if ((gReservedTokenDescriptions[i] != NULL) && !strcmp(gReservedTokenDescriptions[i], N->currentToken))
		{
			NoisyToken *	newToken = noisyLexAllocateToken(N,	i	/* type		*/,
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
	NoisyToken *	newToken = noisyLexAllocateToken(N,	kNoisyIrNodeType_Tidentifier	/* type		*/,
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
makeNumericConst(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerMakeNumericConst);

//fprintf(stderr, "in makeNumericConst(), N->currentToken = [%s]\n", N->currentToken);

	if (N->currentTokenLength == 0)
	{
		noisyFatal(N, EruntTokenInNumericConst);
	}

	/*
	 *	If it begins with a onenine or a zeronine (see regular grammar),
	 *	check if it's an intconst or realconst tokens which begin with 
	 *	numbers but do not make valid consts, make a "illegal identifier"
	 *	token kNoisyIrNodeType_ZbadIdentifier
	 */

	/*
	 *	There are only two valid cases with leading zero: "0" and "0.XXX".
	 *	You cannot have 0XXX or even 0rXXX
	 */
	if (N->currentToken[0] == '0')
	{
		if (N->currentTokenLength == 1)
		{
			NoisyToken *	newToken = noisyLexAllocateToken(N,	kNoisyIrNodeType_TintConst	/* type		*/,
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
			NoisyToken *	newToken = noisyLexAllocateToken(N,	kNoisyIrNodeType_ZbadIdentifier	/* type		*/,
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
		NoisyToken *	newToken = noisyLexAllocateToken(N,	kNoisyIrNodeType_TrealConst	/* type		*/,
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
		NoisyToken *	newToken = noisyLexAllocateToken(N,	kNoisyIrNodeType_TrealConst	/* type		*/,
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
		NoisyToken *	newToken = noisyLexAllocateToken(N,	kNoisyIrNodeType_TintConst	/* type		*/,
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
		NoisyToken *	newToken = noisyLexAllocateToken(N,	kNoisyIrNodeType_ZbadIdentifier	/* type		*/,
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
		NoisyToken *	newToken = noisyLexAllocateToken(N,	kNoisyIrNodeType_TintConst	/* type		*/,
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
isDecimal(NoisyState *  N, char *  string)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerIsDecimal);

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
stringAtLeft(NoisyState *  N, char *  string, char character)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerStringAtLeft);

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
stringAtRight(NoisyState *  N, char *  string, char character)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerStringAtRight);

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
isDecimalSeparatedWithChar(NoisyState *  N, char *  string, char  character)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerIsDecimalSeparatedWithChar);

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
isDecimalOrRealSeparatedWithChar(NoisyState *  N, char *  string, char  character)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerIsDecimalSeparatedWithChar);

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
isRadixConst(NoisyState *  N, char *  string)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerIsRadixConst);

	if (string == NULL || !strchr(string, 'r'))
	{
		return false;
	}

//fprintf(stderr, "isRadixConst(N, %s) = %d\n", string, isDecimalSeparatedWithChar(N, string, 'r'));
	return isDecimalSeparatedWithChar(N, string, 'r');
}


static bool
isRealConst(NoisyState *  N, char *  string)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerIsRealConst);

	if (string == NULL || !strchr(string, '.'))
	{
		return false;
	}

//fprintf(stderr, "isRealConst, string = [%s]\n", string);
//fprintf(stderr, "isRealConst(N, %s) = %d\n", string, isDecimalSeparatedWithChar(N, string, '.'));
	return isDecimalSeparatedWithChar(N, string, '.');
}


static bool
isEngineeringRealConst(NoisyState *  N, char *  string)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerIsEngineeringRealConst);

	if (string == NULL || (!strchr(string, 'e') && !strchr(string, 'E')))
	{
		return false;
	}

//fprintf(stderr, "isEngineeringRealConst(N, %s)...\n", string);
	return (isDecimalOrRealSeparatedWithChar(N, string, 'e') || isDecimalOrRealSeparatedWithChar(N, string, 'E'));
}


static uint64_t
stringToRadixConst(NoisyState *  N, char *  string)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerStringToRadixConst);

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
		noisyParserSyntaxError(N, kNoisyIrNodeType_PintConst);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PintConst);

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
		 *	Noisy supports up to base 36 (e.g., 36rZZZ), which is the most
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
			noisyParserSyntaxError(N, kNoisyIrNodeType_PintConst);
			noisyParserErrorRecovery(N, kNoisyIrNodeType_PintConst);
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
stringToRealConst(NoisyState *  N, char *  string)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerStringToRealConst);

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
		/*
		 *	BUG/TODO: We should make sure that errorRecovery uses setjmp to eject
		 *	us out of here.
		 */
		noisyParserSyntaxError(N, kNoisyIrNodeType_PrealConst);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PrealConst);

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
		/*
		 *	BUG/TODO: We should make sure that errorRecovery uses setjmp to eject
		 *	us out of here.
		 */
		noisyParserSyntaxError(N, kNoisyIrNodeType_PrealConst);
		noisyParserErrorRecovery(N, kNoisyIrNodeType_PrealConst);

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
stringToEngineeringRealConst(NoisyState *  N, char *  string)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerStringToEngineeringRealConst);

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
isOperatorOrSeparator(NoisyState *  N, char c)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerIsOperatorOrSeparator);

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
