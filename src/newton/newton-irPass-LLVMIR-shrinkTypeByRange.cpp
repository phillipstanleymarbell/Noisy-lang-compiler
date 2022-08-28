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
    UNCHANGED   = 8
};

varType
getTypeEnum(double min, double max, bool signFlag)
{
    varType finalType = UNCHANGED;
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
        finalType = UNCHANGED;
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

    varType finalType = getTypeEnum(boundRange.first, boundRange.second, typeInformation.signFlag);

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
        typeInformation.valueType = isPointer ? typeInformation.valueType->getPointerTo(pointerAddr) : typeInformation.valueType;
//        boundValue->mutateType(typeInformation.valueType);
    }

    return typeInformation;
}

Type*
rollbackType(State * N, Instruction *inInstruction, Value *inValue, BasicBlock & llvmIrBasicBlock,
             std::map<Value*, Type*>& typeChangedInst, Type* backType = nullptr)
{
    if (isa<GetElementPtrInst>(inInstruction) && isa<llvm::Constant>(inValue)) {
        return backType;
    }
    if (backType == nullptr) {
        auto tcInstIt = typeChangedInst.find(inValue);
        assert(tcInstIt != typeChangedInst.end() &&
                "backType is nullptr and cannot find it in typeChangedInst");
        backType = tcInstIt->second;
    }

    if (isa<Instruction>(inValue)) {
        /*
         * if it's an instruction that need to roll back, insert a cast
         * */
        Instruction *valueInst = cast<Instruction>(inValue);
        IRBuilder<> Builder(&llvmIrBasicBlock);
        Builder.SetInsertPoint(inInstruction);
        Value * intCast = Builder.CreateIntCast(valueInst, backType, false);
        inInstruction->replaceUsesOfWith(valueInst, intCast);
        int a = 0;
    } else if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(inValue)) {
        auto constValue = (constFp->getValueAPF()).convertToDouble();
        auto newConstant = ConstantFP::get(backType, constValue);
        inInstruction->replaceUsesOfWith(inValue, newConstant);
    } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(inValue)) {
        auto constValue = constInt->getSExtValue();
        auto newConstant = ConstantInt::get(backType, constValue);
        inInstruction->replaceUsesOfWith(inValue, newConstant);
    } else {
        assert(false);
    }
    return backType;
}

/*
 * first > second: greater than 0,
 * first = second: equal to 0,
 * first < second: less than 0,
 * */
int compareType(Type* firstType, Type* secondType)
{
    auto typeEnumConvert = [](Type* inputType) {
        /*
         * extract raw type from pointer or array
         * */
        Type* eleType;
        if (inputType->getTypeID() == Type::PointerTyID) {
            auto pointerAddr = inputType->getPointerAddressSpace();
            eleType = inputType->getPointerElementType();
            auto arrayType = dyn_cast<ArrayType>(eleType);
            if (arrayType != nullptr) {
                eleType = arrayType->getElementType();
            }
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
                    default:
                        assert(false && "unsupported");
                }
            default:
                assert(false && "unsupported");
        }
    };
    return typeEnumConvert(firstType) - typeEnumConvert(secondType);
}

void
matchOperandType(State * N, Instruction *inInstruction, BasicBlock & llvmIrBasicBlock,
                 std::map<Value*, Type*>& typeChangedInst)
{
    auto leftOperand = inInstruction->getOperand(0);
    auto rightOperand = inInstruction->getOperand(1);
    auto leftType = leftOperand->getType();
    auto rightType = rightOperand->getType();
    // debug
    if (isa<ShlOperator>(inInstruction)) {
        int a = 0;
    }

    if (compareType(leftType, rightType) == 0)
        return;

    /*
     * If both are non-constant, roll back
     * */
    if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
    {
        /*
         * todo: can be further improved, by shrink both of them to a smaller type
         * */
        if (typeChangedInst.find(leftOperand) != typeChangedInst.end())
        {
            // roll back
            rollbackType(N, inInstruction, leftOperand, llvmIrBasicBlock, typeChangedInst);
        }
        if (typeChangedInst.find(rightOperand) != typeChangedInst.end())
        {
            // roll back
            rollbackType(N, inInstruction, rightOperand, llvmIrBasicBlock, typeChangedInst);
        }
        return;
    }

    /*
     * If one of them is constant, shrink the constant operand
     * */
    auto rollBackConstantOperand = [N, inInstruction, &llvmIrBasicBlock, &typeChangedInst]
            (Value* constOperand, Value* nonConstOperand) {
        if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(constOperand))
        {
            /*
             * 	both "float" and "double" type can use "convertToDouble"
             */
            auto constValue = (constFp->getValueAPF()).convertToDouble();
            auto nonConstType = nonConstOperand->getType();
            typeInfo realType = getShrinkIntType(N, constOperand, std::make_pair(constValue, constValue));
            if (compareType(realType.valueType, nonConstType) <= 0) {
                rollbackType(N, inInstruction, constOperand, llvmIrBasicBlock,
                             typeChangedInst, nonConstType);
            } else {
                rollbackType(N, inInstruction, nonConstOperand, llvmIrBasicBlock,
                             typeChangedInst);
            }
        } else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(constOperand)) {
            auto constValue = constInt->getSExtValue();
            auto nonConstType = nonConstOperand->getType();
            typeInfo realType = getShrinkIntType(N, constOperand, std::make_pair(constValue, constValue));
            if (compareType(realType.valueType, nonConstType) <= 0) {
                rollbackType(N, inInstruction, constOperand, llvmIrBasicBlock,
                             typeChangedInst, nonConstType);
            } else {
                rollbackType(N, inInstruction, nonConstOperand, llvmIrBasicBlock, typeChangedInst);
            }
        }

    };
    if (isa<llvm::Constant>(leftOperand)) {
        rollBackConstantOperand(leftOperand, rightOperand);
    } else if (isa<llvm::Constant>(rightOperand)) {
        rollBackConstantOperand(rightOperand, leftOperand);
    }
}

typeInfo getTypeInfo(State * N, Instruction *inInstruction,
                     const std::map<llvm::Value *, std::pair<double, double>>& virtualRegisterRange) {
    typeInfo typeInformation;
    typeInformation.signFlag = false;
    typeInformation.valueType = nullptr;

    auto vrRangeIt = virtualRegisterRange.find(inInstruction);
    if (vrRangeIt == virtualRegisterRange.end()) {
        return typeInformation;
    }

    auto inInstType = inInstruction->getType();
    inInstType = inInstType->getTypeID() == Type::PointerTyID ?
                 inInstType->getPointerElementType() : inInstType;
    switch (inInstType->getTypeID()) {
        case Type::IntegerTyID:
            typeInformation = getShrinkIntType(N, inInstruction, vrRangeIt->second);
            break;
        case Type::FloatTyID:
            // todo
            break;
        case Type::DoubleTyID:
            // todo
            break;
        default:
            break;
    }
    return typeInformation;
}

/*
 * if dest type cannot change, roll back operands type
 * if dest type can change and its real type <= operands type, mutate dest type to operand type
 * else roll back operand type to real type of dest and mutate dest type to real type
 *
 * return the real type
 * */
typeInfo
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
    * phi node is only a symbol and no real computation, so just mutate its type
    * */
    if (isa<PHINode>(inInstruction)) {
        typeChangedInst.emplace(inInstruction, inInstruction->getType());
        inInstruction->mutateType(srcType);
        return typeInformation;
    }

    /*
     * Now the type of inInstruction hasn't changed, so we check if it can be further changed (has range information).
     * If not, roll back all operands
     * */
    typeInformation = getTypeInfo(N, inInstruction, virtualRegisterRange);
    if (typeInformation.valueType == nullptr) {
        if (compareType(srcType, inInstType) != 0) {
            Type* operandBackType = nullptr;
            for (size_t id = 0; id < inInstruction->getNumOperands(); id++) {
                operandBackType = rollbackType(N, inInstruction, inInstruction->getOperand(id), llvmIrBasicBlock,
                                               typeChangedInst, operandBackType);
            }
        }
        return typeInformation;
    }

    if (compareType(typeInformation.valueType, srcType) <= 0) {
        /*
         * mutate dest type to srcType
         * */
        typeChangedInst.emplace(inInstruction, inInstruction->getType());
        inInstruction->mutateType(srcType);
    } else {
        /*
         * roll back operands to typeInformation.valueType
         * */
        for (size_t id = 0; id < inInstruction->getNumOperands(); id++) {
            rollbackType(N, inInstruction, inInstruction->getOperand(id), llvmIrBasicBlock, typeChangedInst,
                         typeInformation.valueType);
        }
        /*
         * mutate dest to typeInformation.valueType
         * */
        typeChangedInst.emplace(inInstruction, inInstruction->getType());
        inInstruction->mutateType(typeInformation.valueType);
    }
    return typeInformation;
}

bool
shrinkInstructionType(State * N, Instruction *inInstruction, BasicBlock & llvmIrBasicBlock,
                      const std::map<llvm::Value *, std::pair<double, double>>& virtualRegisterRange,
                      std::map<Value*, Type*>& typeChangedInst, typeInfo typeInformation)
{
    bool changed = false;
    if (typeInformation.valueType == nullptr) {
        typeInformation = getTypeInfo(N, inInstruction, virtualRegisterRange);
        if (typeInformation.valueType == nullptr)
            return changed;
    }

    if (isa<LoadInst>(inInstruction) || isa<GetElementPtrInst>(inInstruction)) {
        typeChangedInst.emplace(inInstruction, inInstruction->getType());
        inInstruction->mutateType(typeInformation.valueType);
    } else {
        changed = true;
        auto valueType = inInstruction->getType();
        IRBuilder<> Builder(&llvmIrBasicBlock);
        Builder.SetInsertPoint(inInstruction->getNextNode());
        auto tmp = inInstruction->clone();
        Value * intCast = Builder.CreateIntCast(tmp, typeInformation.valueType, typeInformation.signFlag);
        inInstruction->replaceAllUsesWith(intCast);
        ReplaceInstWithInst(inInstruction, tmp);
        typeChangedInst.emplace(intCast, valueType);
    }
    return changed;
}

bool
shrinkType(State *N, BoundInfo *boundInfo, Function &llvmIrFunction) {
    bool changed = false;
    /*
     * <Value to be shrink, old type>
     * */
    std::map<Value*, Type*> typeChangedInst;
    typeInfo typeInformation;
    typeInformation.signFlag = false;
    typeInformation.valueType = nullptr;
    for (BasicBlock &llvmIrBasicBlock: llvmIrFunction) {
        for (BasicBlock::iterator itBB = llvmIrBasicBlock.begin(); itBB != llvmIrBasicBlock.end();) {
            Instruction *llvmIrInstruction = &*itBB++;
            switch (llvmIrInstruction->getOpcode()) {
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
                case Instruction::PHI:
                    /*
                     * For binary operator, check if two operands are of the same type
                     * */
                    matchOperandType(N, llvmIrInstruction, llvmIrBasicBlock, typeChangedInst);
                case Instruction::FNeg:
                case Instruction::Load:
                case Instruction::GetElementPtr:
                    /*
                     * Need further check the result type of GEP
                     * */
                    typeInformation = matchDestType(N, llvmIrInstruction, llvmIrBasicBlock,
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
                                                    typeChangedInst, typeInformation);
                    break;
                /*
                 * the return type of storeInst is always void
                 * the return type of cmpInst is always i1
                 * */
                case Instruction::Store:
                case Instruction::ICmp:
                case Instruction::FCmp:
                    matchOperandType(N, llvmIrInstruction, llvmIrBasicBlock, typeChangedInst);
                    break;
                case Instruction::Call:
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

}