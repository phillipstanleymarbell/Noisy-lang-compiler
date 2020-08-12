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


/*
 *	TODO: Add support for signal with multiple indices of the form distance[i].
 *	TODO: The relatedSignalsList structs may only be attached to the first instance of a particular signal. Look into this.
 *	TODO: Function for finding the "nth" instance of a signal struct in the AST.
 */



/*
 *	Function to find the first instance of a Signal
 *	struct with a particular identifier in the AST.
 */
Signal *
findSignalByIdentifier(State * N, char * identifier, char* astNodeStrings[])
{
	Signal * signal = NULL;
	Invariant * invariant = N->invariantList;
	while(invariant)
	{
		IrNode * parameterList = invariant->parameterList;
		int nth = 0;
		IrNode * parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
		signal = parameter->signal;
		while(parameter != NULL && strcmp(parameter->signal->identifier, identifier) != 0)
		{
			nth++;
			parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
			if(parameter == NULL)
			{
				break;
			}
			signal = parameter->signal;
		}
		
		if(strcmp(signal->identifier, identifier) == 0)
		{
			break;
		} else {
			invariant = invariant->next;
		}
	}

	if(strcmp(signal->identifier, identifier) != 0)
	{
		printf("%s%s \n", "No signal found with identifier: ", identifier);
	}

	invariant = N->invariantList;
	return signal;
}


/*
 *	Function to find the first instance of the Signal
 *	in the AST with a particular identifier used in 
 *	an invariant constraint.
 */
Signal *
findSignalByInvariantExpressionIdentifier(State * N, char * identifier, char* astNodeStrings[])
{
	Signal * signal = NULL;
	Invariant * invariant = N->invariantList;
	while(invariant)
	{
		IrNode * parameterList = invariant->parameterList;
		int nth = 0;
		IrNode * parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
		signal = parameter->signal;
		while(parameter != NULL && strcmp(parameter->signal->invariantExpressionIdentifier, identifier) != 0)
		{
			nth++;
			parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
			if(parameter == NULL)
			{
				break;
			}
			signal = parameter->signal;
		}
		
		if(strcmp(signal->invariantExpressionIdentifier, identifier) == 0)
		{
			break;
		} else {
			invariant = invariant->next;
		}
	}

	if(strcmp(signal->invariantExpressionIdentifier, identifier) != 0)
	{
		printf("%s%s \n", "No signal found with identifier: ", identifier);
	}

	invariant = N->invariantList;
	return signal;
}


/*
 *	Function to find the IrNode of a signal with
 *	a particular identifier.
 */
IrNode *
findSignalBaseNodeByIdentifier(State * N, char * identifier, char* astNodeStrings[])
{
	IrNode * signalBaseNode = NULL;

	int nth = 0;
	signalBaseNode = findNthIrNodeOfType(N, N->newtonIrRoot, kNewtonIrNodeType_PbaseSignalDefinition, nth);

	while(signalBaseNode != NULL && strcmp(signalBaseNode->irLeftChild->tokenString, identifier) != 0)
	{
		signalBaseNode = findNthIrNodeOfType(N, N->newtonIrRoot, kNewtonIrNodeType_PbaseSignalDefinition, nth);
		nth++;
	}

	return signalBaseNode;
}


/*
 *	Function to find a constant in the AST by
 *	its identifier. Returns NULL if no constant is found.
 */
IrNode *
findConstantByIdentifier(State * N, char * identifier, char* astNodeStrings[])
{
	IrNode * constantDefinition = NULL;

	int nth = 0;
	constantDefinition = findNthIrNodeOfType(N, N->newtonIrRoot, kNewtonIrNodeType_PconstantDefinition, nth);

	while(constantDefinition != NULL && strcmp(constantDefinition->irLeftChild->tokenString, identifier) != 0)
	{
		constantDefinition = findNthIrNodeOfType(N, N->newtonIrRoot, kNewtonIrNodeType_PconstantDefinition, nth);
		nth++;
	}

	return constantDefinition;
}


/*
 *  Loop through parameterList and attach a signal to each parameter node.
 *  Also add the baseNode and identifier to each signal.
 */
int
attachSignalsToParameterNodes(State * N, char* astNodeStrings[])
{
	Invariant * invariant = N->invariantList;
	while(invariant)
	{
		IrNode * parameterList = invariant->parameterList;

		int nth = 0;
		IrNode * parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
	
		while(parameter != NULL)
		{
			/*
		 	 *	TODO: Add support for associating a sensor with a signal.
		 	 */


			Signal * signal = (Signal *) calloc(1, sizeof(Signal));
			char * identifier = parameter->irRightChild->tokenString;
			char * invariantExpressionIdentifier = parameter->irLeftChild->tokenString;
			signal->baseNode = findSignalBaseNodeByIdentifier(N, identifier, astNodeStrings);
			signal->identifier = identifier;
			signal->invariantExpressionIdentifier = invariantExpressionIdentifier;


			parameter->signal = signal;

			nth++;
			parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
		}
		invariant = invariant->next;
	}
	//	Reset invariant to head.
	invariant = N->invariantList;

	return 0;
}


/*
 *	Function for making a copy of an instance
 *	of the Signal struct.
 */
Signal *
copySignal(State * N, Signal * signal, char* astNodeStrings[])
{
	Signal * copyOfSignal = (Signal *) calloc(1, sizeof(Signal));

	Signal * originSignal = findSignalByIdentifier(N, signal->identifier, astNodeStrings);
	copyOfSignal->baseNode = originSignal->baseNode;
	copyOfSignal->identifier = originSignal->identifier;
	copyOfSignal->invariantExpressionIdentifier = originSignal->invariantExpressionIdentifier;
	copyOfSignal->sensorIdentifier = originSignal->sensorIdentifier;
	copyOfSignal->physicalGroupNumber = originSignal->physicalGroupNumber;

	return copyOfSignal;
}


/*
 *	Function for making a copy of a relatedSignalList.
 */
Signal *
copySignalList(State * N, Signal * signal, char* astNodeStrings[])
{
	while(signal->relatedSignalListPrev != NULL)
	{
		signal = signal->relatedSignalListPrev;
	}
	Signal * currentSignal = (Signal *) calloc(1, sizeof(Signal));

	int length = 0;

			while(signal != NULL)
			{
				if(length == 0)
				{
					Signal * originSignal = findSignalByIdentifier(N, signal->identifier, astNodeStrings);

					currentSignal->baseNode = originSignal->baseNode;
					currentSignal->identifier = originSignal->identifier;
					currentSignal->invariantExpressionIdentifier = originSignal->invariantExpressionIdentifier;
					currentSignal->sensorIdentifier = originSignal->sensorIdentifier;
					currentSignal->physicalGroupNumber = originSignal->physicalGroupNumber;
					length++;
				} else {

					Signal * originSignal = findSignalByIdentifier(N, signal->identifier, astNodeStrings);
					Signal * nextSignal = (Signal *) calloc(1, sizeof(Signal));
					nextSignal->baseNode = originSignal->baseNode;
					nextSignal->identifier = originSignal->identifier;
					nextSignal->invariantExpressionIdentifier = originSignal->invariantExpressionIdentifier;
					nextSignal->sensorIdentifier = originSignal->sensorIdentifier;
					nextSignal->physicalGroupNumber = originSignal->physicalGroupNumber;


					nextSignal->relatedSignalListPrev = currentSignal;
					currentSignal->relatedSignalListNext = nextSignal;
				
					currentSignal = nextSignal;
					length++;
				}
				
				
				if(signal->relatedSignalListNext == NULL)
				{
					break;
				}
				
				signal = signal->relatedSignalListNext;
				
				
				
			}

	while(currentSignal->relatedSignalListPrev != NULL)
	{
		currentSignal = currentSignal->relatedSignalListPrev;
	}

	return currentSignal;

}


/*
 *	Function to check if a particular signal is present
 *	in a relatedSignalList. Returns 1 if present, 0 otherwise.
 */
int
checkIfSignalPresentInList(State * N, Signal * signalList, Signal * signal, char*astNodeStrings[])
{
	int val = 0;
	while(signalList != NULL)
	{
		if(!strcmp(signalList->identifier, signal->identifier))
		{
			val = 1;
		}
		if(signalList->relatedSignalListNext == NULL)
		{
			break;
		}
		signalList = signalList->relatedSignalListNext;
	}

	return val;
}


/*
 *	Function to remove duplicates for a relatedSignalList.
 */
Signal *
removeDuplicates(State * N, Signal * signalList, char* astNodeStrings[])
{
	Signal * newSignalList = NULL;
	Signal * nextSignal = NULL;

	int count = 0;

	while(signalList != NULL)
	{
		if(count == 0)
		{
			newSignalList = copySignal(N, signalList, astNodeStrings);
			count++;
		} else {
			Signal * newSignalListCopy = copySignalList(N, newSignalList, astNodeStrings);
			int check = checkIfSignalPresentInList(N, newSignalListCopy, signalList, astNodeStrings);
			if(check == 1)
			{
				
			} else {
				nextSignal = copySignal(N, signalList, astNodeStrings);
				newSignalList->relatedSignalListNext = nextSignal;
				nextSignal->relatedSignalListPrev = newSignalList;
				newSignalList = nextSignal;
			}
		}
		if(signalList->relatedSignalListNext == NULL)
		{
			break;
		}
		signalList = signalList->relatedSignalListNext;
	}

	while(newSignalList->relatedSignalListPrev != NULL)
	{
		newSignalList = newSignalList->relatedSignalListPrev;
	}

	return newSignalList;
}


/*
 *	Function to annotate the Signal struct instances
 *	with a relatedSignalList of all the other signals
 *	which occur together with a particular signal in
 *	an invariant constraint.
 */
int
annotateSignalsInvariantConstraints(State * N, char* astNodeStrings[])
{
	Invariant * invariant = N->invariantList;

	while(invariant)
	{
		IrNode * invariantDefinition = invariant->parameterList->irParent->irParent;
		IrNode * constraintList = findNthIrNodeOfType(N, invariantDefinition, kNewtonIrNodeType_PconstraintList, 0);
		if(constraintList == NULL)
		{
			invariant = invariant->next;
			continue;
		}
		int kth = 0;
		IrNode * constraint = findNthIrNodeOfType(N, constraintList, kNewtonIrNodeType_Pconstraint, kth);
		while(constraint != NULL)
		{
			if(constraint == NULL)
			{
				break;
			}


			int nth = 0;
			IrNode * identifierNode = findNthIrNodeOfType(N, constraint, kNewtonIrNodeType_Tidentifier, nth);
			
			char * invariantExpressionIdentifier = identifierNode->tokenString;

			
			IrNode * constantDefinition = findConstantByIdentifier(N, invariantExpressionIdentifier, astNodeStrings);
			if(constantDefinition != NULL)
			{
				nth++;
				identifierNode = findNthIrNodeOfType(N, constraint, kNewtonIrNodeType_Tidentifier, nth);
				invariantExpressionIdentifier = identifierNode->tokenString;
			}
			
			
			Signal * signal = (Signal *) calloc(1, sizeof(Signal));
			signal->invariantExpressionIdentifier = invariantExpressionIdentifier;

			Signal * baseSignal = findSignalByInvariantExpressionIdentifier(N, invariantExpressionIdentifier, astNodeStrings);
			
			

			signal->baseNode = baseSignal->baseNode;
			signal->identifier = baseSignal->identifier;
			

			nth++;
			identifierNode = findNthIrNodeOfType(N, constraint, kNewtonIrNodeType_Tidentifier, nth);
	
			while(identifierNode != NULL)
			{

				invariantExpressionIdentifier = identifierNode->tokenString;

				IrNode * constantDefinition = findConstantByIdentifier(N, invariantExpressionIdentifier, astNodeStrings);
				if(constantDefinition != NULL)
				{
					nth++;
					identifierNode = findNthIrNodeOfType(N, constraint, kNewtonIrNodeType_Tidentifier, nth);
					continue;
				}
			
				Signal * nextSignal = (Signal *) calloc(1, sizeof(Signal));
			
				nextSignal->invariantExpressionIdentifier = invariantExpressionIdentifier;

				Signal * baseSignal = findSignalByInvariantExpressionIdentifier(N, invariantExpressionIdentifier, astNodeStrings);
				nextSignal->baseNode = baseSignal->baseNode;
				nextSignal->identifier = baseSignal->identifier;

				signal->relatedSignalListNext = nextSignal;
				nextSignal->relatedSignalListPrev = signal;
				signal = nextSignal;

				nth++;
				identifierNode = findNthIrNodeOfType(N, constraint, kNewtonIrNodeType_Tidentifier, nth);
			}


			Signal * headSignal = signal;
			while(headSignal->relatedSignalListPrev != NULL)
			{
				headSignal = headSignal->relatedSignalListPrev;
			}
			
			
			Signal * headSignalCopy = (Signal *) calloc(1, sizeof(Signal));
			
			
			
			int length = 0;

			while(headSignal != NULL)
			{
				if(length == 0)
				{
					Signal * originSignal = findSignalByIdentifier(N, headSignal->identifier, astNodeStrings);

					headSignalCopy->identifier = originSignal->identifier;
					headSignalCopy->invariantExpressionIdentifier = originSignal->invariantExpressionIdentifier;
					headSignalCopy->baseNode = originSignal->baseNode;
					length++;
				} else {

					Signal * originSignal = findSignalByIdentifier(N, headSignal->identifier, astNodeStrings);
					Signal * headSignalCopyNext = (Signal *) calloc(1, sizeof(Signal));
					headSignalCopyNext->identifier = originSignal->identifier;
					headSignalCopyNext->invariantExpressionIdentifier = originSignal->invariantExpressionIdentifier;
					headSignalCopyNext->baseNode = originSignal->baseNode;


					headSignalCopyNext->relatedSignalListPrev = headSignalCopy;
					headSignalCopy->relatedSignalListNext = headSignalCopyNext;
				
					headSignalCopy = headSignalCopyNext;
					length++;
				}
				
				
				if(headSignal->relatedSignalListNext == NULL)
				{
					break;
				}
				
				headSignal = headSignal->relatedSignalListNext;
				
				
				
			}
			

			while(headSignal->relatedSignalListPrev != NULL)
			{
				headSignal = headSignal->relatedSignalListPrev;
			}

			while(headSignalCopy->relatedSignalListPrev != NULL)
			{
				headSignalCopy = headSignalCopy->relatedSignalListPrev;
			}


			while(headSignal != NULL)
			{
				Signal * baseSignal = findSignalByIdentifier(N, headSignal->identifier, astNodeStrings);
				if(baseSignal->relatedSignalList == NULL)
				{
					baseSignal->relatedSignalList = copySignalList(N, headSignalCopy, astNodeStrings);
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
					baseSignal->relatedSignalListNext = copySignalList(N, headSignalCopy, astNodeStrings);
				}
				if(headSignal->relatedSignalListNext == NULL)
				{
					break;
				}
				headSignal = headSignal->relatedSignalListNext;
			}

			while(headSignal->relatedSignalListPrev != NULL)
			{
				headSignal = headSignal->relatedSignalListPrev;
			}

			kth++;
			constraint = findNthIrNodeOfType(N, constraintList, kNewtonIrNodeType_Pconstraint, kth);
		}
		invariant = invariant->next;
	}

	return 0;
}

void
irPassInvariantSignalAnnotation(State * N, char* astNodeStrings[])
{

    /*
     *  Loop through parameterList and attach a signal to each parameter node.
     *  Also add the baseNode and identifier to each signal.
     */
	attachSignalsToParameterNodes(N, astNodeStrings);

	printf("%s \n", "Middle.");

	/*
	 *	Look at each invariant expression, generate a list of signals used in the
	 *	expression, and add them as relatedSignalList to all signals in the expression.
	 *	Check for duplicates in the relatedSignalList.
	 */
	annotateSignalsInvariantConstraints(N, astNodeStrings);

	/*
	 *	Below code is for testing purposes.
	 */

	char * identifier = "frequency";

	printf("%s %s \n", "The related signals are the following for the signal with identifier:", identifier);
	
	Signal * testSignal = findSignalByIdentifier(N, identifier, astNodeStrings);

	

	testSignal = testSignal->relatedSignalList;
	testSignal = removeDuplicates(N, testSignal, astNodeStrings);
	printf("%s \n", testSignal->identifier);
	
	while(testSignal->relatedSignalListNext != NULL)
	{
		testSignal = testSignal->relatedSignalListNext;
		printf("%s \n", testSignal->identifier);
	}
	

}