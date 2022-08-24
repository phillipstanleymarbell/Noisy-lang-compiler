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
	std::map<std::string, std::pair<double, double>> typeRange;
    std::map<Value *, std::pair<double, double>> virtualRegisterRange;
} BoundInfo;

enum CmpRes {
	Depends	    = 1,
	AlwaysTrue  = 2,
	AlwaysFalse = 3,
	Unsupported = 6,
};

enum varType {
    INT8        = 1,
    INT16       = 2,
    INT32       = 3,
    INT64       = 4,
    FLOAT       = 5,
    DOUBLE      = 6,
    UNCHANGED   = 7
};

Type *
shrinkType(State * N, Value *boundValue, const std::pair<double, double>& boundRange, bool& singFlag);

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
compareFCmpConstWithVariableRange(FCmpInst * llvmIrFCmpInstruction, double variableLowerBound, double variableUpperBound, double constValue)
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
compareFCmpWithVariableRange(FCmpInst * llvmIrFCmpInstruction, double leftVariableLowerBound, double leftVariableUpperBound,
                             double rightVariableLowerBound, double rightVariableUpperBound)
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
            if ((leftVariableLowerBound == rightVariableLowerBound) && (leftVariableUpperBound == rightVariableUpperBound))
            {
                return CmpRes::AlwaysTrue;
            }
            else
            {
                return CmpRes::AlwaysFalse;
            }
        case FCmpInst::FCMP_OGT:
        case FCmpInst::FCMP_UGT:
            if (leftVariableLowerBound > rightVariableUpperBound)
            {
                return CmpRes::AlwaysTrue;
            }
            else if (leftVariableUpperBound <= rightVariableLowerBound)
            {
                return CmpRes::AlwaysFalse;
            }
            else
            {
                return CmpRes::Depends;
            }
        case FCmpInst::FCMP_OGE:
        case FCmpInst::FCMP_UGE:
            if (leftVariableLowerBound >= rightVariableUpperBound)
            {
                return CmpRes::AlwaysTrue;
            }
            else if (leftVariableUpperBound < rightVariableLowerBound)
            {
                return CmpRes::AlwaysFalse;
            }
            else
            {
                return CmpRes::Depends;
            }
        case FCmpInst::FCMP_OLT:
        case FCmpInst::FCMP_ULT:
            if (leftVariableUpperBound < rightVariableLowerBound)
            {
                return CmpRes::AlwaysTrue;
            }
            else if (leftVariableLowerBound >= rightVariableUpperBound)
            {
                return CmpRes::AlwaysFalse;
            }
            else
            {
                return CmpRes::Depends;
            }
        case FCmpInst::FCMP_OLE:
        case FCmpInst::FCMP_ULE:
            if (leftVariableUpperBound <= rightVariableLowerBound)
            {
                return CmpRes::AlwaysTrue;
            }
            else if (leftVariableLowerBound > rightVariableUpperBound)
            {
                return CmpRes::AlwaysFalse;
            }
            else
            {
                return CmpRes::Depends;
            }
        case FCmpInst::FCMP_ONE:
        case FCmpInst::FCMP_UNE:
            if ((leftVariableUpperBound < rightVariableLowerBound) || (leftVariableLowerBound > rightVariableUpperBound))
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
compareICmpConstWithVariableRange(ICmpInst * llvmIrICmpInstruction, double variableLowerBound, double variableUpperBound, double constValue)
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

CmpRes
compareICmpWithVariableRange(ICmpInst * llvmIrICmpInstruction, double leftVariableLowerBound, double leftVariableUpperBound,
                             double rightVariableLowerBound, double rightVariableUpperBound)
{
    switch (llvmIrICmpInstruction->getPredicate())
    {
        /*
         * Ordered means that neither operand is a QNAN while unordered means that either operand may be a QNAN.
         * More details in https://llvm.org/docs/LangRef.html#icmp-instruction
         * */
        case ICmpInst::ICMP_EQ:
            if ((leftVariableLowerBound == rightVariableLowerBound) && (leftVariableUpperBound == rightVariableUpperBound))
            {
                return CmpRes::AlwaysTrue;
            }
            else
            {
                return CmpRes::AlwaysFalse;
            }
        case ICmpInst::ICMP_NE:
            if (leftVariableUpperBound < rightVariableLowerBound || leftVariableLowerBound > rightVariableUpperBound)
            {
                return CmpRes::AlwaysTrue;
            }
            else
            {
                return CmpRes::AlwaysFalse;
            }
        case ICmpInst::ICMP_UGT:
        case ICmpInst::ICMP_SGT:
            if (leftVariableLowerBound > rightVariableUpperBound)
            {
                return CmpRes::AlwaysTrue;
            }
            else if (leftVariableUpperBound <= rightVariableLowerBound)
            {
                return CmpRes::AlwaysFalse;
            }
            else
            {
                return CmpRes::Depends;
            }
        case ICmpInst::ICMP_UGE:
        case ICmpInst::ICMP_SGE:
            if (leftVariableLowerBound >= rightVariableUpperBound)
            {
                return CmpRes::AlwaysTrue;
            }
            else if (leftVariableUpperBound < rightVariableLowerBound)
            {
                return CmpRes::AlwaysFalse;
            }
            else
            {
                return CmpRes::Depends;
            }
        case ICmpInst::ICMP_ULT:
        case ICmpInst::ICMP_SLT:
            if (leftVariableUpperBound < rightVariableLowerBound)
            {
                return CmpRes::AlwaysTrue;
            }
            else if (leftVariableLowerBound >= rightVariableUpperBound)
            {
                return CmpRes::AlwaysFalse;
            }
            else
            {
                return CmpRes::Depends;
            }
        case ICmpInst::ICMP_ULE:
        case ICmpInst::ICMP_SLE:
            if (leftVariableUpperBound <= rightVariableLowerBound)
            {
                return CmpRes::AlwaysTrue;
            }
            else if (leftVariableLowerBound > rightVariableUpperBound)
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

std::pair<Value *, std::pair<double, double>>
inferBound(State * N, BoundInfo * boundInfo, Function & llvmIrFunction)
{
    std::map<Value *, Value *> unionAddress;
    for (BasicBlock & llvmIrBasicBlock : llvmIrFunction)
    {
        for (Instruction & llvmIrInstruction : llvmIrBasicBlock)
        {
            IRBuilder<> Builder(&llvmIrBasicBlock);
            switch (llvmIrInstruction.getOpcode())
            {
                case Instruction::Call:
                    if (auto llvmIrCallInstruction = dyn_cast<CallInst>(&llvmIrInstruction))
                    {
                        Function * calledFunction = llvmIrCallInstruction->getCalledFunction();
                        if (calledFunction->getName().startswith("llvm.dbg.value") ||
                            calledFunction->getName().startswith("llvm.dbg.declare"))
                        {
                            auto firstOperator = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(0));
                            auto localVariableAddressAsMetadata = cast<ValueAsMetadata>(firstOperator->getMetadata());
                            auto localVariableAddress = localVariableAddressAsMetadata->getValue();

                            auto variableMetadata = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(1));
                            auto debugInfoVariable = cast<DIVariable>(variableMetadata->getMetadata());
                            auto variableType = debugInfoVariable->getType();

                            if (const auto *compositeVariableType = dyn_cast<DICompositeType>(variableType))
                            {
                                /*
                                 * It's a composite type, including structure, union, array, and enumeration
                                 * */
                                auto typeTag = compositeVariableType->getTag();
                                if (typeTag == dwarf::DW_TAG_union_type)
                                {
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: DW_TAG_union_type\n");
                                }
                                else if (typeTag == dwarf::DW_TAG_structure_type)
                                {
                                    // todo
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: DW_TAG_structure_type\n");
//                                    auto unionTypeArr = compositeVariableType->getElements();
//                                    for (size_t i = 0; i < unionTypeArr.size(); i++)
//                                    {
//                                        auto typeRangeIt = boundInfo->typeRange.find(unionTypeArr[i]->getName().str());
//                                        if (typeRangeIt != boundInfo->typeRange.end())
//                                        {
//                                            boundInfo->virtualRegisterRange.emplace(localVariableAddress, typeRangeIt->second);
//                                        }
//                                    }
                                }
                                else if (typeTag == dwarf::DW_TAG_array_type)
                                {
                                    // todo
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: DW_TAG_array_type\n");
                                }
                                else if (typeTag == dwarf::DW_TAG_enumeration_type)
                                {
                                    // todo
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: DW_TAG_enumeration_type\n");
                                }
                            }
                            else
                            {
                                /*
                                *	if we find such type in boundInfo->typeRange,
                                *	we record it in the boundInfo->virtualRegisterRange
                                */
                                auto typeRangeIt = boundInfo->typeRange.find(variableType->getName().str());
                                if (typeRangeIt != boundInfo->typeRange.end())
                                {
                                    boundInfo->virtualRegisterRange.emplace(localVariableAddress, typeRangeIt->second);
                                    bool singFlag;
                                    Type * newType = shrinkType(N, localVariableAddress, typeRangeIt->second, singFlag);
                                }
                            }
                        }
                        /*
                         * It's function defined by programmer, eg. %17 = call i32 @abstop12(float %16), !dbg !83
                         * */
                        else
                        {
                            if (!calledFunction || calledFunction->isDeclaration())
                            {
                                flexprint(N->Fe, N->Fm, N->Fperr, "\tCall: CalledFunction %s is nullptr or undeclared.\n",
                                          calledFunction->getName().str().c_str());
                                continue;
                            }
                            /*
                             * Algorithm to infer the range of CallInst's result:
                             * 1. find the CallInst (caller).
                             * 2. check if the CallInst's operands is a variable with range.
                             * 3. infer the range of the operands (if needed).
                             * 4. look into the called function (callee), and get its operands with range in step 3.
                             * 5. if there's a CallInst in the body of called function, go to step 1.
                             *    else infer the range of the return value.
                             * 6. set the range of the result of the CallInst.
                             * */
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: detect CalledFunction %s.\n",
                                      calledFunction->getName().str().c_str());
                            auto innerBoundInfo = new BoundInfo();
                            for (size_t idx = 0; idx < llvmIrCallInstruction->getNumOperands() - 1; idx++)
                            {
                                /*
                                 * First, we check if it's a constant value
                                 * */
                                if (ConstantInt* cInt = dyn_cast<ConstantInt>(llvmIrCallInstruction->getOperand(idx)))
                                {
                                    int64_t constIntValue = cInt->getSExtValue();
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: It's a constant int value: %d.\n", constIntValue);
                                    innerBoundInfo->virtualRegisterRange.emplace(calledFunction->getArg(idx),
                                                                                 std::make_pair(static_cast<double>(constIntValue), static_cast<double>(constIntValue)));
                                    bool singFlag;
                                    shrinkType(N, calledFunction->getArg(idx),
                                               std::make_pair(static_cast<double>(constIntValue), static_cast<double>(constIntValue)), singFlag);
                                }
                                else if (ConstantFP * constFp = dyn_cast<ConstantFP>(llvmIrCallInstruction->getOperand(idx)))
                                {
                                    double constDoubleValue = (constFp->getValueAPF()).convertToDouble();
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: It's a constant double value: %f.\n", constDoubleValue);
                                    innerBoundInfo->virtualRegisterRange.emplace(calledFunction->getArg(idx),
                                                                                 std::make_pair(constDoubleValue, constDoubleValue));
                                    bool singFlag;
                                    shrinkType(N, calledFunction->getArg(idx),
                                               std::make_pair(constDoubleValue, constDoubleValue), singFlag);
                                }
                                else
                                {
                                    /*
                                    *	if we find the operand in boundInfo->virtualRegisterRange,
                                    *	we know it's a variable with range.
                                    */
                                    auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrCallInstruction->getOperand(idx));
                                    if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                                    {
                                        flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: the range of the operand is: %f - %f.\n",
                                        vrRangeIt->second.first, vrRangeIt->second.second);
                                        innerBoundInfo->virtualRegisterRange.emplace(calledFunction->getArg(idx), vrRangeIt->second);
                                        bool signFlag;
                                        shrinkType(N, calledFunction->getArg(idx), vrRangeIt->second, signFlag);
                                    }
                                }
                            }
                            auto returnRange = inferBound(N, innerBoundInfo, *calledFunction);
                            if (returnRange.first != nullptr)
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrCallInstruction, returnRange.second);
                                bool signFlag;
                                shrinkType(N, llvmIrCallInstruction, returnRange.second, signFlag);
                            }
                            boundInfo->virtualRegisterRange.insert(innerBoundInfo->virtualRegisterRange.begin(),
                                                                   innerBoundInfo->virtualRegisterRange.end());
                            /*
                             * Check the return type of the function,
                             * if it's a physical type that records in `boundInfo.typeRange`
                             * but didn't match the range we inferred from `inferBound` algorithm,
                             * we give a warning to the programmer.
                             * But we still believe in the range we inferred from the function body.
                             * */
                            DISubprogram *subProgram = calledFunction->getSubprogram();
                            DITypeRefArray typeArray = subProgram->getType()->getTypeArray();
                            if (typeArray[0] != nullptr) {
                                StringRef returnTypeName = typeArray[0]->getName();
                                auto vrRangeIt = boundInfo->typeRange.find(returnTypeName.str());
                                if (vrRangeIt != boundInfo->typeRange.end() &&
                                (vrRangeIt->second.first != returnRange.second.first || vrRangeIt->second.second != returnRange.second.second))
                                {
                                    flexprint(N->Fe, N->Fm, N->Fperr, "\tCall: the range of the function's return type is: %f - %f, but we inferred as: %f - %f\n",
                                              vrRangeIt->second.first, vrRangeIt->second.second, returnRange.second.first, returnRange.second.second);
                                }
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
                        if ((isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
                        {
                            std::swap(leftOperand, rightOperand);
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tAdd: swap left and right\n");
                        }
                        else if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tAdd: Expression normalization needed.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;

                            /*
                             * 	eg. x1+x2
                             * 	btw, I don't think we should check type here, which should be done in other pass like dimension-check
                             * 	find left operand from the boundInfo->virtualRegisterRange
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            }
                            /*
                             * 	find right operand from the boundInfo->virtualRegisterRange
                             */
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound += vrRangeIt->second.first;
                                upperBound += vrRangeIt->second.second;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                            bool signFlag;
                            Type * newType = shrinkType(N, llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound), signFlag);
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
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair(vrRangeIt->second.first + constValue,
                                                                                       vrRangeIt->second.second + constValue));
                                bool signFlag;
                                Type * newType = shrinkType(N, llvmIrBinaryOperator,
                                                               std::make_pair(vrRangeIt->second.first + constValue,
                                                                              vrRangeIt->second.second + constValue), signFlag);
                                if (newType != nullptr)
                                {
                                    rightOperand->mutateType(newType);
                                }
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
                        // todo: we cannot swap it
                        if ((isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
                        {
                            std::swap(leftOperand, rightOperand);
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tSub: swap left and right\n");
                        }
                        else if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tSub: Expression normalization needed.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;

                            /*
                             * 	eg. x1-x2
                             * 	btw, I don't think we should check type here, which should be done in other pass like dimension-check
                             * 	find left operand from the boundInfo->virtualRegisterRange
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound -= vrRangeIt->second.second;
                                upperBound -= vrRangeIt->second.first;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                            bool signFlag;
                            Type * newType = shrinkType(N, llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound), signFlag);
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
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair(vrRangeIt->second.first - constValue,
                                                                                       vrRangeIt->second.second - constValue));
                                bool signFlag;
                                Type * newType = shrinkType(N, llvmIrBinaryOperator,
                                                                      std::make_pair(vrRangeIt->second.first - constValue,
                                                                                     vrRangeIt->second.second - constValue), signFlag);
                                if (newType != nullptr)
                                {
                                    rightOperand->mutateType(newType);
                                }
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
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair(constValue - vrRangeIt->second.second,
                                                                                       constValue - vrRangeIt->second.first));
                                bool signFlag;
                                Type * newType = shrinkType(N, llvmIrBinaryOperator,
                                                            std::make_pair(constValue - vrRangeIt->second.second,
                                                                           constValue - vrRangeIt->second.first), signFlag);
                                if (newType != nullptr)
                                {
                                    leftOperand->mutateType(newType);
                                }
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tSub: Unexpected error. Might have an invalid operand.\n");
                        }
                    }
                    break;

                case Instruction::Mul:
                case Instruction::FMul:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
                        {
                            std::swap(leftOperand, rightOperand);
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tMul: swap left and right\n");
                        }
                        else if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tMul: Expression normalization needed.\n");
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	todo: let's measure the "var * var" the next time...
                             */
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tMul: It's a var * var expression, which is not supported yet.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	eg. x*2
                             */
                            double constValue = 1.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
                            {
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair(vrRangeIt->second.first * constValue,
                                                                                       vrRangeIt->second.second * constValue));
                                bool signFlag;
                                Type * newType = shrinkType(N, llvmIrBinaryOperator,
                                                            std::make_pair(vrRangeIt->second.first * constValue,
                                                                           vrRangeIt->second.second * constValue), signFlag);
                                if (newType != nullptr)
                                {
                                    rightOperand->mutateType(newType);
                                }
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tMul: Unexpected error. Might have an invalid operand.\n");
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
                        if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tDiv: Expression normalization needed.\n");
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	todo: let's measure the "var / var" the next time...
                             */
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tDiv: It's a var / var expression, which is not supported yet.\n");
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	todo: I don't know if we need to deal with "const / var" here,
                             * 	like 2/x. if x in [-4, 4], then the range of 2/x is (-inf, -0.5] and [0.5, inf)
                             */
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tDiv: It's a const / var expression, which is not supported yet.\n");
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
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair(vrRangeIt->second.first / constValue,
                                                                                       vrRangeIt->second.second / constValue));
                                bool signFlag;
                                Type * newType = shrinkType(N, llvmIrBinaryOperator,
                                                            std::make_pair(vrRangeIt->second.first / constValue,
                                                                           vrRangeIt->second.second / constValue), signFlag);
                                if (newType != nullptr)
                                {
                                    rightOperand->mutateType(newType);
                                }
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tDiv: Unexpected error. Might have an invalid operand.\n");
                        }
                    }
                    break;

                case Instruction::URem:
                case Instruction::SRem:
                case Instruction::FRem:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tRem: Expression normalization needed.\n");
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	todo: let's measure the "var % var" the next time...
                             */
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tRem: It's a var % var expression, which is not supported yet.\n");
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	todo: I don't know if we need to deal with "const % var" here, like 2%x.
                             */
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tRem: It's a const % var expression, which is not supported yet.\n");
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	eg. x%2
                             */
                            double constValue = 1.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
                            {
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            }
                            else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                        std::make_pair(remainder(vrRangeIt->second.first, constValue),
                                                       remainder(vrRangeIt->second.second, constValue)));
                                bool signFlag;
                                Type * newType = shrinkType(N, llvmIrBinaryOperator,
                                                                      std::make_pair(remainder(vrRangeIt->second.first, constValue),
                                                                                     remainder(vrRangeIt->second.second, constValue)), signFlag);
                                if (newType != nullptr)
                                {
                                    rightOperand->mutateType(newType);
                                }
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tRem: Unexpected error. Might have an invalid operand.\n");
                        }
                    }
                break;

                case Instruction::Shl:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShl: Expression normalization needed.\n");
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	todo: I don't know if we need to concern the "var << var"
                             */
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShl: It's a var << var expression, which is not supported yet.\n");
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	todo: I don't know if we need to deal with "const << var" here,
                             */
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShl: It's a const << var expression, which is not supported yet.\n");
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
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair((int)vrRangeIt->second.first << constValue,
                                                                                       (int)vrRangeIt->second.second << constValue));
                                bool signFlag;
                                Type * newType = shrinkType(N, llvmIrBinaryOperator,
                                                            std::make_pair((int)vrRangeIt->second.first << constValue,
                                                                           (int)vrRangeIt->second.second << constValue), signFlag);
                                if (newType != nullptr)
                                {
                                    rightOperand->mutateType(newType);
                                }
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShl: Unexpected error. Might have an invalid operand.\n");
                        }
                    }
                    break;

                case Instruction::LShr:
                case Instruction::AShr:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShr: Expression normalization needed.\n");
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	todo: I don't know if we need to concern the "var >> var"
                             */
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShr: It's a var << var expression, which is not supported yet.\n");
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	todo: I don't know if we need to deal with "const >> var" here,
                             */
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShr: It's a const << var expression, which is not supported yet.\n");
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
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair((int)vrRangeIt->second.first >> constValue,
                                                                                       (int)vrRangeIt->second.second >> constValue));
                                bool singFlag;
                                Type * newType = shrinkType(N, llvmIrBinaryOperator,
                                                            std::make_pair((int)vrRangeIt->second.first >> constValue,
                                                                           (int)vrRangeIt->second.second >> constValue), singFlag);
                                Builder.SetInsertPoint(llvmIrBinaryOperator->getNextNode());
                                auto tmp = llvmIrBinaryOperator->clone();
                                llvmIrBasicBlock.getInstList().insert(llvmIrBinaryOperator->getIterator(), tmp);
                                Value * intCast = Builder.CreateIntCast(tmp, newType, singFlag);
                                llvmIrBinaryOperator->replaceAllUsesWith(intCast);
                                llvmIrBinaryOperator->eraseFromParent();
                                singFlag = true;
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShr: Unexpected error. Might have an invalid operand.\n");
                        }
                    }
                    break;

                case Instruction::And:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tAnd: Expression normalization needed.\n");
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            /*
                             * 	eg. x1 & x2
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                                if (lowerBound < 0 || upperBound < 0)
                                {
                                    break;
                                }
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = (int)lowerBound & (int)vrRangeIt->second.second;
                                upperBound = (int)upperBound & (int)vrRangeIt->second.first;
                                if (lowerBound < 0 || upperBound < 0)
                                {
                                    break;
                                }
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                            bool signFlag;
                            Type * newType = shrinkType(N, llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound), signFlag);
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            std::swap(leftOperand, rightOperand);
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tAnd: swap left and right\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             *	eg. x&2
                             */
                            int constValue = 1.0;
                            if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getZExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair((int)vrRangeIt->second.first & constValue,
                                                                                       (int)vrRangeIt->second.second & constValue));
                                bool signFlag;
                                Type * newType = shrinkType(N, llvmIrBinaryOperator,
                                                            std::make_pair((int)vrRangeIt->second.first & constValue,
                                                                           (int)vrRangeIt->second.second & constValue), signFlag);
                                if (newType != nullptr)
                                {
                                    rightOperand->mutateType(newType);
                                }
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tAnd: Unexpected error. Might have an invalid operand.\n");
                        }
                    }
                    break;

                case Instruction::Or:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tOr: Expression normalization needed.\n");
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            /*
                             * 	eg. x1 | x2
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                                if (lowerBound < 0 || upperBound < 0)
                                {
                                    break;
                                }
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = (int)lowerBound | (int)vrRangeIt->second.second;
                                upperBound = (int)upperBound | (int) vrRangeIt->second.first;
                                if (lowerBound < 0 || upperBound < 0)
                                {
                                    break;
                                }
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                            bool signFlag;
                            Type * newType = shrinkType(N, llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound), signFlag);
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            std::swap(leftOperand, rightOperand);
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tOr: swap left and right\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             *	eg. x|2
                             */
                            int constValue = 1.0;
                            if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getZExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair((int)vrRangeIt->second.first | constValue,
                                                                                       (int)vrRangeIt->second.second | constValue));
                                bool signFlag;
                                Type * newType = shrinkType(N, llvmIrBinaryOperator,
                                                            std::make_pair((int)vrRangeIt->second.first | constValue,
                                                                           (int)vrRangeIt->second.second | constValue), signFlag);
                                if (newType != nullptr)
                                {
                                    rightOperand->mutateType(newType);
                                }
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tOr: Unexpected error. Might have an invalid operand.\n");
                        }
                    }
                    break;

                case Instruction::Xor:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tXor: Expression normalization needed.\n");
                        }
                        else if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	todo: I don't know if we need to concern the "var ^ var"
                             */
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tXor: It's a var ^ var expression, which is not supported yet.\n");
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            std::swap(leftOperand, rightOperand);
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tXor: swap left and right\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             *	eg. x^2
                             */
                            int constValue = 1.0;
                            if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
                            {
                                constValue = constInt->getZExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair((int)vrRangeIt->second.first ^ constValue,
                                                                                       (int)vrRangeIt->second.second ^ constValue));
                                bool signFlag;
                                Type * newType = shrinkType(N, llvmIrBinaryOperator,
                                                            std::make_pair((int)vrRangeIt->second.first ^ constValue,
                                                                           (int)vrRangeIt->second.second ^ constValue), signFlag);
                                if (newType != nullptr)
                                {
                                    rightOperand->mutateType(newType);
                                }
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tXor: Unexpected error. Might have an invalid operand.\n");
                        }
                    }
                    break;

                case Instruction::FPToUI:
                case Instruction::FPToSI:
                case Instruction::SIToFP:
                case Instruction::UIToFP:
                case Instruction::ZExt:
                case Instruction::SExt:
                case Instruction::FPExt:
                {
                    /*
                     * todo: these can be merged in BitCastInst/TruncInst, but need template<Type>
                     * */
                    Value * operand = llvmIrInstruction.getOperand(0);
                    auto	vrRangeIt = boundInfo->virtualRegisterRange.find(operand);
                    if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                    {
                        boundInfo->virtualRegisterRange.emplace(&llvmIrInstruction,
                                                                vrRangeIt->second);
                        bool signFlag;
                        Type * newType = shrinkType(N, &llvmIrInstruction, vrRangeIt->second, signFlag);
                    }
                }
                    break;

                case Instruction::Load:
                    if (auto llvmIrLoadInstruction = dyn_cast<LoadInst>(&llvmIrInstruction))
                    {
                        auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrLoadInstruction->getOperand(0));
                        if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                        {
                            boundInfo->virtualRegisterRange.emplace(llvmIrLoadInstruction, vrRangeIt->second);
                            bool signFlag;
                            Type * newType = shrinkType(N, llvmIrLoadInstruction, vrRangeIt->second, signFlag);

                        }
                    }
                    break;

                case Instruction::Store:
                    if (auto llvmIrStoreInstruction = dyn_cast<StoreInst>(&llvmIrInstruction))
                    {
                        if (isa<llvm::Constant>(llvmIrStoreInstruction->getOperand(0)))
                        {
                            /*
                             * 	eg. store double 5.000000e+00, double* %2, align 8, !dbg !27
                             */
                            double constValue = 0.0;
                            if (ConstantFP *constFp = llvm::dyn_cast<llvm::ConstantFP>(llvmIrStoreInstruction->getOperand(0)))
                            {
                                /*
                                 * 	both "float" and "double" type can use "convertToDouble"
                                 */
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            }
                            else if (ConstantInt *constInt = llvm::dyn_cast<llvm::ConstantInt>(llvmIrStoreInstruction->getOperand(0)))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrStoreInstruction->getOperand(1), std::make_pair(constValue, constValue));
                            bool signFlag;
                            Type * newType = shrinkType(N, llvmIrStoreInstruction->getOperand(1), std::make_pair(constValue, constValue), signFlag);                            if (newType != nullptr)
                            {
                                llvmIrStoreInstruction->getOperand(0)->mutateType(newType);
                            }
                        }
                        else
                        {
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrStoreInstruction->getOperand(0));
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrStoreInstruction->getOperand(1), vrRangeIt->second);
                                bool signFlag;
                                Type * newType = shrinkType(N, llvmIrStoreInstruction->getOperand(1), vrRangeIt->second, signFlag);
                            }
                            /*
                             * Each time if there's a StorInst assign to the unionAddress, it updates the value of union.
                             * */
                            auto uaIt = unionAddress.find(llvmIrStoreInstruction->getOperand(1));
                            if (uaIt != unionAddress.end())
                            {
                                flexprint(N->Fe, N->Fm, N->Fpinfo, "\tStore Union: %f - %f\n",  vrRangeIt->second.first, vrRangeIt->second.second);
                                boundInfo->virtualRegisterRange.emplace(uaIt->second, vrRangeIt->second);
                                bool signFlag;
                                Type * newType = shrinkType(N, uaIt->second, vrRangeIt->second, signFlag);
                            }
                        }
                    }
                    break;

                case Instruction::Trunc:
                    if (auto llvmIrTruncInstruction = dyn_cast<TruncInst>(&llvmIrInstruction))
                    {
                        auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrTruncInstruction->getOperand(0));
                        if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                        {
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFPTrunc: %f - %f\n",  vrRangeIt->second.first, vrRangeIt->second.second);
                            double originLow = vrRangeIt->second.first;
                            double originHigh = vrRangeIt->second.second;
                            double lowRange = 0, highRange = 0;
                            auto DestEleType = llvmIrTruncInstruction->getDestTy();
                            switch (DestEleType->getTypeID())
                            {
                                case Type::IntegerTyID:
                                    switch (DestEleType->getIntegerBitWidth())
                                    {
                                        case 8:
                                            lowRange = static_cast<double>(static_cast<int8_t>(originLow));
                                            highRange = static_cast<double>(static_cast<int8_t>(originHigh));
                                        break;
                                        case 16:
                                            lowRange = static_cast<double>(static_cast<int16_t>(originLow));
                                            highRange = static_cast<double>(static_cast<int16_t>(originHigh));
                                        break;
                                        case 32:
                                            lowRange = static_cast<double>(static_cast<int32_t>(originLow));
                                            highRange = static_cast<double>(static_cast<int32_t>(originHigh));
                                        break;
                                        case 64:
                                            lowRange = static_cast<double>(static_cast<int64_t>(originLow));
                                            highRange = static_cast<double>(static_cast<int64_t>(originHigh));
                                        break;
                                        default:
                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tTrunc: Type::SignedInteger, don't support such bit width yet.");
                                    }
                                break;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrTruncInstruction, std::make_pair(lowRange, highRange));
                            bool signFlag;
                            Type * newType = shrinkType(N, llvmIrTruncInstruction, std::make_pair(lowRange, highRange), signFlag);
                        }
                    }
                break;

                case Instruction::FPTrunc:
                    if (auto llvmIrFPTruncInstruction = dyn_cast<FPTruncInst>(&llvmIrInstruction))
                    {
                        auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrFPTruncInstruction->getOperand(0));
                        if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                        {
                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFPTrunc: %f - %f\n",  vrRangeIt->second.first, vrRangeIt->second.second);
                            boundInfo->virtualRegisterRange.emplace(llvmIrFPTruncInstruction,
                                                                    std::make_pair(static_cast<double>(static_cast<float>(vrRangeIt->second.first)),
                                                                                   static_cast<double>(static_cast<float>(vrRangeIt->second.second))));
                            bool signFlag;
                            Type * newType = shrinkType(N, llvmIrFPTruncInstruction,
                                                        std::make_pair(static_cast<double>(static_cast<float>(vrRangeIt->second.first)),
                                                                       static_cast<double>(static_cast<float>(vrRangeIt->second.second))), signFlag);
                        }
                    }
                    break;

                case Instruction::BitCast:
                    if (auto llvmIrBitCastInstruction = dyn_cast<BitCastInst>(&llvmIrInstruction))
                    {
                        /*
                        * for the union type, LLVM IR uses a store intrinsic to link the variables, e.g.
                         * union {
                         *   float f;
                         *   uint32_t i;
                         * } u = {f};
                         * The IR is:
                         *   %4 = bitcast %union.anon* %3 to double*
                         *   %5 = load double, double* %2, align 8
                         *   store double %5, double* %4, align 8
                         *   %6 = bitcast %union.anon* %3 to i32*
                         *
                         * So the Algorithm to infer the range of Union type is:
                         * 1. record the first bitcast instruction info to a map,
                         *    as we didn't have the actual variable information
                         * 2. check with the store instruction with the records in the map
                         *    and store the actual variable to the %union.anon
                         * 3. get the variable info from the %union.anon by the second bitcast instruction,
                         *    and reinterpret it if necessary
                         * */
                        unionAddress.emplace(llvmIrBitCastInstruction, llvmIrBitCastInstruction->getOperand(0));
                        assert(llvmIrBitCastInstruction->getDestTy()->getTypeID() == Type::PointerTyID);
                        auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrBitCastInstruction->getOperand(0));
                        if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                        {
                            /*
                             * In our current test cases, there's only double->uint64_t or float->uint32_t.
                             * But theoretically there will be uint64_t->double, uint32_t->float or others.
                             * We should rewrite it to
                             * `auto originLow = static_cast<uint64_t>vrRangeIt->second.first;` when it appears.
                             * */
                            double originLow = vrRangeIt->second.first;
                            double originHigh = vrRangeIt->second.second;
                            double lowRange, highRange;
                            auto DestEleType = llvmIrBitCastInstruction->getDestTy()->getPointerElementType();
                            Type * newType = nullptr;
                            bool signFlag;
                            switch (DestEleType->getTypeID())
                            {
                                case Type::FloatTyID:
                                    lowRange = static_cast<double>(*reinterpret_cast<float *>(&originLow));
                                    highRange = static_cast<double>(*reinterpret_cast<float *>(&originHigh));
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::FloatTyID, %f - %f to %f - %f\n",
                                              vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                    boundInfo->virtualRegisterRange.emplace(llvmIrBitCastInstruction, std::make_pair(lowRange, highRange));
                                    newType = shrinkType(N, llvmIrBitCastInstruction, std::make_pair(lowRange, highRange), signFlag);
                                    break;
                                case Type::DoubleTyID:
                                    lowRange = *reinterpret_cast<double *>(&originLow);
                                    highRange = *reinterpret_cast<double *>(&originHigh);
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::DoubleTyID, %f - %f to %f - %f\n",
                                              vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                    boundInfo->virtualRegisterRange.emplace(llvmIrBitCastInstruction, std::make_pair(lowRange, highRange));
                                    newType = shrinkType(N, llvmIrBitCastInstruction, std::make_pair(lowRange, highRange), signFlag);
                                    break;
                                case Type::IntegerTyID:
                                    switch (DestEleType->getIntegerBitWidth())
                                    {
                                        case 32:
                                            lowRange = static_cast<double>(*reinterpret_cast<int32_t *>(&originLow));
                                            highRange = static_cast<double>(*reinterpret_cast<int32_t *>(&originHigh));
                                            break;
                                        case 64:
                                            lowRange = static_cast<double>(*reinterpret_cast<int64_t *>(&originLow));
                                            highRange = static_cast<double>(*reinterpret_cast<int64_t *>(&originHigh));
                                            break;
                                        default:
                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::SignedInteger, don't support such bit width yet.");
                                    }

                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::IntegerTyID, %f - %f to %f - %f\n",
                                              vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                    boundInfo->virtualRegisterRange.emplace(llvmIrBitCastInstruction, std::make_pair(lowRange, highRange));
                                    newType = shrinkType(N, llvmIrBitCastInstruction, std::make_pair(lowRange, highRange), signFlag);
                                    break;
                                case Type::StructTyID:
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::StructTyID, %f - %f to %f - %f\n",
                                              vrRangeIt->second.first, vrRangeIt->second.second, originLow, originHigh);
                                    break;
                                default:
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Do not support other type yet.\n");
                                    boundInfo->virtualRegisterRange.emplace(llvmIrBitCastInstruction, vrRangeIt->second);
                                    newType = shrinkType(N, llvmIrBitCastInstruction, vrRangeIt->second, signFlag);
                                    continue;
                            }
                        }
                    }
                    break;

                case Instruction::GetElementPtr:
                    if (auto llvmIrGetElePtrInstruction = dyn_cast<GetElementPtrInst>(&llvmIrInstruction))
                    {
                        /*
                         * If there's a union type like:
                         * typedef union
                         * {
                         *   double value;
                         *   struct
                         *   {
                         *     __uint32_t lsw;
                         *     __uint32_t msw;
                         *   } parts;
                         * } ieee_double_shape_type;
                         *
                         * ieee_double_shape_type gh_u;
                         * gh_u.value = (d);
                         * (i) = gh_u.parts.msw;
                         *
                         * It's IR is:
                         * %9 = bitcast %union.ieee_double_shape_type* %2 to double*, !dbg !66
                         * store double %0, double* %9, align 8, !dbg !66
                         * %10 = bitcast %union.ieee_double_shape_type* %2 to %struct.anon*, !dbg !66
                         * %11 = getelementptr inbounds %struct.anon, %struct.anon* %10, i32 0, i32 1, !dbg !66
                         *
                         * Our algorithm is:
                         * 1. check if the pointer operand has been recorded in the map that contains bitcast information
                         * 2. get the value-holder bitcast instruction from the map
                         * 3. check if the range of the value-holder has been inferred
                         * 4. get the variable info from the value-holder, and cast it if necessary
                         * */
                        auto uaIt = unionAddress.find(llvmIrGetElePtrInstruction->getPointerOperand());
                        if (uaIt != unionAddress.end())
                        {
                            /*
                             * The pointer operand has been recorded in the unionAddress,
                             * so the value of this elementPtr must be gotten from the nearest bitcasts of union.
                             * */
                            auto it = std::find_if(unionAddress.rbegin(), unionAddress.rend(), [uaIt](const auto & ua){
                                auto valueHolderBitcast = dyn_cast<BitCastInst>(ua.first);
                                assert(valueHolderBitcast != nullptr);
                                auto resTypeId = valueHolderBitcast->getDestTy()->getTypeID();
                                return (ua.second == uaIt->second) && (resTypeId != Type::StructTyID);
                            });
                            if (it != unionAddress.rend())
                            {
                                auto vrRangeIt = boundInfo->virtualRegisterRange.find(it->second);
                                if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                                {
                                    double originLow = vrRangeIt->second.first;
                                    double originHigh = vrRangeIt->second.second;
                                    uint64_t originLowWord = *reinterpret_cast<uint64_t*>(&originLow);
                                    uint64_t originHighWord = *reinterpret_cast<uint64_t*>(&originHigh);
                                    double lowRange, highRange;
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tGetElementPtr: find the value holder.");
                                    auto valueHolderBitcast = dyn_cast<BitCastInst>(it->first);
                                    auto DestEleType = valueHolderBitcast->getDestTy()->getPointerElementType();
                                    // todo: is it necessary to check? or does it have other usage?
                                    unsigned dataBitWidth = DestEleType->getPrimitiveSizeInBits();
                                    /*
                                     * re-interpret the value in structure, e.g. for the example we showed below,
                                     * uint32_t lsw = static_cast<uint32_t>(long_word);
                                     * uint32_t msw = static_cast<uint32_t>(long_word >> 32);
                                     * */
                                    int element_offset = 0, pointer_offset = 0;
                                    if (llvmIrGetElePtrInstruction->getNumIndices() == 1)
                                    {
                                        /*
                                         * It's reference
                                         * */
                                        element_offset = dyn_cast<ConstantInt>(llvmIrGetElePtrInstruction->getOperand(1))->getZExtValue();
                                    }
                                    else
                                    {
                                        /*
                                         * It's array or structure
                                         * */
                                        pointer_offset = dyn_cast<ConstantInt>(llvmIrGetElePtrInstruction->getOperand(1))->getZExtValue();
                                        element_offset = dyn_cast<ConstantInt>(llvmIrGetElePtrInstruction->getOperand(2))->getZExtValue();
                                    }
                                    auto resEleTy = llvmIrGetElePtrInstruction->getResultElementType();
                                    Type * newType = nullptr;
                                    bool signFlag;
                                    switch (resEleTy->getTypeID())
                                    {
                                        case Type::FloatTyID:
                                            lowRange = static_cast<double>(static_cast<float>(originLowWord >> (32 * element_offset)));
                                            highRange = static_cast<double>(static_cast<float>(originHighWord >> (32 * element_offset)));
                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::FloatTyID, %f - %f to %f - %f\n",
                                                      vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                            boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction, std::make_pair(lowRange, highRange));
                                            newType = shrinkType(N, llvmIrGetElePtrInstruction, std::make_pair(lowRange, highRange), signFlag);
                                            break;
                                        case Type::DoubleTyID:
                                            lowRange = static_cast<double>(originLowWord >> (32 * element_offset));
                                            highRange = static_cast<double>(originHighWord >> (32 * element_offset));
                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::DoubleTyID, %f - %f to %f - %f\n",
                                                      vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                            boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction, std::make_pair(lowRange, highRange));
                                            newType = shrinkType(N, llvmIrGetElePtrInstruction, std::make_pair(lowRange, highRange), signFlag);
                                            break;
                                        case Type::IntegerTyID:
                                            switch (resEleTy->getPrimitiveSizeInBits())
                                            {
                                                case 32:
                                                    lowRange = static_cast<double>(static_cast<int32_t>(originLowWord >> (32 * element_offset)));
                                                    highRange = static_cast<double>(static_cast<int32_t>(originHighWord >> (32 * element_offset)));
                                                    break;
                                                case 64:
                                                    lowRange = static_cast<double>(static_cast<int64_t>(originLowWord));
                                                    highRange = static_cast<double>(static_cast<int64_t>(originHighWord));
                                                    break;
                                                default:
                                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::SignedInteger, don't support such bit width yet.");
                                            }

                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::IntegerTyID, %f - %f to %f - %f\n",
                                                      vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                            boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction, std::make_pair(lowRange, highRange));
                                            newType = shrinkType(N, llvmIrGetElePtrInstruction, std::make_pair(lowRange, highRange), signFlag);
                                            break;
                                        default:
                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tGetElePtr: Do not support other type yet.\n");
                                            boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction, vrRangeIt->second);
                                            newType = shrinkType(N, llvmIrGetElePtrInstruction, vrRangeIt->second, signFlag);
                                            continue;
                                    }
                                }
                            }
                        }
                        // todo: infer the range from structure or array
                    }

                case Instruction::Ret:
                    if (auto llvmIrReturnInstruction = dyn_cast<ReturnInst>(&llvmIrInstruction))
                    {
                        if (llvmIrReturnInstruction->getNumOperands() != 0)
                        {
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrReturnInstruction->getOperand(0));
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                return {llvmIrReturnInstruction, vrRangeIt->second};
                            }
                        }
                    }
                    break;
                case Instruction::FNeg:
                    if (auto llvmIrFNegInstruction = dyn_cast<UnaryOperator>(&llvmIrInstruction))
                    {
                        if (llvmIrFNegInstruction->getNumOperands() != 0)
                        {
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrFNegInstruction->getOperand(0));
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(
                                        llvmIrFNegInstruction, std::make_pair(-vrRangeIt->second.first,
                                                                              -vrRangeIt->second.second));
                                bool signFlag;
                                Type * newType = shrinkType(N, llvmIrFNegInstruction, std::make_pair(-vrRangeIt->second.first,
                                                                                    -vrRangeIt->second.second), signFlag);
                            }
                        }
                    }
                    break;
                case Instruction::Br:
                    if (auto llvmIrBrInstruction = dyn_cast<BranchInst>(&llvmIrInstruction))
                    {

                    }
                    break;

                case Instruction::PHI:
                    break;

                case Instruction::Select:
                    break;

                case Instruction::Switch:
                    break;

                case Instruction::IndirectBr:
                case Instruction::Invoke:
                case Instruction::Resume:
                case Instruction::Unreachable:
                case Instruction::CleanupRet:
                case Instruction::CatchRet:
                case Instruction::CatchSwitch:
                case Instruction::CallBr:
                case Instruction::Fence:
                case Instruction::AtomicCmpXchg:
                case Instruction::AtomicRMW:
                case Instruction::PtrToInt:
                case Instruction::IntToPtr:
                case Instruction::AddrSpaceCast:
                case Instruction::CleanupPad:
                case Instruction::CatchPad:
                case Instruction::UserOp1:
                case Instruction::UserOp2:
                case Instruction::VAArg:
                case Instruction::ExtractElement:
                case Instruction::InsertElement:
                case Instruction::ShuffleVector:
                case Instruction::ExtractValue:
                case Instruction::InsertValue:
                case Instruction::LandingPad:
                case Instruction::Freeze:
                default:
                    continue;
            }
        }
    }
    return {nullptr, {}};
}

void
simplifyControlFlow(State * N, BoundInfo * boundInfo, Function & llvmIrFunction)
{
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
                        if (calledFunction->getName().str() == "extractFloat64Exp") {
                            printf("extractFloat64Exp\n");
                        }
                    }
				case Instruction::ICmp:
					if (auto llvmIrICmpInstruction = dyn_cast<ICmpInst>(&llvmIrInstruction))
					{
						auto leftOperand  = llvmIrICmpInstruction->getOperand(0);
						auto rightOperand = llvmIrICmpInstruction->getOperand(1);
                        /*
                         * todo: change prediction signal
                         * */
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
                            auto vrLeftRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            auto vrRightRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrLeftRangeIt != boundInfo->virtualRegisterRange.end() &&
                                vrRightRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: left operand's lower bound: %f, upper bound: %f\n",
                                          vrLeftRangeIt->second.first, vrLeftRangeIt->second.second);
                                flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: right operand's lower bound: %f, upper bound: %f\n",
                                          vrRightRangeIt->second.first, vrRightRangeIt->second.second);
                                CmpRes compareResult = compareICmpWithVariableRange(llvmIrICmpInstruction,
                                                                                    vrLeftRangeIt->second.first,
                                                                                    vrLeftRangeIt->second.second,
                                                                                    vrRightRangeIt->second.first,
                                                                                    vrRightRangeIt->second.second);
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
						}
						if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
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
							 * find the variable from the boundInfo->virtualRegisterRange
							 * */
							auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != boundInfo->virtualRegisterRange.end())
							{
								flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: varibale's lower bound: %f, upper bound: %f\n",
									  vrRangeIt->second.first, vrRangeIt->second.second);
								CmpRes compareResult = compareICmpConstWithVariableRange(llvmIrICmpInstruction,
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
                        /*
                         * todo: change prediction signal
                         * */
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
                            auto vrLeftRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            auto vrRightRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrLeftRangeIt != boundInfo->virtualRegisterRange.end() &&
                                vrRightRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: left operand's lower bound: %f, upper bound: %f\n",
                                          vrLeftRangeIt->second.first, vrLeftRangeIt->second.second);
                                flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: right operand's lower bound: %f, upper bound: %f\n",
                                          vrRightRangeIt->second.first, vrRightRangeIt->second.second);
                                CmpRes compareResult = compareFCmpWithVariableRange(llvmIrFCmpInstruction,
                                                                                    vrLeftRangeIt->second.first,
                                                                                    vrLeftRangeIt->second.second,
                                                                                    vrRightRangeIt->second.first,
                                                                                    vrRightRangeIt->second.second);
                                flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: the comparison result is %d\n", compareResult);
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
                                else if (compareResult == CmpRes::Unsupported)
                                {
                                    flexprint(N->Fe, N->Fm, N->Fperr, "\tICmp: Current ICmp Predicate is not supported.\n");
                                }
                            }
						}
						if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							double constValue = 0.0;
							if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
							{
								/*
								 * both "float" and "double" type can use "convertToDouble"
								 * */
								constValue = (constFp->getValueAPF()).convertToDouble();
							}
                            else
                            {
                                flexprint(N->Fe, N->Fm, N->Fperr, "\tFCmp: it's not a const fp!!!!!!!!!!!\n");
                            }
							flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFCmp: right operand: %f\n", constValue);
							/*
							 * find the variable from the boundInfo->virtualRegisterRange
							 * */
							auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != boundInfo->virtualRegisterRange.end())
							{
								flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFCmp: varibale's lower bound: %f, upper bound: %f\n",
									  vrRangeIt->second.first, vrRangeIt->second.second);
								CmpRes compareResult = compareFCmpConstWithVariableRange(llvmIrFCmpInstruction,
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

Type *
shrinkIntType(State * N, Value *boundValue, const std::pair<double, double>& boundRange, bool& signFlag)
{
    /*
     * Signed-ness in LLVM is not stored into the integer, it is left to
     * interpretation to the instruction that use them.
     * */
    signFlag = boundRange.first < 0;

    varType finalType = UNCHANGED;
    if ((!signFlag && boundRange.second < UINT8_MAX) ||
        (signFlag && boundRange.first > INT8_MIN && boundRange.second < INT8_MAX))
    {
        finalType = INT8;
    }
    else if ((!signFlag && boundRange.second < UINT16_MAX) ||
             (signFlag && boundRange.first > INT16_MIN && boundRange.second < INT16_MAX))
    {
        finalType = INT16;
    }
    else if ((!signFlag && boundRange.second < UINT32_MAX) ||
             (signFlag && boundRange.first > INT32_MIN && boundRange.second < INT32_MAX))
    {
        finalType = INT32;
    }
    else if ((!signFlag && boundRange.second < UINT64_MAX) ||
             (signFlag && boundRange.first > INT64_MIN && boundRange.second < INT64_MAX))
    {
        finalType = INT64;
    }
    else
    {
        finalType = UNCHANGED;
    }

    auto previousType = boundValue->getType();
    auto typeId = previousType->getTypeID();
    auto& context = previousType->getContext();
    unsigned bitWidth;

    bool isPointer = false;
    unsigned pointerAddr;
    if (typeId == Type::PointerTyID)
    {
        typeId = previousType->getPointerElementType()->getTypeID();
        bitWidth = previousType->getPointerElementType()->getIntegerBitWidth();
        isPointer = true;
        pointerAddr = previousType->getPointerAddressSpace();
    }
    else
    {
        bitWidth = previousType->getIntegerBitWidth();
    }
    assert(typeId = Type::IntegerTyID);

    Type *newType = nullptr;
    switch (bitWidth) {
        case 64:
            if (finalType == INT32)
            {
                newType = IntegerType::getInt32Ty(context);
            }
        case 32:
            if (finalType == INT16)
            {
                newType = IntegerType::getInt16Ty(context);
            }
        case 16:
            if (finalType == INT8)
            {
                newType = IntegerType::getInt8Ty(context);
            }
            break;
        default:
            flexprint(N->Fe, N->Fm, N->Fpinfo,
                      "\tshrinkType: Type::Integer, don't support such bit width yet.");
    }

    if (newType != nullptr) {
        newType = isPointer ? newType->getPointerTo(pointerAddr) : newType;
//        boundValue->mutateType(newType);
    }

    return newType;
}

Type *
shrinkType(State * N, Value *boundValue, const std::pair<double, double>& boundRange, bool& signFlag)
{
    auto valueType = boundValue->getType();
    auto valueTypeId = valueType->getTypeID() == Type::PointerTyID ?
            valueType->getPointerElementType()->getTypeID() :
            valueType->getTypeID();

    switch (valueTypeId) {
        case Type::IntegerTyID:
            return shrinkIntType(N, boundValue, boundRange, signFlag);
//        case Type::FloatTyID:
//            break;
//        case Type::DoubleTyID:
//            break;
        default:
            return nullptr;
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

    flexprint(N->Fe, N->Fm, N->Fpinfo, "infer bound\n");
    for (auto & mi : *Mod)
    {
        inferBound(N, boundInfo, mi);
    }

    flexprint(N->Fe, N->Fm, N->Fpinfo, "simplify control flow by range\n");
	for (auto & mi : *Mod)
	{
		simplifyControlFlow(N, boundInfo, mi);
	}

	dumpIR(N, "output", std::move(Mod));
}
}
