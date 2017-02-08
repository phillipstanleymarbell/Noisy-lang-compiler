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
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisy-errors.h"
#include "version.h"
#include "noisy-timeStamps.h"
#include "noisy.h"
#include "noisy-parser.h"
#include "noisy-lexers-helpers.h"
#include "noisy-lexer.h"


extern const char *	gTerminalStrings[];
extern const char *	gReservedTokenDescriptions[];


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



void
noisyLexInit(NoisyState *  N, char *  fileName)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexInit);

	FILE *			filePointer;
	size_t			lineBufferSize;


	N->fileName 		= fileName;
	N->columnNumber		= 1;
	N->lineNumber		= 1;
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
			noisyLexDebugPrintToken(N, p, gReservedTokenDescriptions);
			p = p->next;
		}
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\n");
	}


	return;
}







/*
 *	Local non-exposed routines.
 */










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


void
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

bool
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

void
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
