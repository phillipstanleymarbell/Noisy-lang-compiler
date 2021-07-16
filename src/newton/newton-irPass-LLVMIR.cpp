/*
	Authored 2020. Orestis Kaparounakis.

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

	This code makes the following assumptions for ease of implementation:
	- No signals with subdimensions.
	- Linear model must by factorised (e.g. 4*P+X and not 3*P+X+P).
	- LHS of invariants is assumed to contain solely the state and measurement identifiers.
	- System takes no input in linear case (B=0). // TODO: This is fixable.
	- Invariant parameter lists start with the state variables, in the same order
	  with which they appear in the invariant body.
	- Measure invariant does not skip unused state variables.

	Many of these can and will be lifted as development progresses.

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
#include <math.h>

#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>

extern "C"
{

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
#include "common-symbolTable.h"
#include "newton-types.h"
#include "newton-symbolTable.h"
#include "newton-irPass-cBackend.h"
#include "newton-irPass-autoDiff.h"
#include "newton-irPass-estimatorSynthesisBackend.h"

void    
irPassLLVMIR(State *  N)
{
    if (N->llvmIR == NULL)
    {
        flexprint(N->Fe, N->Fm, N->Fperr, "Please specify the LLVM IR input file\n");
		fatal(N, Esanity);
    }

    flexprint(N->Fe, N->Fm, N->Fpinfo, "Working\n");
	FILE *	irFile;
	//char * buffer = 0;
	//long length;

	irFile = fopen(N->llvmIR, "rb");

	if (irFile == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n%s: %s.\n", Eopen, N->llvmIR);
		consolePrintBuffers(N);
	}

	std::ifstream t(N->llvmIR);
	std::string str((std::istreambuf_iterator<char>(t)),
			                 std::istreambuf_iterator<char>());
	std::cout << str << std::endl;

	fclose (irFile);
	std::cout << "Done\n";
	//if (irFile)
	//{
	//	fseek (irFile, 0, SEEK_END);
	//	length = ftell (irFile);
	//	fseek (irFile, 0, SEEK_SET);
	//	buffer = malloc (length);
	//	if (buffer)
	//	{
	//		fread (buffer, 1, length, irFile);
	//	}
	//	fclose (irFile);
	//}

	//if (buffer)
    //    flexprint(N->Fe, N->Fm, N->Fpinfo, buffer);
}
}
