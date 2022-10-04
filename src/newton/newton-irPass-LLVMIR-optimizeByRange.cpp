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

extern "C"
{

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

    /*
     * get static global variables
     * static constant value is in the code
     * */
//    for (const auto & compileUnit : Mod->debug_compile_units()) {
//        for (const auto & globalVar : compileUnit->getGlobalVariables()) {
//            const DIGlobalVariable *diGV = globalVar->getVariable();
//            const DIExpression *diExpr = globalVar->getExpression();
//            Value* diValue = cast<ValueAsMetadata>(globalVar->getRawVariable())->getValue();
//            for (size_t idx = 0; idx < diExpr->getNumElements(); idx++) {
//                /*
//                 * get variable type
//                 * */
//                auto diType = diGV->getType();
//                std::string baseTypeName;
//                if (const auto * derivedType = dyn_cast<DIDerivedType>(diType)) {
//                    if (derivedType->getTag() == llvm::dwarf::DW_TAG_pointer_type) {
//                        baseTypeName = derivedType->getBaseType()->getName().str();
//                    } else {
//                        baseTypeName = derivedType->getName().str();
//                    }
//                } else {
//                    baseTypeName = derivedType->getName().str();
//                }
//                /*
//                 * it's a constant value
//                 * */
//                if (diExpr->getElement(idx) == dwarf::DW_OP_constu) {
//                    uint64_t u64Value = diExpr->getElement(idx+1);
//                    double realValue = 0;
//                    if (baseTypeName == "double") {
//                        memcpy(&realValue, &u64Value, sizeof(uint64_t));
//                    } else if (baseTypeName == "float") {
//                        memcpy(&realValue, &u64Value, sizeof(uint64_t));
//                    } else {
//                        realValue = u64Value;
//                    }
//                    boundInfo->virtualRegisterRange.emplace(diValue, std::make_pair(realValue, realValue));
//                }
//            }
//        }
//    }

    /*
     * get const global variables
     * */
    for (auto & globalVar : Mod->getGlobalList()) {
        auto constValue = globalVar.getInitializer();
        if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(constValue)) {
            if (constValue->getType()->isFloatTy()) {
                float constValue = constFp->getValueAPF().convertToFloat();
                boundInfo->virtualRegisterRange.emplace(&globalVar, std::make_pair(constValue, constValue));
            } else if (constValue->getType()->isDoubleTy()) {
                double constValue = constFp->getValueAPF().convertToDouble();
                boundInfo->virtualRegisterRange.emplace(&globalVar, std::make_pair(constValue, constValue));
            }
        } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(constValue)) {
            auto constValue = constInt->getSExtValue();
            boundInfo->virtualRegisterRange.emplace(&globalVar, std::make_pair(static_cast<double>(constValue),
                                                                            static_cast<double>(constValue)));
        } else if (ConstantDataArray * constArr = llvm::dyn_cast<llvm::ConstantDataArray>(constValue)) {
            auto arrType = constArr->getElementType();
            if (arrType->isDoubleTy()) {
                for (size_t idx = 0; idx < constArr->getNumElements(); idx++) {
                    double dbValue = constArr->getElementAsDouble(idx);
					boundInfo->virtualRegisterVectorRange[&globalVar].emplace_back(std::make_pair(dbValue, dbValue));
                }
            } else if (arrType->isFloatTy()) {
                for (size_t idx = 0; idx < constArr->getNumElements(); idx++) {
                    double ftValue = constArr->getElementAsFloat(idx);
					boundInfo->virtualRegisterVectorRange[&globalVar].emplace_back(std::make_pair(ftValue, ftValue));
                }
            } else if (arrType->isIntegerTy()) {
                for (size_t idx = 0; idx < constArr->getNumElements(); idx++) {
                    uint64_t intValue = constArr->getElementAsInteger(idx);
					boundInfo->virtualRegisterVectorRange[&globalVar].emplace_back(std::make_pair(intValue, intValue));
                }
            } else if (arrType->isPointerTy()) {
                // todo when meet
            } else {
                // todo when meet: other type
            }
        } else {
            // todo when meet: other type
        }
    }

    legacy::PassManager passManager;

    flexprint(N->Fe, N->Fm, N->Fpinfo, "infer bound\n");
    for (auto & mi : *Mod)
    {
        // todo: only analyze the function with Newton info
        rangeAnalysis(N, boundInfo, mi);
    }

    flexprint(N->Fe, N->Fm, N->Fpinfo, "simplify control flow by range\n");
	for (auto & mi : *Mod)
	{
		simplifyControlFlow(N, boundInfo, mi);
	}

    passManager.add(createCFGSimplificationPass());
    passManager.add(createInstSimplifyLegacyPass());
//    passManager.add(createInstructionCombiningPass());
    passManager.run(*Mod);

    flexprint(N->Fe, N->Fm, N->Fpinfo, "infer bound\n");
    for (auto & mi : *Mod)
    {
        // todo: only analyze the function with Newton info
        rangeAnalysis(N, boundInfo, mi);
    }

    // todo: constant substitution
    flexprint(N->Fe, N->Fm, N->Fpinfo, "constant substitution\n");
    for (auto & mi : *Mod)
    {
        constantSubstitution(N, boundInfo, mi);
    }

    flexprint(N->Fe, N->Fm, N->Fpinfo, "shrink data type by range\n");
    for (auto & mi : *Mod)
    {
        shrinkType(N, boundInfo, mi);
    }

	dumpIR(N, "output", std::move(Mod));
}
}
