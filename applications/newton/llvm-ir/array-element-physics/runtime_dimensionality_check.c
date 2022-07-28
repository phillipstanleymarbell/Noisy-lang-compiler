/*
	Authored 2021. Nikos Mavrogeorgis.
	Authored 2022. Orestis Kaparounakis.

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

enum DefaultConstantsE {
	kSymbolDimensionTableSize	= 1024,
	kMaxDimensions			= 1024
} DefaultConstants;

/*
 *	A table that maps values to signals. The index i corresponds to the symbol
 *	while the value symbolSignalTable[i] corresponds to the signal.
 *
 *	A value of `0` means the index has not been matched to a symbol.
 */
static int64_t **	symbolSignalTable;

void
__newtonInit()
{
	symbolSignalTable = calloc(kSymbolDimensionTableSize, sizeof(int64_t*));
	if (symbolSignalTable == NULL)
	{
		fprintf(stderr, "Newton error: could not allocate memory for Newton runtime");
		exit(EXIT_FAILURE);
	}
}

/**
 *	@brief Inserts dimensions and returns an identifier for them. Use the
 *	identifier to check dimensions using `__newtonCheckDimensions`. Call
 *	`__newtonDelete` when symbol goes out of scope.
 *
 *	@param dimensions The dimensions for the new symbol.
 *	@return size_t The symbol identifier for the just-inserted dimensions.
 */
size_t
__newtonInsert(int64_t dimensions[kMaxDimensions])
{
	for (size_t i = 0; i < kSymbolDimensionTableSize; i++)
	{
		if (symbolSignalTable[i] == NULL)
		{
			symbolSignalTable[i] = calloc(kMaxDimensions, sizeof(int64_t));
			if (symbolSignalTable[i] == NULL)
			{
				fprintf(stderr, "Newton error: could not allocate memory for Newton runtime");
				exit(EXIT_FAILURE);
			}

			for (size_t j = 0; j < kMaxDimensions; j++)
			{
				symbolSignalTable[i][j] = dimensions[j];
			}
			
			return i;
		}		
	}

	fprintf(stderr, "Newton error: no space left");
	exit(EXIT_FAILURE);	
}

/**
 * @brief Calculates the product of the dimensions for the given symbol
 * identifiers and returns the identifier of the product.
 *
 * @param firstFactorSymbolID	The symbol identifier of the first factor of the product.
 * @param secondFactorSymbolID 	The symbol identifier of the second factor of the product.
 * @return size_t 		The symbol identifier for the product.
 */
size_t
__newtonProductInsert(size_t firstFactorSymbolID, size_t secondFactorSymbolID)
{
	int64_t		productDimensions[kMaxDimensions];
	
	for (size_t i = 0; i < kMaxDimensions; i++)
	{
		productDimensions[i] = symbolSignalTable[firstFactorSymbolID][i] + symbolSignalTable[secondFactorSymbolID][i];
	}

	return __newtonInsert(productDimensions);
}

/**
 * @brief Calculates the quotient of the dimensions for the given symbol
 * identifiers and returns the identifier of the quotient.
 *
 * @param firstFactorSymbolID	The symbol identifier of the dividend.
 * @param secondFactorSymbolID 	The symbol identifier of the divisor.
 * @return size_t 		The symbol identifier for the quotient.
 */
size_t
__newtonQuotientInsert(size_t dividendSymbolID, size_t divisorSymbolID)
{
	int64_t		quotientDimensions[kMaxDimensions];
	
	for (size_t i = 0; i < kMaxDimensions; i++)
	{
		quotientDimensions[i] = symbolSignalTable[dividendSymbolID][i] - symbolSignalTable[divisorSymbolID][i];
	}

	return __newtonInsert(quotientDimensions);
}

/**
 * @brief Re-initializes the passed symbol identifer to not refer to any dimensions.
 * 
 * @param symbolID The symbol identifier to release.
 */
void
__newtonDelete(size_t symbolID)
{
	free(symbolSignalTable[symbolID]);
	symbolSignalTable[symbolID] = NULL;
}

/**
 * @brief Get the dimensions array for a given symbol identifier.
 * 
 * @param symbolID 	The symbol identifier for which to fetch the dimensions.
 * @param result 	A pointer to a int64_t array to place the result into.
 * @return int		Zero if successful.
 */
int
__newtonGetSymbolDimensions(size_t symbolID, int64_t  result[kMaxDimensions])
{
	if (symbolSignalTable[symbolID] == NULL)
	{
		return 1;
	}

	for (size_t i = 0; i < kMaxDimensions; i++)
	{
		result[i] = symbolSignalTable[symbolID][i];
		return 0;
	}
}

/**
 * @brief Checks for dimensional homogeneity for the dimensions of the given
 * symbol identifiers.
 *
 * @param firstSymbolID 
 * @param secondSymbolID 
 * @return int			Zero if homogenous.
 */
int
__newtonCheckDimensions(size_t firstSymbolID, size_t secondSymbolID)
{
	printf("Examining %d %d\n", firstSymbolID, secondSymbolID);
	for (size_t i = 0; i < kMaxDimensions; i++)
	{
		if (symbolSignalTable[firstSymbolID][i] != symbolSignalTable[secondSymbolID][i])
		{
			return 1;
		}	
	}
	
	return 0;
}

void 
__array_dimensionality_check(int index, int * array)
{
	printf("Found index: %d, %p\n", index, array);
}
