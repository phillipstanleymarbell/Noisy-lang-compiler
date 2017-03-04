/*
	Authored 2016. Jonathan Lim.

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
#include "newton-data-structures.h"

#include "newton-parser.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton.h"
#include "newton-api.h"
#include "newton-check-pass.h"


NoisyState *
newtonApiInit(char *  newtonFileName)
{
    NoisyState * N = noisyInit(kNoisyModeDefault);
    processNewtonFile(N, newtonFileName);
    return N;
}

Physics *
newtonApiGetPhysicsTypeByName(NoisyState* N, char* nameOfType)
{
    Physics* current = N->newtonIrTopScope->firstPhysics;
    while (current != NULL)
    {
        if (!strcmp(current->identifier, nameOfType))
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}


Invariant * 
newtonApiGetInvariantByParameters(NoisyState* N, NoisyIrNode* parameterTreeRoot)
{
    unsigned long long int targetId = newtonGetInvariantIdByParameters(N, parameterTreeRoot, 1);
    Invariant * current = N->invariantList;
    while (current != NULL)
    {
        if (current->id == targetId)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

NewtonAPIReport * 
newtonApiSatisfiesConstraints(NoisyState* N, NoisyIrNode* parameterTreeRoot)
{
    Invariant* invariant = newtonApiGetInvariantByParameters(N, parameterTreeRoot);
    NewtonAPIReport* report = (NewtonAPIReport*) calloc(1, sizeof(NewtonAPIReport));
    iterateConstraints(N, invariant->constraints, parameterTreeRoot, report);
    return report;
}


