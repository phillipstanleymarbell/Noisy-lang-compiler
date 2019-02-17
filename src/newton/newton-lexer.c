/*
	Authored 2017. Jonathan Lim. Modified 2018, Phillip Stanley-Marbell.

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
#include <stdlib.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "version.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "newton-parser.h"
#include "common-lexers-helpers.h"
#include "newton-lexer.h"


extern const char *	gNewtonTokenDescriptions[];


static void		checkComment(State *  N);
static void		checkSingle(State *  N, IrNodeType tokenType);
static void		checkDoubleQuote(State *  N, bool callFinishTokenFlag);
static void		finishToken(State *  N);
static void		checkEqual(State *  N);
static void		checkGt(State *  N);
static void		checkLt(State *  N);
static void		checkMul(State * N);
static void		checkProportionality(State * N);
static void		checkDot(State *  N);
static void		checkPlusMinus(State *  N, IrNodeType plusOrMinusTokenType);
static void		makeNumericConst(State *  N);
static bool		isOperatorOrSeparator(State *  N, char c);
static void		newtonLex(State *  N, char *  fileName);

bool			gMakeNextTokenNegative = false;


void
newtonLexInit(State *  N, char *  fileName)
{
	N->fileName 		= fileName;
	N->columnNumber		= 1;
	N->lineNumber		= 1;
	N->lineLength		= 0;

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

	newtonLex(N, fileName);

	SourceInfo *	eofSourceInfo = lexAllocateSourceInfo(N,	NULL		/* genealogy	*/,
									N->fileName	/* fileName	*/,
									N->lineNumber	/* lineNumber	*/,
									N->columnNumber	/* columnNumber	*/,
									0		/* length	*/);
	 								
	Token *		eofToken = lexAllocateToken(N,	kNewtonIrNodeType_Zeof		/* type		*/,
									NULL		/* identifier	*/,
									0		/* integerConst	*/,
									0.0		/* realConst	*/,
									NULL		/* stringConst	*/,
									eofSourceInfo	/* sourceInfo	*/);
	lexPut(N, eofToken);

	if (N->verbosityLevel & kNoisyVerbosityDebugLexer)
	{
		Token *	p = N->tokenList;
		while (p != NULL)
		{
			lexDebugPrintToken(N, p, gNewtonTokenDescriptions);
			p = p->next;
		}
	}
}

static void
newtonLex(State *  N, char *  fileName)
{
	size_t			lineBufferSize;

	N->filePointer = fopen(fileName, "r");
	if (N->filePointer == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Could not open file \"%s\".\n", fileName);
		fatal(N, Eopen);
	}

	lineBufferSize = 0;
	while ((N->lineLength = getline(&(N->lineBuffer), &lineBufferSize, N->filePointer)) != -1)
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
					 *	We process the chars seen so far as a finished token, then handle the following chars.
					 */

					/*
					 *	These tokens only occur alone.
					 *
					 *	We process the chars see so far as a finished token, then handle the following chars.
					 */
					case '~':
					{
						checkSingle(N, kNewtonIrNodeType_TdimensionallyMatchingProportional);
						continue;
					}
					case '(':
					{
						checkSingle(N, kNewtonIrNodeType_TleftParen);
						continue;
					}

					case ')':
					{
						checkSingle(N, kNewtonIrNodeType_TrightParen);
						continue;
					}
					case '{':
					{
						checkSingle(N, kNewtonIrNodeType_TleftBrace);
						continue;
					}

					case '}':
					{
						checkSingle(N, kNewtonIrNodeType_TrightBrace);
						continue;
					}
					case '[':
					{
						checkSingle(N, kNewtonIrNodeType_TleftBracket);
						continue;
					}

					case ']':
					{
						checkSingle(N, kNewtonIrNodeType_TrightBracket);
						continue;
					}
					case '|':
					{
						checkSingle(N, kNewtonIrNodeType_TbitwiseOr);
						continue;
					}
					case '%':
					{
						checkSingle(N, kNewtonIrNodeType_Tpercent);
						continue;
					}
					case '/':
					{
						checkSingle(N, kNewtonIrNodeType_Tdiv);
						continue;
					}
					
					case ',':
					{
						checkSingle(N, kNewtonIrNodeType_Tcomma);
						continue;
					}
					case ';':
					{
						checkSingle(N, kNewtonIrNodeType_Tsemicolon);
						continue;
					}
					case ':':
					{
						checkSingle(N, kNewtonIrNodeType_Tcolon);
						continue;
					}
					case '@':
					{
						checkSingle(N, kNewtonIrNodeType_TatSign);
						continue;
					}
					case '.':
					{
						checkDot(N);
						continue;
					}

					/*
					 *	These tokens require special handling beyond being paired with an equals,
					 *	being part of a number, or doubled-up (e.g., ">>", etc.).
					 */
					case '+':
					{
						checkPlusMinus(N, kNewtonIrNodeType_Tplus);
						continue;
					}
					case '-':
					{
						checkPlusMinus(N, kNewtonIrNodeType_Tminus);
						continue;
					}
					case '*':
					{
						checkMul(N);
						continue;
					}
					case '=':
					{
						checkEqual(N);
						continue;
					}
					case 'o':
					{
						checkProportionality(N);
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
					case '"':
					{
						checkDoubleQuote(N, true /* callFinishTokenFlag */);
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
						consolePrintBuffers(N);
						fatal(N, Esanity);
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

	fclose(N->filePointer);

	return;
}



/*
 *	Local non-exposed routines.
 */


static void
checkComment(State *  N)
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
checkSingle(State *  N, IrNodeType tokenType)
{
	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	gobble(N, 1);

	if (N->verbosityLevel & kNoisyVerbosityDebugLexer)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "checkSingle(), tokenType = %d\n", tokenType);
	}

	Token *		newToken = lexAllocateToken(N,			tokenType /* type	*/,
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
checkDoubleQuote(State *  N, bool callFinishTokenFlag)
{
	/*
	 *	TODO/BUG: we do not handle escaped double quotes in a strconst
	 */
	Token *	newToken;

	/*
	 *	Gobble any extant chars. We do not call finishToken() when we
	 *	come here via finishToken() itself in the case of handling 
	 *	include statements.
	 */
	if (callFinishTokenFlag)
	{
		finishToken(N);
	}

	/*
	 *	String constants cannot contain an un-escaped newline;  the current
	 *	N->lineBuffer must contain the closing quote, else we flag this as a
	 *	bad string constant (kNewtonIrNodeType_ZbadStringConst)
	 */
	if (strchr(&N->lineBuffer[N->columnNumber+1], '"') == NULL)
	{
		newToken = lexAllocateToken(N,		kNewtonIrNodeType_ZbadStringConst	/* type		*/,
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
		while ((cur(N) != '"') && N->currentTokenLength < (kNoisyMaxBufferLength - 1) && (N->columnNumber < N->lineLength))
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
			fatal(N, EstringTooLongOrWithNewline);
		}
		else
		{
			/*
			 *	Move past the closing quote
			 */
			N->columnNumber++;
		}

		newToken = lexAllocateToken(N,		kNewtonIrNodeType_TstringConst	/* type		*/,
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
finishToken(State *  N)
{
	if (N->verbosityLevel & kNoisyVerbosityDebugLexer)
	{
		//flexprint(N->Fe, N->Fm, N->Fperr, "in finishToken(), N->currentToken = [%s]\n", N->currentToken);
		//fprintf(stderr, "in finishToken(), N->currentToken = [%s]\n", N->currentToken);
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

	for (int i = 0; i < kCommonIrNodeTypeMax; i++)
	{
		if (!strcmp("include", N->currentToken))
		{
			/*
			 *	Reset the index in the current token to place the filename string
			 *	over the "include" since we don't need to keep that. Then, call
			 *	checkDoubleQuote()
			 */
			N->currentTokenLength = 0;
			checkDoubleQuote(N, false /* callFinishTokenFlag */);

			/*
			 *	Since we don't call done() (which sets the N->currentTokenLength
			 *	to zero and bzero's the N->currentToken buffer), we need to do
			 *	this manually.
			 */
			bzero(N->currentToken, kNoisyMaxBufferLength);
			N->currentTokenLength = 0;

			char *	newFileName = strdup(N->lastToken->stringConst);
			if (!newFileName)
			{
				fatal(N, Emalloc);
			}

			Token *	tmp		= N->lastToken;
			N->lastToken		= N->lastToken->prev;
			if (N->lastToken == NULL)
			{
				N->tokenList = N->lastToken;
			}

			free(tmp->stringConst);
			free(tmp);

			char *	oldFileName	= N->fileName;
			int	oldColumnNumber	= N->columnNumber;
			int	oldLineNumber	= N->lineNumber;
			FILE *	oldFilePointer	= N->filePointer;

			N->fileName 		= newFileName;
			N->columnNumber		= 1;
			N->lineNumber		= 1;
			N->lineLength		= 0;
			N->lineBuffer		= NULL;
			newtonLex(N, newFileName);
			free(newFileName);

			N->fileName		= oldFileName;
			N->filePointer		= oldFilePointer;
			N->lineNumber		= oldLineNumber;

			/*
			 *	Set the columnNumber and lineLength to be same to force
			 *	ourselves to stop chomping on the same line.
			 */
			N->columnNumber		= oldColumnNumber;
			N->lineLength		= oldColumnNumber;

			return;
		}

		if ((gNewtonTokenDescriptions[i] != NULL) && !strcmp(gNewtonTokenDescriptions[i], N->currentToken))
		{
			Token *	newToken = lexAllocateToken(N,			i	/* type		*/,
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
	Token *	newToken = lexAllocateToken(N,			kNewtonIrNodeType_Tidentifier	/* type		*/,
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
makeNumericConst(State *  N)
{
	int	sign = 1;

	if (gMakeNextTokenNegative)
	{
		gMakeNextTokenNegative = false;
		sign = -1;
	}

	if (N->currentTokenLength == 0)
	{
		fatal(N, EruntTokenInNumericConst);
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
			Token *	newToken = lexAllocateToken(N,	kNewtonIrNodeType_TintegerConst	/* type		*/,
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
			Token *	newToken = lexAllocateToken(N,	kNewtonIrNodeType_ZbadIdentifier	/* type		*/,
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
		Token *	newToken = lexAllocateToken(N,	kNewtonIrNodeType_TrealConst /* type		*/,
									NULL				/* identifier	*/,
									0				/* integerConst	*/,
									sign * stringToEngineeringRealConst(N, N->currentToken) /* realConst	*/,
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
		Token *	newToken = lexAllocateToken(N,			kNewtonIrNodeType_TrealConst	 /* type	*/,
									NULL				/* identifier	*/,
									0				/* integerConst	*/,
									sign * stringToRealConst(N, N->currentToken)	/* realConst	*/,
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
		Token *	newToken = lexAllocateToken(N,			kNewtonIrNodeType_TintegerConst	/* type		*/,
									NULL				/* identifier	*/,
									sign * stringToRadixConst(N, N->currentToken)	/* integerConst	*/,
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
		Token *	newToken = lexAllocateToken(N,		kNewtonIrNodeType_ZbadIdentifier	/* type		*/,
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
		Token *	newToken = lexAllocateToken(N,			kNewtonIrNodeType_TintegerConst	/* type		*/,
									NULL				/* identifier	*/,
									sign * decimalValue			/* integerConst	*/,
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
		fatal(N, Esanity);
	}
}


static void
checkGt(State *  N)
{
	IrNodeType		type;

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	if (N->lineLength >= 2 && N->lineBuffer[N->columnNumber+1] == '<')
	{
		gobble(N, 2);
		type = kNewtonIrNodeType_Tmutualinf;
	}
	else if (eqf(N))
	{
		gobble(N, 2);
		type = kNewtonIrNodeType_Tge;
	}
	else
	{
		gobble(N, 1);
		type = kNewtonIrNodeType_Tgt;
	}

	Token *		newToken = lexAllocateToken(N,			type	/* type		*/,
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
checkLt(State *  N)
{
	IrNodeType		type;

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	if (N->lineLength >= 3 && N->lineBuffer[N->columnNumber+1] == '-' && N->lineBuffer[N->columnNumber+2] == '>')
	{
		gobble(N, 3);
		type = kNewtonIrNodeType_Trelated;
	}
	else if (eqf(N))
	{
		gobble(N, 2);
		type = kNewtonIrNodeType_Tle;
	}
	else
	{
		gobble(N, 1);
		type = kNewtonIrNodeType_Tlt;
	}

	Token *		newToken = lexAllocateToken(N,			type	/* type		*/,
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
checkEqual(State *  N)
{
	IrNodeType		type;

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	if (N->lineLength >= 2 && N->lineBuffer[N->columnNumber+1] == '=')
	{
		gobble(N, 2);
		type = kNewtonIrNodeType_Tequals;
	}
	else
	{
		gobble(N, 1);
		type = kNewtonIrNodeType_Tassign;
	}

	Token *		newToken = lexAllocateToken(N,			type	/* type		*/,
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
checkProportionality(State * N)
{
	IrNodeType		type;

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	if (N->lineLength >= 2 && N->lineBuffer[N->columnNumber+1] == '<')
	{
		gobble(N, 2);
		type = kNewtonIrNodeType_TdimensionallyAgnosticProportional;

		Token *		newToken = lexAllocateToken(N,		type	/* type		*/,
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

	return;
}

static void
checkPlusMinus(State *  N, IrNodeType plusOrMinusTokenType)
{
	/*
	 *	If the previous two characters were a number and 'e' or 'E', keep eating chars until the first non-number.
	 */
	if ((N->lineLength > 2) && isdigit(N->lineBuffer[N->columnNumber-2]) && ((N->lineBuffer[N->columnNumber-1] == 'e') || (N->lineBuffer[N->columnNumber-1] == 'E')))
	{
		/*
		 *	Consume the '+' or '-':
		 */
		N->currentToken[N->currentTokenLength++] = N->lineBuffer[N->columnNumber++];

		while (isdigit(N->lineBuffer[N->columnNumber]))
		{
			N->currentToken[N->currentTokenLength++] = N->lineBuffer[N->columnNumber++];
		}

		N->columnNumber++; finishToken(N);

		return;
	}

	/*
	 *	If the next character is not a number, then simply do checkSingle.
	 *	Otherwise, create a positive or negative numeric constant.
	 */
	if (N->lineLength >= 2 && (N->lineBuffer[N->columnNumber+1] < '0' || N->lineBuffer[N->columnNumber+1] > '9'))
	{
		checkSingle(N, plusOrMinusTokenType);

		return;
	}

	/*
	 *	Next character is the beginning of a number. Invoke makeNumericConst()
	 *	with a positive or negative signedness.
	 */
	gMakeNextTokenNegative = (plusOrMinusTokenType == kNewtonIrNodeType_Tminus ? true : false);

	/*
	 *	Next, gobble up the +/- and finish the extant token as though the +/- were
	 *	whitespace (don't call done() to allocate a Token on the tokenstream).
	 */
	N->columnNumber++; finishToken(N);

	/*
	 *	We are now at the beginning of a numeric constant. The next 'sticky' we see
	 *	will trigger gobbling. Set global flag to make the next gobbled token negative.
	 */
	gMakeNextTokenNegative = true;
}

static void
checkMul(State *  N)
{
	IrNodeType		type;

	/*
	 *	Gobble any extant chars.
	 */
	finishToken(N);

	if (N->lineLength >= 2 && N->lineBuffer[N->columnNumber+1] == '*')
	{
		gobble(N, 2);
		type = kNewtonIrNodeType_Texponentiation;
	}
	else
	{
		gobble(N, 1);
		type = kNewtonIrNodeType_Tmul;
	}

	Token *		newToken = lexAllocateToken(N,			type	/* type		*/,
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
checkDot(State *  N)
{
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

	Token *		newToken = lexAllocateToken(N,	kNewtonIrNodeType_Tdot	/* type		*/,
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

static bool
isOperatorOrSeparator(State *  N, char c)
{
	/*
	 *	Unlike in our Yacc-driven compielers, we don't use a "stickies" array
	 */
	switch (c)
	{
		case '~':
		case '@':
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
		case '"':
		case '{':
		case '}':
		case '[':
		case ']':
		case '|':
		case ',':
		case '.':
		case ' ':
		case '%':
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

