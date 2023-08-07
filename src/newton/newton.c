/*
	Authored 2017. Jonathan Lim
	Updated 2018. Phillip Stanley-Marbell, Youchao Wang
	Updated 2019. Kiseki Hirakawa

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

/*
 *	For asprintf()
 */
#define _GNU_SOURCE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <getopt.h>
#include <setjmp.h>
#include <stdint.h>
#include <assert.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "version.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "common-symbolTable.h"
#include "common-irPass-helpers.h"

#include "newton-parser.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton.h"
#include "newton-irPass-cBackend.h"
#include "newton-irPass-targetParamBackend.h"
#include "newton-irPass-RTLBackend.h"
#include "newton-irPass-dotBackend.h"
#include "newton-irPass-smtBackend.h"
#include "newton-irPass-estimatorSynthesisBackend.h"
#include "newton-irPass-invariantSignalAnnotation.h"
#include "newton-irPass-piGroupsSignalAnnotation.h"
#include "newton-irPass-ipsaBackend.h"
#include "newton-irPass-LLVMIR-dimension-check.h"
#include "newton-irPass-LLVMIR-livenessAnalysis.h"
#include "newton-irPass-LLVMIR-optimizeByRange.h"
#include "newton-irPass-dimensionalMatrixAnnotation.h"
#include "newton-irPass-dimensionalMatrixPiGroups.h"
#include "newton-irPass-dimensionalMatrixPrinter.h"
#include "newton-irPass-dimensionalMatrixKernelPrinter.h"
#include "newton-irPass-dimensionalMatrixConvertToList.h"
#include "newton-irPass-dimensionalMatrixPiGroupWeedOut.h"
#include "newton-irPass-dimensionalMatrixPiGroupSorted.h"
#include "newton-irPass-dimensionalMatrixKernelRowCanonicalization.h"
#include "newton-dimension-prescan.h"
#include "newton-irPass-signalTypedefGenerationBackend.h"
#include "newton-irPass-sensors.h"

extern char *	gNewtonAstNodeStrings[kNoisyIrNodeTypeMax];

static State *
processNewtonFileDimensionPass(char * filename);


void
processNewtonFile(State *  N, char *  filename)
{
	TimeStampTraceMacro(kNewtonTimeStampKey);

	/*
	 *	Tokenize input, then parse it and build AST + symbol table.
	 */
	newtonLexInit(N, filename);

	/*
	 *	Create a top-level scope, then parse.
	 */
	N->newtonIrTopScope = commonSymbolTableAllocScope(N);

	State *	N_dim = processNewtonFileDimensionPass(filename);
	N->newtonIrTopScope->firstDimension = N_dim->newtonIrTopScope->firstDimension;

	if (N->newtonIrTopScope->firstDimension == NULL)
	{
		char *	details;

		asprintf(&details, "%s\n", EnoValidDimensions);
		newtonParserSemanticError(N, kNewtonIrNodeType_PnewtonDescription, details);
		free(details);

		newtonParserErrorRecovery(N, kNewtonIrNodeType_PnewtonDescription);
	}

	N->newtonIrRoot = newtonParse(N, N->newtonIrTopScope);

	if (!(N->irPasses & kNewtonIrPassSensorsDisable))
	{
		irPassSensors(N);
	}	

	if (N->irPasses & kNewtonIrPassDimensionalMatrixAnnotation)
	{
		irPassDimensionalMatrixAnnotation(N);

		if (N->verbosityLevel > 0)
		{
			irPassDimensionalMatrixPrinter(N);
		}
	}

	if (N->irPasses & kNewtonIrPassDimensionalMatrixAnnotationByBody)
	{
		irPassDimensionalMatrixAnnotationByBody(N);

		if (N->verbosityLevel > 0)
		{
			irPassDimensionalMatrixPrinter(N);
		}
	}

	if (N->irPasses & kNewtonIrPassDimensionalMatrixPiGroups)
	{
		irPassDimensionalMatrixPiGroups(N);
	}

	if (N->irPasses & kNewtonIrPassDimensionalMatrixKernelRowCanonicalization)
	{
		irPassDimensionalMatrixKernelRowCanonicalization(N);
	}
	if (N->irPasses & kNewtonIrPassDimensionalMatrixPiGroupSorted)
	{
		irPassDimensionalMatrixPiGroupSorted(N);
	}
	if (N->irPasses & kNewtonIrPassDimensionalMatrixPiGroupsWeedOutDuplicates)
	{
		irPassDimensionalMatrixPiGroupsWeedOutDuplicates(N);
	}
	if (N->irPasses & kNewtonIrPassDimensionalMatrixKernelPrinter)
	{
		irPassDimensionalMatrixKernelPrinter(N);
	}
	if (N->irPasses & kNewtonIrPassDimensionalMatrixKernelPrinterFromBody)
	{
		irPassDimensionalMatrixKernelPrinterFromBodyWithNumOfConstant(N);
	}
	if (N->irPasses & kNewtonIrPassDimensionalMatrixConvertToList)
	{
		irPassDimensionalMatrixConvertToList(N);
	}
	if (N->irPasses & KNewtonIrPassDimensionalMatrixConstantPi)
	{
		irPassDimensionalMatrixConstantPi(N);
	}
	if (N->irPasses & kNewtonIrPassInvariantSignalAnnotation)
	{
		irPassInvariantSignalAnnotation(N);
	}
	if (N->irPasses & kNewtonIrPassPiGroupsSignalAnnotation)
	{
		irPassPiGroupsSignalAnnotation(N);
	}
	if (N->irPasses & kNewtonIrPassLLVMIRDimensionCheck)
	{
		irPassLLVMIRDimensionCheck(N);
	}
	if (N->irPasses & kNewtonIrPassLLVMIRLivenessAnalysis)
	{
		irPassLLVMIRLivenessAnalysis(N);
	}
	if (N->irPasses & kNewtonirPassLLVMIROptimizeByRange)
	{
        bool enableQuantization = N->irPasses & kNewtonirPassLLVMIRAutoQuantization;
        bool enableOverload = N->irPasses & kNewtonirPassLLVMIREnableOverload;
        bool enableBuiltinAssume = N->irPasses & kNewtonirPassLLVMIREnableBuiltinAssume;
		irPassLLVMIROptimizeByRange(N, enableQuantization, enableOverload, enableBuiltinAssume);
	}
	/*
	 *	Dot backend.
	 */
	if (N->irBackends & kNewtonIrBackendDot)
	{
		fprintf(stdout, "%s\n", irPassDotBackend(N, N->newtonIrTopScope, N->newtonIrRoot, gNewtonAstNodeStrings));
 	}

	/*
	 *	Smt backend
	 */
	if (N->irBackends & kNewtonIrBackendSmt)
	{
		irPassSmtBackend(N);
	}

	/*
	 *	Target param backend
	 */
	if (N->irBackends & kNewtonIrBackendTargetParam)
	{
		irPassTargetParamBackend(N);
	}

	/*
	 *	C backend
	 */
	if (N->irBackends & kNewtonIrBackendC)
	{
		irPassCBackend(N);
	}

	/*
	 *	LaTeX backend: Hard work is done in the kernel printer. Just Epilogue here.
	 */
	if (N->irBackends & kNewtonIrBackendLatex)
	{
		flexprint(N->Fe, N->Fm, N->Fpmathjax, "\\end{document}\n");
	}

	if (N->mode & kCommonModeCallTracing)
	{
		timeStampDumpTimeline(N);
	}

	if (N->mode & kCommonModeCallStatistics)
	{
		uint64_t	irNodeCount = 0, symbolTableNodeCount = 0;


		timeStampDumpResidencies(N);

		irNodeCount = irPassHelperIrSize(N, N->newtonIrRoot);
		symbolTableNodeCount = irPassHelperSymbolTableSize(N, N->newtonIrTopScope);


		flexprint(N->Fe, N->Fm, N->Fpinfo, "Intermediate Representation Information:\n\n");
		flexprint(N->Fe, N->Fm, N->Fpinfo, "    IR node count                        : %llu\n", irNodeCount);
		flexprint(N->Fe, N->Fm, N->Fpinfo, "    Symbol Table node count              : %llu\n", symbolTableNodeCount);

		/*
		 *	Libflex malloc statistics:
		 */
		if (N->Fm->debug)
		{
			flexmblocksdisplay(N->Fe, N->Fm, N->Fperr);
		}
	}

	/*
	 *	RTL backend
	 */
	if (N->irBackends & kNewtonIrBackendRTL)
	{
		irPassRTLBackend(N);
	}

	/*
	 *	Estimator Synthesis backend
	 */
	if (N->irBackends & kNewtonIrBackendEstimatorSynthesis)
	{
		irPassEstimatorSynthesisBackend(N);
	}

	/*
	 * Ipsa backend
	 */
	if (N->irBackends & kNewtonIrBackendIpsa)
	{
		irPassIpsaBackend(N, gNewtonAstNodeStrings);
	}

	/*
	 * Signal typedef header generation backend
	 */
	if (N->irBackends & kNewtonIrBackendSignalTypedefHeader)
	{
		irPassSignalTypedefGenerationBackend(N);
	}
}

static State*
processNewtonFileDimensionPass(char * filename)
{
	State *		N = init(kCommonModeDefault);
	

	/*
	 *	In this case, put macro here since it needs 'N'
	 */
	TimeStampTraceMacro(kNewtonTimeStampKey);

	newtonLexInit(N, filename);

	N->newtonIrTopScope = commonSymbolTableAllocScope(N);
	newtonDimensionPassParse(N, N->newtonIrTopScope);

	return N;
}
