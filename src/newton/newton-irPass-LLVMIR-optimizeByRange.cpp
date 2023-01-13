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

#ifdef __cplusplus
#include "newton-irPass-LLVMIR-rangeAnalysis.h"
#include "newton-irPass-LLVMIR-simplifyControlFlowByRange.h"
#include "newton-irPass-LLVMIR-constantSubstitution.h"
#include "newton-irPass-LLVMIR-shrinkTypeByRange.h"
#endif /* __cplusplus */

#include <algorithm>
#include <cmath>
#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <string.h>
#include <set>

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

extern "C"{

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

void mergeBoundInfo(BoundInfo* dst, const BoundInfo* src) {
    dst->virtualRegisterRange.insert(src->virtualRegisterRange.begin(),
                                     src->virtualRegisterRange.end());
    return;
}

void collectCalleeBoundInfo(std::map<std::string, BoundInfo*>& funcBoundInfo, const BoundInfo* boundInfo) {
    for (auto & calleeInfo : boundInfo->calleeBound) {
        funcBoundInfo.emplace(calleeInfo.first, calleeInfo.second);
        collectCalleeBoundInfo(funcBoundInfo, calleeInfo.second);
    }
    return;
}

class FunctionNode {
    mutable AssertingVH <Function> F;
    FunctionComparator::FunctionHash Hash;

public:
    // Note the hash is recalculated potentially multiple times, but it is cheap.
    FunctionNode(Function *F)
            : F(F), Hash(FunctionComparator::functionHash(*F)) {}

    Function *getFunc() const { return F; }

    FunctionComparator::FunctionHash getHash() const { return Hash; }
};

GlobalNumberState GlobalNumbers;

class FunctionNodeCmp {
public:
    bool operator()(const FunctionNode &LHS, const FunctionNode &RHS) const {
        // Order first by hashes, then full function comparison.
        if (LHS.getHash() != RHS.getHash())
            return LHS.getHash() < RHS.getHash();
        FunctionComparator FCmp(LHS.getFunc(), RHS.getFunc(), &GlobalNumbers);
        return FCmp.compare() == -1;
    }
};

using hashFuncSet = std::set<FunctionNode, FunctionNodeCmp>;

void
irPassLLVMIROptimizeByRange(State * N)
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

	auto globalBoundInfo = new BoundInfo();
    std::map<std::string, BoundInfo*> funcBoundInfo;

	/*
	 * get sensor info, we only concern the id and range here
	 * */
    std::map<std::string, std::pair<double, double>> typeRange;
	if (N->sensorList != NULL)
	{
		for (Modality * currentModality = N->sensorList->modalityList; currentModality != NULL; currentModality = currentModality->next)
		{
			flexprint(N->Fe, N->Fm, N->Fpinfo, "\tModality: %s\n", currentModality->identifier);
			flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\trangeLowerBound: %f\n", currentModality->rangeLowerBound);
			flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\trangeUpperBound: %f\n", currentModality->rangeUpperBound);
			typeRange.emplace(currentModality->identifier, std::make_pair(currentModality->rangeLowerBound, currentModality->rangeUpperBound));
		}
	}

	/*
	 * get const global variables
	 * */
    std::map<llvm::Value *, std::vector<std::pair<double, double>>> virtualRegisterVectorRange;
	for (auto & globalVar : Mod->getGlobalList())
	{
		if (!globalVar.hasInitializer())
		{
			continue;
		}
		auto constValue = globalVar.getInitializer();
		if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(constValue))
		{
			if (constValue->getType()->isFloatTy())
			{
				float constValue = constFp->getValueAPF().convertToFloat();
				globalBoundInfo->virtualRegisterRange.emplace(&globalVar, std::make_pair(constValue, constValue));
			}
			else if (constValue->getType()->isDoubleTy())
			{
				double constValue = constFp->getValueAPF().convertToDouble();
				globalBoundInfo->virtualRegisterRange.emplace(&globalVar, std::make_pair(constValue, constValue));
			}
		}
		else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(constValue))
		{
			auto constValue = constInt->getSExtValue();
			globalBoundInfo->virtualRegisterRange.emplace(&globalVar, std::make_pair(static_cast<double>(constValue),
											   static_cast<double>(constValue)));
		}
		else if (ConstantDataArray * constArr = llvm::dyn_cast<llvm::ConstantDataArray>(constValue))
		{
			auto arrType = constArr->getElementType();
			if (arrType->isDoubleTy())
			{
				for (size_t idx = 0; idx < constArr->getNumElements(); idx++)
				{
					double dbValue = constArr->getElementAsDouble(idx);
					virtualRegisterVectorRange[&globalVar].emplace_back(std::make_pair(dbValue, dbValue));
				}
			}
			else if (arrType->isFloatTy())
			{
				for (size_t idx = 0; idx < constArr->getNumElements(); idx++)
				{
					double ftValue = constArr->getElementAsFloat(idx);
					virtualRegisterVectorRange[&globalVar].emplace_back(std::make_pair(ftValue, ftValue));
				}
			}
			else if (arrType->isIntegerTy())
			{
				for (size_t idx = 0; idx < constArr->getNumElements(); idx++)
				{
					uint64_t intValue = constArr->getElementAsInteger(idx);
					virtualRegisterVectorRange[&globalVar].emplace_back(std::make_pair(intValue, intValue));
				}
			}
			else if (arrType->isPointerTy())
			{
				flexprint(N->Fe, N->Fm, N->Fperr, "\t\tTODO: Didn't support const pointer!\n");
			}
			else
			{
				flexprint(N->Fe, N->Fm, N->Fperr, "\t\tUnknown constant type!\n");
			}
		}
		else
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\t\tUnknown type!\n");
		}
	}

	legacy::PassManager passManager;

	flexprint(N->Fe, N->Fm, N->Fpinfo, "infer bound\n");
	for (auto & mi : *Mod)
	{
        auto boundInfo = new BoundInfo();
        mergeBoundInfo(boundInfo, globalBoundInfo);
		rangeAnalysis(N, typeRange, virtualRegisterVectorRange, boundInfo, mi);
        funcBoundInfo.emplace(mi.getName(), boundInfo);
        collectCalleeBoundInfo(funcBoundInfo, boundInfo);
	}

	flexprint(N->Fe, N->Fm, N->Fpinfo, "simplify control flow by range\n");
	for (auto & mi : *Mod)
	{
        auto boundInfoIt = funcBoundInfo.find(mi.getName().str());
        if (boundInfoIt != funcBoundInfo.end()) {
            simplifyControlFlow(N, boundInfoIt->second, mi);
        } else {
            assert(false);
        }
	}

	passManager.add(createCFGSimplificationPass());
	passManager.add(createInstSimplifyLegacyPass());
	passManager.run(*Mod);

    /*
     * Compare the functions and remove the redundant one
     * */
    hashFuncSet baseFuncs;
    for (auto & mi : Mod->getFunctionList()) {
        if (!mi.hasName() || mi.getName().empty())
            continue;
        if (mi.getName().startswith("llvm.dbg.value") ||
            mi.getName().startswith("llvm.dbg.declare"))
            continue;
        if (mi.isDeclaration())
            continue;
        baseFuncs.emplace(FunctionNode(&mi));
    }

//	flexprint(N->Fe, N->Fm, N->Fpinfo, "infer bound\n");
//	for (auto & mi : *Mod)
//	{
//		rangeAnalysis(N, boundInfo, mi);
//	}
//
//	flexprint(N->Fe, N->Fm, N->Fpinfo, "constant substitution\n");
//	for (auto & mi : *Mod)
//	{
//		constantSubstitution(N, boundInfo, mi);
//	}
//
//	flexprint(N->Fe, N->Fm, N->Fpinfo, "shrink data type by range\n");
//	for (auto & mi : *Mod)
//	{
//		shrinkType(N, boundInfo, mi);
//	}

	dumpIR(N, "output", std::move(Mod));
}
}
