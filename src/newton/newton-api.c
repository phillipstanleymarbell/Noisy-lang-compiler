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
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "newton-data-structures.h"
#include "common-irHelpers.h"

#include "newton-parser.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton.h"
#include "newton-api.h"
#include "newton-check-pass.h"
#include "newton-dimension-check-pass.h"


/*
 *	I have long ago given up on cleaning this up and am slowly discarding all 
 *	of JL's Newton API code. This file is only used from JL's "Newton API"
 *	which I will no longer maintain --- PSM.
 */


State *
newtonApiInit(char * newtonFileName)
{	
    State *     N = init(kNoisyModeDefault);
    processNewtonFile(N, newtonFileName);
    return N;
}

Physics *
newtonApiGetPhysicsTypeByNameAndSubindex(State * N, char * nameOfType, int subindex)
{
    Physics *   current = N->newtonIrTopScope->firstPhysics;
    while (current != NULL)
    {
        if (!strcmp(current->identifier, nameOfType) && current->subindex == subindex)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

Physics *
newtonApiGetPhysicsTypeByName(State * N, char * nameOfType)
{
    Physics *   current = N->newtonIrTopScope->firstPhysics;
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
newtonApiGetInvariantByParameters(State * N, IrNode * parameterTreeRoot)
{
    unsigned long long int      targetId = newtonGetInvariantIdByParameters(N, parameterTreeRoot, 1);
    Invariant *     current = N->invariantList;
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
newtonApiSatisfiesConstraints(State * N, IrNode * parameterTreeRoot)
{
    Invariant *     invariant = newtonApiGetInvariantByParameters(N, parameterTreeRoot);
    NewtonAPIReport *   report = (NewtonAPIReport *) calloc(1, sizeof(NewtonAPIReport));
    iterateConstraints(N, invariant->constraints, parameterTreeRoot, report);
    return report;
}


void
newtonApiAddLeaf(State *  N, IrNode *  parent, IrNode *  newNode)
{
	IrNode *	node = depthFirstWalk(N, parent);

	if (node == NULL)
    {
        fatal(N, Esanity);
    }

	if (node->irLeftChild == NULL)
    {
        node->irLeftChild = newNode;

        return;
    }

	node->irRightChild = newNode;
}

void
newtonApiAddLeafWithChainingSeqNoLexer(State *  N, IrNode *  parent, IrNode *  newNode)
{
	IrNode *	node = depthFirstWalk(N, parent);

	if (node->irLeftChild == NULL)
    {
        node->irLeftChild = newNode;

        return;
    }

	node->irRightChild = genIrNode(N,	kNoisyIrNodeType_Xseq,
                                      newNode /* left child */,
                                      NULL /* right child */,
                                      NULL/* source info */);
}


void
newtonApiNumberParametersZeroToN(State * N, IrNode * parameterTreeRoot)
{
    IrNode *    parameter;
    int     parameterNumber = 0;

    while((parameter = findNthIrNodeOfType(N, parameterTreeRoot, kNewtonIrNodeType_Pparameter, parameterNumber)) != NULL)
    {
        parameter->parameterNumber = parameterNumber;
        parameterNumber++;
    }
}


ConstraintReport *
newtonApiDimensionCheckTree(State * N, IrNode * tree)
{
    return newtonDimensionCheckExpressionOrStatement(N, tree);
}

