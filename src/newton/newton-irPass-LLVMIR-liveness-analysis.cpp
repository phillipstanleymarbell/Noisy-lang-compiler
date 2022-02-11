/*
	Authored 2021. Nikos Mavrogeorgis.

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
#include <setjmp.h>
#include <stdint.h>
#include <set>
#include <algorithm>

#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DebugInfoMetadata.h"

using namespace llvm;


extern "C"
{

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
#include "newton-irPass-autoDiff.h"
#include "newton-irPass-estimatorSynthesisBackend.h"
#include "newton-irPass-invariantSignalAnnotation.h"


typedef struct LivenessState {
	std::map<BasicBlock *, std::set<Value *>>	upwardExposedVariables;
	std::map<BasicBlock *, std::set<Value *>>	killedVariables;
	std::map<BasicBlock *, std::set<Value *>>	liveOutVariables;
} LivenessState;



void
printBasicBlockSets(const std::map<BasicBlock *, std::set<Value *>>&  basicBlockSets)
{
	for (auto const& basicBlockSet : basicBlockSets)
	{
		outs() << "Basic Block: \n";
		outs() << *(basicBlockSet.first->getFirstNonPHI()) << "\n";
		for (auto const& var : basicBlockSet.second)
		{
			outs() << "		" << *var <<  "\n";
		}
		outs() << "=================================\n";
	}
}


void
initBasicBlock(LivenessState *  livenessState, BasicBlock &  llvmIrBasicBlock)
{
	livenessState->upwardExposedVariables[&llvmIrBasicBlock].empty();
	livenessState->killedVariables[&llvmIrBasicBlock].empty();

	for (Instruction &  llvmIrInstruction : llvmIrBasicBlock)
	{
		/*
		 * Instructions of the form:
		 * 		x <- y op z
		 */
		if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
		{
			auto leftOperand = llvmIrBinaryOperator->getOperand(0);
			auto rightOperand = llvmIrBinaryOperator->getOperand(1);

			if (livenessState->killedVariables[&llvmIrBasicBlock].count(leftOperand) == 0 && !isa<llvm::Constant>(leftOperand))
			{
				livenessState->upwardExposedVariables[&llvmIrBasicBlock].insert(leftOperand);
			}

			if (livenessState->killedVariables[&llvmIrBasicBlock].count(rightOperand) == 0 && !isa<llvm::Constant>(rightOperand))
			{
				livenessState->upwardExposedVariables[&llvmIrBasicBlock].insert(rightOperand);
			}

			livenessState->killedVariables[&llvmIrBasicBlock].insert(llvmIrBinaryOperator);
		}
	}

	livenessState->liveOutVariables[&llvmIrBasicBlock].empty();
}

std::set<Value *>
computeLiveOurVariables(LivenessState *  livenessState, BasicBlock &  llvmIrBasicBlock)
{
	std::set<Value *>	computedLiveOutVariables;

	auto 	terminatorInstruction = llvmIrBasicBlock.getTerminator();
	auto	successorNumber = terminatorInstruction->getNumSuccessors();

	for (unsigned int i = 0; i < successorNumber; i++)
	{
		BasicBlock *	successorBasicBlock = terminatorInstruction->getSuccessor(i);

		std::set<Value *>	successorUpwardExposedVariables = livenessState->upwardExposedVariables[successorBasicBlock];
		std::set<Value *>	successorKilledVariables = livenessState->killedVariables[successorBasicBlock];
		std::set<Value *>	successorLiveOutVariables = livenessState->liveOutVariables[successorBasicBlock];

		std::set<Value *>	successorContributionToLiveOutVariables;
		std::set<Value *>	liveOutVariablesSetDifferenceKilledVariables;

		std::set_difference(successorLiveOutVariables.begin(), successorLiveOutVariables.end(),
							successorKilledVariables.begin(), successorKilledVariables.end(),
							std::inserter(liveOutVariablesSetDifferenceKilledVariables,
										  liveOutVariablesSetDifferenceKilledVariables.end()));

		std::set_union(successorUpwardExposedVariables.begin(), successorUpwardExposedVariables.end(),
					   liveOutVariablesSetDifferenceKilledVariables.begin(), liveOutVariablesSetDifferenceKilledVariables.end(),
					   std::inserter(successorContributionToLiveOutVariables,
									 successorContributionToLiveOutVariables.end()));

		std::set_union(computedLiveOutVariables.begin(), computedLiveOutVariables.end(),
					   successorContributionToLiveOutVariables.begin(), successorContributionToLiveOutVariables.end(),
					   std::inserter(computedLiveOutVariables,
									 computedLiveOutVariables.end()));
	}

	return computedLiveOutVariables;
}

void
livenessAnalysis(State *  N, LivenessState *  livenessState, Function &  llvmIrFunction)
{
	for (BasicBlock &  llvmIrBasicBlock : llvmIrFunction)
	{
		initBasicBlock(livenessState, llvmIrBasicBlock);
	}

	auto	changed = true;
	while (changed)
	{
		changed = false;
		for (BasicBlock &  llvmIrBasicBlock : llvmIrFunction)
		{
			auto 	computedLiveOutVariables = computeLiveOurVariables(livenessState, llvmIrBasicBlock);

			if (computedLiveOutVariables != livenessState->liveOutVariables[&llvmIrBasicBlock])
			{
				changed = true;
				livenessState->liveOutVariables[&llvmIrBasicBlock] = computedLiveOutVariables;
			}
		}
	}
}


void
irPassLLVMIRLivenessAnalysis(State *  N)
{
	if (N->llvmIR == nullptr)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Please specify the LLVM IR input file\n");
		fatal(N, Esanity);
	}

	SMDiagnostic 	Err;
	LLVMContext 	Context;
	std::unique_ptr<Module>	Mod(parseIRFile(N->llvmIR, Err, Context));
	if (!Mod)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Error: Couldn't parse IR file.");
		fatal(N, Esanity);
	}

	auto	livenessState = new LivenessState();

	for (auto & mi : *Mod)
	{
		livenessAnalysis(N, livenessState, mi);
	}
	printBasicBlockSets(livenessState->liveOutVariables);
}

}
