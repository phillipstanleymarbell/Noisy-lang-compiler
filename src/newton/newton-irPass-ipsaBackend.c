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

/*
 * TODO: Add support for I2C write instructions.
 * TODO: Add support for identifying sensor from signal definition.
 */

/*
 * NOTE: It is assumed that the Noisy-lang-compiler and Ipsa-core are in the same directory.
 */


/* 
 * Check user's OS and make according adjustments.
 */

#if defined(_MSC_VER)
#include <direct.h>
#define getcwd _getcwd
#elif defined(__GNUC__)
#include <unistd.h>
#endif



int numberOfSensorInterfaceCommands = 0;
int64_t registerAddressList[10];
int64_t writeValueList[10]; 

FILE *fptr;

/* 
 * Function searches AST and returns a sensor definition node
 * with a matching identifier, generally corresponding to the
 * name of the sensor (e.g. 'BMP180').
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
       printf("%s \n", "ERROR: No matching sensor found.");
   }

   return sensorDefinition;
    
}


/* 
 * Function finds and returns a sensor parameter name corresponding
 * to a parameter identifier. For example, for the BMP180, the 
 * parameter identifier 'temperature' would return 'bmp180temperature'.
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
        printf("%s \n", "ERROR: No matching sensor parameter found.");
    }
    
    return sensorParameterName;

}


/* 
 * Function searches AST from the sensor definition root node
 * and returns a sensor interface definition which matches the
 * sensor parameter name used as the identifier.
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
        printf("%s \n", "ERROR: No matching sensor interface found.");
    }

    return sensorInterfaceStatement;
    
}


/* 
 * Function finds and returns the I2C address associated with
 * a given sensor interface statement node.
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
        printf("%s \n", "ERROR: Currently unsupported sensor interface type (SPI).");
    }
    else
    {
        printf("%s \n", "ERROR: Unrecognized sensor interface type.");
    }

    return i2cAddress;
}


/* 
 * Function generates a list of I2C read and write commands
 * with associated register address and write values for
 * a given sensor interface statement node. '0' corresponds
 * to a read command, '1' corresponds to a write command.
 */

int * generateRWList(State * N, IrNode * sensorInterfaceStatement, char* astNodeStrings[])
{
    static int rwList[10];
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
            numberOfSensorInterfaceCommands++;
        }
        else if (writeRegisterCommand != NULL)
        {
            rwList[nth] = 1;
            
            /*
             * TODO: Need to take care of searching AST for writeRegisterAddress and writeValue.
             */

            registerAddressList[nth] = 1;
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


/* 
 * Function to convert decimal to binary.
 */
long long convert(int64_t n) {
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
 * Function to calculate the number of digits
 * in a binary number.
 */

int numberOfDigits(int64_t num) {
    int count = 0;

    /* Calculate total digits */
    count = (num == 0) ? 1  : (log10(num) + 1);

    //printf("Total digits: %d \n", count);

    return count;

}


/*
 * Function to generate a string of zeroes to 
 * complete an Ipsa instruction with the correct
 * number of zeroes at the front. The 'option' 
 * argument is given as '0' if an instruction length
 * of 8 bits is desired, and '1' if an instruction length
 * of 5 bits is desired.
 */

char * generateZeroes(int len, int option)
{
    int i;

    if (option == 0)
    {
        i = 8 - len;
    }
    else
    {
        i = 5 - len;
    }

    char * zeroes;

    if (i == 0)
    {
        zeroes = "";
    }
    else if (i == 1)
    {
        zeroes = "0";
    }
    else if (i == 2)
    {
        zeroes = "00";
    }
    else if (i == 3)
    {
        zeroes = "000";
    }
    else if (i == 4)
    {
        zeroes = "0000";
    }
    else if (i == 5)
    {
        zeroes = "00000";
    }
    else if (i == 6)
    {
        zeroes = "000000";
    }
    else if (i == 7)
    {
        zeroes = "0000000";
    }
    else if (i == 8)
    {
        zeroes = "00000000";
    }
    else
    {
        zeroes = "";
    }
    
    return zeroes;
}


/*
 * Function to create an Ipsa I2C read instruction
 * and write it to the "instruction_list.v" file.
 */

int create_read_instruction(int currentInstruction, int64_t i2cAddr, int64_t regAddr, int64_t dimIndex) {
    
    int64_t i2cAddrB = convert(i2cAddr);
    int64_t regAddrB = convert(regAddr);
    int64_t dimIndexB = convert(dimIndex);

    int i2cAddrBLen = numberOfDigits(i2cAddrB);
    int regAddrBLen = numberOfDigits(regAddrB);
    int dimIndexBLen = numberOfDigits(dimIndexB);

    char * zeroes1 = generateZeroes(i2cAddrBLen, 0);
    char * zeroes2 = generateZeroes(regAddrBLen, 0);
    char * zeroes3 = generateZeroes(dimIndexBLen, 1);

    fprintf(fptr, "%s%d%s%s%lld%s%s%lld%s%s%lld%s \n", "instr_mem_reg[", currentInstruction, "] = 32'b101_", zeroes1, i2cAddrB, "_" , zeroes2, regAddrB, "_00000001_", zeroes3, dimIndexB, ";");

    return 0;
}


/*
 * Function to create an Ipsa I2C write instruction
 * and write it to the "instruction_list.v" file.
 */

int create_write_instruction(int currentInstruction, int64_t i2cAddr, int64_t regAddr, int64_t immediate) {
    

    int64_t i2cAddrB = convert(i2cAddr);
    int64_t regAddrB = convert(regAddr);
    int64_t immediateB = convert(immediate);

    int i2cAddrBLen = numberOfDigits(i2cAddrB);
    int regAddrBLen = numberOfDigits(regAddrB);
    int immediateBLen = numberOfDigits(immediateB);

    char * zeroes1 = generateZeroes(i2cAddrBLen, 0);
    char * zeroes2 = generateZeroes(regAddrBLen, 0);
    char * zeroes3 = generateZeroes(immediateBLen, 1);

    fprintf(fptr, "%s%d%s%s%lld%s%s%lld%s%s%lld%s%s \n", "instr_mem_reg[", currentInstruction, "] = 32'b100_", zeroes1, i2cAddrB, "_" , zeroes2, regAddrB, "_", zeroes3, immediateB, "_", "00000;");

    return 0;
}


/*
 * Function to create an "intstruction_list.v" file for Ipsa.
 */

int ipsa_create_instructions(int rwList[10], int64_t i2cAddr, int64_t regAddrList[10], int64_t immediateList[10], int64_t dimIndexList[10]){

    char* currentDirectory;

    currentDirectory = getcwd(NULL, 0);

    //printf("%s \n", currentDirectory);
    
    strcat(currentDirectory, "/instruction_list.v");
    
    fptr = fopen(currentDirectory,"w");

    if(fptr == NULL)
    {
        printf("%s \n", "ERROR: Unable to open file.");   
        exit(1);             
    }

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char s[64];
    assert(strftime(s, sizeof(s), "%c", tm));

    fprintf(fptr, "%s %s %s \n", "/*This instruction list was generated by the Ipsa backend of the Newton compiler on", s, "*/");
    fprintf(fptr, "%s \n", "initial begin");
    fprintf(fptr, "%s \n", "instr_mem_reg[0] = 32'b010_00000000000000010000000000000;"); //Barrier
    fprintf(fptr, "%s \n", "instr_mem_reg[1] = 32'b001_0000_000000000000000_00001_00011;"); //Config Start
    fprintf(fptr, "%s \n", "instr_mem_reg[2] = 32'b011_0000_000001000000000_00001_00011;"); //Config End

    int i;
    int rwLen = sizeof(&rwList) / sizeof(rwList[0]);
    //printf("%i \n", rwLen);

    int currentInstruction = 3;

    for (i=0; i<rwLen; i++) {
        fprintf(fptr, "%s%i%s \n", "instr_mem_reg[", currentInstruction ,"] = 32'b010_00000000000000010000000000000;"); //Barrier
        currentInstruction++;
        if (rwList[i] == 0){
            int64_t regAddr; 
            regAddr = regAddrList[i];
            int64_t dimIndex;
            dimIndex = dimIndexList[i];
            create_read_instruction(currentInstruction, i2cAddr, regAddr, dimIndex); //Read

            //printf("%s \n", regAddrH);
            //fprintf(fptr, "%s \n", "Read"); //Read
            currentInstruction++;
        }
        else if (rwList[i] == 1){
            int64_t regAddr; 
            regAddr = regAddrList[i];
            int64_t immediate;
            immediate = immediateList[i];
            create_write_instruction(currentInstruction, i2cAddr, regAddr, immediate); //Write
            
            
            //fprintf(fptr, "%s \n", "Write"); //Write

            currentInstruction++;
        }
        else {

        }
    }

    fprintf(fptr, "%s \n", "end");
    fclose(fptr);
    
    return 0;
}


/*
 * Ipsa backend function.
 */

void irPassIpsaBackend(State *  N, IrNode * noisyIrRoot, char* astNodeStrings[])
{
    printf("%s \n", "Ipsa backend starting.");

    /*
     * TODO: Function to assosciate a signal with a sensor identifier (e.g. "BMP180").
     * TODO: And find parameter name that sensor is measuring (e.g. "temperature").
     */

    IrNode * sensorDef = findSensorDefinitionByIdentifier("BMP180", N, noisyIrRoot);
    //printf("%s \n", astNodeStrings[sensorDef->irRightChild->irLeftChild->irLeftChild->irRightChild->type]);
    //printf("%s \n", sensorDef->irRightChild->irLeftChild->irLeftChild->irRightChild->tokenString);

    char * sensorParameterName = findSensorParameterNameByParameterIdentifier("temperature", N, sensorDef);
    //printf("%s \n", sensorParameterName);

    IrNode * sensorInterfaceStatement = findSensorInterface(sensorParameterName, N, sensorDef);
    //printf("%s \n", sensorInterfaceStatement->irLeftChild->tokenString);

    int64_t i2cAddress = findI2CAddress(N, sensorInterfaceStatement, astNodeStrings);
    //printf("%llu \n", i2cAddress);

    int *rwList;
    rwList = generateRWList(N, sensorInterfaceStatement, astNodeStrings);
    //printf("%i \n", rwList[1]);
    //printf("%i \n", numberOfSensorInterfaceCommands);
    //printf("%lld \n", readRegisterAddressList[1]);
    //printf("%lld \n", writeRegisterAddressList[1]);
    //printf("%lld \n", writeValueList[1]);

    int64_t dimIndexHList[10] = {1, 1};

    ipsa_create_instructions(rwList, i2cAddress, registerAddressList, writeValueList, dimIndexHList);

    system("mv instruction_list.v ../../../Ipsa-core/verilog/toplevel/Instruction_Mem_Examples");

    printf("%s \n", "Ipsa instruction list generated, Ipsa now ready for synthesis.");
    
}