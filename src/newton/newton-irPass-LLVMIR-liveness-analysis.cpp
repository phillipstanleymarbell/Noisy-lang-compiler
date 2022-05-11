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


typedef struct BoundInfo {
  std::map<std::string, std::pair<double, double>> variableBound;
  std::map<std::string, std::pair<double, double>> typeRange;
} BoundInfo;

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
		switch (llvmIrInstruction.getOpcode())
		{
			case Instruction::Add:
			case Instruction::FAdd:
			case Instruction::Sub:
			case Instruction::FSub:
			case Instruction::Mul:
			case Instruction::FMul:
			case Instruction::SDiv:
			case Instruction::FDiv:
			case Instruction::UDiv:
			case Instruction::URem:
			case Instruction::SRem:
			case Instruction::FRem:
			case Instruction::And:
			case Instruction::Or:
			case Instruction::Xor:
			case Instruction::ICmp:
			case Instruction::FCmp:
			{
				auto leftOperand = llvmIrInstruction.getOperand(0);
				auto rightOperand = llvmIrInstruction.getOperand(1);

				if (livenessState->killedVariables[&llvmIrBasicBlock].count(leftOperand) == 0 && !isa<llvm::Constant>(leftOperand))
				{
					livenessState->upwardExposedVariables[&llvmIrBasicBlock].insert(leftOperand);
				}

				if (livenessState->killedVariables[&llvmIrBasicBlock].count(rightOperand) == 0 && !isa<llvm::Constant>(rightOperand))
				{
					livenessState->upwardExposedVariables[&llvmIrBasicBlock].insert(rightOperand);
				}

				livenessState->killedVariables[&llvmIrBasicBlock].insert(&llvmIrInstruction);
			}
			case Instruction::SExt:
			case Instruction::ZExt:
			case Instruction::AShr:
			case Instruction::LShr:
			case Instruction::Shl:
			case Instruction::Trunc:
			case Instruction::SIToFP:
			case Instruction::FNeg:
			case Instruction::FPToUI:
			case Instruction::FPToSI:
			case Instruction::UIToFP:
			case Instruction::FPTrunc:
			case Instruction::FPExt:
			case Instruction::PtrToInt:
			case Instruction::IntToPtr:
			case Instruction::BitCast:
			case Instruction::AddrSpaceCast:
			case Instruction::ExtractElement:
			{
				auto operand = llvmIrInstruction.getOperand(0);

				if (livenessState->killedVariables[&llvmIrBasicBlock].count(operand) == 0 && !isa<llvm::Constant>(operand))
				{
					livenessState->upwardExposedVariables[&llvmIrBasicBlock].insert(operand);
				}

				livenessState->killedVariables[&llvmIrBasicBlock].insert(&llvmIrInstruction);
			}
			case Instruction::Call:
			{
				if (auto llvmIrCallInstruction = dyn_cast<CallInst>(&llvmIrInstruction)) {
					Function *calledFunction = llvmIrCallInstruction->getCalledFunction();
					if (calledFunction->getName().startswith("llvm.dbg.declare")) {
						continue;
					}
					for (auto &argumentIterator: llvmIrCallInstruction->args()) {
						if (livenessState->killedVariables[&llvmIrBasicBlock].count(argumentIterator) == 0 &&
							!isa<llvm::Constant>(*argumentIterator)) {
							livenessState->upwardExposedVariables[&llvmIrBasicBlock].insert(argumentIterator);
						}

						livenessState->killedVariables[&llvmIrBasicBlock].insert(llvmIrCallInstruction);
					}
				}
			}
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
inferBound(State * N, BoundInfo * boundInfo, Function & llvmIrFunction)
{
	std::map<Value *, std::pair<double, double>> virtualRegisterRange;
	std::map<Value *, std::string>		     valueName;
	for (BasicBlock & llvmIrBasicBlock : llvmIrFunction)
	{
		for (Instruction & llvmIrInstruction : llvmIrBasicBlock)
		{
			switch (llvmIrInstruction.getOpcode())
			{
				case Instruction::Call:
					if (auto llvmIrCallInstruction = dyn_cast<CallInst>(&llvmIrInstruction))
					{
						Function * calledFunction = llvmIrCallInstruction->getCalledFunction();
						if (calledFunction->getName().startswith("llvm.dbg.declare"))
						{
							auto firstOperator = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(0));
							auto localVariableAddressAsMetadata = cast<ValueAsMetadata>(firstOperator->getMetadata());
							auto localVariableAddress = localVariableAddressAsMetadata->getValue();

							auto variableMetadata = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(1));
							auto debugInfoVariable = cast<DIVariable>(variableMetadata->getMetadata());
							auto variableType = debugInfoVariable->getType();
							valueName.emplace(localVariableAddress, debugInfoVariable->getName().str());

							/*
							 *	if we find such type in boundInfo->typeRange,
							 *	we get its range and bind the var with it in boundInfo->variableBound
							 *	and record it in the virtualRegisterRange
							 */
							auto typeRangeIt = boundInfo->typeRange.find(variableType->getName().str());
							if (typeRangeIt != boundInfo->typeRange.end())
							{
								boundInfo->variableBound.emplace(debugInfoVariable->getName().str(), typeRangeIt->second);
								virtualRegisterRange.emplace(localVariableAddress, typeRangeIt->second);
							}
						}
					}
					break;

				case Instruction::Add:
				case Instruction::FAdd:
					if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
					{
						Value * leftOperand = llvmIrInstruction.getOperand(0);
						Value * rightOperand = llvmIrInstruction.getOperand(1);
						/*
						 * 	todo: expression normalization needed, which simpily the "const + const" or normalize into the "var + const" form
						 *	so this if-branch is a debug message, and will be deleted after finishing the expression normalization
						 */
						if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)) ||
						    (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tExpression normalization needed.\n");
						}

						if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
							double lowerBound = 0.0;
							double upperBound = 0.0;

							/*
							 * 	eg. x1+x2
							 * 	btw, I don't think we should check type here, which should be done in other pass like dimension-check
							 * 	find left operand from the virtualRegisterRange
							 */
							auto vrRangeIt = virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								lowerBound = vrRangeIt->second.first;
								upperBound = vrRangeIt->second.second;
							}
							/*
							 * 	find right operand from the virtualRegisterRange
							 */
							vrRangeIt = virtualRegisterRange.find(rightOperand);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								lowerBound += vrRangeIt->second.first;
								upperBound += vrRangeIt->second.second;
							}
							virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
						}
						else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							/*
							 * 	eg. x+2
							 */
							double constValue = 0.0;
							if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
							{
								/*
								 * 	both "float" and "double" type can use "convertToDouble"
								 */
								constValue = (constFp->getValueAPF()).convertToDouble();
							}
							auto vrRangeIt = virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								virtualRegisterRange.emplace(llvmIrBinaryOperator,
											     std::make_pair(vrRangeIt->second.first + constValue,
													    vrRangeIt->second.second + constValue));
							}
						}
						else
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tUnexpected error. Might have an invalid operand.\n");
						}
					}
					break;

				case Instruction::Sub:
				case Instruction::FSub:
					if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
					{
						Value * leftOperand = llvmIrInstruction.getOperand(0);
						Value * rightOperand = llvmIrInstruction.getOperand(1);
						/*
						 * 	todo: expression normalization needed, which simpily the "const - const" form
						 * 	so this if-branch is a debug message, and will be deleted after finishing the expression normalization
						 */
						if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tExpression normalization needed.\n");
						}

						if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
							double lowerBound = 0.0;
							double upperBound = 0.0;

							/*
							 * 	eg. x1-x2
							 * 	btw, I don't think we should check type here, which should be done in other pass like dimension-check
							 * 	find left operand from the virtualRegisterRange
							 */
							auto vrRangeIt = virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								lowerBound = vrRangeIt->second.first;
								upperBound = vrRangeIt->second.second;
							}
							vrRangeIt = virtualRegisterRange.find(rightOperand);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								lowerBound -= vrRangeIt->second.second;
								upperBound -= vrRangeIt->second.first;
							}
							virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
						}
						else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							/*
							 * 	eg. x-2
							 */
							double constValue = 0.0;
							if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
							{
								constValue = (constFp->getValueAPF()).convertToDouble();
							}
							auto vrRangeIt = virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								virtualRegisterRange.emplace(llvmIrBinaryOperator,
											     std::make_pair(vrRangeIt->second.first - constValue,
													    vrRangeIt->second.second - constValue));
							}
						}
						else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
							/*
							 * 	eg. 2-x
							 */
							double constValue = 0.0;
							if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(leftOperand))
							{
								constValue = (constFp->getValueAPF()).convertToDouble();
							}
							auto vrRangeIt = virtualRegisterRange.find(rightOperand);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								virtualRegisterRange.emplace(llvmIrBinaryOperator,
											     std::make_pair(constValue - vrRangeIt->second.second,
													    constValue - vrRangeIt->second.first));
							}
						}
						else
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tUnexpected error. Might have an invalid operand.\n");
						}
					}
					break;

				case Instruction::Mul:
				case Instruction::FMul:
					if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
					{
						Value * leftOperand = llvmIrInstruction.getOperand(0);
						Value * rightOperand = llvmIrInstruction.getOperand(1);
						/*
						 * 	todo: expression normalization needed, which simpily the "const * const" or normalize into the "var * const" form
						 * 	so this if-branch is a debug message, and will be deleted after finishing the expression normalization
						 */
						if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)) ||
						    (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tExpression normalization needed.\n");
						}

						if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
							/*
							 * 	todo: let's measure the "var * var" the next time...
							 */
							flexprint(N->Fe, N->Fm, N->Fperr, "\tIt's a var * var expression, which is not supported yet.\n");
						}
						else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							/*
							 * 	eg. x*2
							 */
							double constValue = 1.0;
							if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
							{
								constValue = (constFp->getValueAPF()).convertToDouble();
							}
							auto vrRangeIt = virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								virtualRegisterRange.emplace(llvmIrBinaryOperator,
											     std::make_pair(vrRangeIt->second.first * constValue,
													    vrRangeIt->second.second * constValue));
							}
						}
						else
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tUnexpected error. Might have an invalid operand.\n");
						}
					}
					break;

				case Instruction::SDiv:
				case Instruction::FDiv:
				case Instruction::UDiv:
					if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
					{
						Value * leftOperand = llvmIrInstruction.getOperand(0);
						Value * rightOperand = llvmIrInstruction.getOperand(1);
						/*
						 * 	todo: expression normalization needed, which simpily the "const / const" form
						 * 	and the assertion of "rightOperand.value != 0" should be done in expression normalization too
						 * 	so this if-branch is a debug message, and will be deleted after finishing the expression normalization
						 */
						if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tExpression normalization needed.\n");
						}

						if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
							/*
							 * 	todo: let's measure the "var / var" the next time...
							 */
							flexprint(N->Fe, N->Fm, N->Fperr, "\tIt's a var / var expression, which is not supported yet.\n");
						}
						else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
							/*
							 * 	todo: I don't know if we need to deal with "const / var" here,
							 * 	like 2/x. if x in [-4, 4], then the range of 2/x is (-inf, -0.5] and [0.5, inf)
							 */
							flexprint(N->Fe, N->Fm, N->Fperr, "\tIt's a const / var expression, which is not supported yet.\n");
						}
						else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							/*
							 * 	eg. x/2
							 */
							double constValue = 1.0;
							if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
							{
								constValue = (constFp->getValueAPF()).convertToDouble();
							}
							auto vrRangeIt = virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								virtualRegisterRange.emplace(llvmIrBinaryOperator,
											     std::make_pair(vrRangeIt->second.first / constValue,
													    vrRangeIt->second.second / constValue));
							}
						}
						else
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tUnexpected error. Might have an invalid operand.\n");
						}
					}
					break;

				case Instruction::Shl:
					if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
					{
						Value * leftOperand = llvmIrInstruction.getOperand(0);
						Value * rightOperand = llvmIrInstruction.getOperand(1);
						/*
						 * 	todo: expression normalization needed, which simpily the "const << const" form
						 * 	so this if-branch is a debug message, and will be deleted after finishing the expression normalization
						 */
						if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tExpression normalization needed.\n");
						}

						if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
							/*
							 * 	todo: I don't know if we need to concern the "var << var"
							 */
							flexprint(N->Fe, N->Fm, N->Fperr, "\tIt's a var << var expression, which is not supported yet.\n");
						}
						else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
							/*
							 * 	todo: I don't know if we need to deal with "const << var" here,
							 */
							flexprint(N->Fe, N->Fm, N->Fperr, "\tIt's a const << var expression, which is not supported yet.\n");
						}
						else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							/*
							 * 	eg. x<<2
							 */
							int constValue = 1.0;
							if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
							{
								constValue = constInt->getZExtValue();
							}
							auto vrRangeIt = virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								virtualRegisterRange.emplace(llvmIrBinaryOperator,
											     std::make_pair((int)vrRangeIt->second.first << constValue,
													    (int)vrRangeIt->second.second << constValue));
							}
						}
						else
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tUnexpected error. Might have an invalid operand.\n");
						}
					}
					break;

				case Instruction::AShr:
					if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
					{
						Value * leftOperand = llvmIrInstruction.getOperand(0);
						Value * rightOperand = llvmIrInstruction.getOperand(1);
						/*
						 * 	todo: expression normalization needed, which simpily the "const >> const" form
						 * 	so this if-branch is a debug message, and will be deleted after finishing the expression normalization
						 */
						if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tExpression normalization needed.\n");
						}

						if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
							/*
							 * 	todo: I don't know if we need to concern the "var >> var"
							 */
							flexprint(N->Fe, N->Fm, N->Fperr, "\tIt's a var << var expression, which is not supported yet.\n");
						}
						else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
							/*
							 * 	todo: I don't know if we need to deal with "const >> var" here,
							 */
							flexprint(N->Fe, N->Fm, N->Fperr, "\tIt's a const << var expression, which is not supported yet.\n");
						}
						else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							/*
							 *	eg. x>>2
							 */
							int constValue = 1.0;
							if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
							{
								constValue = constInt->getZExtValue();
							}
							auto vrRangeIt = virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								virtualRegisterRange.emplace(llvmIrBinaryOperator,
											     std::make_pair((int)vrRangeIt->second.first >> constValue,
													    (int)vrRangeIt->second.second >> constValue));
							}
						}
						else
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tUnexpected error. Might have an invalid operand.\n");
						}
					}
					break;

				case Instruction::FPToUI:
				case Instruction::FPToSI:
				case Instruction::SIToFP:
				case Instruction::UIToFP:
				{
					Value * operand = llvmIrInstruction.getOperand(0);
					auto	vrRangeIt = virtualRegisterRange.find(operand);
					if (vrRangeIt != virtualRegisterRange.end())
					{
						virtualRegisterRange.emplace(&llvmIrInstruction,
									     vrRangeIt->second);
					}
				}
				break;

				/*
				* 	Some load instruction may not need this.
				* 	Need to examine that. TODO
				*/
				case Instruction::Load:
					if (auto llvmIrLoadInstruction = dyn_cast<LoadInst>(&llvmIrInstruction))
					{
						auto vrRangeIt = virtualRegisterRange.find(llvmIrLoadInstruction->getOperand(0));
						if (vrRangeIt != virtualRegisterRange.end())
						{
							virtualRegisterRange.emplace(llvmIrLoadInstruction, vrRangeIt->second);
						}
					}
					break;

				case Instruction::Store:
					if (auto llvmIrStoreInstruction = dyn_cast<StoreInst>(&llvmIrInstruction))
					{
						Value * leftTerm = llvmIrStoreInstruction->getOperand(0);
						Value * rightTerm = llvmIrStoreInstruction->getOperand(1);
						/*
						 * 	here we focus on the "var2 = var1" pattern
						 * 	eg. store double %5, double* %3, align 8, !dbg !19
						 */
						if (!leftTerm->hasName() && !rightTerm->hasName())
						{
							auto vrRangeIt = virtualRegisterRange.find(leftTerm);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								/*
							  	 * 	find the variable from the virtualRegisterRange
							  	 */
								auto valueNameIt = valueName.find(rightTerm);
								if (valueNameIt != valueName.end())
								{
									boundInfo->variableBound.emplace(valueNameIt->second, vrRangeIt->second);
								}
							}
						}
					}
					break;

				default:
					continue;
			}
		}
	}
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

  flexprint(N->Fe, N->Fm, N->Fpinfo, "infer bound\n");
  auto boundInfo = new BoundInfo();

	/*
   * 	get sensor info, we only concern the id and range here
   */
  if (N->sensorList != NULL)
  {
    for (Modality * currentModality = N->sensorList->modalityList; currentModality != NULL; currentModality = currentModality->next)
    {
      flexprint(N->Fe, N->Fm, N->Fpinfo, "\tModality: %s\n", currentModality->identifier);
      flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\trangeLowerBound: %f\n", currentModality->rangeLowerBound);
      flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\trangeUpperBound: %f\n", currentModality->rangeUpperBound);
      boundInfo->typeRange.emplace(currentModality->identifier,
                                   std::make_pair(currentModality->rangeLowerBound, currentModality->rangeUpperBound));
    }
  }

  for (auto& mi : *Mod)
  {
    inferBound(N, boundInfo, mi);
  }

  flexprint(N->Fe, N->Fm, N->Fpinfo, "\nafter infer bound\n");
  for (auto& vr: boundInfo->variableBound) {
    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tvariable: %s, range: %f -> %f\n",
              vr.first.data(), vr.second.first, vr.second.second);
  }

	auto	livenessState = new LivenessState();

	for (auto & mi : *Mod)
	{
		livenessAnalysis(N, livenessState, mi);
	}
	printBasicBlockSets(livenessState->liveOutVariables);
}

}
