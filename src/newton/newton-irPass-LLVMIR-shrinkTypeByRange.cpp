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

/*
 * nullptr means cannot do shrinkType
 * else return the shrinkIntType
 * */
Type *
getShrinkIntType(State * N, Value *boundValue, const std::pair<double, double>& boundRange, bool& signFlag)
{
    /*
     * Signed-ness in LLVM is not stored into the integer, it is left to
     * interpretation to the instruction that use them.
     * */
    signFlag = boundRange.first < 0;

    varType finalType = getTypeEnum(boundRange.first, boundRange.second, signFlag);

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
        case 8:
            if (finalType == INT1)
            {
                newType = IntegerType::getInt1Ty(context);
            }
            break;
        default:
            flexprint(N->Fe, N->Fm, N->Fpinfo,
                      "\tgetShrinkIntType: Type::Integer, don't support such bit width yet.");
    }

    if (newType != nullptr) {
        newType = isPointer ? newType->getPointerTo(pointerAddr) : newType;
//        boundValue->mutateType(newType);
    }

    return newType;
}

bool
shrinkInstructionType(State * N, Instruction *inInstruction, BasicBlock & llvmIrBasicBlock,
                      const std::map<llvm::Value *, std::pair<double, double>>& virtualRegisterRange,
                      std::map<Value*, Type*>& typeChangedInst)
{
    bool changed = false;
    auto vrRangeIt = virtualRegisterRange.find(inInstruction);
    if (vrRangeIt == virtualRegisterRange.end())
    {
        return changed;
    }
    Type *newType = nullptr;
    bool signFlag;
    auto valueType = inInstruction->getType();
    auto valueTypeId = valueType->getTypeID() == Type::PointerTyID ?
                       valueType->getPointerElementType()->getTypeID() :
                       valueType->getTypeID();

    switch (valueTypeId) {
        case Type::IntegerTyID:
            newType = getShrinkIntType(N, inInstruction, vrRangeIt->second, signFlag);
            break;
        case Type::FloatTyID:
            break;
        case Type::DoubleTyID:
            break;
        default:
            break;
    }
    if (newType != nullptr)
    {
        if (isa<LoadInst>(inInstruction) || isa<GetElementPtrInst>(inInstruction)) {
            typeChangedInst.emplace(inInstruction, inInstruction->getType());
            inInstruction->mutateType(newType);
        } else {
            changed = true;
            IRBuilder<> Builder(&llvmIrBasicBlock);
            Builder.SetInsertPoint(inInstruction->getNextNode());
            auto tmp = inInstruction->clone();
            Value * intCast = Builder.CreateIntCast(tmp, newType, signFlag);
            inInstruction->replaceAllUsesWith(intCast);
            ReplaceInstWithInst(inInstruction, tmp);
            typeChangedInst.emplace(intCast, valueType);
        }
    }
    return changed;
}

void
rollbackType(State * N, Value *inValue, BasicBlock & llvmIrBasicBlock,
             std::map<Value*, Type*>& typeChangedInst)
{
    auto tcInstIt = typeChangedInst.find(inValue);
    if (tcInstIt == typeChangedInst.end())
        return;

    Type* previousType = tcInstIt->second;
    if (isa<Instruction>(inValue)) {
        Instruction *inInstruction = cast<Instruction>(inValue);
        IRBuilder<> Builder(&llvmIrBasicBlock);
        Builder.SetInsertPoint(inInstruction->getNextNode());
        auto tmp = inInstruction->clone();
        Value * intCast = Builder.CreateIntCast(tmp, previousType, false);
        inInstruction->replaceAllUsesWith(intCast);
        ReplaceInstWithInst(inInstruction, tmp);
    } else if (isa<llvm::Constant>(inValue)) {
        inValue->mutateType(previousType);
    } else {
        assert(false);
    }
    typeChangedInst.erase(inValue);
}

/*
 * first > second: greater than 0,
 * first = second: equal to 0,
 * first < second: less than 0,
 * */
int compareType(Type* firstType, Type* secondType)
{
    int compare = 0;
    if (firstType->getTypeID() == secondType->getTypeID()) {
        if (firstType->getTypeID() == Type::IntegerTyID) {
            compare = firstType->getIntegerBitWidth() - secondType->getIntegerBitWidth();
        } else if (firstType->getTypeID() == Type::PointerTyID) {
            auto firstEleType = firstType->getPointerElementType();
            auto secondEleType = secondType->getPointerElementType();
            compare = compareType(firstEleType, secondEleType);
        } else if (firstType->getTypeID() == Type::ArrayTyID) {
            auto firstEleType = firstType->getArrayElementType();
            auto secondEleType = secondType->getArrayElementType();
            compare = compareType(firstEleType, secondEleType);
        } else {
            compare = 0;
        }
    } else {
        compare = firstType->getTypeID() - secondType->getTypeID();
    }
    return compare;
}

void
matchOperandType(State * N, Instruction *inInstruction, BasicBlock & llvmIrBasicBlock,
                 std::map<Value*, Type*>& typeChangedInst)
{
    auto leftOperand = inInstruction->getOperand(0);
    auto rightOperand = inInstruction->getOperand(1);
    auto leftType = leftOperand->getType();
    auto rightType = rightOperand->getType();
    if (isa<StoreInst>(inInstruction))
        rightType = rightType->getPointerElementType();

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
            rollbackType(N, leftOperand, llvmIrBasicBlock, typeChangedInst);
        }
        if (typeChangedInst.find(rightOperand) != typeChangedInst.end())
        {
            // roll back
            rollbackType(N, rightOperand, llvmIrBasicBlock, typeChangedInst);
        }
        return;
    }

    /*
     * If one of them is constant, shrink the constant operand
     * */
    if (isa<llvm::Constant>(leftOperand)) {
        typeChangedInst.emplace(leftOperand, leftOperand->getType());
        leftOperand->mutateType(rightType);
    } else if (isa<llvm::Constant>(rightOperand)) {
        typeChangedInst.emplace(rightOperand, rightOperand->getType());
        rightOperand->mutateType(leftType);
    }
}

/*
 * return true if run after this function, including originally match or match after roll back
 * return false means need further shrink this instruction
 * */
bool
matchDestType(State * N, Instruction *inInstruction, BasicBlock & llvmIrBasicBlock,
              const std::map<llvm::Value *, std::pair<double, double>>& virtualRegisterRange,
              std::map<Value*, Type*>& typeChangedInst)
{
    auto srcOperand = inInstruction->getOperand(0);
    auto srcType = srcOperand->getType();
    auto inInstType = inInstruction->getType();

    if (isa<LoadInst>(inInstruction))
        srcType = srcType->getPointerElementType();
    if (isa<GetElementPtrInst>(inInstruction)) {
        /*
         * extract raw type from array
         * */
        if (srcType->getTypeID() == Type::PointerTyID) {
            auto pointerAddr = srcType->getPointerAddressSpace();
            auto pointerEleType = srcType->getPointerElementType();
            auto arrayType = dyn_cast<ArrayType>(pointerEleType);
            if (arrayType != nullptr) {
                auto arrayEleType = arrayType->getElementType();
                srcType = arrayEleType->getPointerTo(pointerAddr);
            }
        }
    }

    if (compareType(srcType, inInstType) == 0)
        return true;

    /*
     * Now the type of inInstruction hasn't changed, so we check if it will change in need
     * */
    auto vrRangeIt = virtualRegisterRange.find(inInstruction);
    if (vrRangeIt == virtualRegisterRange.end()) {
        for (size_t id = 0; id < inInstruction->getNumOperands(); id++) {
            rollbackType(N, inInstruction->getOperand(id), llvmIrBasicBlock, typeChangedInst);
        }
        return true;
    }

    bool signFlag;
    Type* realInstType = getShrinkIntType(N, inInstruction, vrRangeIt->second, signFlag);
    /*
     * todo: this can be further improve by deal with '>' and '<' further
     * */
    if ((realInstType == nullptr) || (compareType(srcType, realInstType) != 0)) {
        for (size_t id = 0; id < inInstruction->getNumOperands(); id++) {
            rollbackType(N, inInstruction->getOperand(id), llvmIrBasicBlock, typeChangedInst);
        }
        return true;
    } else {
        /*
         * next step will shrink the inInstruction to the realInstType, so don't need roll back
         * */
        return false;
    }
}

bool
shrinkType(State *N, BoundInfo *boundInfo, Function &llvmIrFunction) {
    bool changed = false;
    /*
     * <Value to be shrink, old type>
     * */
    std::map<Value*, Type*> typeChangedInst;
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
                case Instruction::Store:
                    /*
                     * For binary operator, check if two operands are of the same type
                     * */
                    matchOperandType(N, llvmIrInstruction, llvmIrBasicBlock, typeChangedInst);
                case Instruction::FNeg:
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
                    changed = shrinkInstructionType(N, llvmIrInstruction, llvmIrBasicBlock,
                                                    boundInfo->virtualRegisterRange, typeChangedInst);
                    break;
                case Instruction::ICmp:
                case Instruction::FCmp:
                    matchOperandType(N, llvmIrInstruction, llvmIrBasicBlock, typeChangedInst);
                    break;
                case Instruction::Load:
                case Instruction::GetElementPtr:
                    /*
                     * Need further check the result type of GEP
                     * */
                    if (!matchDestType(N, llvmIrInstruction, llvmIrBasicBlock,
                                       boundInfo->virtualRegisterRange, typeChangedInst))
                        changed = shrinkInstructionType(N, llvmIrInstruction, llvmIrBasicBlock,
                                                        boundInfo->virtualRegisterRange, typeChangedInst);
                    break;
                case Instruction::PHI:
                    matchOperandType(N, llvmIrInstruction, llvmIrBasicBlock, typeChangedInst);
                    if (!matchDestType(N, llvmIrInstruction, llvmIrBasicBlock,
                                       boundInfo->virtualRegisterRange, typeChangedInst))
                        changed = shrinkInstructionType(N, llvmIrInstruction, llvmIrBasicBlock,
                                                        boundInfo->virtualRegisterRange, typeChangedInst);
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