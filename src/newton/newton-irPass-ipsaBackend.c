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
#include "newton-irPass-invariantSignalAnnotation.h"

/*
 *  TODO: Add support for I2C write instructions.
 *  TODO: Add support for identifying sensor from signal definition.
 *  TODO: Add support for dimension index list.
 */


/* 
 *  Function searches AST and returns a sensor definition node
 *  with a matching identifier, generally corresponding to the
 *  name of the sensor (e.g. 'BMP180').
 */

IrNode * findSensorDefinitionByIdentifier(char * identifier, State * N, IrNode * noisyIrRoot)
{
   int nth;
   IrNode * sensorDefinition = NULL;
   for (nth=0; nth<10; nth++)
   {
       sensorDefinition = findNthIrNodeOfType(N, noisyIrRoot, kNewtonIrNodeType_PsensorDefinition, nth);
       
       if (!strcmp(sensorDefinition->irLeftChild->tokenString, identifier))
       {
           break;
       }
       else
       {

       }
   }

   if (sensorDefinition == NULL)
   {
       flexprint(N->Fe, N->Fm, N->Fperr, "ERROR: No matching sensor found.\n");
   }

   return sensorDefinition;
    
}


/* 
 *  Function finds and returns a sensor parameter name corresponding
 *  to a parameter identifier. For example, for the BMP180, the 
 *  parameter identifier 'temperature' would return 'bmp180temperature'.
 */

char * findSensorParameterNameByParameterIdentifier(char * identifier, State * N, IrNode * sensorDefinition)
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
        flexprint(N->Fe, N->Fm, N->Fperr, "ERROR: No matching sensor parameter found.\n");
    }
    
    return sensorParameterName;

}


/* 
 *  Function searches AST from the sensor definition root node
 *  and returns a sensor interface definition which matches the
 *  sensor parameter name used as the identifier.
 */

IrNode * findSensorInterface(char * identifier, State * N, IrNode * sensorDefinition)
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
        flexprint(N->Fe, N->Fm, N->Fperr, "ERROR: No matching sensor interface found.\n");
    }

    return sensorInterfaceStatement;
    
}


/* 
 *  Function finds and returns the I2C address associated with
 *  a given sensor interface statement node.
 */

int64_t findI2CAddress(State * N, IrNode * sensorInterfaceStatement, char* astNodeStrings[])
{
    int64_t i2cAddress = 0;
    
    IrNode * sensorInterfaceType = findNthIrNodeOfType(N, sensorInterfaceStatement, kNewtonIrNodeType_PsensorInterfaceType, 0);
    if (sensorInterfaceType->irLeftChild->type == kNewtonIrNodeType_Ti2c)
    {

        IrNode * integerConst = findNthIrNodeOfType(N, sensorInterfaceStatement->irRightChild, kNewtonIrNodeType_TintegerConst, 0);

        //printf("%s \n", astNodeStrings[sensorInterfaceStatement->irRightChild->irRightChild->type]);

        i2cAddress = integerConst->token->integerConst;
        //i2cAddress = sensorInterfaceStatement->irRightChild->irLeftChild->irRightChild->irRightChild->irLeftChild->irLeftChild->irRightChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild->irLeftChild->token->integerConst;
        //printf("%lld \n", i2cAddress);
        
    }
    else if (sensorInterfaceType->irLeftChild->type == kNewtonIrNodeType_Tspi)
    {
        flexprint(N->Fe, N->Fm, N->Fperr, "ERROR: Currently unsupported sensor interface type (SPI).\n");
    }
    else
    {
        flexprint(N->Fe, N->Fm, N->Fperr, "ERROR: Unrecognized sensor interface type.\n");
    }

    return i2cAddress;
}


/* 
 *  Function generates a list of I2C read and write commands
 *  with associated register address and write values for
 *  a given sensor interface statement node. '0' corresponds
 *  to a read command, '1' corresponds to a write command.
 */

int generateRWList(State * N, IrNode * sensorInterfaceStatement, char* astNodeStrings[], int *numberOfSensorInterfaceCommands, int64_t registerAddressList[10], int64_t writeValueList[10], char *rwList)
{
    int nth;
    for (nth=0; nth<10; nth++)
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

            registerAddressList[nth] = integerConst->token->integerConst;
            writeValueList[nth] = 1;
            *numberOfSensorInterfaceCommands = *numberOfSensorInterfaceCommands + 1;
        }
        else if (writeRegisterCommand != NULL)
        {
            rwList[nth] = 1;
            
            /*
             *  TODO: Need to take care of searching AST for writeRegisterAddress and writeValue.
             */
            IrNode * value = findNthIrNodeOfType(N, writeRegisterCommand, kNewtonIrNodeType_TintegerConst, 0);
            IrNode * registerAddr = findNthIrNodeOfType(N, writeRegisterCommand, kNewtonIrNodeType_TintegerConst, 1);
            registerAddressList[nth] = registerAddr->token->integerConst;
            writeValueList[nth] = value->token->integerConst;
            *numberOfSensorInterfaceCommands = *numberOfSensorInterfaceCommands + 1;
            
        }
        else
        {
            flexprint(N->Fe, N->Fm, N->Fperr, "ERROR: No I2C read or write command found.\n");
        }
        
    }

    return 0;
}


/* 
 *  Function to convert decimal to a binary representation.
 */
long long convertDecimalToBinary(int64_t n) {
    long long bin = 0;
    int rem, i = 1;
    while (n != 0) {
        rem = n % 2;
        n /= 2;
        bin += rem * i;
        i *= 10;
    }
    return bin;
}


/*
 *  Function to create an Ipsa I2C read instruction
 *  and write it to the "instruction_list.v" file.
 */

int create_read_instruction(int * instructionIndex, int64_t i2cAddr, int64_t regAddr, int64_t dimIndex, State * N, int physicalGroupNumber) {
    
    int64_t i2cAddrB = convertDecimalToBinary(i2cAddr);
    int64_t regAddrB = convertDecimalToBinary(regAddr);
    int64_t dimIndexB = convertDecimalToBinary(dimIndex);
    if(physicalGroupNumber == 1)
    {
        flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%d%s%08lld%s%08lld%s%05lld%s \n", "instr_mem_reg[", *instructionIndex, "] = 128'b00000101_", i2cAddrB, "_" , regAddrB, "_00000001_", dimIndexB, "_00000000_00000000_00000000_00000_000_00000000_00000000_00000000_00000000_00000000_00000000_00000000_001;");
    }
    else if(physicalGroupNumber == 2)
    {
        flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%d%s%08lld%s%08lld%s%05lld%s \n", "instr_mem_reg[", *instructionIndex, "] = 128'b00000101_00000000_00000000_00000000_00000_", i2cAddrB, "_" , regAddrB, "_00000001_", dimIndexB, "_000_00000000_00000000_00000000_00000000_00000000_00000000_00000000_010;");
    }
    

    return 0;
}


/*
 *  Function to create an Ipsa I2C write instruction
 *  and write it to the "instruction_list.v" file.
 */

int create_write_instruction(int * instructionIndex, int64_t i2cAddr, int64_t regAddr, int64_t immediate, State * N, int physicalGroupNumber) {
    

    int64_t i2cAddrB = convertDecimalToBinary(i2cAddr);
    int64_t regAddrB = convertDecimalToBinary(regAddr);
    int64_t immediateB = convertDecimalToBinary(immediate);
    
    if(physicalGroupNumber == 1)
    {
        flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%d%s%08lld%s%08lld%s%08lld%s%s \n", "instr_mem_reg[", *instructionIndex, "] = 128'b00000100_", i2cAddrB, "_" , regAddrB, "_", immediateB, "_", "00000_00000000_00000000_00000000_00000_000_00000000_00000000_00000000_00000000_00000000_00000000_00000000_001;");
    }
    else if(physicalGroupNumber == 2)
    {
        flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%d%s%08lld%s%08lld%s%08lld%s%s \n", "instr_mem_reg[", *instructionIndex, "] = 128'b00000100_00000000_00000000_00000000_00000", i2cAddrB, "_" , regAddrB, "_", immediateB, "_", "00000_000_00000000_00000000_00000000_00000000_00000000_00000000_00000000_010;");
    }
    

    return 0;
}


/*
 *  Function to create an "intstruction_list.v" file for Ipsa.
 */

int ipsa_create_instructions(char *rwList, int64_t i2cAddr, int64_t regAddrList[10], int64_t immediateList[10], int64_t dimIndex, int *numberOfSensorInterfaceCommands, State * N, int * instructionIndex, int physicalGroupNumber){

    int i;

    int i2cInterfaceSelect = 1;

    for (i=0; i<*numberOfSensorInterfaceCommands; i++) {
        /*
        flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%i%s \n", "instr_mem_reg[", *instructionIndex,"] = 32'b010_00000000000000010000000000000;");
        *instructionIndex = *instructionIndex + 1;
        */
        if (rwList[i] == 0){
            int64_t regAddr; 
            regAddr = regAddrList[i];
            create_read_instruction(instructionIndex, i2cAddr, regAddr, dimIndex, N, i2cInterfaceSelect);
            *instructionIndex = *instructionIndex + 1;
        }
        else if (rwList[i] == 1){
            int64_t regAddr; 
            regAddr = regAddrList[i];
            int64_t immediate;
            immediate = immediateList[i];
            create_write_instruction(instructionIndex, i2cAddr, regAddr, immediate, N, i2cInterfaceSelect);
            *instructionIndex = *instructionIndex + 1;
        }
        else {

        }
    }
    
    return 0;
}

/*
 *  TODO: Write new functions to parallel those above, but for the case that we want to create
 *  instructions for both I2C1 and I2C2. Or modify the ones above? I think the latter is probably
 *  a better option.
 */


/*
 *  Ipsa backend function.
 */
/*
void irPassIpsaBackend(State *  N, IrNode * noisyIrRoot, char* astNodeStrings[])
{
*/
    /*
     *  Print buffer contents to file.
     */
    /*
    FILE *instructionListFile;

    instructionListFile = fopen(N->outputIpsaFilePath,"w");

    if(instructionListFile == NULL)
    {
        flexprint(N->Fe, N->Fm, N->Fperr, "ERROR: Unable to open file.\n");
        exit(1);             
    }

    fprintf(instructionListFile, "%s", N->Fpipsa->circbuf);

    fclose(instructionListFile);
    


    //system("mv instruction_list.v ../../../Ipsa-core/verilog/toplevel/Instruction_Mem_Examples");

    //printf("%s \n", "Ipsa instruction list generated, Ipsa now ready for synthesis.");
    
}
*/






void
convert(int64_t decimal, char binary[16])
{
    int64_t temp = 0;
    temp = decimal & 1;
    binary[14] = temp + '0';
    int j = 13;

    for(int i=1; i<15; i++)
    {
        temp = (decimal >> i) & 1;
        binary[j] = temp + '0';
        j--;
    }
}


int
createSetupInstructions(State * N, int numberOfSignals, int *instructionIndex)
{
    int jumpToInstruction = 0;

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char s[64];
    assert(strftime(s, sizeof(s), "%c", tm));

    flexprint(N->Fe, N->Fm, N->Fpipsa, "%s %s %s \n", "/*This instruction list was generated by the Ipsa backend of the Newton compiler on", s, "*/");
    flexprint(N->Fe, N->Fm, N->Fpipsa, "%s \n", "initial begin");
    //  Initial Barrier Instruction.
    flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%i%s \n", "instr_mem_reg[", *instructionIndex,"] = 128'b00000010_00000000000000000000000000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000010_00000000_000;");
    *instructionIndex = *instructionIndex + 1;
    jumpToInstruction++;

    /*
     *  TODO: Look into more flexible memory allocation for Ipsa.
     */

    int64_t totalMemorySize = 32767;
    int64_t memoryChunkSize = totalMemorySize / numberOfSignals;

    //printf("%llu \n", memoryChunkSize);

    int64_t memoryStart = 0;
    int64_t memoryEnd = memoryStart + memoryChunkSize;
    char memoryStartB[16];
    char memoryEndB[16];

    for(int dimIndex=1; dimIndex<=numberOfSignals; dimIndex++)
    {
        convert(memoryStart, memoryStartB);
        convert(memoryEnd, memoryEndB);
        Signal * signal = findKthSignal(N, dimIndex - 1);
        signal->dimensionIndex = dimIndex;
        int64_t dimIndexB = convertDecimalToBinary(dimIndex);
        flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%i%s%s%s%05llu%s%s%s \n", "instr_mem_reg[", *instructionIndex,"] = 128'b00000001_0000_", memoryStartB,"_00001_", dimIndexB,"_", memoryEndB,"_0_00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000_00000000_000;");
        *instructionIndex = *instructionIndex + 1;
        jumpToInstruction++;
        memoryStart = memoryEnd + 1;
        memoryEnd = memoryStart + memoryChunkSize;
    }

    return jumpToInstruction;
    
}

int
countAllSignals(State * N)
{
    int count = 0;
    Signal * signal = findKthSignal(N, count);

    if(signal == NULL)
    {
        return count;
    }
    while(signal != NULL)
    {
        count++;
        signal = findKthSignal(N, count);
    }

    return count;    
}


int
countSignalsInList(State * N, Signal * signalList)
{
    while(signalList->relatedSignalListPrev != NULL)
    {
        signalList = signalList->relatedSignalListPrev;
    }

    int count = 0;
    if(signalList == NULL)
    {
        return 0;
    }
    while(signalList != NULL)
    {
        count++;
        if(signalList->relatedSignalListNext == NULL)
        {
            break;
        }
        signalList = signalList->relatedSignalListNext;
    }

    while(signalList->relatedSignalListPrev != NULL)
    {
        signalList = signalList->relatedSignalListPrev;
    }

    return count;
}


Signal *
getKthSignalFromList(State * N, Signal * signalList, int kth)
{
    Signal * signal = NULL;
    for(int i=0; i<=kth; i++)
    {
        if(i == 0)
        {
            
        } else {
            if(signalList->relatedSignalListNext == NULL)
            {
                return NULL;
            }
            signalList = signalList->relatedSignalListNext;
        }
        

    }

    signal = signalList;

    return signal;
    
}


Signal *
randomizeSignalList(State * N, Signal * signalList)
{
    Signal * randomizedList = (Signal *) calloc(1, sizeof(Signal));
    Signal * nextSignal = (Signal *) calloc(1, sizeof(Signal));

    int numberOfSignals = countSignalsInList(N, signalList);
    int positions[numberOfSignals];

    for(int i=0; i<numberOfSignals; i++)
    {
        bool check = true;
        int position = 0;
        while(check)
        {
            //  TODO: Look into better random number generation.
            position = rand() % numberOfSignals;
            bool check2 = false;
            int j = 0;
            for(j=0; j<i; j++)
            {
                if(positions[j] == position)
                {
                    check2 = true;
                    break;
                }
            }
            if(!check2)
            {
                check = false;
                break;
            }
        }
        positions[i] = position;
    }

    for(int i=0; i<numberOfSignals; i++)
    {
        if(i == 0)
        {
            shallowCopySignal(N, getKthSignalFromList(N, signalList, positions[i]), randomizedList);
        } else {
            shallowCopySignal(N, getKthSignalFromList(N, signalList, positions[i]), nextSignal);
            randomizedList->relatedSignalListNext = nextSignal;
            nextSignal->relatedSignalListPrev = randomizedList;
            randomizedList = nextSignal;
        }
    }

    while(randomizedList->relatedSignalListPrev != NULL)
    {
        randomizedList = randomizedList->relatedSignalListPrev;
    }

    return randomizedList;
}


void 
createI2CInstructionsForSignal(State * N, Signal * signal, char* astNodeStrings[], int * instructionIndex)
{
    int numberOfSensorInterfaceCommands = 0;
    int64_t registerAddressList[10];
    int64_t writeValueList[10];

    
    IrNode * sensorDef = findSensorDefinitionByIdentifier(signal->sensorIdentifier, N, N->newtonIrRoot);
    
    
    //  Need to change to take into account axis.
    char * sensorParameterName = findSensorParameterNameByParameterIdentifier(signal->identifier, N, sensorDef);

    IrNode * sensorInterfaceStatement = findSensorInterface(sensorParameterName, N, sensorDef);
    
    
    int64_t i2cAddress = findI2CAddress(N, sensorInterfaceStatement, astNodeStrings);

    
    char rwList[10];
    generateRWList(N, sensorInterfaceStatement, astNodeStrings, &numberOfSensorInterfaceCommands, registerAddressList, writeValueList, rwList);
    

    ipsa_create_instructions(rwList, i2cAddress, registerAddressList, writeValueList, signal->dimensionIndex, &numberOfSensorInterfaceCommands, N, instructionIndex, signal->physicalGroupNumber);

}


void
createI2CInstructionsForSignals(State * N, Signal * firstSignal, Signal * secondSignal, char* astNodeStrings[], int * instructionIndex)
{

}


void
createFinalInstructions(State * N, char* astNodeStrings[], int jumpToInstruction, int * instructionIndex)
{
    int64_t jumpToInstructionB = convertDecimalToBinary(jumpToInstruction);
    flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%d%s%0120lld%s \n", "instr_mem_reg[", *instructionIndex, "] = 128'b00000110_", jumpToInstructionB, ";");
    flexprint(N->Fe, N->Fm, N->Fpipsa, "%s \n", "end");
}


void
irPassIpsaBackend(State * N, char* astNodeStrings[])
{
    int instructionIndex = 0;
    int jumpToInstruction = createSetupInstructions(N, countAllSignals(N), &instructionIndex);

    Signal * signal = findKthSignal(N, 0);
    Signal * relatedSignalList = signal->relatedSignalList;

    Signal * physicalGroup1;
    Signal * physicalGroup2;

    int count1 = 0;
    int count2 = 0;

    while(relatedSignalList != NULL)
    {
        if(relatedSignalList->physicalGroupNumber == 1)
        {
            if(count1 == 0)
            {
                physicalGroup1 = (Signal *) calloc(1, sizeof(Signal));
                shallowCopySignal(N, relatedSignalList, physicalGroup1);
                count1++;
            } else {
                Signal * nextSignal1 = (Signal *) calloc(1, sizeof(Signal));
                shallowCopySignal(N, relatedSignalList, nextSignal1);
                physicalGroup1->relatedSignalListNext = nextSignal1;
                nextSignal1->relatedSignalListPrev = physicalGroup1;
                physicalGroup1 = nextSignal1;
            }
        }
        if(relatedSignalList->physicalGroupNumber == 2)
        {
            if(count2 == 0)
            {
                physicalGroup2 = (Signal *) calloc(1, sizeof(Signal));
                shallowCopySignal(N, relatedSignalList, physicalGroup2);
                count2++;
            } else {
                Signal * nextSignal2 = (Signal *) calloc(1, sizeof(Signal));
                shallowCopySignal(N, relatedSignalList, nextSignal2);
                physicalGroup2->relatedSignalListNext = nextSignal2;
                nextSignal2->relatedSignalListPrev = physicalGroup2;
                physicalGroup2 = nextSignal2;
            }
        }

        if(relatedSignalList->relatedSignalListNext == NULL)
        {
            break;
        }
        relatedSignalList = relatedSignalList->relatedSignalListNext;
    }


    while(relatedSignalList->relatedSignalListPrev != NULL)
    {
        relatedSignalList = relatedSignalList->relatedSignalListPrev;
    }

    if(physicalGroup1 != NULL)
    {
        while(physicalGroup1->relatedSignalListPrev != NULL)
        {
            physicalGroup1 = physicalGroup1->relatedSignalListPrev;
        }

    }

    if(physicalGroup2 != NULL)
    {
        while(physicalGroup2->relatedSignalListPrev != NULL)
        {
            physicalGroup2 = physicalGroup2->relatedSignalListPrev;
        }

    }
    

    if(physicalGroup1 != NULL && physicalGroup2 == NULL)
    {
        Signal * randomGroup1 = randomizeSignalList(N, physicalGroup1);

        while(randomGroup1 != NULL)
        {
            createI2CInstructionsForSignal(N, randomGroup1, astNodeStrings, &instructionIndex);
            if(randomGroup1->relatedSignalListNext == NULL)
            {
                break;
            }
            randomGroup1 = randomGroup1->relatedSignalListNext;
        }
        

        createFinalInstructions(N, astNodeStrings, jumpToInstruction, &instructionIndex);


    }
    else if(physicalGroup1 == NULL && physicalGroup2 != NULL)
    {

        Signal * randomGroup2 = randomizeSignalList(N, physicalGroup2);

        while(randomGroup2 != NULL)
        {
            createI2CInstructionsForSignal(N, randomGroup2, astNodeStrings, &instructionIndex);
            if(randomGroup2->relatedSignalListNext == NULL)
            {
                break;
            }
            randomGroup2 = randomGroup2->relatedSignalListNext;
        }
        

        createFinalInstructions(N, astNodeStrings, jumpToInstruction, &instructionIndex);        

    }
    else if(physicalGroup1 != NULL && physicalGroup2 != NULL)
    {

    }
    

    

    
}