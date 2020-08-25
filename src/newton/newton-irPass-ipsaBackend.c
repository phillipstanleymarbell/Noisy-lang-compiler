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

char * findSensorParameterNameByParameterIdentifierAndAxis(char * identifier, int axis, State * N, IrNode * sensorDefinition)
{
    char * sensorParameterName = NULL;

    int nth;
    IrNode * parameter;
    for (nth=0; nth<10; nth++)
    {
        parameter = findNthIrNodeOfType(N, sensorDefinition, kNewtonIrNodeType_Pparameter, nth);
        if (strcmp(parameter->irRightChild->tokenString, identifier) == 0 && parameter->signal->axis == axis)
        {
            sensorParameterName = parameter->irLeftChild->tokenString;
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
        flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%d%s%08lld%s%08lld%s%08lld%s%s \n", "instr_mem_reg[", *instructionIndex, "] = 128'b00000100_00000000_00000000_00000000_00000_", i2cAddrB, "_" , regAddrB, "_", immediateB, "_", "00000_000_00000000_00000000_00000000_00000000_00000000_00000000_00000000_010;");
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

int ipsa_create_read_instructions(char *rwList, int64_t i2cAddr, int64_t regAddrList[10], int64_t immediateList[10], int64_t dimIndex, int *numberOfSensorInterfaceCommands, State * N, int * instructionIndex, int physicalGroupNumber){

    int i;

    for (i=0; i<*numberOfSensorInterfaceCommands; i++) {
        /*
        flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%i%s \n", "instr_mem_reg[", *instructionIndex,"] = 32'b010_00000000000000010000000000000;");
        *instructionIndex = *instructionIndex + 1;
        */
        if (rwList[i] == 0){
            int64_t regAddr; 
            regAddr = regAddrList[i];
            create_read_instruction(instructionIndex, i2cAddr, regAddr, dimIndex, N, physicalGroupNumber);
            *instructionIndex = *instructionIndex + 1;
        }
        else if (rwList[i] == 1){
            /*
            int64_t regAddr; 
            regAddr = regAddrList[i];
            int64_t immediate;
            immediate = immediateList[i];
            create_write_instruction(instructionIndex, i2cAddr, regAddr, immediate, N, i2cInterfaceSelect);
            *instructionIndex = *instructionIndex + 1;
            */
        }
        else {

        }
    }
    
    return 0;
}


int ipsa_create_write_instructions(char *rwList, int64_t i2cAddr, int64_t regAddrList[10], int64_t immediateList[10], int64_t dimIndex, int *numberOfSensorInterfaceCommands, State * N, int * instructionIndex, int physicalGroupNumber){

    int i;


    for (i=0; i<*numberOfSensorInterfaceCommands; i++) {
        /*
        flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%i%s \n", "instr_mem_reg[", *instructionIndex,"] = 32'b010_00000000000000010000000000000;");
        *instructionIndex = *instructionIndex + 1;
        */
        if (rwList[i] == 0){
            /*
            int64_t regAddr; 
            regAddr = regAddrList[i];
            create_read_instruction(instructionIndex, i2cAddr, regAddr, dimIndex, N, i2cInterfaceSelect);
            *instructionIndex = *instructionIndex + 1;
            */
        }
        else if (rwList[i] == 1){
            int64_t regAddr; 
            regAddr = regAddrList[i];
            int64_t immediate;
            immediate = immediateList[i];
            create_write_instruction(instructionIndex, i2cAddr, regAddr, immediate, N, physicalGroupNumber);
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
 *  Function to create parallel Ipsa I2C read instructions
 *  and write it to the "instruction_list.v" file.
 */
int create_parallel_read_instruction(int * instructionIndex, int64_t i2cAddr1, int64_t regAddr1, int64_t dimIndex1, int physicalGroupNumber1, int64_t i2cAddr2, int64_t regAddr2, int64_t dimIndex2, int physicalGroupNumber2, State * N) {
    
    int64_t i2cAddrB1 = convertDecimalToBinary(i2cAddr1);
    int64_t regAddrB1 = convertDecimalToBinary(regAddr1);
    int64_t dimIndexB1 = convertDecimalToBinary(dimIndex1);
    int64_t i2cAddrB2 = convertDecimalToBinary(i2cAddr2);
    int64_t regAddrB2 = convertDecimalToBinary(regAddr2);
    int64_t dimIndexB2 = convertDecimalToBinary(dimIndex2);

    flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%d%s%08lld%s%08lld%s%05lld%s%08lld%s%08lld%s%05lld%s \n", "instr_mem_reg[", *instructionIndex, "] = 128'b00000101_", i2cAddrB1, "_" , regAddrB1, "_00000001_", dimIndexB1, "_", i2cAddrB2, "_" , regAddrB2, "_00000001_", dimIndexB2, "_000_00000000_00000000_00000000_00000000_00000000_00000000_00000000_011;");
    
    return 0;
}


/*
 *  Function to create parallel Ipsa I2C write instructions
 *  and write it to the "instruction_list.v" file.
 */
int create_parallel_write_instruction(int * instructionIndex, int64_t i2cAddr1, int64_t regAddr1, int64_t immediate1, int physicalGroupNumber1, int64_t i2cAddr2, int64_t regAddr2, int64_t immediate2, int physicalGroupNumber2, State * N) {
    

    int64_t i2cAddrB1 = convertDecimalToBinary(i2cAddr1);
    int64_t regAddrB1 = convertDecimalToBinary(regAddr1);
    int64_t immediateB1 = convertDecimalToBinary(immediate1);
    int64_t i2cAddrB2 = convertDecimalToBinary(i2cAddr2);
    int64_t regAddrB2 = convertDecimalToBinary(regAddr2);
    int64_t immediateB2 = convertDecimalToBinary(immediate2);

    flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%d%s%08lld%s%08lld%s%08lld%s%08lld%s%08lld%s%08lld%s \n", "instr_mem_reg[", *instructionIndex, "] = 128'b00000100_", i2cAddrB1, "_" , regAddrB1, "_", immediateB1, "_00000_", i2cAddrB2, "_" , regAddrB2, "_", immediateB2, "_00000_000_00000000_00000000_00000000_00000000_00000000_00000000_00000000_011;");
    
    return 0;
}



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

void
convert50(int64_t decimal, char binary[50])
{
    int64_t temp = 0;
    temp = decimal & 1;
    binary[48] = temp + '0';
    int j = 47;

    for(int i=1; i<49; i++)
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

    flexprint(N->Fe, N->Fm, N->Fpipsa, "%s %s %s \n", "/* This instruction list was generated by the Ipsa backend of the Newton compiler on", s, "*/");
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
            //  TODO: Look into better random number generation. Random number generation slows down execution significantly.
            srand(time(NULL));
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
createWriteI2CInstructionsForSignal(State * N, Signal * signal, char* astNodeStrings[], int * instructionIndex)
{
    int numberOfSensorInterfaceCommands = 0;
    int64_t registerAddressList[10];
    int64_t writeValueList[10];

    
    IrNode * sensorDef = findSensorDefinitionByIdentifier(signal->sensorIdentifier, N, N->newtonIrRoot);
    
    
    //  Need to change to take into account axis.
    char * sensorParameterName = findSensorParameterNameByParameterIdentifierAndAxis(signal->identifier, signal->axis, N, sensorDef);

    IrNode * sensorInterfaceStatement = findSensorInterface(sensorParameterName, N, sensorDef);
    
    
    int64_t i2cAddress = findI2CAddress(N, sensorInterfaceStatement, astNodeStrings);

    
    char rwList[10];
    generateRWList(N, sensorInterfaceStatement, astNodeStrings, &numberOfSensorInterfaceCommands, registerAddressList, writeValueList, rwList);
    

    ipsa_create_write_instructions(rwList, i2cAddress, registerAddressList, writeValueList, signal->dimensionIndex, &numberOfSensorInterfaceCommands, N, instructionIndex, signal->physicalGroupNumber);

}


void 
createReadI2CInstructionsForSignal(State * N, Signal * signal, char* astNodeStrings[], int * instructionIndex)
{
    int numberOfSensorInterfaceCommands = 0;
    int64_t registerAddressList[10];
    int64_t writeValueList[10];

    
    IrNode * sensorDef = findSensorDefinitionByIdentifier(signal->sensorIdentifier, N, N->newtonIrRoot);
    
    
    //  Need to change to take into account axis.
    char * sensorParameterName = findSensorParameterNameByParameterIdentifierAndAxis(signal->identifier, signal->axis, N, sensorDef);

    IrNode * sensorInterfaceStatement = findSensorInterface(sensorParameterName, N, sensorDef);
    
    
    int64_t i2cAddress = findI2CAddress(N, sensorInterfaceStatement, astNodeStrings);

    
    char rwList[10];
    generateRWList(N, sensorInterfaceStatement, astNodeStrings, &numberOfSensorInterfaceCommands, registerAddressList, writeValueList, rwList);
    

    ipsa_create_read_instructions(rwList, i2cAddress, registerAddressList, writeValueList, signal->dimensionIndex, &numberOfSensorInterfaceCommands, N, instructionIndex, signal->physicalGroupNumber);

}


void
generateRWListsForSignalLists(State * N, Signal * signalList1, Signal * signalList2, char* astNodeStrings[], int * instructionIndex, int64_t i2cAddressList1[100], int64_t registerAddressList1[100], int64_t writeValueList1[100], char *rwList1, int64_t i2cAddressList2[100], int64_t registerAddressList2[100], int64_t writeValueList2[100], char *rwList2, int *numberOfSensorInterfaceCommands1, int *numberOfSensorInterfaceCommands2, int *numberOfWriteCommands1, int *numberOfWriteCommands2, int physicalGroupNumberList1[100], int physicalGroupNumberList2[100], int64_t dimIndexList1[100], int64_t dimIndexList2[100])
{
    int kth = 0;
    while(signalList1 != NULL)
    {
        IrNode * sensorDef = findSensorDefinitionByIdentifier(signalList1->sensorIdentifier, N, N->newtonIrRoot);
        char * sensorParameterName = findSensorParameterNameByParameterIdentifierAndAxis(signalList1->identifier, signalList1->axis, N, sensorDef);
        IrNode * sensorInterfaceStatement = findSensorInterface(sensorParameterName, N, sensorDef);
        int64_t i2cAddress = findI2CAddress(N, sensorInterfaceStatement, astNodeStrings);
        int nth = 0;
        IrNode * sensorInterfaceCommand = findNthIrNodeOfType(N, sensorInterfaceStatement, kNewtonIrNodeType_PsensorInterfaceCommand, nth);
        while(sensorInterfaceCommand != NULL)
        {
            if(sensorInterfaceCommand == NULL)
            {
                break;
            }
            IrNode * readRegisterCommand = findNthIrNodeOfType(N, sensorInterfaceCommand, kNewtonIrNodeType_PreadRegisterCommand, 0);
            IrNode * writeRegisterCommand = findNthIrNodeOfType(N, sensorInterfaceCommand, kNewtonIrNodeType_PwriteRegisterCommand, 0);

            if (readRegisterCommand != NULL)
            {
                rwList1[kth] = 0;
                IrNode * integerConst = findNthIrNodeOfType(N, readRegisterCommand, kNewtonIrNodeType_TintegerConst, 0);

                registerAddressList1[kth] = integerConst->token->integerConst;
                writeValueList1[kth] = 1;
                i2cAddressList1[kth] = i2cAddress;
                physicalGroupNumberList1[kth] = signalList1->physicalGroupNumber;
                dimIndexList1[kth] = signalList1->dimensionIndex;
                *numberOfSensorInterfaceCommands1 = *numberOfSensorInterfaceCommands1 + 1;
            }
            else if (writeRegisterCommand != NULL)
            {
                rwList1[kth] = 1;
            
                IrNode * value = findNthIrNodeOfType(N, writeRegisterCommand, kNewtonIrNodeType_TintegerConst, 0);
                IrNode * registerAddr = findNthIrNodeOfType(N, writeRegisterCommand, kNewtonIrNodeType_TintegerConst, 1);
                registerAddressList1[kth] = registerAddr->token->integerConst;
                writeValueList1[kth] = value->token->integerConst;
                i2cAddressList1[kth] = i2cAddress;
                physicalGroupNumberList1[kth] = signalList1->physicalGroupNumber;
                dimIndexList1[kth] = signalList1->dimensionIndex;
                *numberOfSensorInterfaceCommands1 = *numberOfSensorInterfaceCommands1 + 1;
                *numberOfWriteCommands1 = *numberOfWriteCommands1 + 1;
            
            }
            else
            {
                flexprint(N->Fe, N->Fm, N->Fperr, "ERROR: No I2C read or write command found.\n");
            }
            kth++;
            nth++;
            sensorInterfaceCommand = findNthIrNodeOfType(N, sensorInterfaceStatement, kNewtonIrNodeType_PsensorInterfaceCommand, nth);
        }

        if(signalList1->relatedSignalListNext == NULL)
        {
            break;
        }
        signalList1 = signalList1->relatedSignalListNext;

    }

    int kth2 = 0;
    while(signalList2 != NULL)
    {
        IrNode * sensorDef = findSensorDefinitionByIdentifier(signalList2->sensorIdentifier, N, N->newtonIrRoot);
        char * sensorParameterName = findSensorParameterNameByParameterIdentifierAndAxis(signalList2->identifier, signalList2->axis, N, sensorDef);
        IrNode * sensorInterfaceStatement = findSensorInterface(sensorParameterName, N, sensorDef);
        int64_t i2cAddress = findI2CAddress(N, sensorInterfaceStatement, astNodeStrings);

        int nth = 0;
        IrNode * sensorInterfaceCommand = findNthIrNodeOfType(N, sensorInterfaceStatement, kNewtonIrNodeType_PsensorInterfaceCommand, nth);
        while(sensorInterfaceCommand != NULL)
        {
            if(sensorInterfaceCommand == NULL)
            {
                break;
            }
            IrNode * readRegisterCommand = findNthIrNodeOfType(N, sensorInterfaceCommand, kNewtonIrNodeType_PreadRegisterCommand, 0);
            IrNode * writeRegisterCommand = findNthIrNodeOfType(N, sensorInterfaceCommand, kNewtonIrNodeType_PwriteRegisterCommand, 0);

            if (readRegisterCommand != NULL)
            {
                rwList2[kth2] = 0;
                IrNode * integerConst = findNthIrNodeOfType(N, readRegisterCommand, kNewtonIrNodeType_TintegerConst, 0);

                registerAddressList2[kth2] = integerConst->token->integerConst;
                writeValueList2[kth2] = 1;
                i2cAddressList2[kth2] = i2cAddress;
                physicalGroupNumberList2[kth2] = signalList2->physicalGroupNumber;
                dimIndexList2[kth2] = signalList2->dimensionIndex;
                *numberOfSensorInterfaceCommands2 = *numberOfSensorInterfaceCommands2 + 1;
            }
            else if (writeRegisterCommand != NULL)
            {
                rwList2[kth2] = 1;
            
                IrNode * value = findNthIrNodeOfType(N, writeRegisterCommand, kNewtonIrNodeType_TintegerConst, 0);
                IrNode * registerAddr = findNthIrNodeOfType(N, writeRegisterCommand, kNewtonIrNodeType_TintegerConst, 1);
                registerAddressList2[kth2] = registerAddr->token->integerConst;
                writeValueList2[kth2] = value->token->integerConst;
                i2cAddressList2[kth2] = i2cAddress;
                physicalGroupNumberList2[kth2] = signalList2->physicalGroupNumber;
                dimIndexList2[kth2] = signalList2->dimensionIndex;
                *numberOfSensorInterfaceCommands2 = *numberOfSensorInterfaceCommands2 + 1;
                *numberOfWriteCommands2 = *numberOfWriteCommands2 + 1;
            
            }
            else
            {
                flexprint(N->Fe, N->Fm, N->Fperr, "ERROR: No I2C read or write command found.\n");
            }
            kth2++;
            nth++;
            sensorInterfaceCommand = findNthIrNodeOfType(N, sensorInterfaceStatement, kNewtonIrNodeType_PsensorInterfaceCommand, nth);
        }

        if(signalList2->relatedSignalListNext == NULL)
        {
            break;
        }
        signalList2 = signalList1->relatedSignalListNext;

    }
}


void
createFinalInstructions(State * N, char* astNodeStrings[], int jumpToInstruction, int * instructionIndex)
{
    int64_t jumpToInstructionB = convertDecimalToBinary(jumpToInstruction);
    flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%d%s%0120lld%s \n", "instr_mem_reg[", *instructionIndex, "] = 128'b00000110_", jumpToInstructionB, ";");
    flexprint(N->Fe, N->Fm, N->Fpipsa, "%s \n", "end");
}

bool
checkIfReadsOnlyForSignal(State * N, Signal * signal, char* astNodeStrings[])
{
    bool readsOnly = true;
    
    int numberOfSensorInterfaceCommands = 0;
    int64_t registerAddressList[10];
    int64_t writeValueList[10];

    
    IrNode * sensorDef = findSensorDefinitionByIdentifier(signal->sensorIdentifier, N, N->newtonIrRoot);
    
    
    //  Need to change to take into account axis.
    char * sensorParameterName = findSensorParameterNameByParameterIdentifierAndAxis(signal->identifier, signal->axis, N, sensorDef);

    IrNode * sensorInterfaceStatement = findSensorInterface(sensorParameterName, N, sensorDef);
    
    
    //int64_t i2cAddress = findI2CAddress(N, sensorInterfaceStatement, astNodeStrings);

    
    char rwList[10];
    generateRWList(N, sensorInterfaceStatement, astNodeStrings, &numberOfSensorInterfaceCommands, registerAddressList, writeValueList, rwList);

    for(int i=0; i<10; i++)
    {
        if(rwList[i] == 1)
        {
            readsOnly = false;
            break;
        }
    }


    return readsOnly;
}


Signal *
sortSignalsByI2CReadInstruction(State * N, Signal * signalList, char* astNodeStrings[])
{
    Signal * sortedSignals = NULL;
    Signal * nextSignal = NULL;

    //  Generate the RWList for the each Signal, check if it consists of only reads, if yes, add to the list, if no, skip it.
    while(signalList->relatedSignalListPrev != NULL)
    {
        signalList = signalList->relatedSignalListPrev;
    }

    int count = 0;
    while(signalList != NULL)
    {
        bool readsOnly = checkIfReadsOnlyForSignal(N, signalList, astNodeStrings);
        if(readsOnly)
        {
            if(count == 0)
            {
                sortedSignals = (Signal *) calloc(1, sizeof(Signal));
                shallowCopySignal(N, signalList, sortedSignals);
                count++;
            } else {
                nextSignal = (Signal *) calloc(1, sizeof(Signal));
                shallowCopySignal(N, signalList, nextSignal);
                nextSignal->relatedSignalListPrev = sortedSignals;
                sortedSignals->relatedSignalListNext = nextSignal;
                sortedSignals = nextSignal;
            }
        }

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


    while(signalList != NULL)
    {
        if(count == 0)
        {
            sortedSignals = (Signal *) calloc(1, sizeof(Signal));
            shallowCopySignal(N, signalList, sortedSignals);
            count++;
        } else {
            nextSignal = (Signal *) calloc(1, sizeof(Signal));
            shallowCopySignal(N, signalList, nextSignal);
            sortedSignals->relatedSignalListNext = nextSignal;
            nextSignal->relatedSignalListPrev = sortedSignals;
            sortedSignals = nextSignal;
        }

        if(signalList->relatedSignalListNext == NULL)
        {
            break;
        }
        signalList = signalList->relatedSignalListNext;
    }
    
    while(sortedSignals->relatedSignalListPrev != NULL)
    {
        sortedSignals = sortedSignals->relatedSignalListPrev;
    }
    

    Signal * fullySortedSignals = removeDuplicates(N, sortedSignals);


    return fullySortedSignals;
}


void
createInstructionsForSignal(State * N, Signal * signal, int * instructionIndex, char* astNodeStrings[])
{
    Signal * relatedSignalList = signal->relatedSignalList;

    Signal * physicalGroup1 = NULL;
    Signal * physicalGroup2 = NULL;

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


    
    
    for(int k=0; k<4; k++)
    {
        if(physicalGroup1 != NULL && physicalGroup2 == NULL)
        {
            Signal * randomGroup1 = randomizeSignalList(N, physicalGroup1);

            while(randomGroup1 != NULL)
            {
                createWriteI2CInstructionsForSignal(N, randomGroup1, astNodeStrings, instructionIndex);
                if(randomGroup1->relatedSignalListNext == NULL)
                {
                    break;
                }
                randomGroup1 = randomGroup1->relatedSignalListNext;
            }

            while(randomGroup1->relatedSignalListPrev != NULL)
            {
                randomGroup1 = randomGroup1->relatedSignalListPrev;
            }

            //  TODO: Create new ordering of Signals where Signals with only "read" instructions are placed first.
        
            Signal * readGroup1 = sortSignalsByI2CReadInstruction(N, randomGroup1, astNodeStrings);

            while(readGroup1->relatedSignalListPrev != NULL)
            {
                readGroup1 = readGroup1->relatedSignalListPrev;
            }

        

            while(readGroup1 != NULL)
            {
                createReadI2CInstructionsForSignal(N, readGroup1, astNodeStrings, instructionIndex);
                if(readGroup1->relatedSignalListNext == NULL)
                {
                    break;
                }
                readGroup1 = readGroup1->relatedSignalListNext;
            }

            while(readGroup1->relatedSignalListPrev != NULL)
            {
                readGroup1 = readGroup1->relatedSignalListPrev;
            }

            char binaryDelay[50];
            int64_t delay = countSignalsInList(N, randomGroup1) << 13;
            convert50(delay, binaryDelay);
            binaryDelay[49] = '\0';
            flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%d%s%s%s%s \n", "instr_mem_reg[", *instructionIndex, "] = 128'b00000010_", "00000000000000000000000000000000000000000000000000000000000000000000000_", binaryDelay, ";");
            *instructionIndex = *instructionIndex + 1;
        
        }
        else if(physicalGroup1 == NULL && physicalGroup2 != NULL)
        {
            Signal * randomGroup2 = randomizeSignalList(N, physicalGroup2);

            while(randomGroup2 != NULL)
            {
                createWriteI2CInstructionsForSignal(N, randomGroup2, astNodeStrings, instructionIndex);
                if(randomGroup2->relatedSignalListNext == NULL)
                {
                    break;
                }
                randomGroup2 = randomGroup2->relatedSignalListNext;
            }

            while(randomGroup2->relatedSignalListPrev != NULL)
            {
                randomGroup2 = randomGroup2->relatedSignalListPrev;
            }
        
            Signal * readGroup2 = sortSignalsByI2CReadInstruction(N, randomGroup2, astNodeStrings);

            while(readGroup2->relatedSignalListPrev != NULL)
            {
                readGroup2 = readGroup2->relatedSignalListPrev;
            }

        

            while(readGroup2 != NULL)
            {
                createReadI2CInstructionsForSignal(N, readGroup2, astNodeStrings, instructionIndex);
                if(readGroup2->relatedSignalListNext == NULL)
                {
                    break;
                }
                readGroup2 = readGroup2->relatedSignalListNext;
            }

            while(readGroup2->relatedSignalListPrev != NULL)
            {
                readGroup2 = readGroup2->relatedSignalListPrev;
            }

            char binaryDelay[50];
            int64_t delay = countSignalsInList(N, randomGroup2) << 13;
            convert50(delay, binaryDelay);
            binaryDelay[49] = '\0';
            flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%d%s%s%s%s \n", "instr_mem_reg[", *instructionIndex, "] = 128'b00000010_", "00000000000000000000000000000000000000000000000000000000000000000000000_", binaryDelay, ";");
            *instructionIndex = *instructionIndex + 1;

        }
        else if(physicalGroup1 != NULL && physicalGroup2 != NULL)
        {
        
            Signal * randomGroup1 = randomizeSignalList(N, physicalGroup1);
            Signal * randomGroup2 = randomizeSignalList(N, physicalGroup2);

            int numberOfSensorInterfaceCommands1 = 0;
            int numberOfSensorInterfaceCommands2 = 0;
            int numberOfWriteCommands1 = 0;
            int numberOfWriteCommands2 = 0;
            int64_t i2cAddressList1[100];
            int64_t registerAddressList1[100];
            int64_t writeValueList1[100];
            char rwList1[100];
            int64_t i2cAddressList2[100];
            int64_t registerAddressList2[100];
            int64_t writeValueList2[100];
            char rwList2[100];
            int physicalGroupNumberList1[100];
            int physicalGroupNumberList2[100];
            int64_t dimIndexList1[100];
            int64_t dimIndexList2[100];


            generateRWListsForSignalLists(N, randomGroup1, randomGroup2, astNodeStrings, instructionIndex, i2cAddressList1, registerAddressList1, writeValueList1, rwList1, i2cAddressList2, registerAddressList2, writeValueList2, rwList2, &numberOfSensorInterfaceCommands1, &numberOfSensorInterfaceCommands2, &numberOfWriteCommands1, &numberOfWriteCommands2, physicalGroupNumberList1, physicalGroupNumberList2, dimIndexList1, dimIndexList2);

            int secondaryStartPosition = 0;
            int j = 0;
            int shorterLength = 0;
            int difference = 0;
            if(numberOfWriteCommands1 > numberOfWriteCommands2)
            {
                secondaryStartPosition = ((numberOfWriteCommands1 + 1) / 2) - (numberOfWriteCommands2 / 2) - 1;
                j = numberOfWriteCommands1;
                shorterLength = numberOfWriteCommands2;
                difference = numberOfWriteCommands1 - numberOfWriteCommands2;

            } else if(numberOfWriteCommands1 < numberOfWriteCommands2) {
                secondaryStartPosition = ((numberOfWriteCommands2 + 1) / 2) - (numberOfWriteCommands1 / 2) - 1;
                j = numberOfWriteCommands2;
                shorterLength = numberOfWriteCommands1;
                difference = numberOfWriteCommands2 - numberOfWriteCommands1;
            } else {
                secondaryStartPosition = 0;
                shorterLength = numberOfWriteCommands1;
                j = numberOfWriteCommands1;
                difference = 0;
            }

            bool parallel = false;

            if(numberOfWriteCommands1 == 0 || numberOfWriteCommands2 == 0)
            {
                parallel = false;
            } else {
                parallel = true;
            }
        

            for(int i=0; i<j; i++)
            {
                if(rwList1[i] == 1 && rwList2[i] == 1)
                {
                    create_parallel_write_instruction(instructionIndex, i2cAddressList1[i], registerAddressList1[i], writeValueList1[i], physicalGroupNumberList1[i], i2cAddressList2[i], registerAddressList2[i], writeValueList2[i], physicalGroupNumberList2[i], N);
                    *instructionIndex = *instructionIndex + 1;

                }
                else if(rwList1[i] == 1 && rwList2[i] != 1)
                {
                    create_write_instruction(instructionIndex, i2cAddressList1[i], registerAddressList1[i], writeValueList1[i], N, physicalGroupNumberList1[i]);
                    *instructionIndex = *instructionIndex + 1;
                }
                else if(rwList2[i] == 1 && rwList1[i] != 1)
                {
                    create_write_instruction(instructionIndex, i2cAddressList2[i], registerAddressList2[i], writeValueList2[i], N, physicalGroupNumberList2[i]);
                    *instructionIndex = *instructionIndex + 1;
                }
            
            
            }

            int shortLength = 0;
            int longLength = 0;
            bool listOneIsLonger = false;
            bool sameLength = false;

            if(numberOfSensorInterfaceCommands1 > numberOfSensorInterfaceCommands2)
            {
                shortLength = numberOfSensorInterfaceCommands2;
                longLength = numberOfSensorInterfaceCommands1;
                listOneIsLonger = true;
            }
            else if(numberOfSensorInterfaceCommands2 > numberOfSensorInterfaceCommands1)
            {
                shortLength = numberOfSensorInterfaceCommands1;
                longLength = numberOfSensorInterfaceCommands2;
                listOneIsLonger = false;
            }
            else if(numberOfSensorInterfaceCommands1 == numberOfSensorInterfaceCommands2)
            {
                sameLength = true;
                shortLength = numberOfSensorInterfaceCommands1;
                longLength = numberOfSensorInterfaceCommands1;
            }

            for(int i=0; i<longLength; i++)
            {
                if(listOneIsLonger && !sameLength)
                {
                    if(i < shortLength && (i+difference) < longLength)
                    {
                        if(rwList1[i+difference] == 0 && rwList2[i] == 0)
                        {
                            create_parallel_read_instruction(instructionIndex, i2cAddressList1[i+difference], registerAddressList1[i+difference], dimIndexList1[i+difference], physicalGroupNumberList1[i+difference], i2cAddressList2[i], registerAddressList2[i], dimIndexList2[i], physicalGroupNumberList2[i], N);
                            *instructionIndex = *instructionIndex + 1;
                        }
                    
                    }
                    else if(i >= shortLength && (i+difference) < longLength)
                    {
                        if(rwList1[i+difference])
                        {
                            create_read_instruction(instructionIndex, i2cAddressList1[i+difference], registerAddressList1[i+difference], dimIndexList1[i+difference], N, physicalGroupNumberList1[i+difference]);
                            *instructionIndex = *instructionIndex + 1;
                        }
                    }
                }
                if(!listOneIsLonger && !sameLength)
                {
                    if(i < shortLength && (i+difference) < longLength)
                    {
                        if(rwList1[i] == 0 && rwList2[i+difference] == 0)
                        {
                            create_parallel_read_instruction(instructionIndex, i2cAddressList1[i], registerAddressList1[i], dimIndexList1[i], physicalGroupNumberList1[i], i2cAddressList2[i+difference], registerAddressList2[i+difference], dimIndexList2[i+difference], physicalGroupNumberList2[i+difference], N);
                            *instructionIndex = *instructionIndex + 1;
                        }
                    
                    }
                    else if(i >= shortLength && (i+difference) < longLength)
                    {
                        if(rwList2[i+difference] == 0)
                        {
                            create_read_instruction(instructionIndex, i2cAddressList2[i+difference], registerAddressList2[i+difference], dimIndexList2[i+difference], N, physicalGroupNumberList2[i+difference]);
                            *instructionIndex = *instructionIndex + 1;
                        }
                    }
                }
                if(sameLength)
                {
                    if(numberOfWriteCommands1 > numberOfWriteCommands2)
                    {
                        if(i < shortLength && (i+difference) < longLength)
                        {
                            if(rwList1[i+difference] == 0 && rwList2[i] == 0)
                            {
                                create_parallel_read_instruction(instructionIndex, i2cAddressList1[i+difference], registerAddressList1[i+difference], dimIndexList1[i+difference], physicalGroupNumberList1[i+difference], i2cAddressList2[i], registerAddressList2[i], dimIndexList2[i], physicalGroupNumberList2[i], N);
                                *instructionIndex = *instructionIndex + 1;
                            }
                    
                        } else {
                            if(rwList2[i] == 0)
                            {
                                create_read_instruction(instructionIndex, i2cAddressList2[i], registerAddressList2[i], dimIndexList2[i], N, physicalGroupNumberList2[i]);
                                *instructionIndex = *instructionIndex + 1;
                            }
                        
                        }
                    }
                    else if(numberOfWriteCommands2 > numberOfWriteCommands1)
                    {
                        if(i < shortLength && (i+difference) < longLength)
                        {
                            if(rwList1[i] == 0 && rwList2[i+difference] == 0)
                            {
                                create_parallel_read_instruction(instructionIndex, i2cAddressList1[i], registerAddressList1[i], dimIndexList1[i], physicalGroupNumberList1[i], i2cAddressList2[i+difference], registerAddressList2[i+difference], dimIndexList2[i+difference], physicalGroupNumberList2[i+difference], N);
                                *instructionIndex = *instructionIndex + 1;
                            }
                    
                        } else {
                            if(rwList1[i] == 0)
                            {
                                create_read_instruction(instructionIndex, i2cAddressList1[i], registerAddressList1[i], dimIndexList1[i], N, physicalGroupNumberList1[i]);
                                *instructionIndex = *instructionIndex + 1;
                            }
                        
                        }
                    } else {
                        if(rwList1[i] == 0 && rwList2[i] == 0)
                        {
                            create_parallel_read_instruction(instructionIndex, i2cAddressList1[i], registerAddressList1[i], dimIndexList1[i], physicalGroupNumberList1[i], i2cAddressList2[i], registerAddressList2[i], dimIndexList2[i], physicalGroupNumberList2[i], N);
                            *instructionIndex = *instructionIndex;
                        }
                    
                    }
                }
            
            }

            char binaryDelay[50];
            int64_t delay = longLength << 13;
            convert50(delay, binaryDelay);
            binaryDelay[49] = '\0';
            flexprint(N->Fe, N->Fm, N->Fpipsa, "%s%d%s%s%s%s \n", "instr_mem_reg[", *instructionIndex, "] = 128'b00000010_", "00000000000000000000000000000000000000000000000000000000000000000000000_", binaryDelay, ";");
            *instructionIndex = *instructionIndex + 1;
        

 
        }
    }
}


void
irPassIpsaBackend(State * N, char* astNodeStrings[])
{
    int instructionIndex = 0;
    int jumpToInstruction = createSetupInstructions(N, countAllSignals(N), &instructionIndex);

    int kth = 0;
    Signal * signal = findKthSignal(N, kth);

    /*
     *  TODO: The loop below will most likely not handle duplicate Signals well. (ie Signals which are the same, but exist as duplicates in multiple places on the AST) Look into this.
     */
    while(signal != NULL)
    {
        createInstructionsForSignal(N, signal, &instructionIndex, astNodeStrings);
        kth++;
        signal = findKthSignal(N, kth);
    }
    
    createFinalInstructions(N, astNodeStrings, jumpToInstruction, &instructionIndex); 
    
}