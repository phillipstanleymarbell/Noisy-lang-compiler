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

*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <setjmp.h>

#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"

#include "common-errors.h"

#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"

#include "common-irHelpers.h"

Sensor *
irPassSensorsLoadSensor(State *  N, IrNode *  sensorNode)
{
	int		needed = 0;
	IrNode *	parameterTuple = NULL;

	Sensor *	sensor = calloc(1, sizeof(Sensor));
	if (sensor == NULL)
	{
		fatal(N, Emalloc);
	}

	sensor->baseNode = sensorNode;

	/*
	 *	Copy identifier string
	 */
	needed = snprintf(NULL, 0, "%s", sensorNode->irLeftChild->tokenString) + 1;
	sensor->identifier = malloc(needed);
	if (sensor->identifier == NULL)
	{
		fatal(N, Emalloc);
	}
	snprintf(sensor->identifier, needed, "%s", sensorNode->irLeftChild->tokenString);

	sensor->erasureToken = (uint16_t)0x00;	// TODO: Write actual value

	/*
	 *	Loop over sensor parameter list
	 */
	parameterTuple = RL(sensorNode);	/* The iterator for the loop */
	while (parameterTuple != NULL)
	{
		IrNode *  	parameter = parameterTuple->irLeftChild;
		Modality *	modality = calloc(1, sizeof(Modality));
		if (modality == NULL)
		{
			fatal(N, Emalloc);
		}
		printf("%p", parameter);
		if (parameter->irLeftChild->type != kNewtonIrNodeType_Tidentifier)
		{
			fatal(N, Esanity);
		}

		/*
		 *	Modality Identifier
		 */
		needed = snprintf(NULL, 0, "%s", parameter->irLeftChild->tokenString) + 1;
		modality->identifier = malloc(needed);
		if (modality->identifier == NULL)
		{
			fatal(N, Emalloc);
		}
		snprintf(modality->identifier, needed, "%s", parameter->irLeftChild->tokenString);


		/*
		 *	Modality Signal
		 */

		// modality->signal;

		IrNode *	tempNode = NULL;

		/*
		 *	Modality sensing range
		 */
		tempNode = NULL;
		int		n = 0;
		do
		{
			tempNode = findNthIrNodeOfType(N, sensorNode, kNewtonIrNodeType_PrangeStatement, n++);
			if (tempNode == NULL)
			{
				error(N, "no range statement found for modality");
				break;
			}
		} while (strcmp(modality->identifier, tempNode->irLeftChild->tokenString) != 0);
		if (tempNode != NULL)
		{
			modality->rangeLowerBound = RL(tempNode)->value;
			modality->rangeUpperBound = RR(R(tempNode))->value;
		}
		

		/*
		 *	Modality accuracy
		 */
		


		/*
		 *	Prepare iterator for next `while` iteration
		 */
		parameterTuple = parameterTuple->irRightChild;
	}



	// parameterTuple = RL(sensorNode);
	// propertyList = RRL(sensorNode);
	return sensor;
}

void
irPassSensors(State *  N)
{
	int		n = 0;
	IrNode *	sensorIrNode = NULL;
	Sensor *	sensorListLast = N->sensorList;

	while ((sensorIrNode = findNthIrNodeOfType(N, N->newtonIrRoot, kNewtonIrNodeType_PsensorDefinition, n++)) != NULL)
	{
		Sensor *	currentSensor = irPassSensorsLoadSensor(N, sensorIrNode);

		sensorListLast->next = currentSensor;
		sensorListLast = currentSensor;
	}

	return;
}

Sensor *
getSensorByIdentifier(State *  N, const char *  identifier)
{
	return NULL;
}

char *
irPassSensorsSensorToJSON(Sensor *  s)
{
	// TODO: Implement?
	return "";
}
