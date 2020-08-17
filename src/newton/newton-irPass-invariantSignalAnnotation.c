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
 *	TODO: Add support for associating a sensor with a Signal.
 */


/*
 *	Function to find the kth instance of a signal struct
 *	with a particular identifier in the AST.
 */
Signal *
findKthSignalByIdentifier(State * N, char * identifier, int kth)
{
	Signal * signal = NULL;
	Invariant * invariant = N->invariantList;
	int count = 0;
	while(invariant)
	{
		IrNode * parameterList = invariant->parameterList;
		int nth = 0;
		IrNode * parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
		while(parameter != NULL)
		{
			if(strcmp(parameter->signal->identifier, identifier) == 0)
			{
				if(count == kth)
				{
					signal = parameter->signal;
					break;
				} else {
					count++;
				}
			}
			nth++;
			parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
			if(parameter == NULL)
			{
				break;
			}

		}

		if(signal != NULL)
		{
			break;
		} else {
			invariant = invariant->next;
		}
	}

	if(signal == NULL)
	{
		//printf("%s%s \n", "No signal found with identifier: ", identifier);
		flexprint(N->Fe, N->Fm, N->Fperr, "%s%s \n", "No signal found with identifier: ", identifier);
	}

	invariant = N->invariantList;
	return signal;
}


/*
 *	Function to find the first instance of a signal by identifier and axis.
 */
Signal *
findSignalByIdentifierAndAxis(State * N, char * identifier, int axis)
{
	Signal * signal = NULL;

	int i = 0;
	signal = findKthSignalByIdentifier(N, identifier, i);
	while(signal != NULL)
	{
		if(signal->axis == axis)
		{
			break;
		} else {
			i++;
		}
		signal = findKthSignalByIdentifier(N, identifier, i);
	}

	if(signal == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s%s %s%i \n", "No signal found with identifier: ", identifier, "and axis: ", axis);
	}

	return signal;
}


/*
 *	Function to find the kth instance of a
 *	Signal with a particular invariant expression
 *	identifier.
 */
Signal *
findKthSignalByInvariantExpressionIdentifier(State * N, char * identifier, int kth)
{
	Signal * signal = NULL;
	Invariant * invariant = N->invariantList;
	int count = 0;
	while(invariant)
	{
		IrNode * parameterList = invariant->parameterList;
		int nth = 0;
		IrNode * parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
		while(parameter != NULL)
		{
			if(strcmp(parameter->signal->invariantExpressionIdentifier, identifier) == 0)
			{
				if(count == kth)
				{
					signal = parameter->signal;
					break;
				} else {
					count++;
				}
			}
			nth++;
			parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
			if(parameter == NULL)
			{
				break;
			}
		}
		
		if(signal != NULL)
		{
			break;
		} else {
			invariant = invariant->next;
		}
	}

	/*
	if(signal == NULL)
	{
		//printf("%s%s \n", "No signal found with identifier: ", identifier);
		flexprint(N->Fe, N->Fm, N->Fperr, "%s%s \n", "No signal found with invaraint expression identifier: ", identifier);
	}
	*/
	invariant = N->invariantList;
	return signal;
}


/*
 *	Function to find the first instance of a signal by invariant expression identifier and axis.
 */
Signal *
findSignalByInvariantExpressionIdentifierAndAxis(State * N, char * identifier, int axis)
{
	Signal * signal = NULL;

	int i = 0;
	signal = findKthSignalByInvariantExpressionIdentifier(N, identifier, i);
	while(signal != NULL)
	{
		if(signal->axis == axis)
		{
			break;
		} else {
			i++;
		}
		signal = findKthSignalByInvariantExpressionIdentifier(N, identifier, i);
	}

	if(signal == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s%s %s%i \n", "No signal found with invariant expression identifier: ", identifier, "and axis: ", axis);
	}
	
	return signal;
}


/*
 *	Function to get the Signal axis of a Signal referred
 *	to with a particular invariant expression identifier
 *	within an invariant definition.
 */
int
getSignalAxis(State * N, Invariant * invariant, char * invariantExpressionIdentifier)
{
	int axis = 0;
	IrNode * parameterList = invariant->parameterList;
	int nth = 0;
	IrNode * parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
	while(parameter != NULL)
	{
		if(!strcmp(parameter->signal->invariantExpressionIdentifier, invariantExpressionIdentifier))
		{
			break;
		}

		nth++;
		parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
	}

	if(parameter == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s%s \n", "No signal found with invariant expression identifier: ", invariantExpressionIdentifier);
	} else {
		axis = parameter->signal->axis;
	}

	return axis;
}


/*
 *	Function to find the IrNode of a signal with
 *	a particular identifier.
 */
IrNode *
findSignalBaseNodeByIdentifier(State * N, char * identifier)
{
	IrNode * signalBaseNode = NULL;

	int nth = 0;
	signalBaseNode = findNthIrNodeOfType(N, N->newtonIrRoot, kNewtonIrNodeType_PbaseSignalDefinition, nth);

	while(signalBaseNode != NULL && strcmp(signalBaseNode->irLeftChild->tokenString, identifier) != 0)
	{
		signalBaseNode = findNthIrNodeOfType(N, N->newtonIrRoot, kNewtonIrNodeType_PbaseSignalDefinition, nth);
		nth++;
	}

	if(signalBaseNode == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s%s \n", "No signal base node found with invariant constraint expression identifier: ", identifier);
	}

	return signalBaseNode;
}


/*
 *	Function to find a constant in the AST by
 *	its identifier. Returns NULL if no constant is found.
 */
IrNode *
findConstantByIdentifier(State * N, char * identifier)
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
void
attachSignalsToParameterNodes(State * N)
{
	Invariant * invariant = N->invariantList;
	if(invariant == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s \n", "Newton description contains no invariant definitions.");
	}
	while(invariant)
	{
		IrNode * parameterList = invariant->parameterList;

		if(parameterList == NULL)
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "%s \n", "Invariant definition contains no parameters.");
		}

		int nth = 0;
		IrNode * parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
	
		while(parameter != NULL)
		{
			/*
		 	 *	TODO: Add support for associating a sensor with a signal.
		 	 */


			char * identifier = parameter->irRightChild->tokenString;
			char * invariantExpressionIdentifier = parameter->irLeftChild->tokenString;
			parameter->signal->baseNode = findSignalBaseNodeByIdentifier(N, identifier);
			parameter->signal->identifier = identifier;
			parameter->signal->invariantExpressionIdentifier = invariantExpressionIdentifier;

			nth++;
			parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
		}
		invariant = invariant->next;
	}
	//	Reset invariant to head.
	invariant = N->invariantList;

}


/*
 *	Function for making a copy of an instance
 *	of the Signal struct.
 */
Signal *
copySignal(State * N, Signal * signal)
{
	Signal * copyOfSignal = (Signal *) calloc(1, sizeof(Signal));

	Signal * originSignal = findSignalByIdentifierAndAxis(N, signal->identifier, signal->axis);
	copyOfSignal->baseNode = originSignal->baseNode;
	copyOfSignal->identifier = originSignal->identifier;
	copyOfSignal->invariantExpressionIdentifier = originSignal->invariantExpressionIdentifier;
	copyOfSignal->axis = originSignal->axis;
	copyOfSignal->sensorIdentifier = originSignal->sensorIdentifier;
	copyOfSignal->physicalGroupNumber = originSignal->physicalGroupNumber;

	return copyOfSignal;
}


/*
 *	Function for making a copy of a relatedSignalList.
 */
Signal *
copySignalList(State * N, Signal * signal)
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
					Signal * originSignal = findSignalByIdentifierAndAxis(N, signal->identifier, signal->axis);

					currentSignal->baseNode = originSignal->baseNode;
					currentSignal->identifier = originSignal->identifier;
					currentSignal->invariantExpressionIdentifier = originSignal->invariantExpressionIdentifier;
					currentSignal->axis = originSignal->axis;
					currentSignal->sensorIdentifier = originSignal->sensorIdentifier;
					currentSignal->physicalGroupNumber = originSignal->physicalGroupNumber;
					length++;
				} else {

					Signal * originSignal = findSignalByIdentifierAndAxis(N, signal->identifier, signal->axis);
					Signal * nextSignal = (Signal *) calloc(1, sizeof(Signal));
					nextSignal->baseNode = originSignal->baseNode;
					nextSignal->identifier = originSignal->identifier;
					nextSignal->invariantExpressionIdentifier = originSignal->invariantExpressionIdentifier;
					nextSignal->axis = originSignal->axis;
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
checkIfSignalPresentInList(State * N, Signal * signalList, Signal * signal)
{
	int val = 0;
	while(signalList != NULL)
	{
		if(strcmp(signalList->identifier, signal->identifier) == 0 && signalList->axis == signal->axis)
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
removeDuplicates(State * N, Signal * signalList)
{
	Signal * newSignalList = NULL;
	Signal * nextSignal = NULL;

	int count = 0;

	while(signalList != NULL)
	{
		if(count == 0)
		{
			newSignalList = copySignal(N, signalList);
			count++;
		} else {
			Signal * newSignalListCopy = copySignalList(N, newSignalList);
			int check = checkIfSignalPresentInList(N, newSignalListCopy, signalList);
			if(check == 1)
			{
				
			} else {
				nextSignal = copySignal(N, signalList);
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
void
annotateSignalsInvariantConstraints(State * N)
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


			Signal * tempSignal = findKthSignalByInvariantExpressionIdentifier(N, invariantExpressionIdentifier, 0);
			while(tempSignal == NULL)
			{
				nth++;
				identifierNode = findNthIrNodeOfType(N, constraint, kNewtonIrNodeType_Tidentifier, nth);
				if(identifierNode == NULL)
				{
					break;
				}
				invariantExpressionIdentifier = identifierNode->tokenString;
				tempSignal = findKthSignalByInvariantExpressionIdentifier(N, invariantExpressionIdentifier, 0);
			}
			if(identifierNode == NULL)
			{

			}
			
			
			Signal * signal = (Signal *) calloc(1, sizeof(Signal));
			signal->invariantExpressionIdentifier = invariantExpressionIdentifier;

			int axis = getSignalAxis(N, invariant, invariantExpressionIdentifier);
			Signal * baseSignal = findSignalByInvariantExpressionIdentifierAndAxis(N, invariantExpressionIdentifier, axis);
			
		
			signal->baseNode = baseSignal->baseNode;
			signal->identifier = baseSignal->identifier;
			signal->axis = baseSignal->axis;
			

			nth++;
			identifierNode = findNthIrNodeOfType(N, constraint, kNewtonIrNodeType_Tidentifier, nth);
	
			while(identifierNode != NULL)
			{

				invariantExpressionIdentifier = identifierNode->tokenString;


				tempSignal = findKthSignalByInvariantExpressionIdentifier(N, invariantExpressionIdentifier, 0);
				while(tempSignal == NULL)
				{
					nth++;
					identifierNode = findNthIrNodeOfType(N, constraint, kNewtonIrNodeType_Tidentifier, nth);
					if(identifierNode == NULL)
					{
						break;
					}
					invariantExpressionIdentifier = identifierNode->tokenString;
					tempSignal = findKthSignalByInvariantExpressionIdentifier(N, invariantExpressionIdentifier, 0);
				}

				if(identifierNode == NULL)
				{
					kth++;
					constraint = findNthIrNodeOfType(N, constraintList, kNewtonIrNodeType_Pconstraint, kth);
					continue;
				}
				
			
				Signal * nextSignal = (Signal *) calloc(1, sizeof(Signal));
			
				nextSignal->invariantExpressionIdentifier = invariantExpressionIdentifier;

				axis = getSignalAxis(N, invariant, invariantExpressionIdentifier);
				Signal * baseSignal = findSignalByInvariantExpressionIdentifierAndAxis(N, invariantExpressionIdentifier, axis);
				nextSignal->baseNode = baseSignal->baseNode;
				nextSignal->identifier = baseSignal->identifier;
				nextSignal->axis = baseSignal->axis;

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
					Signal * originSignal = findSignalByIdentifierAndAxis(N, headSignal->identifier, headSignal->axis);

					headSignalCopy->identifier = originSignal->identifier;
					headSignalCopy->invariantExpressionIdentifier = originSignal->invariantExpressionIdentifier;
					headSignalCopy->baseNode = originSignal->baseNode;
					headSignalCopy->axis = originSignal->axis;
					length++;
				} else {

					Signal * originSignal = findSignalByIdentifierAndAxis(N, headSignal->identifier, headSignal->axis);
					Signal * headSignalCopyNext = (Signal *) calloc(1, sizeof(Signal));
					headSignalCopyNext->identifier = originSignal->identifier;
					headSignalCopyNext->invariantExpressionIdentifier = originSignal->invariantExpressionIdentifier;
					headSignalCopyNext->baseNode = originSignal->baseNode;
					headSignalCopyNext->axis = originSignal->axis;


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
				Signal * baseSignal = findSignalByIdentifierAndAxis(N, headSignal->identifier, headSignal->axis);
				if(baseSignal->relatedSignalList == NULL)
				{
					baseSignal->relatedSignalList = copySignalList(N, headSignalCopy);
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
					baseSignal->relatedSignalListNext = copySignalList(N, headSignalCopy);
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
}


/*
 *	Function to find the kth signal in the AST.
 */
Signal *
findKthSignal(State * N, int kth)
{
	Signal * signal = NULL;
	int count = 0;
	Invariant * invariant = N->invariantList;
	while(invariant)
	{
		int nth = 0;
		IrNode * parameterList = invariant->parameterList;
		IrNode * parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
		while(parameter != NULL)
		{
			if(count == kth)
			{
				signal = parameter->signal;
				break;
			} else {
				count++;
				nth++;
				parameter = findNthIrNodeOfType(N, parameterList, kNewtonIrNodeType_Pparameter, nth);
			}
		}
		if(count == kth && parameter != NULL)
		{
			break;
		}
		invariant = invariant->next;
	}
	/*
	if(signal == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s%i \n", "No Signal found in AST at kth=", kth);
	}
	*/
	return signal;
}

/*
 *	Function to copy the relatedSignalsList for the first instance
 *	of each Signal with a particular identifier and axis
 *	to every other instance of a Signal with the same identifier
 *	and axis in the AST.
 */
void
copyRelatedSignalsToAllSignals(State * N)
{
	int kth = 0;
	Signal * signal = findKthSignal(N, kth);
	while(signal != NULL)
	{
		Signal * originSignal = findSignalByIdentifierAndAxis(N, signal->identifier, signal->axis);
		signal->relatedSignalList = copySignalList(N, originSignal->relatedSignalList);
		kth++;
		signal = findKthSignal(N, kth);
	}
}

void
irPassInvariantSignalAnnotation(State * N)
{

    /*
     *  Loop through parameterList and attach a signal to each parameter node.
     *  Also add the baseNode and identifier to each signal.
     */
	attachSignalsToParameterNodes(N);


	/*
	 *	Look at each invariant expression, generate a list of signals used in the
	 *	expression, and add them as relatedSignalList to all signals in the expression.
	 *	Check for duplicates in the relatedSignalList.
	 */
	annotateSignalsInvariantConstraints(N);


	/*
 	 *	Copy the relatedSignalsList for the first instance
	 *	of each Signal with a particular identifier and axis
	 *	to every other instance of a Signal with the same identifier
	 *	and axis in the AST.
 	 */
	copyRelatedSignalsToAllSignals(N);



	/*
	 *	Code for debugging.
	 */

	
	char * identifier = "distance";
	int axis = 0;

	printf("%s %s %s%i \n", "The related signals are the following for the signal with identifier:", identifier, "and axis: ", axis);
	
	Signal * testSignal = findSignalByIdentifierAndAxis(N, identifier, axis);
	//Signal * testSignal = findKthSignalByIdentifier(N, identifier, 0);
	//printf("%i \n", testSignal->axis);
	

	testSignal = testSignal->relatedSignalList;
	testSignal = removeDuplicates(N, testSignal);
	printf("%s %i \n", testSignal->identifier, testSignal->axis);
	
	while(testSignal->relatedSignalListNext != NULL)
	{
		testSignal = testSignal->relatedSignalListNext;
		printf("%s %i \n", testSignal->identifier, testSignal->axis);
	}
	

}