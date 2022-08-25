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
    INT8        = 1,
    INT16       = 2,
    INT32       = 3,
    INT64       = 4,
    FLOAT       = 5,
    DOUBLE      = 6,
    UNCHANGED   = 7
};

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

void
shrinkType(State * N, Instruction *inInstruction, BasicBlock & llvmIrBasicBlock, const std::pair<double, double>& boundRange)
{
    Type *newType = nullptr;
    bool signFlag;
    auto valueType = inInstruction->getType();
    auto valueTypeId = valueType->getTypeID() == Type::PointerTyID ?
            valueType->getPointerElementType()->getTypeID() :
            valueType->getTypeID();

    switch (valueTypeId) {
        case Type::IntegerTyID:
            newType = shrinkIntType(N, inInstruction, boundRange, signFlag);
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
        IRBuilder<> Builder(&llvmIrBasicBlock);
        auto bbIt = inInstruction->getIterator();
        bbIt++;
        Builder.SetInsertPoint(inInstruction->getNextNode());
        auto tmp = inInstruction->clone();
//        llvmIrBasicBlock.getInstList().insertAfter(inInstruction->getIterator(), tmp);
        Value * intCast = Builder.CreateIntCast(tmp, newType, signFlag);
        inInstruction->replaceAllUsesWith(intCast);
//        inInstruction->removeFromParent();
        ReplaceInstWithInst(inInstruction, tmp);
    }
}

}