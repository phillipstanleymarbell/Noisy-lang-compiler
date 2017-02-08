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
#include "noisy-timeStamps.h"
#include "noisy.h"
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
noisyLexGet(NoisyState *  N, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax])
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
		noisyLexDebugPrintToken(N, t, tokenDescriptionArray);	
	}

	return t;
}


NoisyToken *
noisyLexPeek(NoisyState *  N, int lookAhead)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexPeek);

	if (N->tokenList == NULL)
	{
		noisyFatal(N, Esanity);
	}

	NoisyToken *	tmp = N->tokenList;
	int 		which = 1;
	while ((tmp != NULL) && (which++ < lookAhead))
	{
//fprintf(stderr, "*** which=%d, lookAhead=%d\n", which, lookAhead);
		tmp = tmp->next;
	}

	return tmp;
}


void
noisyLexPrintToken(NoisyState *  N, NoisyToken *  t, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax])
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexPrintToken);

	switch (t->type)
	{
		case kNoisyIrNodeType_Tidentifier:
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\"%s\"", t->identifier);
			break;
		}

		case kNoisyIrNodeType_TintConst:
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
				flexprint(N->Fe, N->Fm, N->Fperr, ">>>BUG: unhandled type [%d] in noisyLexPrintToken <<<", t->type);
				//noisyFatal(N, Esanity);
			}
		}
	}
}

void
noisyLexDebugPrintToken(NoisyState *  N, NoisyToken *  t, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax])
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexDebugPrintToken);

    /* for now replacing gterminalstrings with tokendescription array because Newton doesn't use it */
	flexprint(N->Fe, N->Fm, N->Fperr, "Token %30s: ", tokenDescriptionArray[t->type]);

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
			if (tokenDescriptionArray[t->type] != NULL)
			{
				flexprint(N->Fe, N->Fm, N->Fperr, "%22s, ", tokenDescriptionArray[t->type]);
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fperr, ">>>BUG: unhandled type [%d] in noisyLexDebugPrintToken <<<", t->type);
				//noisyFatal(N, Esanity);
			}
		}
	}

	flexprint(N->Fe, N->Fm, N->Fperr, "source file: %16s, line %3d, pos %3d, length %3d\n",
		t->sourceInfo->fileName, t->sourceInfo->lineNumber, t->sourceInfo->columnNumber, t->sourceInfo->length);
}

void
noisyLexPeekPrint(NoisyState *  N, int maxTokens, int formatCharacters, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax])
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyLexPeekPrint);

	if (N->tokenList == NULL)
	{
		noisyFatal(N, Esanity);
	}

	int		tripCharacters = 0, done = 0;
	NoisyToken *	tmp = N->tokenList;

//flexprint(E, M, P, "\tline %5d, token %3d\t", tmp->linenum, tmp->colnum);
	flexprint(N->Fe, N->Fm, N->Fperr, "\t\tline %5d, token %3d\t", tmp->sourceInfo->lineNumber, tmp->sourceInfo->columnNumber);
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

				case kNoisyIrNodeType_TintConst:
				{
					flexprint(N->Fe, N->Fm, N->Fperr, "'%llu' ", tmp->integerConst);

					char	dummy[64];
					tripCharacters += sprintf(dummy, "'%llu' ", tmp->integerConst);

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
						flexprint(N->Fe, N->Fm, N->Fperr, ">>>BUG/TODO: un-handled type [%d] in noisyLexPeekPrint <<<", tmp->type);
						noisyFatal(N, Esanity);
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
