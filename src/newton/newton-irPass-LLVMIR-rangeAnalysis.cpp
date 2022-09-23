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

#include "newton-irPass-LLVMIR-rangeAnalysis.h"

using namespace llvm;

extern "C"
{

const bool valueRangeDebug = false;

std::pair<double, double>
getGEPArrayRange(State* N, GetElementPtrInst* llvmIrGetElePtrInstruction,
                 std::map<llvm::Value *, std::pair<double, double>> virtualRegisterRange)
{
    /*
     * if it's a constant
     * */
    if (auto * constVar = dyn_cast<llvm::Constant>(llvmIrGetElePtrInstruction->getOperand(0))) {
        auto ptrIndexValue = llvmIrGetElePtrInstruction->getOperand(1);
        int ptrIndex = 0;
        if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(ptrIndexValue)) {
            ptrIndex = constInt->getZExtValue();
        } else {
            assert(false && "gep pointer index is not constant int");
        }
        auto realVar = constVar->getOperand(ptrIndex);
        if (auto *constArr = dyn_cast<ConstantDataArray>(realVar)) {
            auto arrIndexValue = llvmIrGetElePtrInstruction->getOperand(2);
            auto dynRangeRes = [&](std::vector <uint32_t> idxVec) {
                auto arrType = constArr->getElementType();
                double minRes = 0, maxRes = 0;
                if (arrType->isDoubleTy()) {
                    std::vector<double> dbResVec;
                    for (auto idx: idxVec) {
                        dbResVec.emplace_back(constArr->getElementAsDouble(idx));
                    }
                    minRes = *std::min_element(std::begin(dbResVec), std::end(dbResVec));
                    maxRes = *std::max_element(std::begin(dbResVec), std::end(dbResVec));
                } else if (arrType->isFloatTy()) {
                    std::vector<float> ftResVec;
                    for (auto idx: idxVec) {
                        ftResVec.emplace_back(constArr->getElementAsFloat(idx));
                    }
                    minRes = *std::min_element(std::begin(ftResVec), std::end(ftResVec));
                    maxRes = *std::max_element(std::begin(ftResVec), std::end(ftResVec));
                } else if (arrType->isIntegerTy()) {
                    std::vector <uint64_t> intResVec;
                    for (auto idx: idxVec) {
                        intResVec.emplace_back(constArr->getElementAsInteger(idx));
                    }
                    minRes = static_cast<double>(
                            *std::min_element(std::begin(intResVec), std::end(intResVec)));
                    maxRes = static_cast<double>(
                            *std::max_element(std::begin(intResVec), std::end(intResVec)));
                } else if (arrType->isPointerTy()) {
                    assert(!valueRangeDebug && "pointer: implement when meet");
                } else {
                    assert(!valueRangeDebug && "other type: implement when meet");
                }
                return std::make_pair(minRes, maxRes);
            };
            if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(arrIndexValue)) {
                /*
                 * this also should be done in "--instsimplify"
                 * */
                int arrIndex = constInt->getZExtValue();
                auto resVec = dynRangeRes({arrIndex});
                return resVec;
            } else {
                std::vector <uint32_t> dynIdx;
                auto vrRangeIt = virtualRegisterRange.find(arrIndexValue);
                if (vrRangeIt != virtualRegisterRange.end()) {
                    // todo: if we need assert or other check here?
                    uint32_t min = vrRangeIt->second.first < 0 ? 0 : vrRangeIt->second.first;
                    uint32_t max = vrRangeIt->second.second < 0 ? 0 : vrRangeIt->second.second;
                    for (size_t idx = min; idx <= max; idx++) {
                        dynIdx.emplace_back(idx);
                    }
                } else {
                    /*the range is [min(arr_elements), max(arr_elements)]*/
                    for (size_t idx = 0; idx < constArr->getNumElements(); idx++) {
                        dynIdx.emplace_back(idx);
                    }
                }
                auto resVec = dynRangeRes(dynIdx);
                return resVec;
            }
        } else {
            // todo: get range other type of array
            assert(!valueRangeDebug && "implement when meet");
            return std::make_pair(0, 0);
        }
    } else {
        // todo: get range from variable array
        assert(!valueRangeDebug && "implement when meet");
        return std::make_pair(0, 0);
    }
}

/*
 * infer the result range of instruction based on the incoming value of Phi node.
 * e.g.
 *  %x = phi double [%a, %bb0], [5, %bb1], [undef, %bb2]
 *  %y = sub double %x, 10
 * if we know the range of %a, then the range of %x should be [min(min_a, 5), max(max_a, 5)]
 *
 * but it could be more accuracy for IntSet.
 * */
bool checkPhiRange(State * N, PHINode* phiNode,
                   const std::vector<Value*>& incomingValueVec,
                   std::map<llvm::Value *, std::pair<double, double>>& virtualRegisterRange) {
    std::vector<double> minValueVec, maxValueVec;
    for (auto phiValue : incomingValueVec) {
        if (isa<llvm::Constant>(phiValue)) {
            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(phiValue)) {
                if (phiValue->getType()->isFloatTy()) {
                    float constValue = constFp->getValueAPF().convertToFloat();
                    minValueVec.emplace_back(constValue);
                    maxValueVec.emplace_back(constValue);
                } else if (phiValue->getType()->isDoubleTy()) {
                    double constValue = constFp->getValueAPF().convertToDouble();
                    minValueVec.emplace_back(constValue);
                    maxValueVec.emplace_back(constValue);
                }
            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(phiValue)) {
                auto constValue = constInt->getSExtValue();
                minValueVec.emplace_back(static_cast<double>(constValue));
                maxValueVec.emplace_back(static_cast<double>(constValue));
            } else if (auto pointerPhi = dyn_cast<PointerType>(phiValue->getType())) {
                // todo: get the constant GEP from PhiNode by create a loadInst then remove it.
                return false;
            } else if (isa<UndefValue>(phiValue) || isa<PoisonValue>(phiValue)) {
                /*do nothing*/
            } else {
                assert(!valueRangeDebug && "implement when meet");
            }
        } else {
            auto vrRangeIt = virtualRegisterRange.find(phiValue);
            if (vrRangeIt != virtualRegisterRange.end())
            {
                minValueVec.emplace_back(vrRangeIt->second.first);
                maxValueVec.emplace_back(vrRangeIt->second.second);
            } else {
                assert(!valueRangeDebug && "failed to get range");
                return false;
            }
        }
    }
    double minRes = *std::min_element(minValueVec.begin(), minValueVec.end());
    double maxRes = *std::max_element(maxValueVec.begin(), maxValueVec.end());
    virtualRegisterRange.emplace(phiNode, std::make_pair(minRes, maxRes));
    return true;
}

std::pair<Value *, std::pair<double, double>>
rangeAnalysis(State * N, BoundInfo * boundInfo, Function & llvmIrFunction)
{
    /*
     * information for the union data structure
     * */
    std::map<Value *, Value *> unionAddress;
    /*
     * function arguments
     * */
    std::map<Value *, Value *> storeParamMap;
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
                        if (calledFunction->getName().startswith("llvm.dbg.value") ||
                            calledFunction->getName().startswith("llvm.dbg.declare"))
                        {
                            auto firstOperator = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(0));
                            auto localVariableAddressAsMetadata = cast<ValueAsMetadata>(firstOperator->getMetadata());
                            auto localVariableAddress = localVariableAddressAsMetadata->getValue();

                            auto variableMetadata = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(1));
                            auto debugInfoVariable = cast<DIVariable>(variableMetadata->getMetadata());
                            const DIType * variableType = debugInfoVariable->getType();

                            auto recordType = [&](const DIType *variableType) {
                                if (const auto *derivedVariableType = dyn_cast<DIDerivedType>(variableType))
                                {
                                    std::string baseTypeName;
                                    /*
                                    *	if we find such type in boundInfo->typeRange,
                                    *	we record it in the boundInfo->virtualRegisterRange
                                    */
                                    if (derivedVariableType->getTag() == llvm::dwarf::DW_TAG_pointer_type) {
                                        baseTypeName = derivedVariableType->getBaseType()->getName().str();
                                    } else {
                                        baseTypeName = derivedVariableType->getName().str();
                                    }
                                    auto typeRangeIt = boundInfo->typeRange.find(baseTypeName);
                                    if (typeRangeIt != boundInfo->typeRange.end())
                                    {
                                        boundInfo->virtualRegisterRange.emplace(localVariableAddress, typeRangeIt->second);
                                        auto spIt = storeParamMap.find(localVariableAddress);
                                        if (spIt != storeParamMap.end()) {
                                            boundInfo->virtualRegisterRange.emplace(spIt->second, typeRangeIt->second);
                                        }
                                    }
                                    else {
                                        //todo: other DIDerivedType, like size_t
                                    }
                                }
                                else if (const auto *basicVariableType = dyn_cast<DIBasicType>(variableType))
                                {
                                    /*
                                     * if it's a basic type, insert the basic, todo
                                     * */
//                                boundInfo->virtualRegisterRange.emplace(localVariableAddress, size);
                                }
                            };

                            if (const auto *compositeVariableType = dyn_cast<DICompositeType>(variableType))
                            {
                                /*
                                 * It's a composite type, including structure, union, array, and enumeration
                                 * Extract from composite type
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
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: DW_TAG_array_type\n");
                                    const DIType *ElemType = compositeVariableType->getBaseType();
                                    recordType(ElemType);
                                }
                                else if (typeTag == dwarf::DW_TAG_enumeration_type)
                                {
                                    // todo
                                    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: DW_TAG_enumeration_type\n");
                                }
                            }
                            recordType(variableType);

                        }
                        /*
                         * It's function defined by programmer, eg. %17 = call i32 @abstop12(float %16), !dbg !83
                         * */
                        else
                        {
                            if (!calledFunction)
                            {
                                flexprint(N->Fe, N->Fm, N->Fperr, "\tCall: CalledFunction %s is nullptr or undeclared.\n",
                                          calledFunction->getName().str().c_str());
                                continue;
                            } else if (calledFunction->isDeclaration()) {
                                /*
                                 * the primary definition of this global value is outside the current translation unit.
                                 * */
                                std::string funcName = calledFunction->getName().str();
//                                std::vector<std::string> monotonicLibFunc = {"log", "sqrt", "log1p", "exp", "scalbn"};
//                                std::vector<std::string> trigonometircFunc = {"sin", "cos"};
//                                std::vector<std::string> monotonicLLVMFunc = {"llvm.floor", "llvm.ceil"};
                                std::map<uint32_t, std::pair<double, double>> argRanges;
                                for (size_t idx = 0; idx < llvmIrCallInstruction->getNumOperands() - 1; idx++)
                                {
                                    auto vrRangeIt = boundInfo->virtualRegisterRange.find(
                                            llvmIrCallInstruction->getOperand(idx));
                                    if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                                    {
                                        argRanges.emplace(idx, vrRangeIt->second);
                                    } else {
                                        assert(!valueRangeDebug && "failed to get range");
                                        break;
                                    }
                                }
                                double lowRange, highRange;
                                // todo: reconstruct by MACRO or template
                                if (funcName == "log") {
                                    lowRange = log(argRanges[0].first);
                                    highRange = log(argRanges[0].second);
                                } else if (funcName == "exp") {
                                    lowRange = exp(argRanges[0].first);
                                    highRange = exp(argRanges[0].second);
                                } else if (funcName == "sqrt") {
                                    lowRange = sqrt(argRanges[0].first);
                                    highRange = sqrt(argRanges[0].second);
                                } else if (funcName == "log1p") {
                                    lowRange = log1p(argRanges[0].first);
                                    highRange = log1p(argRanges[0].second);
                                } else if (funcName == "scalbn") {
                                    lowRange = scalbn(argRanges[0].first, argRanges[1].first);
                                    highRange = scalbn(argRanges[0].second, argRanges[1].second);
                                } else if (funcName == "sin" || funcName == "cos") {
                                    lowRange = -1;
                                    highRange = 1;
                                } else if (calledFunction->getName().startswith("llvm.fabs")) {
                                    lowRange = min(fabs(argRanges[0].first),
                                                   fabs(argRanges[0].second));
                                    highRange = max(fabs(argRanges[0].first),
                                                    fabs(argRanges[0].second));
                                } else if (calledFunction->getName().startswith("llvm.floor")) {
                                    lowRange = floor(argRanges[0].first);
                                    highRange = floor(argRanges[0].second);
                                } else if (calledFunction->getName().startswith("llvm.ceil")) {
                                    lowRange = ceil(argRanges[0].first);
                                    highRange = ceil(argRanges[0].second);
                                } else {
                                    assert(!valueRangeDebug && "didn't support such function yet");
                                    break;
                                }
                                boundInfo->virtualRegisterRange.emplace(llvmIrCallInstruction,
                                                                        std::make_pair(lowRange, highRange));
                            } else {
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
                                    }
                                    else if (ConstantFP * constFp = dyn_cast<ConstantFP>(llvmIrCallInstruction->getOperand(idx)))
                                    {
                                        double constDoubleValue = (constFp->getValueAPF()).convertToDouble();
                                        flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCall: It's a constant double value: %f.\n", constDoubleValue);
                                        innerBoundInfo->virtualRegisterRange.emplace(calledFunction->getArg(idx),
                                                std::make_pair(constDoubleValue, constDoubleValue));
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
                                        } else {
                                            assert(!valueRangeDebug && "failed to get range");
                                        }
                                    }
                                }
                                auto returnRange = rangeAnalysis(N, innerBoundInfo, *calledFunction);
                                if (returnRange.first != nullptr)
                                {
                                    boundInfo->virtualRegisterRange.emplace(llvmIrCallInstruction, returnRange.second);
                                }
                                boundInfo->virtualRegisterRange.insert(innerBoundInfo->virtualRegisterRange.begin(),
                                        innerBoundInfo->virtualRegisterRange.end());
                                /*
                                 * Check the return type of the function,
                                 * if it's a physical type that records in `boundInfo.typeRange`
                                 * but didn't match the range we inferred from `rangeAnalysis` algorithm,
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
                             * 	e.g. x1+x2
                             * 	btw, I don't think we should check type here, which should be done in other pass like dimension-check
                             * 	find left operand from the boundInfo->virtualRegisterRange
                             * 	range: [x1_min+x2_min, x1_max+x2_max]
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            /*
                             * 	find right operand from the boundInfo->virtualRegisterRange
                             */
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound += vrRangeIt->second.first;
                                upperBound += vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
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
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tUnexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
                        }
                    }
                    break;

                case Instruction::Sub:
                case Instruction::FSub:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        Value * leftOperand = llvmIrInstruction.getOperand(0);
                        Value * rightOperand = llvmIrInstruction.getOperand(1);
                        if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tSub: Expression normalization needed.\n");
                        }
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            /*
                             * 	e.g. x1-x2
                             * 	btw, I don't think we should check type here, which should be done in other pass like dimension-check
                             * 	find left operand from the boundInfo->virtualRegisterRange
                             * 	range: [x1_min-x2_max, x1_max-x2_min]
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound -= vrRangeIt->second.second;
                                upperBound -= vrRangeIt->second.first;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
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
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
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
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tSub: Unexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
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
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            /*
                             * 	e.g. x1*x2
                             * 	range: [min(x1_min*x2_min, x1_min*x2_max, x1_max*x2_min, x1_max*x2_max),
                             * 	        max(x1_min*x2_min, x1_min*x2_max, x1_max*x2_min, x1_max*x2_max)]
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                auto leftMin = lowerBound;
                                auto leftMax = upperBound;
                                auto rightMin = vrRangeIt->second.first;
                                auto rightMax = vrRangeIt->second.second;
                                lowerBound = min(min(min(leftMin * rightMin,
                                                         leftMin * rightMax),
                                                     leftMax * rightMin),
                                                 leftMax * rightMax);
                                upperBound = max(max(max(leftMin * rightMin,
                                                         leftMin * rightMax),
                                                     leftMax * rightMin),
                                                 leftMax * rightMax);
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                        } else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
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
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        } else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tMul: Unexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
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
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            /*
                             * 	e.g. x1/x2
                             * 	range: [min(x1_min/x2_min, x1_min/x2_max, x1_max/x2_min, x1_max/x2_max),
                             * 	        max(x1_min/x2_min, x1_min/x2_max, x1_max/x2_min, x1_max/x2_max)]
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                auto leftMin = lowerBound;
                                auto leftMax = upperBound;
                                auto rightMin = vrRangeIt->second.first;
                                auto rightMax = vrRangeIt->second.second;
                                lowerBound = min(min(min(leftMin / rightMin,
                                                         leftMin / rightMax),
                                                     leftMax / rightMin),
                                                 leftMax / rightMax);
                                upperBound = max(max(max(leftMin / rightMin,
                                                         leftMin / rightMax),
                                                     leftMax / rightMin),
                                                 leftMax / rightMax);
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	eg. 2/x
                             */
                            double constValue = 1.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(leftOperand))
                            {
                                constValue = (constFp->getValueAPF()).convertToDouble();
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(leftOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                auto rightMin = vrRangeIt->second.first;
                                auto rightMax = vrRangeIt->second.second;
                                double lowerBound = min(constValue / rightMin, constValue / rightMax);
                                double upperBound = max(constValue / rightMin, constValue / rightMax);
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                                                        std::make_pair(lowerBound, upperBound));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
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
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tDiv: Unexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
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
                        /*
                         * todo:
                         * for positive: [0, rhs]
                         * for negative: [rhs, 0]
                         * */
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
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            /*
                             * 	e.g. x1 << x2
                             * 	range: [min(x1_min<<x2_min, x1_min<<x2_max, x1_max<<x2_min, x1_max<<x2_max),
                             * 	        max(x1_min<<x2_min, x1_min<<x2_max, x1_max<<x2_min, x1_max<<x2_max)]
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                auto leftMin = lowerBound;
                                auto leftMax = upperBound;
                                auto rightMin = vrRangeIt->second.first;
                                auto rightMax = vrRangeIt->second.second;
                                lowerBound = min(min(min((int)leftMin << (int)rightMin,
                                                         (int)leftMin << (int)rightMax),
                                                     (int)leftMax << (int)rightMin),
                                                 (int)leftMax << (int)rightMax);
                                upperBound = max(max(max((int)leftMin << (int)rightMin,
                                                         (int)leftMin << (int)rightMax),
                                                     (int)leftMax << (int)rightMin),
                                                 (int)leftMax << (int)rightMax);
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	e.g. 2 << x
                             * 	range: [min(2<<x2_min, 2<<x2_max),
                             * 	        max(2<<x2_min, 2<<x2_max)]
                             */
                            uint64_t constValue = 1.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(leftOperand))
                            {
                                constValue = static_cast<uint64_t>((constFp->getValueAPF()).convertToDouble());
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(leftOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                // todo: if we need assert or other check here?
                                uint64_t rightMin = vrRangeIt->second.first < 0 ? 0 : vrRangeIt->second.first;
                                uint64_t rightMax = vrRangeIt->second.second < 0 ? 0 : vrRangeIt->second.second;
                                double lowerBound = min(constValue << rightMin, constValue << rightMax);
                                double upperBound = max(constValue << rightMin, constValue << rightMax);
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                        std::make_pair(lowerBound, upperBound));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
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
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShl: Unexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
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
                        if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            double lowerBound = 0.0;
                            double upperBound = 0.0;
                            /*
                             * 	e.g. x1 >> x2
                             * 	range: [min(x1_min>>x2_min, x1_min>>x2_max, x1_max>>x2_min, x1_max>>x2_max),
                             * 	        max(x1_min>>x2_min, x1_min>>x2_max, x1_max>>x2_min, x1_max>>x2_max)]
                             */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                lowerBound = vrRangeIt->second.first;
                                upperBound = vrRangeIt->second.second;
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                auto leftMin = lowerBound;
                                auto leftMax = upperBound;
                                auto rightMin = vrRangeIt->second.first;
                                auto rightMax = vrRangeIt->second.second;
                                lowerBound = min(min(min((int)leftMin >> (int)rightMin,
                                                         (int)leftMin >> (int)rightMax),
                                                     (int)leftMax >> (int)rightMin),
                                                 (int)leftMax >> (int)rightMax);
                                upperBound = max(max(max((int)leftMin >> (int)rightMin,
                                                         (int)leftMin >> (int)rightMax),
                                                     (int)leftMax >> (int)rightMin),
                                                 (int)leftMax >> (int)rightMax);
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
                            boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator, std::make_pair(lowerBound, upperBound));
                        }
                        else if (isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
                        {
                            /*
                             * 	e.g. 2 >> x
                             * 	range: [min(2>>x2_min, 2>>x2_max),
                             * 	        max(2>>x2_min, 2>>x2_max)]
                             */
                            uint64_t constValue = 1.0;
                            if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(leftOperand))
                            {
                                constValue = static_cast<uint64_t>((constFp->getValueAPF()).convertToDouble());
                            } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(leftOperand))
                            {
                                constValue = constInt->getSExtValue();
                            }
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                // todo: if we need assert or other check here?
                                uint64_t rightMin = vrRangeIt->second.first < 0 ? 0 : vrRangeIt->second.first;
                                uint64_t rightMax = vrRangeIt->second.second < 0 ? 0 : vrRangeIt->second.second;
                                double lowerBound = min(constValue >> rightMin, constValue >> rightMax);
                                double upperBound = max(constValue >> rightMin, constValue >> rightMax);
                                boundInfo->virtualRegisterRange.emplace(llvmIrBinaryOperator,
                                        std::make_pair(lowerBound, upperBound));
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                                break;
                            }
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
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
                        else
                        {
                            flexprint(N->Fe, N->Fm, N->Fperr, "\tShr: Unexpected error. Might have an invalid operand.\n");
                            assert(!valueRangeDebug && "failed to get range");
                        }
                    }
                    break;

                case Instruction::And:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        /*
                         * todo: it's non-linear function for decimal, but might get range in binary
                         * */
                    }
                    break;

                case Instruction::Or:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        /*
                         * todo: it's non-linear function for decimal, but might get range in binary
                         * */
                    }
                    break;

                case Instruction::Xor:
                    if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
                    {
                        /*
                         * todo: it's non-linear function for decimal, but might get range in binary
                         * */
                    }
                    break;

                case Instruction::FPToUI:
                case Instruction::FPToSI:
                case Instruction::SIToFP:
                case Instruction::UIToFP:
                case Instruction::ZExt:
                case Instruction::SExt:
                case Instruction::FPExt:
                case Instruction::Trunc:
                case Instruction::FPTrunc:
                {
                    Value * operand = llvmIrInstruction.getOperand(0);
                    auto	vrRangeIt = boundInfo->virtualRegisterRange.find(operand);
                    if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                    {
                        boundInfo->virtualRegisterRange.emplace(&llvmIrInstruction,
                                                                vrRangeIt->second);
                    } else {
                        assert(!valueRangeDebug && "failed to get range");
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

                        } else {
                            assert(!valueRangeDebug && "failed to get range");
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
                        }
                        else
                        {
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrStoreInstruction->getOperand(0));
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrStoreInstruction->getOperand(1), vrRangeIt->second);
                            } else {
                                /*
                                 * llvmIrStoreInstruction->getOperand(0) is the param of function
                                 * */
                                if (isa<Argument>(llvmIrStoreInstruction->getOperand(0))) {
                                    storeParamMap.emplace(llvmIrStoreInstruction->getOperand(1),
                                                          llvmIrStoreInstruction->getOperand(0));
                                } else {
                                    assert(!valueRangeDebug && "failed to get range");
                                }
                            }
                            /*
                             * Each time if there's a StorInst assign to the unionAddress, it updates the value of union.
                             * */
                            auto uaIt = unionAddress.find(llvmIrStoreInstruction->getOperand(1));
                            if (uaIt != unionAddress.end())
                            {
                                flexprint(N->Fe, N->Fm, N->Fpinfo, "\tStore Union: %f - %f\n",  vrRangeIt->second.first, vrRangeIt->second.second);
                                boundInfo->virtualRegisterRange.emplace(uaIt->second, vrRangeIt->second);
                            }
                        }
                    }
                    break;

//                case Instruction::Trunc:
//                    if (auto llvmIrTruncInstruction = dyn_cast<TruncInst>(&llvmIrInstruction))
//                    {
//                        auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrTruncInstruction->getOperand(0));
//                        if (vrRangeIt != boundInfo->virtualRegisterRange.end())
//                        {
//                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFPTrunc: %f - %f\n",  vrRangeIt->second.first, vrRangeIt->second.second);
//                            double originLow = vrRangeIt->second.first;
//                            double originHigh = vrRangeIt->second.second;
//                            double lowRange = 0, highRange = 0;
//                            auto DestEleType = llvmIrTruncInstruction->getDestTy();
//                            switch (DestEleType->getTypeID())
//                            {
//                                case Type::IntegerTyID:
//                                    switch (DestEleType->getIntegerBitWidth())
//                                    {
//                                        case 8:
//                                            lowRange = static_cast<double>(static_cast<int8_t>(originLow));
//                                            highRange = static_cast<double>(static_cast<int8_t>(originHigh));
//                                        break;
//                                        case 16:
//                                            lowRange = static_cast<double>(static_cast<int16_t>(originLow));
//                                            highRange = static_cast<double>(static_cast<int16_t>(originHigh));
//                                        break;
//                                        case 32:
//                                            lowRange = static_cast<double>(static_cast<int32_t>(originLow));
//                                            highRange = static_cast<double>(static_cast<int32_t>(originHigh));
//                                        break;
//                                        case 64:
//                                            lowRange = static_cast<double>(static_cast<int64_t>(originLow));
//                                            highRange = static_cast<double>(static_cast<int64_t>(originHigh));
//                                        break;
//                                        default:
//                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tTrunc: Type::SignedInteger, don't support such bit width yet.");
//                                    }
//                                break;
//                            }
//                            boundInfo->virtualRegisterRange.emplace(llvmIrTruncInstruction, std::make_pair(lowRange, highRange));
//                        }
//                    }
//                break;
//
//                case Instruction::FPTrunc:
//                    if (auto llvmIrFPTruncInstruction = dyn_cast<FPTruncInst>(&llvmIrInstruction))
//                    {
//                        auto vrRangeIt = boundInfo->virtualRegisterRange.find(llvmIrFPTruncInstruction->getOperand(0));
//                        if (vrRangeIt != boundInfo->virtualRegisterRange.end())
//                        {
//                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFPTrunc: %f - %f\n",  vrRangeIt->second.first, vrRangeIt->second.second);
//                            boundInfo->virtualRegisterRange.emplace(llvmIrFPTruncInstruction,
//                                                                    std::make_pair(static_cast<double>(static_cast<float>(vrRangeIt->second.first)),
//                                                                                   static_cast<double>(static_cast<float>(vrRangeIt->second.second))));
//                        }
//                    }
//                    break;

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
                            /*
                             * if it's a structure type, we use reinterpret_cast
                             * todo: not very sure, need further check
                             * */
                            if (llvmIrBitCastInstruction->getSrcTy()->isStructTy()) {
                                switch (DestEleType->getTypeID())
                                {
                                    case Type::FloatTyID:
                                        lowRange = static_cast<double>(*reinterpret_cast<float *>(&originLow));
                                        highRange = static_cast<double>(*reinterpret_cast<float *>(&originHigh));
                                        flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::FloatTyID, %f - %f to %f - %f\n",
                                        vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                        boundInfo->virtualRegisterRange.emplace(llvmIrBitCastInstruction, std::make_pair(lowRange, highRange));
                                        break;
                                    case Type::DoubleTyID:
                                        lowRange = *reinterpret_cast<double *>(&originLow);
                                        highRange = *reinterpret_cast<double *>(&originHigh);
                                        flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::DoubleTyID, %f - %f to %f - %f\n",
                                        vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                        boundInfo->virtualRegisterRange.emplace(llvmIrBitCastInstruction, std::make_pair(lowRange, highRange));
                                        break;
                                    case Type::IntegerTyID:
                                        switch (DestEleType->getIntegerBitWidth())
                                        {
                                            case 8:
                                                lowRange = static_cast<double>(*reinterpret_cast<int8_t *>(&originLow));
                                                highRange = static_cast<double>(*reinterpret_cast<int8_t *>(&originHigh));
                                                break;
                                            case 16:
                                                lowRange = static_cast<double>(*reinterpret_cast<int16_t *>(&originLow));
                                                highRange = static_cast<double>(*reinterpret_cast<int16_t *>(&originHigh));
                                                break;
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
                                        break;
                                    case Type::StructTyID:
                                        flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::StructTyID, %f - %f to %f - %f\n",
                                        vrRangeIt->second.first, vrRangeIt->second.second, originLow, originHigh);
                                        break;
                                    default:
                                        flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Do not support other type yet.\n");
                                        boundInfo->virtualRegisterRange.emplace(llvmIrBitCastInstruction, vrRangeIt->second);
                                        continue;
                                }
                            } else {
                                boundInfo->virtualRegisterRange.emplace(llvmIrBitCastInstruction, vrRangeIt->second);
                            }
                        } else {
                            assert(!valueRangeDebug && "failed to get range");
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
                                    switch (resEleTy->getTypeID())
                                    {
                                        case Type::FloatTyID:
                                            lowRange = static_cast<double>(static_cast<float>(originLowWord >> (32 * element_offset)));
                                            highRange = static_cast<double>(static_cast<float>(originHighWord >> (32 * element_offset)));
                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::FloatTyID, %f - %f to %f - %f\n",
                                                      vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                            boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction, std::make_pair(lowRange, highRange));
                                            break;
                                        case Type::DoubleTyID:
                                            lowRange = static_cast<double>(originLowWord >> (32 * element_offset));
                                            highRange = static_cast<double>(originHighWord >> (32 * element_offset));
                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tBitCast: Type::DoubleTyID, %f - %f to %f - %f\n",
                                                      vrRangeIt->second.first, vrRangeIt->second.second, lowRange, highRange);
                                            boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction, std::make_pair(lowRange, highRange));
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
                                            break;
                                        default:
                                            flexprint(N->Fe, N->Fm, N->Fpinfo, "\tGetElePtr: Do not support other type yet.\n");
                                            boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction, vrRangeIt->second);
                                            continue;
                                    }
                                } else {
                                    assert(!valueRangeDebug && "failed to get range");
                                }
                            }
                        }
                        /*
                         * infer the range from structure or array
                         * */
                        else if (llvmIrGetElePtrInstruction->getPointerOperandType()
                                    ->getPointerElementType()->isArrayTy()) {
                            auto resVec = getGEPArrayRange(N, llvmIrGetElePtrInstruction,
                                                           boundInfo->virtualRegisterRange);
                            boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction,
                                                                    std::make_pair(resVec.first, resVec.second));
                        } else if (llvmIrGetElePtrInstruction->getPointerOperandType()
                                    ->getPointerElementType()->isStructTy()) {
                            // todo: get range from structure
                            assert(!valueRangeDebug && "implement when meet");
                        } else {
                            /*
                             * e.g. getelementptr inbounds i8, i8* %0, i64 0
                             * */
                            auto vrRangeIt = boundInfo->virtualRegisterRange.find(
                                    llvmIrGetElePtrInstruction->getOperand(0));
                            if (vrRangeIt != boundInfo->virtualRegisterRange.end())
                            {
                                boundInfo->virtualRegisterRange.emplace(llvmIrGetElePtrInstruction,
                                                                        vrRangeIt->second);
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
                            }
                        }
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
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
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
                            } else {
                                assert(!valueRangeDebug && "failed to get range");
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
                    if (auto llvmIRPhiNode = dyn_cast<PHINode>(&llvmIrInstruction)) {
                        std::vector<Value*> incomingValueVec;
                        for (size_t idx = 0; idx < llvmIRPhiNode->getNumIncomingValues(); idx++) {
                            auto incomingValue = llvmIRPhiNode->getIncomingValue(idx);
                            incomingValueVec.emplace_back(incomingValue);
                        }
                        checkPhiRange(N, llvmIRPhiNode, incomingValueVec, boundInfo->virtualRegisterRange);
                    }
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
}
