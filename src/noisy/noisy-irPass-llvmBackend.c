/*
	Authored 2018, Zhengyang Gu
 
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
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/time.h>

#ifdef NoisyOsLinux
#include <time.h>
#endif

#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "version.h"
#include "noisy-irPass-llvmBackends"
#include "common-data-structures.h"
#include "noisy-parser.h"
#include "noisy-lexer.h"
#include "common-irPass-helpers.h"
#include "noisy-types.h"

char *
nyTypeToLlvm(IrNode node)
{
	switch(node->type)
	{
		case kNoisyIrNodeType_Tbool:
		case kNoisyIrNodeType_T:
		case kNoisyIrNodeType_T:
		{
			return "i8";
		}
	}
}
/*
 * Emits the source_filename, and place holders for target informations
 */

LlvmBackendState *
llvmBackendStateInit()
{
	LlvmBackendState *  Nl;

	Nl = (LlvmBackendState *) calloc(1, sizeof(LlvmBackendState));
	if (Nl == NULL)
	{
		fatal(NULL, Emalloc);
	}

	return Nl;
}

void
irPassLlvmEmitHeader(State *  N)
{
	flexprint(N->Fe, N->Fm, N->Fpllvm, "source_filename = \"%s\"", N->fileName);
	flexprint(N->Fe, N->Fm, N->Fpllvm, "target datalayout = \"<target-data-layout>\"\n");
	flexprint(N->Fe, N->Fm, N->Fpllvm, "target triple = \"<target-triple>\"\n\n");

	return;
}

void
irPassLlvmEmitProgTypeNameDecl(State *  N, IrNode *  node)
{
	switch(R(node)->type)
	{
		case kNoisyIrNodeType_PnamegenDeclaration:
		{
			break;
		}
		case kNoisyIrNodeType_TrealConst:
		{

			flexprint(N->Fe, N->Fm, N->Fpllvm, "@%s = constant float %x, align 4\n",
				L(node)->identifier, *(unsigned int*)&(R(node)->realConst));
			break;
		}
		case kNoisyIrNodeType_TintConst:
		{
			flexprint(N->Fe, N->Fm, N->Fpllvm, "@%s = constant int %d, align 4\n",
				L(node)->identifier, *(unsigned int*)&(R(node)->intConst));
			break;
		}
		case kNoisyIrNodeType_TboolConst:
		{
			flexprint(N->Fe, N->Fm, N->Fpllvm, "@%s = constant i8 %d, align 4\n",
				L(node)->identifier, *(unsigned int*)&(R(node)->intConst));
			break;
		}
		case kNoisyIrNodeType_PtypeDeclaration:
		{
			LlvmBackendState *	llvmState = (LlvmBackendState *) N->backendState;
			llvmState->nStructs += 1;
			llvmState->structs = (StructFields **) realloc(llvmState->structs,
				llvmState->nStructs * sizeof(StructFields *));
			
			if (llvmState->firstStruct == NULL)
			{
				fatal(NULL, Emalloc);
			}

			llvmState->structs[llvmState->nStructs - 1] = (StructFields *) calloc(1, sizeof(StructFields));
			StructFields *	currentStruct = llvmState->structs[llvmState->nStructs - 1];

			if (currentStruct == NULL)
			{
				fatal(NULL, Emalloc);
			}

			currentStruct->name = L(node)->tokenString;
			flexprint(N->Fe, N->Fm, N->Fpllvm, "%%struct.%s = type {", L(node)->tokenString);
			
			assert(L(R(node))->type == kNoisyIrNodeType_PadtTypeDeclaration);
			
			currentStruct->nFields = 0;

			/*
			 * We first iterrate over the tree once to get the number of fields before we
			 * can malloc
			 */
			for (IrNode *	current = L(R(node); current != NULL; current = R(R(current)))
			{
				currentStruct->nFields++;
			}

			currentStruct->fields = (char **)calloc(1, sizeof(char *) * currentStruct->nFields);

			/*
			 * For an ADT, we store its field name under llvmState, and write the type of its
			 * fields in the LLVM file, e.g.
			 * for 
	 		 * Pixel : adt
	   		   {
				r : int8;
				g : int8;
				b : int8;
				a : int8;
	   		   };
			 * ["r", "g", "b", "a"] will be stored under N->backendState->structs, and 
			 * `%struct.Pixel = type { i8, i8, i8, i8 }` will be written to the output file.
			 * 
			 * Since LLVM IR does not permit extra comma after the last field, and that Noisy
			 * ADTs must have at least one field, we treat the first field seperately.
			 */

			IrNode *	currentNode = L(R(node);
			currentStruct->fields[0] = currentNode->tokenString;
			flexprint(N->Fe, N->Fm, N->Fpllvm, " %s", nyTypeToLlvm(L(R(currentNode))));
			currentNode = R(R(currentNode));

			for (int i = 1; i < currentStruct->nFields; ++i)
			{
				currentStruct->fields[i] = currentNode->tokenString;
				flexprint(N->Fe, N->Fm, N->Fpllvm, ", %s", nyTypeToLlvm(L(R(currentNode))));
			}

			flexprint(N->Fe, N->Fm, N->Fpllvm, " }\n");
			break;
		}
		default:
		{
			fatal(N, )
		}
	}

}

void
irPassLlvmEmitProgtypeBody(State *  N, IrNode *  node)
{
	assert(node->type = kNoisyIrNodeType_PprogtypeBody);

	for(IrNode *	current = node; current != NULL; current = R(current))
	{
		irPassLlvmEmitProgTypeNameDecl(N, L(N));
	}

	return;
}

void
irPassLlvmEmitProgType(State *  N, IrNode *  node)
{
	assert(node->type == kNoisyIrNodeType_PprogtypeDeclaration);
	flexprint(N->Fe, N->Fm, N->Fpllvm, "; ModuleID = %s\n", L(node)->identifier);

	irPassLlvmEmitProgtypeBody(N, R(node));

	return;
}

void
irPassLlvmEmitProgram(State *  N, IrNode *  node)
{
	LlvmBackendState *	Nl = llvmBackendStateInit();
	N->backendState = Nl;

	assert(node->type == kNoisyIrNodeType_Pprogram);
	irPassLlvmEmitHeader();
	irPassLlvmEmitProgType(N, L(node));

	for (IrNode *  current = R(node); current != NULL; current = R(current))
	{
		irPassLlvmEmitNameGen(L(current));
	}

}