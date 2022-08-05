/*
	Authored 2022. Pei Mu.

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
#include <string.h>
#include <set>
#include <algorithm>

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Path.h"

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

typedef struct BoundInfo {
	std::map<std::string, std::pair<double, double>> variableBound;
	std::map<std::string, std::pair<double, double>> typeRange;
} BoundInfo;

enum CmpRes {
	Depends	    = 1,
	AlwaysTrue  = 2,
	AlwaysFalse = 3,
	Unsupported = 6,
};

void
dumpIR(State * N, std::string fileSuffix, std::unique_ptr<Module> Mod)
{
	StringRef   filePath(N->llvmIR);
	std::string dirPath	= std::string(sys::path::parent_path(filePath)) + "/";
	std::string fileName	= std::string(sys::path::stem(filePath)) + "_" + fileSuffix + ".bc";
	std::string filePathStr = dirPath + fileName;
	filePath		= StringRef(filePathStr);

	flexprint(N->Fe, N->Fm, N->Fpinfo, "Dump IR of: %s\n", filePath.str().c_str());
	std::error_code errorCode(errno, std::generic_category());
	raw_fd_ostream	dumpedFile(filePath, errorCode);
	WriteBitcodeToFile(*Mod, dumpedFile);
	dumpedFile.close();
}

CmpRes
compareFCmpWithVariableRange(FCmpInst * llvmIrFCmpInstruction, double variableLowerBound, double variableUpperBound, double constValue)
{
	switch (llvmIrFCmpInstruction->getPredicate())
	{
		case FCmpInst::FCMP_TRUE:
			return CmpRes::AlwaysTrue;
		case FCmpInst::FCMP_FALSE:
			return CmpRes::AlwaysFalse;
			/*
			 * Ordered means that neither operand is a QNAN while unordered means that either operand may be a QNAN.
			 * More details in https://llvm.org/docs/LangRef.html#fcmp-instruction
			 * */
		case FCmpInst::FCMP_OEQ:
		case FCmpInst::FCMP_UEQ:
			if ((variableLowerBound == variableUpperBound) && (variableUpperBound == constValue))
			{
				return CmpRes::AlwaysTrue;
			}
			else
			{
				return CmpRes::AlwaysFalse;
			}
		case FCmpInst::FCMP_OGT:
		case FCmpInst::FCMP_UGT:
			if (variableLowerBound > constValue)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (variableUpperBound <= constValue)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		case FCmpInst::FCMP_OGE:
		case FCmpInst::FCMP_UGE:
			if (variableLowerBound >= constValue)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (variableUpperBound < constValue)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		case FCmpInst::FCMP_OLT:
		case FCmpInst::FCMP_ULT:
			if (variableUpperBound < constValue)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (variableLowerBound >= constValue)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		case FCmpInst::FCMP_OLE:
		case FCmpInst::FCMP_ULE:
			if (variableUpperBound <= constValue)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (variableLowerBound > constValue)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		case FCmpInst::FCMP_ONE:
		case FCmpInst::FCMP_UNE:
			if ((variableLowerBound == variableUpperBound) && (variableUpperBound != constValue))
			{
				return CmpRes::AlwaysTrue;
			}
			else
			{
				return CmpRes::AlwaysFalse;
			}
		default:
			return CmpRes::Unsupported;
	}
}

CmpRes
compareICmpWithVariableRange(ICmpInst * llvmIrICmpInstruction, double variableLowerBound, double variableUpperBound, double constValue)
{
	switch (llvmIrICmpInstruction->getPredicate())
	{
			/*
			 * Ordered means that neither operand is a QNAN while unordered means that either operand may be a QNAN.
			 * More details in https://llvm.org/docs/LangRef.html#icmp-instruction
			 * */
		case ICmpInst::ICMP_EQ:
			if ((variableLowerBound == variableUpperBound) && (variableUpperBound == constValue))
			{
				return CmpRes::AlwaysTrue;
			}
			else
			{
				return CmpRes::AlwaysFalse;
			}
		case ICmpInst::ICMP_NE:
			if ((variableLowerBound == variableUpperBound) && (variableUpperBound != constValue))
			{
				return CmpRes::AlwaysTrue;
			}
			else
			{
				return CmpRes::AlwaysFalse;
			}
		case ICmpInst::ICMP_UGT:
		case ICmpInst::ICMP_SGT:
			if (variableLowerBound > constValue)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (variableUpperBound <= constValue)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		case ICmpInst::ICMP_UGE:
		case ICmpInst::ICMP_SGE:
			if (variableLowerBound >= constValue)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (variableUpperBound < constValue)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		case ICmpInst::ICMP_ULT:
		case ICmpInst::ICMP_SLT:
			if (variableUpperBound < constValue)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (variableLowerBound >= constValue)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		case ICmpInst::ICMP_ULE:
		case ICmpInst::ICMP_SLE:
			if (variableUpperBound <= constValue)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (variableLowerBound > constValue)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		default:
			return CmpRes::Unsupported;
	}
}

static Type *
GetCompareTy(Value * Op)
{
	return CmpInst::makeCmpResultType(Op->getType());
}

/*
 * For a boolean type or a vector of boolean type, return false or a vector
 * with every element false.
 * */
static llvm::Constant *
getFalse(Type * Ty)
{
	return ConstantInt::getFalse(Ty);
}

/*
 * For a boolean type or a vector of boolean type, return true or a vector
 * with every element true.
 * */
static llvm::Constant *
getTrue(Type * Ty)
{
	return ConstantInt::getTrue(Ty);
}

void
simplifyControlFlow(State * N, BoundInfo * boundInfo, Function & llvmIrFunction)
{
	/*
	 * Change attr of the function
	 * */
	llvmIrFunction.removeFnAttr(Attribute::OptimizeNone);

	std::map<Value *, std::pair<double, double>> virtualRegisterRange;
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
							auto firstOperator		    = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(0));
							auto localVariableAddressAsMetadata = cast<ValueAsMetadata>(firstOperator->getMetadata());
							auto localVariableAddress	    = localVariableAddressAsMetadata->getValue();

							auto variableMetadata  = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(1));
							auto debugInfoVariable = cast<DIVariable>(variableMetadata->getMetadata());
							auto variableType      = debugInfoVariable->getType();

							/*
							 * if we find such type in boundInfo->typeRange,
							 * we get its range and bind the var with it in boundInfo->variableBound
							 * and record it in the virtualRegisterRange
							 * */
							auto typeRangeIt = boundInfo->typeRange.find(variableType->getName().str());
							if (typeRangeIt != boundInfo->typeRange.end())
							{
								boundInfo->variableBound.emplace(debugInfoVariable->getName().str(), typeRangeIt->second);
								virtualRegisterRange.emplace(localVariableAddress, typeRangeIt->second);
							}
						}
					}
					break;

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
				case Instruction::ICmp:
					if (auto llvmIrICmpInstruction = dyn_cast<ICmpInst>(&llvmIrInstruction))
					{
						auto leftOperand  = llvmIrICmpInstruction->getOperand(0);
						auto rightOperand = llvmIrICmpInstruction->getOperand(1);
						if ((isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
						{
							std::swap(leftOperand, rightOperand);
							flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: swap left and right\n");
						}
						else if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tICmp: Expression normalization needed.\n");
						}
						else if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
						}
						else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							double constValue = 0.0;
							if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
							{
								constValue = constInt->getSExtValue();
							}
							else
							{
								flexprint(N->Fe, N->Fm, N->Fperr, "\tICmp: it's not a const fp!!!!!!!!!!!\n");
							}
							flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: right operand: %f\n", constValue);
							/*
							 * find the variable from the virtualRegisterRange
							 * */
							auto vrRangeIt = virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: varibale's lower bound: %f, upper bound: %f\n",
									  vrRangeIt->second.first, vrRangeIt->second.second);
								CmpRes compareResult = compareICmpWithVariableRange(llvmIrICmpInstruction,
														    vrRangeIt->second.first, vrRangeIt->second.second, constValue);
								flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: the comparison result is %d\n", compareResult);
								/*
								 * Fold trivial predicates.
								 * */
								Type *	retTy	 = GetCompareTy(leftOperand);
								Value * resValue = nullptr;
								if (compareResult == CmpRes::AlwaysTrue)
								{
									resValue = getTrue(retTy);
									llvmIrICmpInstruction->replaceAllUsesWith(resValue);
								}
								else if (compareResult == CmpRes::AlwaysFalse)
								{
									resValue = getFalse(retTy);
									llvmIrICmpInstruction->replaceAllUsesWith(resValue);
								}
								else if (compareResult == CmpRes::Unsupported)
								{
									flexprint(N->Fe, N->Fm, N->Fperr, "\tICmp: Current ICmp Predicate is not supported.\n");
								}
							}
							else
							{
								flexprint(N->Fe, N->Fm, N->Fperr, "\tICmp: Unknown variable\n");
							}
						}
					}
					break;

				case Instruction::FCmp:
					if (auto llvmIrFCmpInstruction = dyn_cast<FCmpInst>(&llvmIrInstruction))
					{
						auto leftOperand  = llvmIrFCmpInstruction->getOperand(0);
						auto rightOperand = llvmIrFCmpInstruction->getOperand(1);
						if ((isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
						{
							std::swap(leftOperand, rightOperand);
							flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFCmp: swap left and right\n");
						}
						else if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tFCmp: Expression normalization needed.\n");
						}
						else if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
						}
						else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							double constValue = 0.0;
							if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
							{
								/*
								 * both "float" and "double" type can use "convertToDouble"
								 * */
								constValue = (constFp->getValueAPF()).convertToDouble();
							}
							flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFCmp: right operand: %f\n", constValue);
							/*
							 * find the variable from the virtualRegisterRange
							 * */
							auto vrRangeIt = virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFCmp: varibale's lower bound: %f, upper bound: %f\n",
									  vrRangeIt->second.first, vrRangeIt->second.second);
								CmpRes compareResult = compareFCmpWithVariableRange(llvmIrFCmpInstruction,
														    vrRangeIt->second.first, vrRangeIt->second.second, constValue);
								flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFCmp: the comparison result is %d\n", compareResult);
								/*
								 * Fold trivial predicates.
								 * */
								Type *	retTy	 = GetCompareTy(leftOperand);
								Value * resValue = nullptr;
								if (compareResult == CmpRes::AlwaysTrue)
								{
									resValue = getTrue(retTy);
									llvmIrFCmpInstruction->replaceAllUsesWith(resValue);
								}
								else if (compareResult == CmpRes::AlwaysFalse)
								{
									resValue = getFalse(retTy);
									llvmIrFCmpInstruction->replaceAllUsesWith(resValue);
								}
							}
							else
							{
								flexprint(N->Fe, N->Fm, N->Fperr, "\tFCmp: Unknown variable\n");
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
irPassLLVMIRSimplifyControlFlowByRange(State * N)
{
	if (N->llvmIR == nullptr)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Please specify the LLVM IR input file\n");
		fatal(N, Esanity);
	}

	SMDiagnostic		Err;
	LLVMContext		Context;
	std::unique_ptr<Module> Mod(parseIRFile(N->llvmIR, Err, Context));
	if (!Mod)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Error: Couldn't parse IR file.");
		fatal(N, Esanity);
	}

	flexprint(N->Fe, N->Fm, N->Fpinfo, "simplify control flow by range\n");
	auto boundInfo = new BoundInfo();

	/*
	 * get sensor info, we only concern the id and range here
	 * */
	if (N->sensorList != NULL)
	{
		for (Modality * currentModality = N->sensorList->modalityList; currentModality != NULL; currentModality = currentModality->next)
		{
			flexprint(N->Fe, N->Fm, N->Fpinfo, "\tModality: %s\n", currentModality->identifier);
			flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\trangeLowerBound: %f\n", currentModality->rangeLowerBound);
			flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\trangeUpperBound: %f\n", currentModality->rangeUpperBound);
			boundInfo->typeRange.emplace(currentModality->identifier, std::make_pair(currentModality->rangeLowerBound, currentModality->rangeUpperBound));
		}
	}

	for (auto & mi : *Mod)
	{
		simplifyControlFlow(N, boundInfo, mi);
	}

	dumpIR(N, "output", std::move(Mod));
}
}
