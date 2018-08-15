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
#include "noisy-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "noisy-irPass-llvmBackend.h"
#include "noisy-parser.h"
#include "noisy-lexer.h"
#include "common-irPass-helpers.h"
#include "noisy-types.h"

char *
nyTypeToLlvm(IrNode *  node)
{
	switch(node->type)
	{
		case kNoisyIrNodeType_Tbool:
		case kNoisyIrNodeType_Tint4:
		case kNoisyIrNodeType_Tint8:
		{
			return "i8";
		}
		default:
		{
			fatal(NULL, "Not supported Type");
		}
	}
}

void
setupADT(IrNode *  node, NoisyADT *  adt)
{
	
}
 
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

NoisyFunc *
registerFunc(State *  N, LlvmBackendState *  Nl, IrNode *  node)
{
	Nl->nFuncs++;
	Nl->funcs = (NoisyFunc **) realloc(Nl->funcs, Nl->nFuncs * sizeof(NoisyFunc *));

	if (Nl->funcs == NULL)
	{
		fatal(NULL, Emalloc);
	}

	Nl->funcs[Nl->nFuncs] = (NoisyFunc *) calloc(1, sizeof(NoisyFunc));
	NoisyFunc *  currentFunc = Nl->funcs[Nl->nFuncs];

	if (currentFunc == NULL)
	{
		fatal(NULL, Emalloc);
	}

	currentFunc->name = L(node)->tokenString;

	IrNode *  inputNode, *  outputNode;

	switch(node->type)
	{
		case kNoisyIrNodeType_PfunctionDecl:
		{
			inputNode = L(L(L(R(node))));
			outputNode = L(L(L(L(node))));
			break;
		}
		case kNoisyIrNodeType_PfunctionDefn:
		{
			inputNode = L(R(node));
			outputNode = L(R(R(node)));
			break;
		}
		default:
		{
			fatal(NULL, EtokenUnrecognized);
		}
	}
	currentFunc->outputVar = L(outputNode)->tokenString;
	currentFunc->outputType = nyTypeToLlvm(L(R(outputNode)));
	currentFunc->inputVar = L(inputNode)->tokenString;
	currentFunc->inputType = nyTypeToLlvm(L(R(inputNode)));
	// This should be the proper way of doing it
	// irPassLlvmEmitFuncIOTypes(N, Nl, inputNode, currentFunc);
	// irPassLlvmEmitFuncIOTypes(N, Nl, outputNode, currentFunc);
	return currentFunc;
}

/*
 * Searches for the function in the list of known function. If not known,
 * register it.
 */
NoisyFunc *
searchRegFunc(State *  N, LlvmBackendState *  Nl, IrNode *  node)
{
	char *  funcName = L(node)->tokenString;
	
	int i = 0;
	for (; i < Nl->nFuncs; ++i)
	{
		if (strcmp(funcName, Nl->funcs[i]->name) == 0)
		{
			return Nl->funcs[i];
		}
	}

	return registerFunc(N, Nl, node);
}
/*
NoisyADT *
irPassLlvmEmitFuncIOTypes(State *  N, LlvmBackendState *  Nl, IrNode *  node, NoisyFunc *  currentFunc)
{
	NoisyADT *  currentADT;
	char *  varName;

	switch(node->type)
	{
		case kNoisyIrNodeType_PwriteTypeSignature:
		{
			currentFunc->inputADT = (NoisyADT *) calloc(1, sizeof(NoisyADT));
			asprintf(&(currentFunc->inputADT->name), "%s.input", currentFunc->name);
			currentADT = currentFunc->inputADT;
		}
		case kNoisyIrNodeType_PreadTypeSignature:
		{
			currentFunc->outputADT = (NoisyADT *) calloc(1, sizeof(NoisyADT));
			asprintf(&(currentFunc->outputADT->name), "%s.input", currentFunc->name);
			currentADT = currentFunc->outputADT;
		}
		default:
		{
			fatal(N, EtokenUnrecognized);
		}
	}

	if (currentADT == NULL)
	{
		fatal(NULL, Emalloc);
	}

	asprintf(&(currentADT->name), "%s.input", currentFunc->name, vara);

	IrNode *  currentNode = R(L(node));

	for (IrNode *  current = currentNode; current != NULL; current = R(current))
	{
		currentADT->nFields++;
	}

	currentADT->fields = (char **)calloc(1, sizeof(char *) * currentADT->nFields);
	currentADT->fieldTypes = (IrNode **)calloc(1, sizeof(IrNode *) * currentADT->nFields);
	if (currentADT->fields == NULL || currentADT->fieldTypes == NULL)
	{
		fatal(NULL, Emalloc);
	}

	for (int i = 0; currentADT->nFields; ++i)
	{
		currentADT
	}

}
*/
void
irPassLlvmEmitHeader(State *  N)
{
	flexprint(N->Fe, N->Fm, N->Fpllvm, "source_filename = \"%s\"", N->fileName);
	flexprint(N->Fe, N->Fm, N->Fpllvm, "target datalayout = \"<target-data-layout>\"\n");
	flexprint(N->Fe, N->Fm, N->Fpllvm, "target triple = \"<target-triple>\"\n\n");

	return;
}

void
irPassLlvmEmitProgTypeNameDecl(State *  N, LlvmBackendState *  Nl, IrNode *  node)
{
	switch(R(node)->type)
	{
		case kNoisyIrNodeType_TrealConst:
		{

			flexprint(N->Fe, N->Fm, N->Fpllvm, "@%s = constant float %x, align 4\n",
				L(node)->tokenString, *(unsigned int*)&(R(node)->token->realConst));
			break;
		}
		case kNoisyIrNodeType_TintegerConst:
		{
			flexprint(N->Fe, N->Fm, N->Fpllvm, "@%s = constant int %d, align 4\n",
				L(node)->tokenString, *(unsigned int*)&(R(node)->token->integerConst));
			break;
		}
		case kNoisyIrNodeType_TboolConst:
		{
			flexprint(N->Fe, N->Fm, N->Fpllvm, "@%s = constant i8 %d, align 4\n",
				L(node)->tokenString, *(unsigned int*)&(R(node)->token->integerConst));
			break;
		}
		case kNoisyIrNodeType_PfunctionDecl:
		{
			registerFunc(N, Nl, node);
			break;
		}
		case kNoisyIrNodeType_PtypeDecl:
		{
			fatal(NULL, "ADT not supported yet!");
			/*
			Nl->nStructs++;
			Nl->structs = (NoisyADT **) realloc(Nl->structs, Nl->nStructs * sizeof(NoisyADT *));
			
			if (Nl->Struct == NULL)
			{
				fatal(NULL, Emalloc);
			}

			Nl->structs[Nl->nStructs - 1] = (NoisyADT *) calloc(1, sizeof(NoisyADT));
			NoisyADT *	currentStruct = Nl->structs[Nl->nStructs - 1];

			if (currentStruct == NULL)
			{
				fatal(NULL, Emalloc);
			}

			currentStruct->name = L(node)->tokenString;
			currentStruct->sourceInfo = node->sourceInfo;
			
			if(L(R(node))->type != kNoisyIrNodeType_PadtTypeDeclaration)
			{
				fatal(N, EtokenUnrecognized);
			}
			
			currentStruct->nFields = 0;
			*/
			/*
			 * We first iterrate over the tree once to get the number of fields before we
			 * can malloc.
			 * 
			 * Does not supprot a,b,c:i8
			 */
			/*
			for (IrNode *	current = L(R(node); current != NULL; current = R(R(current)))
			{
				currentStruct->nFields++;
			}
			
			currentStruct->fields = (char **)calloc(1, sizeof(char *) * currentStruct->nFields);
			currentStruct->fieldTypes = (IrNode **)calloc(1, sizeof(IrNode *) * currentStruct->nFields);
			if (currentStruct->fields == NULL || currentStruct->fieldTypes == NULL)
			{
				fatal(NULL, Emalloc);
			}
			*/
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
			 */
			/*
			for (int i = 0; i < currentStruct->nFields; ++i)
			{
				currentStruct->fields[i] = currentNode->tokenString;
				currentStruct->fieldTypes[i] = L(R(currentNode));
				currentNode = R(R(currentNode));
			}

			irPassLlvmEmitADT(N, Nl, currentStruct);
			break;
			*/
		}
		default:
		{
			fatal(N, EtokenUnrecognized);
		}
	}

}

void
irPassLlvmEmitModuleDeclBody(State *  N, LlvmBackendState *  Nl, IrNode *  node)
{
	if (node->type != kNoisyIrNodeType_PmoduleDeclBody)
	{
		fatal(N, EtokenUnrecognized);
	}
	for(IrNode *	current = node; current != NULL; current = R(current))
	{
		irPassLlvmEmitProgTypeNameDecl(N, Nl, L(node));
	}

	return;
}

void
irPassLlvmEmitModuleDecl(State *  N, LlvmBackendState *  Nl, IrNode *  node)
{
	if(node->type != kNoisyIrNodeType_PmoduleDecl)
	{
		fatal(N, EtokenUnrecognized);
	}
	Nl->module = L(node)->tokenString;
	flexprint(N->Fe, N->Fm, N->Fpllvm, "; ModuleID = %s\n", L(node)->tokenString);

	irPassLlvmEmitModuleDeclBody(N, Nl, R(node));

	return;
}

void
irPassLlvmEmitFunction(State *  N, LlvmBackendState *  Nl, IrNode *  node)
{
	if(node->type != kNoisyIrNodeType_PfunctionDefn)
	{
		fatal(N, EtokenUnrecognized);
	}

	NoisyFunc *  currentFunc = searchRegFunc(N, Nl, node);
	flexprint(N->Fe, N->Fm, N->Fpllvm, "define %s @%s( %s ) #0 {\n", currentFunc->outputType,
			  currentFunc->name, currentFunc->inputType);
	
	flexprint(N->Fe, N->Fm, N->Fpllvm, "\t%%1 = alloca %s, align 4\n, \tstore %s %%0, %s* %%1, align 3\n",
			  currentFunc->inputType, currentFunc->inputType);
	
	flexprint(N->Fe, N->Fm, N->Fpllvm, "\t%%1 = alloca %s, align 4\n", currentFunc->outputType);

	Nl->lastReg = 2;

	IrNode *  currentNode = R(R(R(node)));
	if (currentNode->type != kNoisyIrNodeType_PscopedStatementList ||
		L(currentNode)->type != kNoisyIrNodeType_PstatementList)
	{
		fatal(NULL, EtokenUnrecognized);
	}

	for (currentNode = L(currentNode); currentNode != NULL; currentNode = R(currentNode))
	{
		continue;
	}	
}

void
irPassLlvmEmitProgram(State *  N, LlvmBackendState *  Nl, IrNode *  node)
{
	if(node->type != kNoisyIrNodeType_Pprogram)
	{
		fatal(N, EtokenUnrecognized);
	}
	irPassLlvmEmitHeader(N);
	irPassLlvmEmitModuleDecl(N, Nl, L(node));

	for (IrNode *  current = R(node); current != NULL; current = R(current))
	{
		irPassLlvmEmitFunction(N, Nl, L(current));
	}
}

void
irPassLlvmBackend(State *  N)
{
	LlvmBackendState *	Nl = llvmBackendStateInit();
	N->backendState = Nl;

	irPassLlvmEmitProgram(N, Nl, N->noisyIrRoot);

	if (N->outputLlvmFilePath)
	{
		FILE *  llvmFile = fopen(N->outputLlvmFilePath, "w");
		if (llvmFile == NULL)
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\n%s: %s.\n", Eopen, N->outputLlvmFilePath);
			consolePrintBuffers(N);
		}

		fprintf(llvmFile, "%s", N->Fpllvm->circbuf);
		fclose(llvmFile);
	}
}