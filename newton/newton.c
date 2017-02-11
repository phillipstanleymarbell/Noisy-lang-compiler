#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <getopt.h>
#include <setjmp.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "version.h"
#include "common-timeStamps.h"
#include "data-structures.h"

#include "newton-parser.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton.h"

/*
 * TODO: change this to be more flexible and take an arg from command line
 * https://github.com/phillipstanleymarbell/Noisy-lang-compiler/issues/28
 */
void		
processNewtonFile(NoisyState *  N, char *  filename)
{

	/*
	 *	Tokenize input, then parse it and build AST + symbol table.
	 */
	newtonLexInit(N, filename);

	/*
	 *	Create a top-level scope, then parse.
	 */
	N->newtonIrTopScope = newtonSymbolTableAllocScope(N);
	N->newtonIrRoot = newtonParse(N, N->newtonIrTopScope);

	/*
	 *	Dot backend.
	 */
	if (N->irBackends & kNoisyIrBackendDot)
	{
		// fprintf(stdout, "%s\n", noisyIrPassDotBackend(N, N->noisyIrTopScope, N->noisyIrRoot));
	}
    


	// if (N->mode & kNoisyConfigModeCallTracing)
	// {
	// 	noisyConfigTimeStampDumpTimeline(N);
	// }
    
    noisyConsolePrintBuffers(N);
}

