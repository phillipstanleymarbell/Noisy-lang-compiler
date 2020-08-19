/*
	Authored 2020. Oliver Gustafsson.
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
#include "noisy-lexer.h"
#include "common-irPass-helpers.h"
#include "newton-types.h"
#include "newton-symbolTable.h"
#include "common-irHelpers.h"
#include "newton-irPass-invariantSignalAnnotation.h"


/*
 *	TODO: Add support for associating a Signal with a sensor.
 */

/*
 *	Definitions for several of the functions used here can be 
 *	found in "newton-irPass-invariantSignalAnnotation.c".
 */


/*
 *  Function to find the Nth parameter/signal identifier from
 *  an invariant parameter list.
 */
Signal *
findNthSignalFromInvariant(State * N, Invariant * invariant, int nth)
{
	Signal * signal = NULL;
    IrNode * parameterList = invariant->parameterList;
    
    IrNode * parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
	signal = parameter->signal;

    return signal;
}


/*
 *  Function to get the indices of parameters
 *  included in a particular dimensionless group.
 *  (ie the parameters raised to a non-zero power).
 */
void getPiGroupParameterIndices(Invariant * invariant, int countKernel, int pi, char *parameterIndexList, int *numberOfParametersWithNonZeroPower)
{

    int *		tmpPosition = (int *)calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));

    for (int j = 0; j < invariant->dimensionalMatrixColumnCount; j++)
	{
		tmpPosition[invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + j]] = j;
	}

    int i = 0;
	for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
	{
		if(invariant->nullSpace[countKernel][tmpPosition[row]][pi] != 0)
        {
            parameterIndexList[i] = row;
            *numberOfParametersWithNonZeroPower = *numberOfParametersWithNonZeroPower + 1;
            i++; 
        } else {

        }
	}

    free(tmpPosition);
}


/*
 *  Function to add signals to the relatedSignalsList
 *  of the signals used in invariants. All the signals
 *  that appear alongside a signal in a Pi group are added
 *  to the relatedSignalsList of that signal.
 */
void
annotateSignalsPiGroups(State * N)
{
   Invariant * invariant = N->invariantList;

   while(invariant)
   {
        for(int countKernel = 0; countKernel < invariant->numberOfUniqueKernels; countKernel++)
        {
            for(int pi = 0; pi < invariant->kernelColumnCount; pi++)
            {
                char parameterIndexList[invariant->dimensionalMatrixColumnCount];
                int numberOfParametersWithNonZeroPower = 0;

                getPiGroupParameterIndices(invariant, countKernel, pi, parameterIndexList, &numberOfParametersWithNonZeroPower);

                 int count = 0;
                 Signal * newSignal;

                for(int i = 0; i < numberOfParametersWithNonZeroPower; i++)
                {
                    int index = parameterIndexList[i];
                    Signal * nthSignal = findNthSignalFromInvariant(N, invariant, index);
					
                    Signal * originSignal = findSignalByIdentifierAndAxis(N, nthSignal->identifier, nthSignal->axis);

                    if(count == 0)
                    {
                        newSignal = copySignal(N, originSignal);
                        count++;
                    } else {
                        Signal * nextSignal = copySignal(N, originSignal);

                        nextSignal->relatedSignalListPrev = newSignal;
                        newSignal->relatedSignalListNext = nextSignal;
                        newSignal = nextSignal;
                    }

                }

                while(newSignal->relatedSignalListPrev != NULL)
                {
                    newSignal = newSignal->relatedSignalListPrev;
                }
                while(newSignal != NULL)
                {
                    Signal * baseSignal = findSignalByIdentifierAndAxis(N, newSignal->identifier, newSignal->axis);
                    if(baseSignal->relatedSignalList == NULL)
                    {
                        baseSignal->relatedSignalList = copySignalList(N, newSignal);
                    } else {
                        baseSignal = baseSignal->relatedSignalList;
                        while(baseSignal != NULL)
                        {
                            if(baseSignal->relatedSignalListNext == NULL)
                            {
                                break;
                            }
                            baseSignal = baseSignal->relatedSignalListNext;
                        }
                        baseSignal->relatedSignalListNext = copySignalList(N, newSignal);
                    }
                    if(newSignal->relatedSignalListNext == NULL)
                    {
                        break;
                    }
                    newSignal = newSignal->relatedSignalListNext;
                }
                while(newSignal->relatedSignalListPrev != NULL)
                {
                    newSignal = newSignal->relatedSignalListPrev;
                }

                printf("%s %i %s %i \n", "Signals contained in Kernel:", countKernel, "and Pi group:", pi);
                printf("%s \n", newSignal->identifier);
                while(newSignal->relatedSignalListNext != NULL)
                {
                    newSignal = newSignal->relatedSignalListNext;
                    printf("%s \n", newSignal->identifier);
                }

				freeAllSignalsInList(N, newSignal);
            }
        }

       invariant = invariant->next;
   }
}



void
irPassPiGroupsSignalAnnotation(State * N)
{


    attachSignalsToParameterNodes(N);


    /*
     *
     *  The function below generates a list of sensors
     *  and signal identifiers used in each dimensionless
     *  group generated for a kernel of the invariant.
     */
    annotateSignalsPiGroups(N);


	/*
	 *	Copy the relatedSignalsList for the first instance
	 *	of each Signal with a particular identifier and axis
	 *	to every other instance of a Signal with the same identifier
	 *	and axis in the AST.
	 */
	copyRelatedSignalsToAllSignals(N);


	/*
	 *	Below code is for testing purposes. Prints out the relatedSignalList of all Signals.
	 */
	
	int i = 0;
	Signal * testSignal = findKthSignal(N, i);
	char * identifier;
	int axis;
	while(testSignal != NULL)
	{
		identifier = testSignal->identifier;
		axis = testSignal->axis;
		printf("%s %s %s %i \n", "The related signals are the following for the signal with identifier:", identifier, "and axis:", axis);
		if(testSignal->relatedSignalList == NULL)
		{
			i++;
			testSignal = findKthSignal(N, i);
			continue;
		}
		testSignal = testSignal->relatedSignalList;
		testSignal = removeDuplicates(N, testSignal);
		printf("%s %i \n", testSignal->identifier, testSignal->axis);
		while(testSignal->relatedSignalListNext != NULL)
		{
			testSignal = testSignal->relatedSignalListNext;
			printf("%s %i \n", testSignal->identifier, testSignal->axis);
		}
		i++;
		testSignal = findKthSignal(N, i);
	}
	
}