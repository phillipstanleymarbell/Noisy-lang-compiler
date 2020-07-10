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
#include <math.h>

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


int numberOfSensorInterfaceCommands = 0;
int64_t readRegisterAddressList[5];
int64_t writeRegisterAddressList[5];
int64_t writeValueList[5]; 


IrNode * findSensorDefinitionByIdentifier(char * identifier, State * N, IrNode * noisyIrRoot, Scope *  noisyIrTopScope)
{
   int nth;
   IrNode * sensorDefinition = NULL;
   for (nth=0; nth<10; nth++)
   {
       sensorDefinition = findNthIrNodeOfType(N, noisyIrRoot, kNewtonIrNodeType_PsensorDefinition, nth);
       
       if (!strcmp(sensorDefinition->irLeftChild->tokenString, identifier))
       {
           //printf("%s \n", "test");
           break;
       }
       else
       {
           //printf("%s \n", "test2");
       }
   }

   if (sensorDefinition == NULL)
   {
       printf("%s \n", "ERROR: No matching sensor found.");
   }

   return sensorDefinition;
    
}


char * findSensorParameterNameByParameterIdentifier(char * identifier, State * N, IrNode * sensorDefinition, Scope *  noisyIrTopScope)
{
    char * sensorParameterName = NULL;

    int nth;
    IrNode * parameterTuple;
    for (nth=0; nth<10; nth++)
    {
        parameterTuple = findNthIrNodeOfType(N, sensorDefinition, kNewtonIrNodeType_PparameterTuple, nth);
        if (!strcmp(parameterTuple->irLeftChild->irRightChild->tokenString, identifier))
        {
            sensorParameterName = parameterTuple->irLeftChild->irLeftChild->tokenString;
            break;
        }
    }

    if (sensorParameterName == NULL)
    {
        printf("%s \n", "ERROR: No matching sensor parameter found.");
    }
    
    return sensorParameterName;

}


IrNode * findSensorInterface(char * identifier, State * N, IrNode * sensorDefinition, Scope * noisyIrTopScope)
{
    
    IrNode * sensorInterfaceStatement = NULL;
    int nth;
    for (nth=0; nth<10; nth++)
    {
        sensorInterfaceStatement = findNthIrNodeOfType(N, sensorDefinition, kNewtonIrNodeType_PsensorInterfaceStatement, nth);

        if (!strcmp(sensorInterfaceStatement->irLeftChild->tokenString, identifier))
        {
            break;
        }
    }

    if (sensorInterfaceStatement == NULL)
    {
        printf("%s \n", "ERROR: No matching sensor interface found.");
    }

    return sensorInterfaceStatement;
    
}


int64_t findI2CAddress(State * N, IrNode * sensorInterfaceStatement, Scope * noisyIrTopScope, char* astNodeStrings[])
{
    int64_t i2cAddress = 0;
    
    IrNode * sensorInterfaceType = findNthIrNodeOfType(N, sensorInterfaceStatement, kNewtonIrNodeType_PsensorInterfaceType, 0);
    if (sensorInterfaceType->irLeftChild->type == kNewtonIrNodeType_Ti2c)
    {
        printf("%s \n", "Test");
        IrNode * integerConst = findNthIrNodeOfType(N, sensorInterfaceStatement->irRightChild, kNewtonIrNodeType_TintegerConst, 0);

        //printf("%s \n", astNodeStrings[sensorInterfaceStatement->irRightChild->irRightChild->type]);

        i2cAddress = integerConst->token->integerConst;
        //i2cAddress = sensorInterfaceStatement->irRightChild->irLeftChild->irRightChild->irRightChild->irLeftChild->irLeftChild->irRightChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild->token->integerConst;
        //printf("%lld \n", i2cAddress);
        
    }
    else if (sensorInterfaceType->irLeftChild->type == kNewtonIrNodeType_Tspi)
    {
        printf("%s \n", "ERROR: Currently unsupported sensor interface type (SPI).");
    }
    else
    {
        printf("%s \n", "ERROR: Unrecognized sensor interface type.");
    }

    return i2cAddress;
}



int * generateRWList(State * N, IrNode * sensorInterfaceStatement, Scope * noisyIrTopScope, char* astNodeStrings[])
{
    static int rwList[5];
    int nth;
    for (nth=0; nth<5; nth++)
    {
        IrNode * sensorInterfaceCommand = findNthIrNodeOfType(N, sensorInterfaceStatement, kNewtonIrNodeType_PsensorInterfaceCommand, nth);
        if (sensorInterfaceCommand == NULL)
        {
            break;
        }
        IrNode * readRegisterCommand = findNthIrNodeOfType(N, sensorInterfaceCommand, kNewtonIrNodeType_PreadRegisterCommand, 0);
        IrNode * writeRegisterCommand = findNthIrNodeOfType(N, sensorInterfaceCommand, kNewtonIrNodeType_PwriteRegisterCommand, 0);
        if (readRegisterCommand != NULL)
        {
            rwList[nth] = 0;
            IrNode * integerConst = findNthIrNodeOfType(N, readRegisterCommand, kNewtonIrNodeType_TintegerConst, 0);

            readRegisterAddressList[nth] = integerConst->token->integerConst;
            writeRegisterAddressList[nth] = 1;
            writeValueList[nth] = 1;
            numberOfSensorInterfaceCommands++;
        }
        else if (writeRegisterCommand != NULL)
        {
            rwList[nth] = 1;
            
            /*
             * TODO: Need to take care of searching AST for writeRegisterAddress and writeValue.
             */

            readRegisterAddressList[nth] = 1;
            writeRegisterAddressList[nth] = 1;
            writeValueList[nth] = 1;
            numberOfSensorInterfaceCommands++;
            
        }
        else
        {
            printf("%s \n", "ERROR: No I2C read or write command found.");
        }
        
    }
    //IrNode * sensorInterfaceCommand = findNthIrNodeOfType(N, sensorInterfaceStatement, kNewtonIrNodeType_PsensorInterfaceCommand, nth);
    //printf("%s \n", astNodeStrings[sensorInterfaceCommand->type]);

    return rwList;
}



void irPassIpsaBackend(State *  N, Scope *  noisyIrTopScope, IrNode * noisyIrRoot, char* astNodeStrings[])
{
    printf("%s \n", "Made it to the Ipsa backend!");

    IrNode * sensorDef = findSensorDefinitionByIdentifier("BMP180", N, noisyIrRoot, noisyIrTopScope);
    //printf("%s \n", astNodeStrings[sensorDef->irRightChild->irLeftChild->irLeftChild->irRightChild->type]);
    //printf("%s \n", sensorDef->irRightChild->irLeftChild->irLeftChild->irRightChild->tokenString);

    char * sensorParameterName = findSensorParameterNameByParameterIdentifier("temperature", N, sensorDef, noisyIrTopScope);
    //printf("%s \n", sensorParameterName);

    IrNode * sensorInterfaceStatement = findSensorInterface(sensorParameterName, N, sensorDef, noisyIrTopScope);
    //printf("%s \n", sensorInterfaceStatement->irLeftChild->tokenString);

    int64_t i2cAddress = findI2CAddress(N, sensorInterfaceStatement, noisyIrTopScope, astNodeStrings);
    printf("%llu \n", i2cAddress);

    int *rwList;
    rwList = generateRWList(N, sensorInterfaceStatement, noisyIrTopScope, astNodeStrings);
    printf("%i \n", rwList[1]);
    //printf("%i \n", numberOfSensorInterfaceCommands);
    printf("%lld \n", readRegisterAddressList[1]);
    printf("%lld \n", writeRegisterAddressList[1]);
    printf("%lld \n", writeValueList[1]);
    


}