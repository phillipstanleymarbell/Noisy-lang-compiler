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

#include "newton-irPass-LLVMIR-shrinkTypeByRange.h"

using namespace llvm;

extern "C"
{

enum varType {
    INT1        = 1,
    INT8        = 2,
    INT16       = 3,
    INT32       = 4,
    INT64       = 5,
    FLOAT       = 6,
    DOUBLE      = 7,
    UNKNOWN     = 8,
};

varType
getIntegerTypeEnum(double min, double max, bool signFlag)
{
    varType finalType;
    if (!signFlag && max <= 1)
    {
        finalType = INT1;
    }
    else if ((!signFlag && max < UINT8_MAX) ||
             (signFlag && min > INT8_MIN && max < INT8_MAX))
    {
        finalType = INT8;
    }
    else if ((!signFlag && max < UINT16_MAX) ||
             (signFlag && min > INT16_MIN && max < INT16_MAX))
    {
        finalType = INT16;
    }
    else if ((!signFlag && max < UINT32_MAX) ||
             (signFlag && min > INT32_MIN && max < INT32_MAX))
    {
        finalType = INT32;
    }
    else if ((!signFlag && max < UINT64_MAX) ||
             (signFlag && min > INT64_MIN && max < INT64_MAX))
    {
        finalType = INT64;
    }
    else
    {
        finalType = UNKNOWN;
    }
    return finalType;
}

varType
getFloatingTypeEnum(double min, double max)
{
    varType finalType;
    if ((std::abs(min) < FLT_MAX) && (std::abs(max) < FLT_MAX))
    {
        finalType = FLOAT;
    }
    else if ((std::abs(min) < DBL_MAX) && (std::abs(max) < DBL_MAX))
    {
        finalType = DOUBLE;
    }
    else
    {
        // not support yet
        finalType = UNKNOWN;
    }
    return finalType;
}

typedef struct typeInfo {
    Type* valueType;
    bool signFlag;
} typeInfo;

/*
 * nullptr means cannot do shrinkType
 * else return the shrinkIntType
 * */
typeInfo
getShrinkIntType(State * N, Value *boundValue, const std::pair<double, double>& boundRange)
{
    typeInfo typeInformation;
    typeInformation.valueType = nullptr;
    /*
     * Signed-ness in LLVM is not stored into the integer, it is left to
     * interpretation to the instruction that use them.
     * */
    typeInformation.signFlag = boundRange.first < 0;

    varType finalType = getIntegerTypeEnum(boundRange.first, boundRange.second, typeInformation.signFlag);

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
    assert(typeId == Type::IntegerTyID);

    switch (bitWidth) {
        case 64:
            if (finalType == INT32)
            {
                typeInformation.valueType = IntegerType::getInt32Ty(context);
            }
        case 32:
            if (finalType == INT16)
            {
                typeInformation.valueType = IntegerType::getInt16Ty(context);
            }
        case 16:
            if (finalType == INT8)
            {
                typeInformation.valueType = IntegerType::getInt8Ty(context);
            }
        case 8:
            if (finalType == INT1)
            {
                typeInformation.valueType = IntegerType::getInt1Ty(context);
            }
            break;
        default:
            flexprint(N->Fe, N->Fm, N->Fpinfo,
                      "\tgetShrinkIntType: Type::Integer, don't support such bit width yet.");
    }

    if (typeInformation.valueType != nullptr) {
        typeInformation.valueType = isPointer ?
                typeInformation.valueType->getPointerTo(pointerAddr) :
                typeInformation.valueType;
    }

    return typeInformation;
}

typeInfo getShrinkDoubleType(State * N, Value *boundValue, const std::pair<double, double>& boundRange) {
    typeInfo typeInformation;
    typeInformation.valueType = nullptr;
    typeInformation.signFlag = boundRange.first < 0;

    varType finalType = getFloatingTypeEnum(boundRange.first, boundRange.second);

    auto previousType = boundValue->getType();
    auto typeId = previousType->getTypeID();
    auto& context = previousType->getContext();

    bool isPointer = false;
    unsigned pointerAddr;
    if (typeId == Type::PointerTyID)
    {
        typeId = previousType->getPointerElementType()->getTypeID();
        isPointer = true;
        pointerAddr = previousType->getPointerAddressSpace();
    }
    assert(typeId == Type::DoubleTyID);

    if (finalType == FLOAT) {
        typeInformation.valueType = Type::getFloatTy(context);
    } else if (finalType == DOUBLE) {
        typeInformation.valueType = Type::getDoubleTy(context);
    } else {
        // todo: not support yet, like HalfTyID, BFloatTyID, X86_FP80TyID, FP128TyID, PPC_FP128TyID, etc.
    }

    typeInformation.valueType = isPointer ?
            typeInformation.valueType->getPointerTo(pointerAddr) :
            typeInformation.valueType;

    return typeInformation;
}

typeInfo
getTypeInfo(State * N, Value *inValue,
            const std::map<llvm::Value *, std::pair<double, double>>& virtualRegisterRange) {
    typeInfo typeInformation;
    typeInformation.signFlag = false;
    typeInformation.valueType = nullptr;

    auto vrRangeIt = virtualRegisterRange.find(inValue);
    if (vrRangeIt == virtualRegisterRange.end()) {
        return typeInformation;
    }

    auto inInstType = inValue->getType();
    inInstType = inInstType->getTypeID() == Type::PointerTyID ?
            inInstType->getPointerElementType() : inInstType;
    switch (inInstType->getTypeID()) {
        case Type::IntegerTyID:
            typeInformation = getShrinkIntType(N, inValue, vrRangeIt->second);
            break;
        case Type::FloatTyID:
            // todo: convert floating point to integer
            break;
        case Type::DoubleTyID:
            typeInformation = getShrinkDoubleType(N, inValue, vrRangeIt->second);
            break;
        default:
            break;
    }
    return typeInformation;
}

/*
 * return the instruction after roll back
 * */
void
rollbackType(State * N, Instruction *inInstruction, unsigned operandIdx, BasicBlock & llvmIrBasicBlock,
             std::map<Value*, Type*>& typeChangedInst, Type* backType = nullptr)
{
    Value *inValue = inInstruction->getOperand(operandIdx);
    if (isa<GetElementPtrInst>(inInstruction) && isa<llvm::Constant>(inValue)) {
        return;
    }
    if (backType == nullptr) {
        auto tcInstIt = typeChangedInst.find(inValue);
        assert(tcInstIt != typeChangedInst.end() &&
                "backType is nullptr and cannot find it in typeChangedInst");
        backType = tcInstIt->second;
        assert(backType != nullptr && "backType cannot be nullptr");
    }

    if (Instruction *valueInst = llvm::dyn_cast<llvm::Instruction>(inValue)) {
        /*
         * if it's an instruction that need to roll back, insert a cast
         * */
        if (PHINode *inInstPhi = llvm::dyn_cast<llvm::PHINode>(inInstruction)) {
            /*
             * if it's a phi node, insert the cast instruction in the incoming basic block
             * */
            auto incomingBB = inInstPhi->getIncomingBlock(operandIdx);
            IRBuilder<> Builder(incomingBB);
            auto terminatorInst = incomingBB->getTerminator();
            Builder.SetInsertPoint(terminatorInst);
            auto tmp = inInstruction->clone();
            Value * castInst;
            if (backType->isIntegerTy()) {
                castInst = Builder.CreateIntCast(valueInst, backType, false);
            } else {
                castInst = Builder.CreateFPCast(valueInst, backType);
            }
            inInstruction->replaceUsesOfWith(valueInst, castInst);
        } else {
            IRBuilder<> Builder(&llvmIrBasicBlock);
            Builder.SetInsertPoint(inInstruction);
            Value * castInst;
            if (backType->isIntegerTy()) {
                castInst = Builder.CreateIntCast(valueInst, backType, false);
            } else {
                castInst = Builder.CreateFPCast(valueInst, backType);
            }
            inInstruction->replaceUsesOfWith(valueInst, castInst);
        }
    } else if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(inValue)) {
        auto constValue = (constFp->getValueAPF()).convertToDouble();
        auto newConstant = ConstantFP::get(backType, constValue);
        inInstruction->replaceUsesOfWith(inValue, newConstant);
    } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(inValue)) {
        auto constValue = constInt->getSExtValue();
        auto newConstant = ConstantInt::get(backType, constValue);
        inInstruction->replaceUsesOfWith(inValue, newConstant);
    } else if (isa<UndefValue>(inValue)) {

    } else {
        assert(false);
    }
    return;
}

/*
 * first > second: greater than 0,
 * first = second: equal to 0,
 * first < second: less than 0,
 * */
int compareType(Type* firstType, Type* secondType)
{
    std::function<varType(Type* inputType)> typeEnumConvert;
    typeEnumConvert = [&](Type* inputType) -> varType {
        /*
         * extract raw type from pointer or array
         * */
        Type* eleType;
        if (inputType->getTypeID() == Type::PointerTyID) {
            eleType = inputType->getPointerElementType();
            return typeEnumConvert(eleType);
        } else if (inputType->getTypeID() == Type::ArrayTyID) {
            eleType = inputType->getArrayElementType();
            return typeEnumConvert(eleType);
        } else {
            eleType = inputType;
        }
        uint64_t intBitWidth;
        switch (eleType->getTypeID()) {
            case Type::FloatTyID:
                return varType::FLOAT;
            case Type::DoubleTyID:
                return varType::DOUBLE;
            case Type::IntegerTyID:
                intBitWidth = eleType->getIntegerBitWidth();
                switch (intBitWidth) {
                    case 1:
                        return varType::INT1;
                    case 8:
                        return varType::INT8;
                    case 16:
                        return varType::INT16;
                    case 32:
                        return varType::INT32;
                    case 64:
                        return varType::INT64;
                    default:
                        return varType::UNKNOWN;
                }
            default:
                return varType::UNKNOWN;
        }
    };
    return typeEnumConvert(firstType) - typeEnumConvert(secondType);
}

void
matchPhiOperandType(State * N, Instruction *inInstruction, BasicBlock & llvmIrBasicBlock,
                    std::map<Value*, Type*>& typeChangedInst)
{
    std::vector<Value*> operands;
    for (size_t id = 0; id < inInstruction->getNumOperands(); id++) {
        operands.emplace_back(inInstruction->getOperand(id));
    }
    if (std::all_of(operands.begin(), operands.end(), [&](Value* const& v) {
        return compareType(v->getType(), operands.front()->getType()) == 0;
    }))
        return;

    /*
     * If both are non-constant, roll back
     * */
    if (std::all_of(operands.begin(), operands.end(), [&](Value* const& v) {
        return !isa<llvm::Constant>(v);
    })) {
        Type* backType = nullptr;
        bool noPrevType = false;
        for (size_t id = 0; id < operands.size(); id++) {
            if (typeChangedInst.find(operands[id]) != typeChangedInst.end()) {
                rollbackType(N, inInstruction, id, llvmIrBasicBlock,
                             typeChangedInst);
            } else {
                /*
                 * if current op didn't have previous type, reset all ops into this type
                 * */
                backType = operands[id]->getType();
                noPrevType = true;
                break;
            }
        }
        if (noPrevType) {
            assert(backType != nullptr && "backType is not set");
            for (size_t id = 0; id < operands.size(); id++) {
                rollbackType(N, inInstruction, id, llvmIrBasicBlock,
                             typeChangedInst, backType);
            }
        }
    } else {
        /*
         * If one of them is constant, shrink the constant operand
         * */
        auto opIt = std::find_if(operands.begin(), operands.end(), [&](Value* const& v) {
            return isa<llvm::Constant>(v);
        });
        assert(opIt != operands.end() && "didn't find constant operand");
        Type* constantType = (*opIt)->getType();
        for (size_t id = 0; id < operands.size(); id++) {
            /*
             * todo: can be further improved, by shrink both of them to greatest common type
             * */
            rollbackType(N, inInstruction, id, llvmIrBasicBlock,
                         typeChangedInst, constantType);
        }
    }
}

void
matchOperandType(State * N, Instruction *inInstruction, BasicBlock & llvmIrBasicBlock,
                 const std::map<llvm::Value *, std::pair<double, double>>& virtualRegisterRange,
                 std::map<Value*, Type*>& typeChangedInst)
{
    auto leftOperand = inInstruction->getOperand(0);
    auto rightOperand = inInstruction->getOperand(1);
    auto leftType = leftOperand->getType();
    auto rightType = rightOperand->getType();

    if (compareType(leftType, rightType) == 0)
        return;

    /*
     * If both are non-constant, roll back
     * */
    if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
    {
        if (isa<StoreInst>(inInstruction) && isa<Argument>(leftOperand)) {
            unsigned pointerAddr = rightType->getPointerAddressSpace();
            IRBuilder<> Builder(&llvmIrBasicBlock);
            Builder.SetInsertPoint(inInstruction);
            Value * castInst = Builder.CreateBitCast(rightOperand, leftType->getPointerTo(pointerAddr));
            inInstruction->replaceUsesOfWith(rightOperand, castInst);
            return;
        }

        /*
         * todo: can be further improved, by shrink both of them to greatest common type
         * */
        Type* backType = nullptr;
        if (typeChangedInst.find(leftOperand) != typeChangedInst.end())
        {
            /*
             * roll back left operand
             * */
            rollbackType(N, inInstruction, 0, llvmIrBasicBlock,
                         typeChangedInst);
        } else {
            backType = leftType;
        }
        if (typeChangedInst.find(rightOperand) != typeChangedInst.end())
        {
            /*
             * roll back right operand
             * */
            rollbackType(N, inInstruction, 1, llvmIrBasicBlock,
                         typeChangedInst, backType);
        } else {
            /*
             * Special for StoreInst, the leftOperand doesn't have '*' but the rightOperand have.
             * e.g. store i32 %45, i32* %47
             * */
            if (isa<StoreInst>(inInstruction) && (rightType->getTypeID() == Type::PointerTyID)) {
                rightType = rightType->getPointerElementType();
            }
            rollbackType(N, inInstruction, 0, llvmIrBasicBlock,
                         typeChangedInst, rightType);
        }
        return;
    }

    /*
     * If one of them is constant, shrink the constant operand
     * */
    auto rollBackConstantOperand = [N, inInstruction, &llvmIrBasicBlock, &typeChangedInst]
            (unsigned constOperandIdx, unsigned nonConstOperandIdx) {
        Value* constOperand = inInstruction->getOperand(constOperandIdx);
        Value* nonConstOperand = inInstruction->getOperand(nonConstOperandIdx);
        if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(constOperand))
        {
            /*
             * 	both "float" and "double" type can use "convertToDouble"
             */
            auto constValue = (constFp->getValueAPF()).convertToDouble();
            auto nonConstType = nonConstOperand->getType();
            std::map<Value*, std::pair<double, double>> constOperandRange =
                    {{constOperand, std::make_pair(constValue, constValue)}};
            typeInfo realType = getTypeInfo(N, constOperand, constOperandRange);
            if (compareType(realType.valueType, nonConstType) <= 0) {
                rollbackType(N, inInstruction, constOperandIdx, llvmIrBasicBlock,
                             typeChangedInst, nonConstType);
            } else {
                rollbackType(N, inInstruction, nonConstOperandIdx, llvmIrBasicBlock,
                             typeChangedInst);
            }
        } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(constOperand)) {
            auto constValue = constInt->getSExtValue();
            auto nonConstType = nonConstOperand->getType();
            std::map<Value*, std::pair<double, double>> constOperandRange =
                    {{constOperand, std::make_pair(constValue, constValue)}};
            typeInfo realType = getTypeInfo(N, constOperand, constOperandRange);
            if (compareType(realType.valueType, nonConstType) <= 0) {
                rollbackType(N, inInstruction, constOperandIdx, llvmIrBasicBlock,
                             typeChangedInst, nonConstType);
            } else {
                rollbackType(N, inInstruction, nonConstOperandIdx, llvmIrBasicBlock, typeChangedInst);
            }
        }

    };
    // todo: if it's a storeInst, like store double 5.000000e+00, double* %2
    //       the leftType should remove '*'
    if (isa<llvm::Constant>(leftOperand)) {
        /*
         * leftOperand is constant, rightOperand is non-constant
         * */
        rollBackConstantOperand(0, 1);
    } else if (isa<llvm::Constant>(rightOperand)) {
        /*
         * rightOperand is constant, leftOperand is non-constant
         * */
        rollBackConstantOperand(1, 0);
    }
}

/*
 * if dest type cannot change, roll back operands type
 * if dest type can change and its real type <= operands type, mutate dest type to operand type
 * else roll back operand type to real type of dest and mutate dest type to real type
 *
 * */
void
matchDestType(State * N, Instruction *inInstruction, BasicBlock & llvmIrBasicBlock,
              const std::map<llvm::Value *, std::pair<double, double>>& virtualRegisterRange,
              std::map<Value*, Type*>& typeChangedInst)
{
    typeInfo typeInformation;
    typeInformation.valueType = nullptr;
    typeInformation.signFlag = false;
    auto srcOperand = inInstruction->getOperand(0);
    auto srcType = srcOperand->getType();
    auto inInstType = inInstruction->getType();

    /*
     * Now the type of inInstruction hasn't changed, so we check if it can be further changed (has range information).
     * If not, roll back all operands
     * */
    typeInformation = getTypeInfo(N, inInstruction, virtualRegisterRange);
    if (typeInformation.valueType == nullptr) {
        if (compareType(srcType, inInstType) != 0) {
            for (size_t id = 0; id < inInstruction->getNumOperands(); id++) {
                rollbackType(N, inInstruction, id, llvmIrBasicBlock, typeChangedInst, inInstType);
            }
        }
        return;
    }

    if (compareType(typeInformation.valueType, srcType) <= 0) {
        /*
         * mutate dest type to srcType
         * */
        Type* tmpType = nullptr;
        if (isa<LoadInst>(inInstruction) && (srcType->getTypeID() == Type::PointerTyID)) {
            tmpType = srcType->getPointerElementType();
        } else {
            tmpType = srcType;
        }
        typeChangedInst.emplace(inInstruction, inInstruction->getType());
        inInstruction->mutateType(tmpType);
    } else {
        /*
         * roll back operands to typeInformation.valueType
         * */
        for (size_t id = 0; id < inInstruction->getNumOperands(); id++) {
            rollbackType(N, inInstruction, id, llvmIrBasicBlock, typeChangedInst,
                         typeInformation.valueType);
        }
        /*
         * mutate dest to typeInformation.valueType
         * */
        if (isa<LoadInst>(inInstruction) && (typeInformation.valueType->getTypeID() == Type::PointerTyID)) {
            typeInformation.valueType = typeInformation.valueType->getPointerElementType();
        }
        typeChangedInst.emplace(inInstruction, inInstruction->getType());
        inInstruction->mutateType(typeInformation.valueType);
    }
    return;
}

bool
shrinkInstructionType(State * N, Instruction *inInstruction, BasicBlock & llvmIrBasicBlock,
                      const std::map<llvm::Value *, std::pair<double, double>>& virtualRegisterRange,
                      std::map<Value*, Type*>& typeChangedInst)
{
    bool changed = false;
    typeInfo typeInformation = getTypeInfo(N, inInstruction, virtualRegisterRange);
    if (typeInformation.valueType == nullptr)
        return changed;

    if (isa<LoadInst>(inInstruction) && (typeInformation.valueType->getTypeID() == Type::PointerTyID)) {
        typeInformation.valueType = typeInformation.valueType->getPointerElementType();
    }

    changed = true;
    auto valueType = inInstruction->getType();
    IRBuilder<> Builder(&llvmIrBasicBlock);
    Builder.SetInsertPoint(inInstruction->getNextNode());
    auto tmp = inInstruction->clone();
    Value * castInst;
    if (typeInformation.valueType->isIntegerTy()) {
        castInst = Builder.CreateIntCast(tmp, typeInformation.valueType, typeInformation.signFlag);
    } else {
        castInst = Builder.CreateFPCast(tmp, typeInformation.valueType);
    }
    /*
     * it must be the first meet inst, so it's safe to RAUW
     * */
    inInstruction->replaceAllUsesWith(castInst);
    ReplaceInstWithInst(inInstruction, tmp);
    typeChangedInst.emplace(castInst, valueType);
    return changed;
}

bool
shrinkType(State *N, BoundInfo *boundInfo, Function &llvmIrFunction) {
    bool changed = false;
    /*
     * <Value to be shrink, old type>
     * */
    std::map<Value*, Type*> typeChangedInst;
    std::vector<AllocaInst*> allocaVec;
    for (BasicBlock &llvmIrBasicBlock: llvmIrFunction) {
        for (BasicBlock::iterator itBB = llvmIrBasicBlock.begin(); itBB != llvmIrBasicBlock.end();) {
            Instruction *llvmIrInstruction = &*itBB++;
            switch (llvmIrInstruction->getOpcode()) {
                case Instruction::Call:
                    if (auto llvmIrCallInstruction = dyn_cast<CallInst>(llvmIrInstruction)) {
                        Function *calledFunction = llvmIrCallInstruction->getCalledFunction();
                        if (calledFunction->getName().startswith("llvm.dbg.value") ||
                            calledFunction->getName().startswith("llvm.dbg.declare")) {
                            auto firstOperator = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(0));
                            auto localVariableAddressAsMetadata = cast<ValueAsMetadata>(firstOperator->getMetadata());
                            auto localVariableAddress = localVariableAddressAsMetadata->getValue();

                            // todo: don't know how to change the type of llvmIrCallInstruction->getOperand(1)
                            auto variableMetadata = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(1));
                            auto debugInfoVariable = cast<DIVariable>(variableMetadata->getMetadata());
                            auto variableType = debugInfoVariable->getRawType();
                            auto diDerivedType = dyn_cast<DIDerivedType>(variableType);

                            typeInfo realType = getTypeInfo(N, localVariableAddress, boundInfo->virtualRegisterRange);
                            if (realType.valueType != nullptr) {
                                typeChangedInst.emplace(localVariableAddress, localVariableAddress->getType());
                                localVariableAddress->mutateType(realType.valueType);
                                if (auto allocaInst = dyn_cast<AllocaInst>(localVariableAddress)) {
                                    auto itAV = std::find(allocaVec.begin(), allocaVec.end(), allocaInst);
                                    if (itAV != allocaVec.end()) {
                                        Type* allocateType;
                                        if (realType.valueType->getTypeID() == Type::PointerTyID) {
                                            allocateType = realType.valueType->getPointerElementType();
                                        } else {
                                            allocateType = realType.valueType;
                                        }
                                        (*itAV)->setAllocatedType(allocateType);
                                        typeChangedInst.emplace((*itAV), (*itAV)->getType());
                                        (*itAV)->mutateType(realType.valueType);
                                    }
                                }
                                /*visit this function from beginning*/
                                itBB = llvmIrBasicBlock.begin();
                            }
                        } else {
                            /*don't change the type of lib function call's params*/
                            if (!calledFunction || calledFunction->isDeclaration())
                            {
                                flexprint(N->Fe, N->Fm, N->Fperr,
                                          "\tCall: CalledFunction %s is nullptr or undeclared.\n",
                                          calledFunction->getName().str().c_str());
                                for (size_t idx = 0; idx < llvmIrCallInstruction->getNumOperands() - 1; idx++)
                                {
                                    auto tcInstIt = typeChangedInst.find(llvmIrCallInstruction->getOperand(idx));
                                    if (tcInstIt != typeChangedInst.end()) {
                                        rollbackType(N, llvmIrCallInstruction, idx, llvmIrBasicBlock,
                                                     typeChangedInst);
                                    }
                                }
                            } else {
                                for (size_t idx = 0; idx < llvmIrCallInstruction->getNumOperands() - 1; idx++) {
                                    typeInfo realOperandType = getTypeInfo(N, llvmIrCallInstruction->getOperand(idx),
                                                                        boundInfo->virtualRegisterRange);
                                    if (realOperandType.valueType != nullptr) {
                                        rollbackType(N, llvmIrCallInstruction, idx, llvmIrBasicBlock,
                                                     typeChangedInst, realOperandType.valueType);
                                    }
                                }
                            }
                        }
                    }
                case Instruction::Alloca:
                    /*
                     * change alloca type based on the metedate of call
                     * */
                    if (auto llvmIrAllocaInstruction = dyn_cast<AllocaInst>(llvmIrInstruction)) {
                        allocaVec.emplace_back(llvmIrAllocaInstruction);
                    }
                    break;
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
                case Instruction::Shl:
                case Instruction::LShr:
                case Instruction::AShr:
                case Instruction::And:
                case Instruction::Or:
                case Instruction::Xor:
                    /*
                     * For binary operator, check if two operands are of the same type
                     * */
                    matchOperandType(N, llvmIrInstruction, llvmIrBasicBlock,
                                     boundInfo->virtualRegisterRange, typeChangedInst);
                case Instruction::FNeg:
                case Instruction::Load:
                case Instruction::GetElementPtr:
                    matchDestType(N, llvmIrInstruction, llvmIrBasicBlock,
                                  boundInfo->virtualRegisterRange, typeChangedInst);

                case Instruction::FPToUI:
                case Instruction::FPToSI:
                case Instruction::SIToFP:
                case Instruction::UIToFP:
                case Instruction::ZExt:
                case Instruction::SExt:
                case Instruction::FPExt:
                case Instruction::Trunc:
                case Instruction::FPTrunc:
                case Instruction::BitCast:
                    /*
                     * only shrink the type because the computation of current operation
                     * */
                    changed = shrinkInstructionType(N, llvmIrInstruction, llvmIrBasicBlock,
                                                    boundInfo->virtualRegisterRange,
                                                    typeChangedInst);
                    break;
                /*
                 * the return type of storeInst is always void
                 * the return type of cmpInst is always i1
                 * */
                case Instruction::Store:
                case Instruction::ICmp:
                case Instruction::FCmp:
                    matchOperandType(N, llvmIrInstruction, llvmIrBasicBlock,
                                     boundInfo->virtualRegisterRange, typeChangedInst);
                    break;
                case Instruction::PHI:
                    matchPhiOperandType(N, llvmIrInstruction, llvmIrBasicBlock, typeChangedInst);
                    /*
                     * match dest:
                     * phi node is only a symbol and no real computation, so just mutate its type
                     * */
                    if (isa<PHINode>(llvmIrInstruction)) {
                        auto srcOperand = llvmIrInstruction->getOperand(0);
                        auto srcType = srcOperand->getType();
                        auto inInstType = llvmIrInstruction->getType();
                        if (compareType(inInstType, srcType) != 0) {
                            typeChangedInst.emplace(llvmIrInstruction, llvmIrInstruction->getType());
                            llvmIrInstruction->mutateType(srcType);
                        }
                    }
                    break;
                case Instruction::Br:
                case Instruction::Select:
                case Instruction::Switch:
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
                case Instruction::Ret:
                    break;
                default:
                    break;
            }
        }
    }
    return changed;
}

void
mergeCast(State *N, Function &llvmIrFunction) {
    /*
     * Merge the redundant cast instruction, prototype:
     *   %a = castInst type1 %x to type2 // sourceInst
     *   ... (several instructions that don't contain %a)
     *   %b = castInst type2 %a to type3 // destInst
     *   %c = someInst %b
     *   ============>
     *   %a = castInst type1 %x to type3
     *
     * If type1 is equal to type3, remove these two castInst, prototype:
     *   ============>
     *   %c = someInst %x
     * */
    std::vector<Value *> sourceInstVec;
    for (BasicBlock &llvmIrBasicBlock: llvmIrFunction) {
        for (BasicBlock::iterator itBB = llvmIrBasicBlock.begin(); itBB != llvmIrBasicBlock.end();) {
            Instruction *llvmIrInstruction = &*itBB++;
            switch (llvmIrInstruction->getOpcode()) {
                case Instruction::ZExt:
                case Instruction::SExt:
                case Instruction::FPExt:
                case Instruction::Trunc:
                case Instruction::FPTrunc:
                case Instruction::BitCast:
                {
                    if (isa<TruncInst>(llvmIrInstruction)) {
                        int a = 0;
                    }
                    auto sourceInst = llvm::dyn_cast<llvm::Instruction>(llvmIrInstruction->getOperand(0));
                    if (sourceInst != nullptr) {
                        auto siIt = std::find(sourceInstVec.begin(), sourceInstVec.end(), sourceInst);
                        if (siIt != sourceInstVec.end()) {
                            /*
                             * substitute destInst to sourceInst
                             * */
                            sourceInstVec.erase(siIt);
                            Instruction* sourceOperand = llvm::dyn_cast<llvm::Instruction>(sourceInst->getOperand(0));
                            IRBuilder<> Builder(&llvmIrBasicBlock);
                            Builder.SetInsertPoint(sourceInst->getNextNode());
                            if (compareType(sourceOperand->getType(), llvmIrInstruction->getType()) == 0) {
                                llvmIrInstruction->replaceAllUsesWith(sourceOperand);
                            } else {
                                Value * castInst;
                                if (llvmIrInstruction->getType()->isIntegerTy()) {
                                    castInst = Builder.CreateIntCast(sourceOperand, llvmIrInstruction->getType(),
                                                                     llvmIrInstruction->getOpcode() == Instruction::SExt);
                                } else {
                                    castInst = Builder.CreateFPCast(sourceOperand, llvmIrInstruction->getType());
                                }
                                Instruction * newCastInst = llvm::dyn_cast<llvm::Instruction>(castInst);
                                llvmIrInstruction->replaceAllUsesWith(newCastInst);
                                sourceInstVec.emplace_back(newCastInst);
                            }
                            sourceInst->removeFromParent();
                            llvmIrInstruction->removeFromParent();
                        } else {
                            sourceInstVec.emplace_back(llvmIrInstruction);
                        }
                    }
                    break;
                }
                case Instruction::Call:
                    if (auto llvmIrCallInstruction = dyn_cast<CallInst>(llvmIrInstruction)) {
                        Function * calledFunction = llvmIrCallInstruction->getCalledFunction();
                        if (calledFunction->getName().startswith("llvm.dbg.value") ||
                            calledFunction->getName().startswith("llvm.dbg.declare")) {
                            auto firstOperator = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(0));
                            auto localVariableAddressAsMetadata = cast<ValueAsMetadata>(firstOperator->getMetadata());
                            auto localVariableAddress = localVariableAddressAsMetadata->getValue();
                            auto siIt = std::find(sourceInstVec.begin(), sourceInstVec.end(),
                                                  localVariableAddress);
                            if (siIt != sourceInstVec.end()) {
                                sourceInstVec.erase(siIt);
                            }
                        } else {
                            for (size_t idx = 0; idx < llvmIrCallInstruction->getNumOperands() - 1; idx++)
                            {
                                auto siIt = std::find(sourceInstVec.begin(), sourceInstVec.end(),
                                                      llvmIrCallInstruction->getOperand(idx));
                                if (siIt != sourceInstVec.end()) {
                                    sourceInstVec.erase(siIt);
                                }
                            }
                        }
                        break;
                    }
                default:
                    for (size_t idx = 0; idx < llvmIrInstruction->getNumOperands(); idx++) {
                        auto siIt = std::find(sourceInstVec.begin(), sourceInstVec.end(),
                                              llvmIrInstruction->getOperand(idx));
                        if (siIt != sourceInstVec.end()) {
                            sourceInstVec.erase(siIt);
                        }
                    }
                    break;
            }
        }
    }
    return;
}

}