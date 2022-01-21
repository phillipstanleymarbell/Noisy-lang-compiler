/*
	Authored 2021. Vasileios Tsoutsouras.

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

void
irPassSignalTypedefGenerationProcessNewtonSources(State *  N)
{
	/*
	 *	FIXME -- Add a different timestamp type
	 */
	TimeStampTraceMacro(kNewtonTimeStampKeyIrPassSignalTypedefGenerationBackend);

	flexprint(N->Fe, N->Fm, N->Fph, "/*\n *\tGenerated .h file from Newton file: %s\n */\n\n", N->fileName);

	int count = 0;
    
	IrNode * oneIrNode = findNthIrNodeOfType(N, N->newtonIrRoot, kNewtonIrNodeType_PbaseSignalDefinition, count++);

	while(oneIrNode != NULL)
    {
        flexprint(N->Fe, N->Fm, N->Fph, "typedef %s %s;\n", N->signalTypedefDatatype, oneIrNode->irLeftChild->tokenString);

        oneIrNode = findNthIrNodeOfType(N, N->newtonIrRoot, kNewtonIrNodeType_PbaseSignalDefinition, count++);
    }

	flexprint(N->Fe, N->Fm, N->Fph, "\n/*\n *\tEnd of the generated .h file\n */\n\n");
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
		else
		{
			fprintf(signalTypedefHeaderFile, "%s", N->Fph->circbuf);

			fclose(signalTypedefHeaderFile);
		}
	}
}
