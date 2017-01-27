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
#include "noisy-lexer.h"
#include "noisy-lexers-helpers.h"

void
checkTokenLength(NoisyState *  N, int  count)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCheckTokenLength);

	if (N->currentTokenLength+count >= kNoisyMaxBufferLength)
	{
		noisyFatal(N, EtokenTooLong);
	}
}
char
cur(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerCur);

	return N->lineBuffer[N->columnNumber];
}

void
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

void
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

bool
eqf(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexerEqf);

	return (N->lineLength >= 2 && N->lineBuffer[N->columnNumber+1] == '=');
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


char *
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

char *
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


bool
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

bool
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


bool
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


bool
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


bool
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


uint64_t
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


double
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


double
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



