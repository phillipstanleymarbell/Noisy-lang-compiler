#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "version.h"
#include "noisyconfig.h"
#include "noisyconfig-errors.h"
#include "noisyconfig-parser-errors.h"

/*
 *	Exported non-parse routines
 */

void
noisyConfigParserSyntaxError(NoisyConfigState *  N, NoisyConfigIrNodeType currentlyParsingTokenOrProduction, NoisyConfigIrNodeType expectedProductionOrToken)
{
	int		seen = 0;


	//errors++;

	/*
	 *	TODO: Other places where we need the string form of a NoisyConfigIrNodeType
	 *	should also use gNoisyConfigAstNodeStrings[] like we do here, rather than 
	 *	gProductionStrings[] and gTerminalStrings[].
	 */
	flexprint(N->Fe, N->Fm, N->Fperr, "\n-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --\n");
	if (N->mode & kNoisyConfigModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "<b>");
	}

	if (N->mode & kNoisyConfigModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\t%s, line %d position %d, %s %s\"",
						EsyntaxA,
						noisyConfigLexPeek(N, 1)->sourceInfo->lineNumber,
						noisyConfigLexPeek(N, 1)->sourceInfo->columnNumber,
						EsyntaxD,
						kNoisyConfigErrorTokenHtmlTagOpen);
		noisyConfigLexPrintToken(N, noisyConfigLexPeek(N, 1));
		flexprint(N->Fe, N->Fm, N->Fperr, "\"%s %s %s.<br><br>%s%s", kNoisyConfigErrorTokenHtmlTagClose, EsyntaxB, gProductionDescriptions[currentlyParsingTokenOrProduction], kNoisyConfigErrorDetailHtmlTagOpen, EsyntaxC);
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\t%s, %s line %d position %d, %s \"",
						EsyntaxA,
						noisyConfigLexPeek(N, 1)->sourceInfo->fileName,
						noisyConfigLexPeek(N, 1)->sourceInfo->lineNumber,
						noisyConfigLexPeek(N, 1)->sourceInfo->columnNumber,
						EsyntaxD);
		noisyConfigLexPrintToken(N, noisyConfigLexPeek(N, 1));
		flexprint(N->Fe, N->Fm, N->Fperr, "\" %s %s.\n\n\t%s", EsyntaxB, gProductionDescriptions[currentlyParsingTokenOrProduction], EsyntaxC);
	}

	if (((expectedProductionOrToken > kNoisyConfigIrNodeType_TMax) && (expectedProductionOrToken < kNoisyConfigIrNodeType_PMax)) || (expectedProductionOrToken == kNoisyConfigIrNodeTypeMax))
	{
		flexprint(N->Fe, N->Fm, N->Fperr, " one of:\n\n\t\t");
		for (int i = 0; i < kNoisyConfigIrNodeTypeMax && gNoisyConfigFirsts[currentlyParsingTokenOrProduction][i] != kNoisyConfigIrNodeTypeMax; i++)
		{
			if (seen > 0)
			{
				flexprint(N->Fe, N->Fm, N->Fperr, ",\n\t\t");
			}

			flexprint(N->Fe, N->Fm, N->Fperr, "'%s'", gReservedTokenDescriptions[gNoisyConfigFirsts[currentlyParsingTokenOrProduction][i]]);
			seen++;
		}
	}
	else if ((currentlyParsingTokenOrProduction == kNoisyConfigIrNodeTypeMax) && (expectedProductionOrToken < kNoisyConfigIrNodeType_TMax))
	{
		flexprint(N->Fe, N->Fm, N->Fperr, ":\n\n\t\t");
		flexprint(N->Fe, N->Fm, N->Fperr, "'%s'", gReservedTokenDescriptions[expectedProductionOrToken]);
	}
	else
	{
		noisyConfigFatal(N, Esanity);
	}

	flexprint(N->Fe, N->Fm, N->Fperr, ".\n\n\tInstead, saw:\n\n");
	noisyConfigLexPeekPrint(N, 5, 0);
	
	if (N->mode & kNoisyConfigModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s</b>", kNoisyConfigErrorDetailHtmlTagClose);
	}

	flexprint(N->Fe, N->Fm, N->Fperr, "\n-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --\n\n");
}


void
noisyConfigParserSemanticError(NoisyConfigState *  N, const char * format, ...)
{
	va_list	arg;

	/*
	 *	We use varargs so that we can pass in a list of strings that should
	 *	get concatenated, akin to joining them with "+" in Limbo.
	 */
	if (N->verbosityLevel & kNoisyConfigVerbosityDebugParser)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "In noisyConfigParserSemanticError(), Ignoring semantic error...\n");
		flexprint(N->Fe, N->Fm, N->Fperr, "In noisyConfigParserSemanticError(), Source file line %llu\n", noisyConfigLexPeek(N, 1)->sourceInfo->lineNumber);
	}

	va_start(arg, format);
//	flexprint(N->Fe, N->Fm, N->Fperr, format, arg);
	va_end(arg);
}


void
noisyConfigParserErrorRecovery(NoisyConfigState *  N, NoisyConfigIrNodeType expectedProductionOrToken)
{
	if (N->verbosityLevel & kNoisyConfigVerbosityDebugParser)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "In noisyConfigParserErrorRecovery(), about to discard tokens...\n");
	}

	/*
	while (!noisyConfigInFollow(N, expectedProductionOrToken) && N->tokenList != NULL)
	{
		 *
		 *	Retrieve token and discard...
		 *
		NoisyConfigToken *	token = noisyConfigLexGet(N);
		if (N->verbosityLevel & kNoisyConfigVerbosityDebugParser)
		{
			noisyConfigLexDebugPrintToken(N, token);
		}
	}
	*/

	if ((N != NULL) && (N->jmpbufIsValid))
	{
fprintf(stderr, "doing longjmp");

		/*
		 *	Could pass in case-specific info here, but just
		 *	pass 0.
		 *
		 *	TODO: We could, e.g., return info on which line
		 *	number of the input we have reached, and let, e.g.,
		 *	the CGI version highlight the point at which
		 *	processing stopped.
		 */
		longjmp(N->jmpbuf, 0);
	}

	/*
	 *	Not reached if N->jmpbufIsValid
	 */
	noisyConfigConsolePrintBuffers(N);

	exit(EXIT_SUCCESS);
}



