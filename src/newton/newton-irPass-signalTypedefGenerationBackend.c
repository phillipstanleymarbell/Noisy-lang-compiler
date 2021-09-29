/*
	Authored 2018. Youchao Wang.
	Updated 2020. Orestis Kaparounakis.

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

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "version.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "noisy-parser.h"
#include "newton-parser.h"
#include "noisy-lexer.h"
#include "newton-lexer.h"
#include "common-irPass-helpers.h"
#include "common-lexers-helpers.h"
#include "common-irHelpers.h"
#include "newton-types.h"
#include "newton-symbolTable.h"
#include "newton-irPass-invariantSignalAnnotation.h"

extern const char *		gNewtonTokenDescriptions[kCommonIrNodeTypeMax];

/*
 *  Function returns the number of Signals in the AST. (Used)
 */
// int
// countAllSignals(State * N)
// {
//     int count = 0;
//     Signal * signal = findKthSignal(N, count);

//     if(signal == NULL)
//     {
//         return count;
//     }
//     while(signal != NULL)
//     {
//         count++;
//         signal = findKthSignal(N, count);
//     }

//     return count;    
// }

void
irPassSignalTypedefGenerationProcessNewtonSources(State *  N)
{
	/*
	 *	FIXME -- Add a different timestamp type
	 */
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassSignalTypedefGenerationBackend);

	flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tGenerated .h file from Newton\n */\n");

	// int count = 0;
    
	// Signal * signal = findKthSignal(N, count);

	IrNode * oneIrNode = findNthIrNodeOfType(N, N->newtonIrRoot, kNewtonIrNodeType_PbaseSignalDefinition, 0);

	flexprint(N->Fe, N->Fm, N->Fpc, "\n\ttypedef double %s;\n", oneIrNode->tokenString);

    // while(signal != NULL)
    // {
    //     count++;

	// 	flexprint(N->Fe, N->Fm, N->Fpc, "\n\ttypedef double %s;\n", signal->identifier);

    //     signal = findKthSignal(N, count);
    // }
	
	flexprint(N->Fe, N->Fm, N->Fpc, "/*\n *\tEnd of the generated .h file\n */\n");
}

void
irPassSignalTypedefGenerationBackend(State *  N)
{
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassSignalTypedefGenerationBackend);

	FILE *	signalTypedefHeaderFile;

	irPassSignalTypedefGenerationProcessNewtonSources(N);

	if (N->outputSignalTypedefHeaderFilePath)
	{
		signalTypedefHeaderFile = fopen(N->outputSignalTypedefHeaderFilePath, "w");

		if (signalTypedefHeaderFile == NULL)
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\n%s: %s.\n", Eopen, N->outputSignalTypedefHeaderFilePath);
			
			consolePrintBuffers(N);
		}

		fprintf(signalTypedefHeaderFile, "%s", N->Fpc->circbuf);

		fclose(signalTypedefHeaderFile);
	}
}
