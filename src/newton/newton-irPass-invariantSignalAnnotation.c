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
 *	Function for making a copy of an instance
 *	of the Signal struct.
 */
Signal *
copySignal(Signal * signal)
{
	Signal * copyOfSignal = (Signal *) calloc(1, sizeof(Signal));
	copyOfSignal->baseNode = signal->baseNode;
	copyOfSignal->identifier = signal->identifier;
	copyOfSignal->invariantExpressionIdentifier = signal->invariantExpressionIdentifier;
	copyOfSignal->sensorIdentifier = signal->sensorIdentifier;
	copyOfSignal->physicalGroupNumber = signal->physicalGroupNumber;

	return copyOfSignal;
}


/*
 *	TODO: Unused.
 */
Signal *
copySignalWithRelatedSignalList(Signal * signal)
{
	Signal * copyOfSignal = (Signal *) calloc(1, sizeof(Signal));
	copyOfSignal->baseNode = signal->baseNode;
	copyOfSignal->identifier = signal->identifier;
	copyOfSignal->invariantExpressionIdentifier = signal->invariantExpressionIdentifier;
	copyOfSignal->sensorIdentifier = signal->sensorIdentifier;
	copyOfSignal->physicalGroupNumber = signal->physicalGroupNumber;
	copyOfSignal->relatedSignalList = signal->relatedSignalList;
	copyOfSignal->relatedSignalListNext = signal->relatedSignalListNext;
	copyOfSignal->relatedSignalListPrev = signal->relatedSignalListPrev;

	return copyOfSignal;
}

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
			//printf("%s \n", signal->identifier);
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
		
		//invariant = invariant->next;
	}

	if(strcmp(signal->identifier, identifier) != 0)
	{
		printf("%s%s \n", "No signal found with identifier: ", identifier);
	}

	invariant = N->invariantList;
	return signal;
}

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
			//printf("%s \n", signal->identifier);
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
		
		//invariant = invariant->next;
	}

	if(strcmp(signal->invariantExpressionIdentifier, identifier) != 0)
	{
		printf("%s%s \n", "No signal found with identifier: ", identifier);
	}

	invariant = N->invariantList;
	return signal;
}

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

int
annotateSignals(State * N, char* astNodeStrings[])
{
	Invariant * invariant = N->invariantList;

	while(invariant)
	{
		IrNode * invariantDefinition = invariant->parameterList->irParent->irParent;
		IrNode * constraintList = findNthIrNodeOfType(N, invariantDefinition, kNewtonIrNodeType_PconstraintList, 0);
		int kth = 0;
		printf("%i \n", kth);
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

			printf("%s \n", invariantExpressionIdentifier);

			Signal * baseSignal = findSignalByInvariantExpressionIdentifier(N, invariantExpressionIdentifier, astNodeStrings);
			signal->baseNode = baseSignal->baseNode;
			signal->identifier = baseSignal->identifier;
			
		
			//printf("%s \n", invariantExpressionIdentifier);

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
			
			

				//printf("%s \n", invariantExpressionIdentifier);
				//printf("%s \n", astNodeStrings[identifierNode->irRightChild->type]);


				nth++;
				identifierNode = findNthIrNodeOfType(N, constraint, kNewtonIrNodeType_Tidentifier, nth);
			}


			Signal * headSignal = signal;
			while(headSignal->relatedSignalListPrev != NULL)
			{
				headSignal = headSignal->relatedSignalListPrev;
			}

			//printf("%s \n", headSignal->relatedSignalListNext->relatedSignalListNext->identifier);
	
	
			int i = 0;
			int j = 0;

	
			while(headSignal != NULL)
			{
		
				while(headSignal->relatedSignalListPrev != NULL)
				{
					headSignal = headSignal->relatedSignalListPrev;
				}
				for(j=0; j<i; j++)
				{
					headSignal = headSignal->relatedSignalListNext;
				}
				if(headSignal == NULL)
				{
					break;
				}
		
				Signal * baseSignal = findSignalByIdentifier(N, headSignal->identifier, astNodeStrings);


				Signal * baseRelatedSignals = baseSignal->relatedSignalList;
				
				Signal * currentRelatedSignal;
				Signal * nextRelatedSignal;
				if(baseSignal->relatedSignalList == NULL)
				{
					//This deals with adding signals to the baseSignal related signal list if the related signal list is empty.

					while(headSignal->relatedSignalListPrev != NULL)
					{
						headSignal = headSignal->relatedSignalListPrev;
					}


					while(headSignal != NULL)
					{
						//currentRelatedSignal = copySignal(headSignal);
						//nextRelatedSignal = copySignal(headSignal->relatedSignalListNext);
						if(!strcmp(headSignal->identifier, baseSignal->identifier))
						{
							//Don't add to list.
						} else {
							currentRelatedSignal = copySignal(headSignal);
							//printf("%s \n", "Copied current related signal.");
							//printf("%s \n", headSignal->identifier);
							headSignal = headSignal->relatedSignalListNext;
							break;
						}
						headSignal = headSignal->relatedSignalListNext;
					}

					while(headSignal != NULL)
					{
						if(!strcmp(headSignal->identifier, baseSignal->identifier))
						{
							//Don't add to list.
						} else {
							//printf("%s \n", "Adding next signal.");
							//printf("%s \n", headSignal->identifier);
							nextRelatedSignal = copySignal(headSignal);
							currentRelatedSignal->relatedSignalListNext = nextRelatedSignal;
							nextRelatedSignal->relatedSignalListPrev = currentRelatedSignal;
							currentRelatedSignal = nextRelatedSignal;
						}
						if(headSignal->relatedSignalListNext == NULL)
						{
							break;
						}
						headSignal = headSignal->relatedSignalListNext;
					}

					while(currentRelatedSignal->relatedSignalListPrev != NULL)
					{
						currentRelatedSignal = currentRelatedSignal->relatedSignalListPrev;
					}
					//printf("%s \n", currentRelatedSignal->identifier);
					//printf("%s \n", headSignal->identifier);
					baseSignal->relatedSignalList = currentRelatedSignal;
				} else {
					//Do this if the baseSignal already has items in its related signal list.
					//TODO: Still need to implement and test this.
					
					printf("%s \n", "else:");
					printf("%s \n", baseSignal->identifier);

					
					while(headSignal->relatedSignalListPrev != NULL)
					{
						headSignal = headSignal->relatedSignalListPrev;
					}

					
					int count = 0;

					while(headSignal != NULL)
					{
						int match = 0;

						while(baseRelatedSignals != NULL)
						{
							if(!strcmp(headSignal->identifier, baseSignal->identifier) || !strcmp(headSignal->identifier, baseRelatedSignals->identifier))
							{
								printf("%s \n", "Match!");
								match = 1;
							}

							if(baseRelatedSignals->relatedSignalListNext == NULL)
							{
								break;
							}
							baseRelatedSignals = baseRelatedSignals->relatedSignalListNext;
						}
						if(match == 0)
						{
							if(count == 0)
							{
								currentRelatedSignal = copySignal(headSignal);
								//printf("%s \n", "Copied current related signal.");
								//printf("%s \n", headSignal->identifier);
								headSignal = headSignal->relatedSignalListNext;
								count++;
							} else {
								nextRelatedSignal = copySignal(headSignal);
								currentRelatedSignal->relatedSignalListNext = nextRelatedSignal;
								nextRelatedSignal->relatedSignalListPrev = currentRelatedSignal;
								currentRelatedSignal = nextRelatedSignal;
								count++;
							}
							printf("%s \n", currentRelatedSignal->identifier);
						}
						printf("%s \n", "test");
						while(baseRelatedSignals->relatedSignalListPrev != NULL)
						{
							printf("%s \n", "test2");
							baseRelatedSignals = baseRelatedSignals->relatedSignalListPrev;
						}
						if(headSignal->relatedSignalListNext == NULL)
						{
							printf("%s \n", "test3");
							break;
						}
						headSignal = headSignal->relatedSignalListNext;
					}

					Signal * tempBase = baseSignal->relatedSignalList;
					Signal * baseTailCurrent;
					Signal * baseTailNext;

					printf("%s \n", tempBase->identifier);

					int i = 0;
					while(tempBase != NULL)
					{
						if(i == 0)
						{
							baseTailCurrent = copySignal(tempBase);
							i++;
						} else {
							baseTailNext = copySignal(tempBase);
							baseTailCurrent->relatedSignalListNext = baseTailNext;
							baseTailNext->relatedSignalListPrev = baseTailCurrent;
							baseTailCurrent = baseTailNext;
						}
						if(tempBase->relatedSignalListNext == NULL)
						{
							break;
						}
						tempBase = tempBase->relatedSignalListNext;
					}

					baseTailCurrent->relatedSignalListNext = currentRelatedSignal;
					while(baseTailCurrent->relatedSignalListPrev != NULL)
					{
						baseTailCurrent = baseTailCurrent->relatedSignalListPrev;
					}
					baseSignal->relatedSignalList = baseTailCurrent;
					

				}

				while(headSignal->relatedSignalListPrev != NULL)
				{
					headSignal = headSignal->relatedSignalListPrev;
				}

				i++;
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
    //printf("%s \n", "Made it to the invariant signal annotation pass!");

	
    Invariant * invariant = N->invariantList;

    //IrNode * parameterList = invariant->parameterList;

    /*
     *  Loop through parameterList and attach a signal to each parameter node.
     *  Also add the baseNode and identifier to each signal.
     */
	attachSignalsToParameterNodes(N, astNodeStrings);

	/*
	 *	Look at each invariant expression, generate a list of signals used in the
	 *	expression, and add them as relatedSignalList to all signals in the expression.
	 *	Check for duplicates in the relatedSignalList.
	 */
	annotateSignals(N, astNodeStrings);


	invariant = N->invariantList;


	printf("%s \n", "Test");
	Signal * testSignal = findSignalByIdentifier(N, "distance", astNodeStrings);
	printf("%s \n", testSignal->relatedSignalList->identifier);
	printf("%s \n", testSignal->relatedSignalList->relatedSignalListNext->identifier);
	printf("%s \n", testSignal->relatedSignalList->relatedSignalListNext->relatedSignalListNext->identifier);
	printf("%s \n", testSignal->relatedSignalList->relatedSignalListNext->relatedSignalListNext->relatedSignalListNext->identifier);
	printf("%s \n", testSignal->relatedSignalList->relatedSignalListNext->relatedSignalListNext->relatedSignalListNext->relatedSignalListNext->identifier);



}