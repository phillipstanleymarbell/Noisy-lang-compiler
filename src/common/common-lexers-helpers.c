/*
	Authored 2015, Phillip Stanley-Marbell.

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

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTorS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTorS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TorT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
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
#include <inttypes.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "noisy-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "common-lexers-helpers.h"

void
checkTokenLength(State *  N, int  count)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerCheckTokenLength);

	if (N->currentTokenLength+count >= kNoisyMaxBufferLength)
	{
		fatal(N, EtokenTooLong);
	}
}
char
cur(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerCur);

	return N->lineBuffer[N->columnNumber];
}

void
gobble(State *  N, int count)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerGobble);

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
done(State *  N, Token *  newToken)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerDone);

	newToken->sourceInfo = lexAllocateSourceInfo(N,	NULL				/*   genealogy 	*/,
								N->fileName			/*   fileName 	*/,
								N->lineNumber			/*   lineNumber */,
								N->columnNumber - N->currentTokenLength /* columnNumber */,
								N->currentTokenLength		/*   length 	*/);

	bzero(N->currentToken, kNoisyMaxBufferLength);
	N->currentTokenLength = 0;
	lexPut(N, newToken);
}

bool
eqf(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerEqf);

	return (N->lineLength >= 2 && N->lineBuffer[N->columnNumber+1] == '=');
}



bool
isDecimal(State *  N, char *  string)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerIsDecimal);

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
stringAtLeft(State *  N, char *  string, char character)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerStringAtLeft);

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
stringAtRight(State *  N, char *  string, char character)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerStringAtRight);

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
isDecimalSeparatedWithChar(State *  N, char *  string, char  character)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerIsDecimalSeparatedWithChar);

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

	/*
	 *	stringAtLeft() makes a copy, which needs to be freed.
	 *	(stringAtRight on the other hand does not need to make
	 *	a copy, and doesn't).
	 */
	free(left);


	return result;
}

bool
isDecimalOrRealSeparatedWithChar(State *  N, char *  string, char  character)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerIsDecimalSeparatedWithChar);

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

	/*
	 *	stringAtLeft() makes a copy, which needs to be freed.
	 *	(stringAtRight on the other hand does not need to make
	 *	a copy, and doesn't).
	 */
	free(left);


	return result;
}


bool
isRadixConst(State *  N, char *  string)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerIsRadixConst);

	if (string == NULL || !strchr(string, 'r'))
	{
		return false;
	}

	return isDecimalSeparatedWithChar(N, string, 'r');
}


bool
isRealConst(State *  N, char *  string)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerIsRealConst);

	if (string == NULL || !strchr(string, '.'))
	{
		return false;
	}

	return isDecimalSeparatedWithChar(N, string, '.');
}


bool
isEngineeringRealConst(State *  N, char *  string)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerIsEngineeringRealConst);

	if (string == NULL || (!strchr(string, 'e') && !strchr(string, 'E')))
	{
		return false;
	}

	return (isDecimalOrRealSeparatedWithChar(N, string, 'e') || isDecimalOrRealSeparatedWithChar(N, string, 'E'));
}


uint64_t
stringToRadixConst(State *  N, char *  string)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerStringToRadixConst);

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
		fatal(N, Esanity);

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
			fatal(N, Esanity);
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
stringToRealConst(State *  N, char *  string)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerStringToRealConst);

	char		tmp;
	char *		ep = &tmp;
	double		result;


	result = strtod(string, &ep);
	if (*ep != '\0')
	{
		fatal(N, Esanity);

		/* Not reached */
	}


	return result;
}


double
stringToEngineeringRealConst(State *  N, char *  string)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexerStringToEngineeringRealConst);

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




SourceInfo *
lexAllocateSourceInfo(	State *  N, char **  genealogy, char *  fileName,
				uint64_t lineNumber, uint64_t columnNumber, uint64_t length)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexAllocateSourceInfo);

	SourceInfo *	newSourceInfo;

	newSourceInfo = (SourceInfo *) calloc(1, sizeof(SourceInfo));
	if (newSourceInfo == NULL)
	{
		fatal(N, Emalloc);
	}

	newSourceInfo->genealogy	= genealogy;
	newSourceInfo->fileName		= (fileName == NULL ? NULL : strdup(fileName));
	newSourceInfo->lineNumber	= lineNumber;
	newSourceInfo->columnNumber	= columnNumber;
	newSourceInfo->length		= length;

	return newSourceInfo;
}


Token *
lexAllocateToken(	State *  N, IrNodeType type, char *  identifier,
			uint64_t integerConst, double realConst, char * stringConst,
			SourceInfo *  sourceInfo)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexAllocateToken);

	Token *	newToken;

	newToken = (Token *) calloc(1, sizeof(Token));
	if (newToken == NULL)
	{
		fatal(N, Emalloc);
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
lexPut(State *  N, Token *  newToken)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexPut);

	if (newToken == NULL)
	{
		fatal(N, Esanity);
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


Token *
lexGet(State *  N, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax])
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexGet);

	if (N->tokenList == NULL)
	{
		fatal(N, Esanity);
	}

	Token *	t = N->tokenList;
	
	if (t->next != NULL)
	{
		N->tokenList = N->tokenList->next;
	}
	else if (t->type != kNoisyIrNodeType_Zeof)
	{
		fatal(N, Esanity);
	}

	if (N->verbosityLevel & kNoisyVerbosityDebugLexer)
	{
		lexDebugPrintToken(N, t, tokenDescriptionArray);	
	}

	return t;
}


Token *
lexPeek(State *  N, int lookAhead)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexPeek);

	if (N->tokenList == NULL)
	{
		fatal(N, Esanity);
	}

	Token *	tmp = N->tokenList;
	int 		which = 1;
	while ((tmp != NULL) && (which++ < lookAhead))
	{
		tmp = tmp->next;
	}

	return tmp;
}


void
lexPrintToken(State *  N, Token *  t, const char *tokenDescriptionArray[])
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexPrintToken);

	switch (t->type)
	{
		case kNoisyIrNodeType_Tidentifier:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%s\"", t->identifier);
			break;
		}

		case kNoisyIrNodeType_TintegerConst:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%d\"", t->integerConst);
			break;
		}

		case kNoisyIrNodeType_TrealConst:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%f\"", t->realConst);
			break;
		}

		case kNoisyIrNodeType_TstringConst:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%s\"", t->stringConst);
			break;
		}

		default:
		{
			if (tokenDescriptionArray[t->type] != NULL)
			{
				flexprint(N->Fe, N->Fm, N->Fperr, "%s", tokenDescriptionArray[t->type]);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fperr, ">>>Unhandled IR node type [%d] in lexPrintToken (code is still specific to Noisy).<<<", t->type);
				//fatal(N, Esanity);
			}
		}
	}
}

void
lexDebugPrintToken(State *  N, Token *  t, const char *tokenDescriptionArray[])
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexDebugPrintToken);

    /* for now replacing gterminalstrings with tokendescription array because Newton doesn't use it */
	flexprint(N->Fe, N->Fm, N->Fperr, "Token %30s: ", tokenDescriptionArray[t->type]);

	switch (t->type)
	{
		case kNoisyIrNodeType_Tidentifier:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%20s\", ", t->identifier);
			break;
		}

		case kNoisyIrNodeType_TintegerConst:
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
			if (tokenDescriptionArray[t->type] != NULL)
			{
				flexprint(N->Fe, N->Fm, N->Fperr, "%22s, ", tokenDescriptionArray[t->type]);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fperr, ">>>Unhandled IR node type [%d] in lexPrintToken (code is still specific to Noisy).<<<", t->type);
				//fatal(N, Esanity);
			}
		}
	}

	if (N->mode & kNoisyModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "line %3d, position %3d, length %3d\n",
			t->sourceInfo->lineNumber, t->sourceInfo->columnNumber, t->sourceInfo->length);
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "source file: %16s, line %3d, position %3d, length %3d\n",
			t->sourceInfo->fileName, t->sourceInfo->lineNumber, t->sourceInfo->columnNumber, t->sourceInfo->length);
	}
}

void
lexPeekPrint(State *  N, int maxTokens, int formatCharacters, const char *tokenDescriptionArray[])
{
	TimeStampTraceMacro(kNoisyTimeStampKeyLexPeekPrint);

	if (N->tokenList == NULL)
	{
		fatal(N, Esanity);
	}

	int		tripCharacters = 0, done = 0;
	Token *	tmp = N->tokenList;

	if (N->mode & kNoisyModeCGI)
        {
		flexprint(N->Fe, N->Fm, N->Fperr, "\tline %5d, token %3d\t", tmp->sourceInfo->lineNumber, tmp->sourceInfo->columnNumber);
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\tsource file: %40s, line %5d, position %3d\t", tmp->sourceInfo->fileName, tmp->sourceInfo->lineNumber, tmp->sourceInfo->columnNumber);
	}

	while (tmp != NULL)
	{
		if (maxTokens > 0 && (done++ > maxTokens))
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "...");
			break;
		}


		//else
		{
			/*
			 *	NOTE: We currently don't keep newlines and whitespace on the token list...
			 */
			switch (tmp->type)
			{
				case kNewtonIrNodeType_Tidentifier:
				case kNoisyIrNodeType_Tidentifier:
				{
					flexprint(N->Fe, N->Fm, N->Fperr, "'%s' (identifier) ", tmp->identifier);

					/*
					 *	Account for the output string and the
					 *	two guarding "'" quotes.
					 */
					tripCharacters += strlen(tmp->identifier) + 14;

					break;
				}

				case kNoisyIrNodeType_TintegerConst:
				{
					flexprint(N->Fe, N->Fm, N->Fperr, "'%"PRIu64"' ", tmp->integerConst);

					char	dummy[64];
					tripCharacters += sprintf(dummy, "'%"PRIu64"' ", tmp->integerConst);

					break;
				}

				case kNewtonIrNodeType_Tnumber:
				case kNoisyIrNodeType_TrealConst:
				{
					flexprint(N->Fe, N->Fm, N->Fperr, "'%f' ", tmp->realConst);

					char	dummy[64];
					tripCharacters += sprintf(dummy, "'%f' ", tmp->realConst);

					break;
				}

				case kNewtonIrNodeType_TstringConst:
				case kNoisyIrNodeType_TstringConst:
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
					if (tokenDescriptionArray[tmp->type] != NULL)
					{
						flexprint(N->Fe, N->Fm, N->Fperr, "%s ", tokenDescriptionArray[tmp->type]);

						/*
						 *	Account for the trailing space
						 */
						tripCharacters += strlen(tokenDescriptionArray[tmp->type]) + 1;

					}
					else
					{
						flexprint(N->Fe, N->Fm, N->Fperr, ">>>Unhandled IR node type [%d] in lexPrintToken (code is still specific to Noisy).<<<", tmp->type);
						//fatal(N, Esanity);
					}
				}
			}

			if ((tmp->next != NULL) && (tmp->sourceInfo->lineNumber != tmp->next->sourceInfo->lineNumber))
			{
				//flexprint(N->Fe, N->Fm, N->Fperr, "(newlines)");
				tripCharacters = 0;

				if (N->mode & kNoisyModeCGI)
				{
					flexprint(N->Fe, N->Fm, N->Fperr, "\n\tline %5d\t\t", tmp->next->sourceInfo->lineNumber);
				}
				else
				{
					flexprint(N->Fe, N->Fm, N->Fperr, "\n\tsource file: %40s, line %5d\t\t", tmp->sourceInfo->fileName, tmp->next->sourceInfo->lineNumber);
				}
			}
			else if (tripCharacters >= formatCharacters)
			{
				tripCharacters = 0;
				if (N->mode & kNoisyModeCGI)
				{
					flexprint(N->Fe, N->Fm, N->Fperr, "\n\t\t\t\t");
				}
				else
				{
					flexprint(N->Fe, N->Fm, N->Fperr, "\n\t\t\t\t\t\t\t\t\t\t\t");
				}
			}
		}

		tmp = tmp->next;
	}
	flexprint(N->Fe, N->Fm, N->Fperr, "\n");
}
