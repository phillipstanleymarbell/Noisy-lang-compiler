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

/*
 * this macro can also move to the compiler config
 * */
#define UNSIGNED_SHRINK 1

using namespace llvm;

extern "C"
{

enum varType {
	INT1	= 1,
	INT8	= 2,
	INT16	= 3,
	INT32	= 4,
	INT64	= 5,
	FLOAT	= 6,
	DOUBLE	= 7,
	UNKNOWN = 8,
};

#ifdef UNSIGNED_SHRINK
varType
getIntegerTypeEnum(double min, double max, bool signFlag)
{
    varType finalType;
    if ((!signFlag && max < UINT8_MAX) || (signFlag && min > INT8_MIN && max < INT8_MAX))
    {
        finalType = INT8;
    }
    else if ((!signFlag && max < UINT16_MAX) || (signFlag && min > INT16_MIN && max < INT16_MAX))
    {
        finalType = INT16;
    }
    else if ((!signFlag && max < UINT32_MAX) || (signFlag && min > INT32_MIN && max < INT32_MAX))
    {
        finalType = INT32;
    }
    else if ((!signFlag && max < UINT64_MAX) || (signFlag && min > INT64_MIN && max < INT64_MAX))
    {
        finalType = INT64;
    }
    else
    {
        finalType = UNKNOWN;
    }
    return finalType;
}
#else
/*
 * get the possible minimum int type.
 * to simplify the problem, only keep signed type.
 * */
varType
getIntegerTypeEnum(double min, double max, bool signFlag)
{
	varType finalType;
	if ((signFlag && min > INT8_MIN && max < INT8_MAX))
	{
		finalType = INT8;
	}
	else if ((signFlag && min > INT16_MIN && max < INT16_MAX))
	{
		finalType = INT16;
	}
	else if ((signFlag && min > INT32_MIN && max < INT32_MAX))
	{
		finalType = INT32;
	}
	else if ((signFlag && min > INT64_MIN && max < INT64_MAX))
	{
		finalType = INT64;
	}
	else
	{
		finalType = UNKNOWN;
	}
	return finalType;
}
#endif

varType
getFloatingTypeEnum(double min, double max)
{
	varType finalType;
    if ((FLT_EPSILON < std::abs(min) && std::abs(min) < FLT_MAX) &&
        (FLT_EPSILON < std::abs(max) && std::abs(max) < FLT_MAX))
	{
		finalType = FLOAT;
	}
    else if ((DBL_EPSILON < std::abs(min) && std::abs(min) < DBL_MAX) &&
             (DBL_EPSILON < std::abs(max) && std::abs(max) < DBL_MAX))
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
	Type * valueType;
	bool   signFlag;
} typeInfo;

/*
 * nullptr means cannot do shrinkType
 * else return the shrinkIntType
 * */
typeInfo
getShrinkIntType(State * N, Value * boundValue, const std::pair<double, double> & boundRange)
{
	typeInfo typeInformation;
	typeInformation.valueType = nullptr;
	/*
	 * Signed-ness in LLVM is not stored into the integer, it is left to
	 * interpretation to the instruction that use them.
	 * */
	typeInformation.signFlag = boundRange.first < 0;

	varType finalType = getIntegerTypeEnum(boundRange.first, boundRange.second, typeInformation.signFlag);

	auto	 previousType = boundValue->getType();
	auto	 typeId	      = previousType->getTypeID();
	auto &	 context      = previousType->getContext();
	unsigned bitWidth;

	bool	 isPointer = false;
	unsigned pointerAddr;
	if (typeId == Type::PointerTyID)
	{
		typeId	    = previousType->getPointerElementType()->getTypeID();
		bitWidth    = previousType->getPointerElementType()->getIntegerBitWidth();
		isPointer   = true;
		pointerAddr = previousType->getPointerAddressSpace();
	}
	else
	{
		bitWidth = previousType->getIntegerBitWidth();
	}
	assert(typeId == Type::IntegerTyID);

	switch (bitWidth)
	{
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
				  "\tgetShrinkIntType: Type::Integer, don't support such bit width yet.\n");
	}

	if (typeInformation.valueType != nullptr)
	{
		typeInformation.valueType = isPointer ? typeInformation.valueType->getPointerTo(pointerAddr) : typeInformation.valueType;
	}

	return typeInformation;
}

typeInfo
getShrinkDoubleType(State * N, Value * boundValue, const std::pair<double, double> & boundRange)
{
	typeInfo typeInformation;
	typeInformation.valueType = nullptr;
	typeInformation.signFlag  = boundRange.first < 0;

	varType finalType = getFloatingTypeEnum(boundRange.first, boundRange.second);

	auto   previousType = boundValue->getType();
	auto   typeId	    = previousType->getTypeID();
	auto & context	    = previousType->getContext();

	bool	 isPointer = false;
	unsigned pointerAddr;
	if (typeId == Type::PointerTyID)
	{
		typeId	    = previousType->getPointerElementType()->getTypeID();
		isPointer   = true;
		pointerAddr = previousType->getPointerAddressSpace();
	}
	assert(typeId == Type::DoubleTyID);

	if (finalType == FLOAT)
	{
		typeInformation.valueType = Type::getFloatTy(context);
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\tgetShrinkDoubleType: Unsupported type!\n");
	}

	if (typeInformation.valueType != nullptr)
	{
		typeInformation.valueType = isPointer ? typeInformation.valueType->getPointerTo(pointerAddr) : typeInformation.valueType;
	}

	return typeInformation;
}

typeInfo
getTypeInfo(State * N, Value * inValue,
	    const std::map<llvm::Value *, std::pair<double, double>> & virtualRegisterRange)
{
	typeInfo typeInformation;
	typeInformation.signFlag  = true;
	typeInformation.valueType = nullptr;

	auto vrRangeIt = virtualRegisterRange.find(inValue);
	if (vrRangeIt == virtualRegisterRange.end())
	{
		return typeInformation;
	}

	auto inInstType = inValue->getType();
	inInstType	= inInstType->getTypeID() == Type::PointerTyID ? inInstType->getPointerElementType() : inInstType;
	switch (inInstType->getTypeID())
	{
		case Type::IntegerTyID:
			typeInformation = getShrinkIntType(N, inValue, vrRangeIt->second);
			break;
		case Type::FloatTyID:
			break;
		case Type::DoubleTyID:
//			typeInformation = getShrinkDoubleType(N, inValue, vrRangeIt->second);
			break;
		default:
			break;
	}
	return typeInformation;
}
/*
 * extract raw type from pointer or array
 * */
Type *
getRawType(Type * inputType, std::vector<Value *> indexValue = std::vector<Value *>(),
	   unsigned * opId = nullptr)
{
	if (opId != nullptr)
	{
		(*opId)++;
	}
	Type * eleType = nullptr;
	if (inputType->getTypeID() == Type::PointerTyID)
	{
		eleType = inputType->getPointerElementType();
		return getRawType(eleType, indexValue, opId);
	}
	else if (inputType->getTypeID() == Type::ArrayTyID)
	{
		eleType = inputType->getArrayElementType();
		return getRawType(eleType, indexValue, opId);
	}
	else if (inputType->getTypeID() == Type::StructTyID)
	{
		/*
		 * special case:
		 *  %12 = getelementptr inbounds %struct.sincos_t, %struct.sincos_t* %2, i32 0, i32 6
		 *
		 * */
		auto stType = cast<StructType>(inputType);
		if (opId == nullptr)
		{
			/*
			 * need further check: this might should be removed.
			 * */
            if (0 != stType->getNumContainedTypes()) {
                eleType = stType->getContainedType(0);
            } else {
                return stType;
            }
		}
		else
		{
			eleType = stType->getTypeAtIndex(indexValue[*opId]);
		}
		return getRawType(eleType, indexValue, opId);
	}
	else
	{
		eleType = inputType;
	}
	assert(eleType != nullptr && "get raw type failed or input type is nullptr");
	return eleType;
}

/*
 * first > second: greater than 0,
 * first = second: equal to 0,
 * first < second: less than 0,
 * */
int
compareType(Type * firstType, Type * secondType)
{
	auto typeEnumConvert = [&](Type * inputType) -> varType {
		Type *	 eleType = getRawType(inputType);
		uint64_t intBitWidth;
		switch (eleType->getTypeID())
		{
			case Type::FloatTyID:
				return varType::FLOAT;
			case Type::DoubleTyID:
				return varType::DOUBLE;
			case Type::IntegerTyID:
				intBitWidth = eleType->getIntegerBitWidth();
				switch (intBitWidth)
				{
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

/*
 * return the instruction after roll back
 * */
Value *
rollbackType(State * N, Instruction * inInstruction, unsigned operandIdx, BasicBlock & llvmIrBasicBlock,
	     std::map<Value *, typeInfo> & typeChangedInst, typeInfo backType = typeInfo())
{
	Value * inValue	 = inInstruction->getOperand(operandIdx);
	Value * newValue = nullptr;
	if (isa<GetElementPtrInst>(inInstruction) && isa<llvm::Constant>(inValue))
	{
		return newValue;
	}
	/*
	 * check if the backType is specified by caller,
	 *  if so, use the specified backType
	 *  else, use the original backType
	 *
	 * */
	bool specifiedBackType = true;
	if (backType.valueType == nullptr)
	{
		specifiedBackType = false;
		auto tcInstIt	  = typeChangedInst.find(inValue);
		assert(tcInstIt != typeChangedInst.end() &&
		       "backType is nullptr and cannot find it in typeChangedInst");
		backType = tcInstIt->second;
		assert(backType.valueType != nullptr && "backType cannot be nullptr");
	}
	if (compareType(backType.valueType, inValue->getType()) == 0)
	{
		return newValue;
	}

	if (Instruction * valueInst = llvm::dyn_cast<llvm::Instruction>(inValue))
	{
		std::function<Type *(Value * inValue)> findOriginalType;
		findOriginalType = [&](Value * inValue) -> Type * {
			auto tcIt = typeChangedInst.find(inValue);
			if (tcIt == typeChangedInst.end())
			{
				return inValue->getType();
			}
			else
			{
				if (Instruction * castValueInst = llvm::dyn_cast<llvm::CastInst>(inValue))
				{
					/*
					 * castInst
					 * */
					return findOriginalType(castValueInst->getOperand(0));
				}
				else
				{
					/*
					 * constant
					 * */
					return tcIt->second.valueType;
				}
			}
		};
		typeInfo instPrevTypeInfo;
		instPrevTypeInfo.signFlag  = backType.signFlag;
		instPrevTypeInfo.valueType = specifiedBackType ? backType.valueType : findOriginalType(valueInst);
		/*
		 * store the previous value type
		 * if it's an instruction that need to roll back, insert a cast
		 * */
		if (PHINode * inInstPhi = llvm::dyn_cast<llvm::PHINode>(inInstruction))
		{
			/*
			 * if it's a phi node, insert the cast instruction in the incoming basic block
			 * */
			auto	    incomingBB = inInstPhi->getIncomingBlock(operandIdx);
			IRBuilder<> Builder(incomingBB);
			auto	    terminatorInst = incomingBB->getTerminator();
			Builder.SetInsertPoint(terminatorInst);
			if (instPrevTypeInfo.valueType->isIntegerTy())
			{
				newValue = Builder.CreateIntCast(valueInst, instPrevTypeInfo.valueType, instPrevTypeInfo.signFlag);
			}
			else if (instPrevTypeInfo.valueType->isDoubleTy())
			{
				newValue = Builder.CreateFPCast(valueInst, instPrevTypeInfo.valueType);
			}
            else
            {
                newValue = Builder.CreateBitCast(valueInst, instPrevTypeInfo.valueType);
            }
			inInstruction->setOperand(operandIdx, newValue);
			typeChangedInst.emplace(newValue, instPrevTypeInfo);
		}
		else
		{
			IRBuilder<> Builder(&llvmIrBasicBlock);
			Builder.SetInsertPoint(inInstruction);
			if (instPrevTypeInfo.valueType->isIntegerTy())
			{
				newValue = Builder.CreateIntCast(valueInst, instPrevTypeInfo.valueType, instPrevTypeInfo.signFlag);
			}
			else if (instPrevTypeInfo.valueType->isDoubleTy())
			{
				newValue = Builder.CreateFPCast(valueInst, instPrevTypeInfo.valueType);
			}
            else
            {
                newValue = Builder.CreateBitCast(valueInst, instPrevTypeInfo.valueType);
            }
			inInstruction->replaceUsesOfWith(valueInst, newValue);
			typeChangedInst.emplace(newValue, instPrevTypeInfo);
		}
	}
	else if (isa<llvm::Constant>(inValue))
	{
		/*
		 * Store the previous constant type.
		 * If
		 * */
		auto	 tcIt = typeChangedInst.find(inValue);
		typeInfo constTypeInfo;
		constTypeInfo.signFlag = backType.signFlag;
		if (tcIt == typeChangedInst.end())
		{
			constTypeInfo.valueType = inValue->getType();
		}
		else
		{
			constTypeInfo.valueType = tcIt->second.valueType;
		}

		if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(inValue))
		{
			if (inValue->getType()->isFloatTy())
			{
				float constValue = constFp->getValueAPF().convertToFloat();
				newValue	 = ConstantFP::get(backType.valueType, constValue);
			}
			else if (inValue->getType()->isDoubleTy())
			{
				double constValue = constFp->getValueAPF().convertToDouble();
				newValue	  = ConstantFP::get(backType.valueType, constValue);
			}
			else
			{
				assert(false && "unknown floating type");
			}
			inInstruction->replaceUsesOfWith(inValue, newValue);
			typeChangedInst.emplace(newValue, constTypeInfo);
		}
		else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(inValue))
		{
			if (backType.signFlag)
			{
				auto constValue = constInt->getSExtValue();
				newValue	= ConstantInt::get(backType.valueType, constValue, true);
			}
			else
			{
				auto constValue = constInt->getZExtValue();
				newValue	= ConstantInt::get(backType.valueType, constValue, false);
			}
			inInstruction->replaceUsesOfWith(inValue, newValue);
			typeChangedInst.emplace(newValue, constTypeInfo);
		}
		else
		{
			assert(false && "implement when meet");
		}
	}
	else if (isa<UndefValue>(inValue))
	{
		// do nothing
	}
	else
	{
		assert(false);
	}
	return newValue;
}

bool
isSignedValue(Value * inValue)
{
    // todo: get the sign bit from type system
	bool signFlag = true;
	if (Instruction * valueInst = llvm::dyn_cast<llvm::Instruction>(inValue))
	{
		signFlag = valueInst->getOpcode() != Instruction::ZExt;
	}
	return signFlag;
}

void
matchPhiOperandType(State * N, Instruction * inInstruction, BasicBlock & llvmIrBasicBlock,
		    std::map<llvm::Value *, std::pair<double, double>> & virtualRegisterRange,
		    std::map<Value *, typeInfo> &			 typeChangedInst)
{
	std::vector<Value *> operands;
	for (size_t id = 0; id < inInstruction->getNumOperands(); id++)
	{
		operands.emplace_back(inInstruction->getOperand(id));
	}
	if (std::all_of(operands.begin(), operands.end(), [&](Value * const & v) {
		    return compareType(v->getType(), operands.front()->getType()) == 0;
	    }))
		return;

	/*
	 * If both are non-constant, roll back
	 * todo: can be optimized by roll back to the largest type
	 * */
	if (std::all_of(operands.begin(), operands.end(), [&](Value * const & v) {
		    return !isa<llvm::Constant>(v);
	    }))
	{
		typeInfo backType;
		backType.valueType = nullptr;
		backType.signFlag  = true;
		bool noPrevType	   = false;
		for (size_t id = 0; id < operands.size(); id++)
		{
			if (typeChangedInst.find(operands[id]) != typeChangedInst.end())
			{
				auto newTypeValue = rollbackType(N, inInstruction, id, llvmIrBasicBlock,
								 typeChangedInst);
				auto vrIt	  = virtualRegisterRange.find(inInstruction->getOperand(id));
				if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
				{
					virtualRegisterRange.emplace(newTypeValue, vrIt->second);
				}
			}
			else
			{
				/*
				 * if current op didn't have previous type, reset all ops into this type
				 * */
				backType.valueType = operands[id]->getType();
				backType.signFlag  = isSignedValue(operands[id]);
				noPrevType	   = true;
				break;
			}
		}
		if (noPrevType)
		{
			assert(backType.valueType != nullptr && "backType is not set");
			for (size_t id = 0; id < operands.size(); id++)
			{
				auto newTypeValue = rollbackType(N, inInstruction, id, llvmIrBasicBlock,
								 typeChangedInst, backType);
				auto vrIt	  = virtualRegisterRange.find(inInstruction->getOperand(id));
				if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
				{
					virtualRegisterRange.emplace(newTypeValue, vrIt->second);
				}
			}
		}
	}
	else
	{
		/*
		 * If one of them is constant, shrink the constant operand
		 * */
		auto opIt = std::find_if(operands.begin(), operands.end(), [&](Value * const & v) {
			return isa<llvm::Constant>(v);
		});
		assert(opIt != operands.end() && "didn't find constant operand");
		Type *	 constantType = (*opIt)->getType();
		typeInfo backType;
		backType.valueType = constantType;
		backType.signFlag  = isSignedValue(*opIt);
		for (size_t id = 0; id < operands.size(); id++)
		{
			auto newTypeValue = rollbackType(N, inInstruction, id, llvmIrBasicBlock,
							 typeChangedInst, backType);
			auto vrIt	  = virtualRegisterRange.find(inInstruction->getOperand(id));
			if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
			{
				virtualRegisterRange.emplace(newTypeValue, vrIt->second);
			}
		}
	}
}

/*
 * Make sure the operands have the same type
 * */
void
matchOperandType(State * N, Instruction * inInstruction, BasicBlock & llvmIrBasicBlock,
		 std::map<llvm::Value *, std::pair<double, double>> & virtualRegisterRange,
		 std::map<Value *, typeInfo> &			      typeChangedInst)
{
	auto leftOperand  = inInstruction->getOperand(0);
	auto rightOperand = inInstruction->getOperand(1);
	auto leftType	  = leftOperand->getType();
	auto rightType	  = rightOperand->getType();

	if (compareType(leftType, rightType) == 0)
		return;

	/*
	 * aimType: is the 'backType' of 'rollbackType', which means the type that need to change to
	 * siblingType: is the type of sibling operand
	 * */
	auto changeStoreInstSiblingType = [](Type * aimType, Type * siblingType) {
		/*
		 * Special for StoreInst, the leftOperand doesn't have '*' but the rightOperand have.
		 * e.g. store i32 %45, i32* %47
		 * */
		Type * retType = nullptr;
		if (aimType->getTypeID() != Type::PointerTyID)
		{
			unsigned pointerAddr = siblingType->getPointerAddressSpace();
			retType		     = aimType->getPointerTo(pointerAddr);
		}
		else if (aimType->getTypeID() == Type::PointerTyID)
		{
			retType = aimType->getPointerElementType();
		}
		else
		{
			assert(false && "what happened in matchOperandType?");
		}
		return retType;
	};
	/*
	 * If both are non-constant, roll back
	 * */
	if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
	{
		if (isa<StoreInst>(inInstruction) && isa<Argument>(leftOperand))
		{
			/*
			 * Don't change the type of argument
			 * */
			unsigned    pointerAddr = rightType->getPointerAddressSpace();
			IRBuilder<> Builder(&llvmIrBasicBlock);
			Builder.SetInsertPoint(inInstruction);
			Value * castInstValue = Builder.CreateBitCast(rightOperand, leftType->getPointerTo(pointerAddr));
			auto	allocaValue   = rightOperand;
			rightOperand->replaceAllUsesWith(castInstValue);
			auto castInst = llvm::dyn_cast<llvm::Instruction>(castInstValue);
			castInst->replaceUsesOfWith(castInst->getOperand(0), allocaValue);
			auto vrIt = virtualRegisterRange.find(allocaValue);
			if (castInstValue != nullptr && vrIt != virtualRegisterRange.end())
			{
				virtualRegisterRange.emplace(castInstValue, vrIt->second);
			}
			return;
		}

		typeInfo realLeftType  = getTypeInfo(N, leftOperand, virtualRegisterRange);
		typeInfo realRightType = getTypeInfo(N, rightOperand, virtualRegisterRange);
		typeInfo backType{nullptr, true};
		if (compareType(leftType, rightType) < 0)
		{
			backType.signFlag  = realLeftType.signFlag;
			backType.valueType = rightType;
			if (isa<StoreInst>(inInstruction))
			{
				backType.valueType = changeStoreInstSiblingType(backType.valueType, leftType);
			}
			auto newTypeValue = rollbackType(N, inInstruction, 0, llvmIrBasicBlock, typeChangedInst, backType);
			auto vrIt	  = virtualRegisterRange.find(inInstruction->getOperand(0));
			if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
			{
				virtualRegisterRange.emplace(newTypeValue, vrIt->second);
			}
		}
		else if (compareType(leftType, rightType) > 0)
		{
			backType.signFlag  = realRightType.signFlag;
			backType.valueType = leftType;
			if (isa<StoreInst>(inInstruction))
			{
				backType.valueType = changeStoreInstSiblingType(backType.valueType, rightType);
			}
			auto newTypeValue = rollbackType(N, inInstruction, 1, llvmIrBasicBlock, typeChangedInst, backType);
			auto vrIt	  = virtualRegisterRange.find(inInstruction->getOperand(1));
			if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
			{
				virtualRegisterRange.emplace(newTypeValue, vrIt->second);
			}
		}
		return;
	}

	/*
	 * If one of them is constant, shrink the constant operand
	 * */
	auto rollBackConstantOperand = [N, inInstruction, &llvmIrBasicBlock,
					&virtualRegisterRange, &typeChangedInst,
					changeStoreInstSiblingType](unsigned constOperandIdx, unsigned nonConstOperandIdx) {
		Value * constOperand	= inInstruction->getOperand(constOperandIdx);
		Value * nonConstOperand = inInstruction->getOperand(nonConstOperandIdx);
		auto	constType	= constOperand->getType();
		auto	nonConstType	= nonConstOperand->getType();
        bool nonConstSign = true;
        auto tcIt = typeChangedInst.find(nonConstOperand);
        if (tcIt != typeChangedInst.end()) {
            nonConstSign = tcIt->second.signFlag;
        }
		if (!isa<llvm::ConstantData>(constOperand))
		{
			/*
			 * rollback the non-constant operand
			 *
			 * */
			typeInfo backType;
			backType.valueType = constType;
			backType.signFlag  = nonConstSign;
			if (isa<StoreInst>(inInstruction))
			{
				backType.valueType = changeStoreInstSiblingType(backType.valueType, nonConstType);
			}
			auto newTypeValue = rollbackType(N, inInstruction, nonConstOperandIdx, llvmIrBasicBlock,
							 typeChangedInst, backType);
			auto vrIt	  = virtualRegisterRange.find(inInstruction->getOperand(nonConstOperandIdx));
			if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
			{
				virtualRegisterRange.emplace(newTypeValue, vrIt->second);
			}
			return;
		}

		if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(constOperand))
		{
			double constValue;
			if (constFp->getType()->isFloatTy())
			{
				constValue = constFp->getValueAPF().convertToFloat();
			}
			else if (constFp->getType()->isDoubleTy())
			{
				constValue = constFp->getValueAPF().convertToDouble();
			}
			else
			{
				assert(false && "unknown floating type");
			}
			std::map<Value *, std::pair<double, double>> constOperandRange =
			    {{constOperand, std::make_pair(constValue, constValue)}};
			typeInfo realType = getTypeInfo(N, constOperand, constOperandRange);
			if ((realType.valueType != nullptr) &&
			    (compareType(realType.valueType, nonConstType) <= 0))
			{
				typeInfo backType;
				backType.valueType = nonConstType;
				backType.signFlag  = nonConstSign;
				if (isa<StoreInst>(inInstruction))
				{
					backType.valueType = changeStoreInstSiblingType(backType.valueType, constType);
				}
				auto newTypeValue = rollbackType(N, inInstruction, constOperandIdx, llvmIrBasicBlock,
								 typeChangedInst, backType);
				auto vrIt	  = virtualRegisterRange.find(inInstruction->getOperand(constOperandIdx));
				if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
				{
					virtualRegisterRange.emplace(newTypeValue, vrIt->second);
				}
			}
			else
			{
				typeInfo backType;
				backType.valueType = constType;
				backType.signFlag  = nonConstSign;
				if (isa<StoreInst>(inInstruction))
				{
					backType.valueType = changeStoreInstSiblingType(backType.valueType, nonConstType);
				}
				auto newTypeValue = rollbackType(N, inInstruction, nonConstOperandIdx, llvmIrBasicBlock,
								 typeChangedInst, backType);
				auto vrIt	  = virtualRegisterRange.find(inInstruction->getOperand(nonConstOperandIdx));
				if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
				{
					virtualRegisterRange.emplace(newTypeValue, vrIt->second);
				}
			}
		}
		else if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(constOperand))
		{
			std::map<Value *, std::pair<double, double>> constOperandRange;
			auto					     constValue = constInt->getSExtValue();
			constOperandRange					= {{constOperand, std::make_pair(constValue, constValue)}};
			typeInfo realType					= getTypeInfo(N, constOperand, constOperandRange);
			if ((realType.valueType != nullptr) &&
			    (compareType(realType.valueType, nonConstType) <= 0))
			{
				typeInfo backType;
				backType.valueType = nonConstType;
				backType.signFlag  = nonConstSign;
				if (isa<StoreInst>(inInstruction))
				{
					backType.valueType = changeStoreInstSiblingType(backType.valueType, constType);
				}
				auto newTypeValue = rollbackType(N, inInstruction, constOperandIdx, llvmIrBasicBlock,
								 typeChangedInst, backType);
				auto vrIt	  = virtualRegisterRange.find(inInstruction->getOperand(constOperandIdx));
				if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
				{
					virtualRegisterRange.emplace(newTypeValue, vrIt->second);
				}
			}
			else
			{
				typeInfo backType;
				backType.valueType = constType;
				backType.signFlag = nonConstSign;
				if (isa<StoreInst>(inInstruction))
				{
					backType.valueType = changeStoreInstSiblingType(backType.valueType, nonConstType);
				}
				auto newTypeValue = rollbackType(N, inInstruction, nonConstOperandIdx, llvmIrBasicBlock,
								 typeChangedInst, backType);
				auto vrIt	  = virtualRegisterRange.find(inInstruction->getOperand(nonConstOperandIdx));
				if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
				{
					virtualRegisterRange.emplace(newTypeValue, vrIt->second);
				}
			}
		}
		else
		{
			assert(false && "implement when meet");
		}
	};
	if (isa<llvm::Constant>(leftOperand))
	{
		/*
		 * leftOperand is constant, rightOperand is non-constant
		 * */
		rollBackConstantOperand(0, 1);
	}
	else if (isa<llvm::Constant>(rightOperand))
	{
		/*
		 * rightOperand is constant, leftOperand is non-constant
		 * */
		rollBackConstantOperand(1, 0);
	}
}

/*
 * There are only three place do the inplace mutation:
 * 1. Mutate operand constant.
 *    The constant operand type might be changed for several time by different reasons.
 *    So it should always store the original type.
 * 2. Mutate destination type of each instruction.
 *    The destInst type can only be changed once, so we have an assert to check it.
 * 3. `Alloca` node.
 * */
/*
 * if dest type cannot change, roll back operands type
 * if dest type can change and its real type <= operands type, mutate dest type to operand type
 * else roll back operand type to real type of dest and mutate dest type to real type
 * */
void
matchDestType(State * N, Instruction * inInstruction, BasicBlock & llvmIrBasicBlock,
	      std::map<llvm::Value *, std::pair<double, double>> & virtualRegisterRange,
	      std::map<Value *, typeInfo> &			   typeChangedInst)
{
	typeInfo typeInformation;
	typeInformation.valueType = nullptr;
	typeInformation.signFlag  = true;
	auto inInstType		  = inInstruction->getType();
	auto srcOperand		  = inInstruction->getOperand(0);
	auto srcType		  = srcOperand->getType();
	if (isa<GetElementPtrInst>(inInstruction))
	{
		unsigned ptAddressSpace = srcType->getPointerAddressSpace();
		srcType			= srcType->getPointerElementType();
        if (srcType->isAggregateType()) {
            /*
             * we don't shrink the aggregate type
             * */
            return;
        }
		std::vector<Value *> indexValue;
		for (size_t idx = 0; idx < inInstruction->getNumOperands() - 1; idx++)
		{
			indexValue.emplace_back(inInstruction->getOperand(idx));
		}
		unsigned opId = 0;
		srcType	      = getRawType(srcType, indexValue, &opId);
		srcType	      = srcType->getPointerTo(ptAddressSpace);
	}

	/*
	 * if operand type < dest type
	 * Now the type of inInstruction hasn't changed, so we check if it can be further changed (has range information).
	 * If not, roll back all operands
	 * */
	assert(compareType(srcType, inInstType) <= 0 && "srcType must <= inInstType");
	typeInformation = getTypeInfo(N, inInstruction, virtualRegisterRange);
	if (typeInformation.valueType == nullptr)
	{
		if (compareType(srcType, inInstType) != 0)
		{
			typeInfo backType;
			backType.signFlag  = isSignedValue(inInstruction);
			backType.valueType = inInstType;
            if (isa<LoadInst>(inInstruction))
            {
                unsigned ptAddressSpace = srcType->getPointerAddressSpace();
                backType.valueType	= backType.valueType->getPointerTo(ptAddressSpace);
            }
			for (size_t id = 0; id < inInstruction->getNumOperands(); id++)
			{
				auto newTypeValue = rollbackType(N, inInstruction, id, llvmIrBasicBlock, typeChangedInst, backType);
				auto vrIt	  = virtualRegisterRange.find(inInstruction->getOperand(id));
				if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
				{
					virtualRegisterRange.emplace(newTypeValue, vrIt->second);
				}
			}
		}
		return;
	}

	if (compareType(typeInformation.valueType, srcType) <= 0)
	{
		if (compareType(inInstType, srcType) == 0)
		{
			/*
			 * we only care if the inInstType and srcType are matched
			 * */
			return;
		}
		/*
		 * mutate dest type to srcType
		 * */
		Type * tmpType = nullptr;
		if (isa<LoadInst>(inInstruction) && (srcType->getTypeID() == Type::PointerTyID))
		{
			tmpType = srcType->getPointerElementType();
		}
		else
		{
			tmpType = srcType;
		}
		typeInfo instPrevTypeInfo{inInstruction->getType(), typeInformation.signFlag};
		assert(typeChangedInst.find(inInstruction) == typeChangedInst.end() && "destInst can only be mutated once");
		inInstruction->mutateType(tmpType);
		if (auto gepInst = dyn_cast<GetElementPtrInst>(inInstruction))
		{
			gepInst->setSourceElementType(srcType->getPointerElementType());
			gepInst->setResultElementType(srcType->getPointerElementType());
		}
		typeChangedInst.emplace(inInstruction, instPrevTypeInfo);
	}
	else
	{
		/*
		 * roll back operands to typeInformation.valueType
		 * */
        if (isa<LoadInst>(inInstruction))
        {
            unsigned ptAddressSpace	  = srcType->getPointerAddressSpace();
            typeInformation.valueType = typeInformation.valueType->getPointerTo(ptAddressSpace);
        }
        size_t roll_backed_op_num = isa<GetElementPtrInst>(inInstruction) ? 1 : inInstruction->getNumOperands();
        for (size_t id = 0; id < roll_backed_op_num; id++)
		{
			typeInfo operandPrevTypeInfo{typeInformation.valueType,
						     isSignedValue(inInstruction->getOperand(id))};
			auto	 newTypeValue = rollbackType(N, inInstruction, id, llvmIrBasicBlock, typeChangedInst,
							     operandPrevTypeInfo);
			auto	 vrIt	      = virtualRegisterRange.find(inInstruction->getOperand(id));
			if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
			{
				virtualRegisterRange.emplace(newTypeValue, vrIt->second);
			}
		}
		/*
		 * mutate dest to typeInformation.valueType
		 * */
		if (isa<LoadInst>(inInstruction) && (typeInformation.valueType->getTypeID() == Type::PointerTyID))
		{
			typeInformation.valueType = typeInformation.valueType->getPointerElementType();
		}
		typeInfo instPrevTypeInfo{inInstruction->getType(), typeInformation.signFlag};
		assert(typeChangedInst.find(inInstruction) == typeChangedInst.end() && "destInst can only be mutated once");
		inInstruction->mutateType(typeInformation.valueType);
		if (auto gepInst = dyn_cast<GetElementPtrInst>(inInstruction))
		{
			gepInst->setSourceElementType(typeInformation.valueType->getPointerElementType());
			gepInst->setResultElementType(typeInformation.valueType->getPointerElementType());
		}
		typeChangedInst.emplace(inInstruction, instPrevTypeInfo);
	}
	return;
}

bool
shrinkStrategy(State * N, Instruction * inInstruction)
{
	/*
	 * first strategy:
	 *  check its user is a compute instruction and its sibling is a constant, then shrink its type
	 * */
	for (auto * user : inInstruction->users())
	{
		auto * instUser = dyn_cast<Instruction>(user);
		if (!instUser)
			continue;
		if (!isa<BinaryOperator>(instUser))
			continue;
		for (size_t idx = 0; idx < instUser->getNumOperands(); idx++)
		{
			if (isa<llvm::ConstantFP>(instUser->getOperand(idx)))
				return true;
		}
	}
	return false;
}

bool
shrinkInstructionType(State * N, Instruction * inInstruction, BasicBlock & llvmIrBasicBlock,
		      std::map<llvm::Value *, std::pair<double, double>> & virtualRegisterRange,
		      std::map<Value *, typeInfo> &			   typeChangedInst)
{
	bool	 changed	 = false;
	typeInfo typeInformation = getTypeInfo(N, inInstruction, virtualRegisterRange);
	if (typeInformation.valueType == nullptr)
		return changed;

	if (isa<LoadInst>(inInstruction) && (typeInformation.valueType->getTypeID() == Type::PointerTyID))
	{
		typeInformation.valueType = typeInformation.valueType->getPointerElementType();
	}

	changed			= true;
	auto	      inInstType = inInstruction->getType();
	IRBuilder<>   Builder(&llvmIrBasicBlock);
	Instruction * insertPoint = inInstruction->getNextNode();
	while (isa<PHINode>(insertPoint))
	{
		insertPoint = insertPoint->getNextNode();
	}
	Builder.SetInsertPoint(insertPoint);
	auto cloneInst = inInstruction->clone();
	/*
	 * update the typeChangedInst with cloneInst
	 * */
	auto tcIt = typeChangedInst.find(inInstruction);
	if (tcIt != typeChangedInst.end())
	{
		typeChangedInst.emplace(cloneInst, tcIt->second);
	}
	Value * castInst;
	if (typeInformation.valueType->isIntegerTy())
	{
		castInst = Builder.CreateIntCast(cloneInst, typeInformation.valueType, typeInformation.signFlag);
	}
	else if (typeInformation.valueType->isDoubleTy())
	{
		castInst = Builder.CreateFPCast(cloneInst, typeInformation.valueType);
	}
    else
    {
        castInst = Builder.CreateBitCast(cloneInst, typeInformation.valueType);
    }
	auto vrIt = virtualRegisterRange.find(inInstruction);
	if (castInst != nullptr && vrIt != virtualRegisterRange.end())
	{
		virtualRegisterRange.emplace(castInst, vrIt->second);
	}
	/*
	 * it must be the first meet inst, so it's safe to RAUW
	 * */
	inInstruction->replaceAllUsesWith(castInst);
	ReplaceInstWithInst(inInstruction, cloneInst);
	typeInfo instPrevTypeInfo{inInstType, typeInformation.signFlag};
	typeChangedInst.emplace(castInst, instPrevTypeInfo);
	return changed;
}

void
rollBackBasicBlock(State * N, BasicBlock & llvmIrBasicBlock,
		   std::map<llvm::Value *, std::pair<double, double>> & virtualRegisterRange,
		   std::map<Value *, typeInfo>				typeChangedInst)
{
	for (Instruction & llvmIrInstruction : llvmIrBasicBlock)
	{
		/*
		 * roll back operands
		 * */
		for (size_t idx = 0; idx < llvmIrInstruction.getNumOperands(); idx++)
		{
			auto tcInstIt = typeChangedInst.find(llvmIrInstruction.getOperand(idx));
			if (tcInstIt != typeChangedInst.end())
			{
				auto newTypeValue = rollbackType(N, &llvmIrInstruction, idx, llvmIrBasicBlock,
								 typeChangedInst);
				auto vrIt	  = virtualRegisterRange.find(llvmIrInstruction.getOperand(idx));
				if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
				{
					virtualRegisterRange.emplace(newTypeValue, vrIt->second);
				}
			}
		}
		/*
		 * roll back destination
		 * */
		auto tcInstIt = typeChangedInst.find(&llvmIrInstruction);
		if (tcInstIt != typeChangedInst.end())
		{
			llvmIrInstruction.mutateType(tcInstIt->second.valueType);
			if (auto llvmIrAllocaInstruction = dyn_cast<AllocaInst>(&llvmIrInstruction))
			{
				llvmIrAllocaInstruction->setAllocatedType(
				    tcInstIt->second.valueType->getPointerElementType());
			}
		}
	}
}

bool
rollBackStrategy(State * N, const std::vector<Value *> & depLink)
{
	bool willBack = true;
	for (Value * value : depLink)
	{
		if (isa<ConstantFP>(value))
		{
			willBack = false;
			break;
		}
	}
	return willBack;
}

void
rollBackDependencyLink(State * N, const std::vector<Value *> & depLink,
		       std::map<llvm::Value *, std::pair<double, double>> & virtualRegisterRange,
		       std::map<Value *, typeInfo>			    typeChangedInst)
{
	for (Value * value : depLink)
	{
		if (Instruction * valueInst = dyn_cast<Instruction>(value))
		{
			/*
			 * roll back operands
			 * */
			for (size_t idx = 0; idx < valueInst->getNumOperands(); idx++)
			{
				auto tcInstIt = typeChangedInst.find(valueInst->getOperand(idx));
				if (tcInstIt != typeChangedInst.end())
				{
					auto newTypeValue = rollbackType(N, valueInst, idx, *valueInst->getParent(),
									 typeChangedInst);
					auto vrIt	  = virtualRegisterRange.find(valueInst->getOperand(idx));
					if (newTypeValue != nullptr && vrIt != virtualRegisterRange.end())
					{
						virtualRegisterRange.emplace(newTypeValue, vrIt->second);
					}
				}
			}
		}
		/*
		 * roll back destination
		 * */
		auto tcInstIt = typeChangedInst.find(value);
		if (tcInstIt != typeChangedInst.end())
		{
			value->mutateType(tcInstIt->second.valueType);
			if (auto llvmIrAllocaInstruction = dyn_cast<AllocaInst>(value))
			{
				llvmIrAllocaInstruction->setAllocatedType(
				    tcInstIt->second.valueType->getPointerElementType());
			}
		}
	}
}

/*
 * In the design of `shrinkInstType`, we ideally only generate new castInst to cast type,
 * but sometimes we change the type inplace (for example by `matchDestType`).
 * So here we replace the castInst to a new one.
 * This function is a bit similar with `matchDestType`,
 * but it only care about the type before shrinkage.
 *
 * Considering the type shrinkage won't change between integer, floating point, and pointer,
 * we only check the `ext` and `trunc`
 *
 * Example:
 *  %srcInst = op bigType %a, %b
 *  %inst = ext bigType %srcInst to smallType
 * =======>
 *  %srcInst = op bigType %a, %b
 *  %inst = trunc bigType %srcInst to smallType
 * */
bool matchCastType(State * N, Instruction * inInstruction, BasicBlock & llvmIrBasicBlock,
                    std::map<llvm::Value *, std::pair<double, double>> & virtualRegisterRange,
                    std::map<Value *, typeInfo> & typeChangedInst) {
    bool	 changed	 = false;

    auto inInstType = inInstruction->getType();
    auto srcInst = inInstruction->getOperand(0);
    auto srcType = srcInst->getType();

    // todo: get the sign bit from type system
    bool signFlag = true;
    auto tcIt = typeChangedInst.find(inInstruction);
    if (tcIt != typeChangedInst.end()) {
        signFlag = tcIt->second.signFlag;
    }

    Value * castInst;
    IRBuilder<>   Builder(&llvmIrBasicBlock);
    Builder.SetInsertPoint(inInstruction->getNextNode());

    if (compareType(inInstType, srcType) > 0) {
        if (inInstruction->getOpcode() == Instruction::Trunc) {
            castInst = Builder.CreateIntCast(srcInst, inInstType, signFlag);
            changed = true;
        } else if (inInstruction->getOpcode() == Instruction::FPTrunc) {
            castInst = Builder.CreateFPCast(srcInst, inInstType);
            changed = true;
        }
    } else if (compareType(inInstType, srcType) > 0) {
        if (inInstruction->getOpcode() == Instruction::ZExt ||
            inInstruction->getOpcode() == Instruction::SExt) {
            castInst = Builder.CreateIntCast(srcInst, inInstType, signFlag);
            changed = true;
        } else if (inInstruction->getOpcode() == Instruction::FPExt) {
            castInst = Builder.CreateFPCast(srcInst, inInstType);
            changed = true;
        }
    } else {
        /* mergeCast will do this */
    }

    if (!changed) {
        return changed;
    }

    auto vrIt = virtualRegisterRange.find(inInstruction);
    if (castInst != nullptr && vrIt != virtualRegisterRange.end())
    {
        virtualRegisterRange.emplace(castInst, vrIt->second);
    }

    if (tcIt != typeChangedInst.end()) {
        typeChangedInst.emplace(castInst, tcIt->second);
    }

    Instruction * newCastInst = llvm::dyn_cast<llvm::Instruction>(castInst);
    inInstruction->replaceAllUsesWith(newCastInst);
    inInstruction->removeFromParent();

    return changed;
}

/*
 * Some special instructions that need to pay attention:
 * %i = alloca type, instType is type*
 * %i = call retType @func_name (type %p1, ...)
 * call void @llvm.dbg.declare/value (metadata type %p, ...)
 * %i = load type, type* %op, instType is type
 * %i = gep type, type1* %op1, type2 %op2, (type3 %op3)
 * %i = castInst type1 %op1 to type2
 * store type %op1, type* %op2
 * %.i = phi type [%op1, %bb1], [%op2, %bb2], ...
 *
 * todo: Either skip the aggregate type or analyze it. Mainly structure.
 * */
std::map<Value *, typeInfo>
shrinkInstType(State * N, BoundInfo * boundInfo, Function & llvmIrFunction)
{
	/*
	 * <Value to be shrink, old type>
	 * */
	std::map<Value *, typeInfo> typeChangedInst;
	std::vector<AllocaInst *>   allocaVec;

	/*
	 * Shrink the type of function parameters
	 * e.g.
	 *  define i32* @func_name(i32* %0, i32* %1) {
	 *  ======================>
	 *  define i32* @func_name(i32* %0, i32* %1) {
	 *      %2 = bitcast i32* %0 to i8*
	 *      %3 = bitcast i32* %1 to i8*
	 * */
	for (int idx = 0; idx < llvmIrFunction.arg_size(); idx++)
	{
		auto	 paramOp	 = llvmIrFunction.getArg(idx);
		typeInfo typeInformation = getTypeInfo(N, paramOp, boundInfo->virtualRegisterRange);
		if (typeInformation.valueType != nullptr)
		{
			typeInfo    instPrevTypeInfo{paramOp->getType(), typeInformation.signFlag};
			IRBuilder<> Builder(&llvmIrFunction.getEntryBlock(), llvmIrFunction.getEntryBlock().begin());
			Value *	    castValue;
			if (typeInformation.valueType->isIntegerTy())
			{
				castValue = Builder.CreateIntCast(paramOp, typeInformation.valueType, typeInformation.signFlag);
			}
			else if (typeInformation.valueType->isDoubleTy())
			{
				castValue = Builder.CreateFPCast(paramOp, typeInformation.valueType);
			}
            else
            {
                castValue = Builder.CreateBitCast(paramOp, typeInformation.valueType);
            }
			auto vrIt = boundInfo->virtualRegisterRange.find(paramOp);
			if (castValue != nullptr && vrIt != boundInfo->virtualRegisterRange.end())
			{
				boundInfo->virtualRegisterRange.emplace(castValue, vrIt->second);
			}
			paramOp->replaceAllUsesWith(castValue);
			if (auto castInst = dyn_cast<CastInst>(castValue))
			{
				castInst->setOperand(0, paramOp);
			}
			typeChangedInst.emplace(castValue, instPrevTypeInfo);
		}
	}
	for (BasicBlock & llvmIrBasicBlock : llvmIrFunction)
	{
		for (BasicBlock::iterator itBB = llvmIrBasicBlock.begin(); itBB != llvmIrBasicBlock.end();)
		{
			Instruction * llvmIrInstruction = &*itBB++;
			switch (llvmIrInstruction->getOpcode())
			{
				case Instruction::Alloca:
					if (auto llvmIrAllocaInstruction = dyn_cast<AllocaInst>(llvmIrInstruction))
					{
						typeInfo realType = getTypeInfo(N, llvmIrAllocaInstruction,
										boundInfo->virtualRegisterRange);
						if (realType.valueType != nullptr)
						{
							typeInfo instPrevTypeInfo{llvmIrAllocaInstruction->getType(), realType.signFlag};
							typeChangedInst.emplace(llvmIrAllocaInstruction,
										instPrevTypeInfo);
							llvmIrAllocaInstruction->setAllocatedType(
							    realType.valueType->getPointerElementType());
							llvmIrAllocaInstruction->mutateType(realType.valueType);
						}
					}
					break;
				case Instruction::Call:
					if (auto llvmIrCallInstruction = dyn_cast<CallInst>(llvmIrInstruction))
					{
						Function * calledFunction = llvmIrCallInstruction->getCalledFunction();
						if (calledFunction == nullptr || !calledFunction->hasName() || calledFunction->getName().empty())
							break;
						if (!calledFunction->getName().startswith("llvm.dbg.value") &&
						    !calledFunction->getName().startswith("llvm.dbg.declare") &&
						    !calledFunction->getName().startswith("llvm.dbg.label"))
						{
							/*
							 * don't change the type of function call's params
							 * */
							for (size_t idx = 0; idx < llvmIrCallInstruction->getNumOperands() - 1; idx++)
							{
								auto tcInstIt = typeChangedInst.find(llvmIrCallInstruction->getOperand(idx));
								if (tcInstIt != typeChangedInst.end())
								{
									auto newTypeValue = rollbackType(N, llvmIrCallInstruction, idx, llvmIrBasicBlock,
													 typeChangedInst);
									auto vrIt	  = boundInfo->virtualRegisterRange.find(llvmIrCallInstruction->getOperand(idx));
									if (newTypeValue != nullptr && vrIt != boundInfo->virtualRegisterRange.end())
									{
										boundInfo->virtualRegisterRange.emplace(newTypeValue, vrIt->second);
									}
								}
							}
							/*
							 * shrink the type of callInst result
							 * */
							shrinkInstructionType(N, llvmIrInstruction, llvmIrBasicBlock,
									      boundInfo->virtualRegisterRange,
									      typeChangedInst);
						}
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
				case Instruction::PHI:
					if (isa<PHINode>(llvmIrInstruction))
					{
						matchPhiOperandType(N, llvmIrInstruction, llvmIrBasicBlock,
								    boundInfo->virtualRegisterRange, typeChangedInst);
					}
				case Instruction::FNeg:
				case Instruction::Load:
				case Instruction::GetElementPtr:
					matchDestType(N, llvmIrInstruction, llvmIrBasicBlock,
						      boundInfo->virtualRegisterRange, typeChangedInst);
					/*
					 * only shrink the type because the computation of current operation
					 * */
					shrinkInstructionType(N, llvmIrInstruction, llvmIrBasicBlock,
							      boundInfo->virtualRegisterRange,
							      typeChangedInst);
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
                case Instruction::BitCast:
                    matchCastType(N, llvmIrInstruction, llvmIrBasicBlock,
                                   boundInfo->virtualRegisterRange,
                                   typeChangedInst);
                    /*
                     * update the llvmIrInstruction,
                     * maybe there's a better way
                     *
                     * question: why `--` get the next instruction?
                     * */
                    llvmIrInstruction = &*itBB--;
                    llvmIrInstruction = &*itBB++;
                    shrinkInstructionType(N, llvmIrInstruction, llvmIrBasicBlock,
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
				case Instruction::Ret:
					if (auto llvmIrReturnInstruction = dyn_cast<ReturnInst>(llvmIrInstruction))
					{
						/*
						 * If the function return type not match, roll back the returnType
						 * e.g.
						 *  define i32* @funcName {
						 *    ...
						 *    ret i8* %x
						 *  }
						 *  ======================>
						 *  define i32* @funcName {
						 *    ...
						 *    %y = bitcast i8* %x to i32*
						 *    ret i32* %y
						 *  }
						 * */
						auto funcRetType = llvmIrFunction.getReturnType();
						auto retInstType = llvmIrReturnInstruction->getType();
						if (compareType(funcRetType, retInstType) != 0)
						{
							IRBuilder<> Builder(&llvmIrBasicBlock);
							Builder.SetInsertPoint(llvmIrReturnInstruction);
							Value * castInst;
							auto	retValue = llvmIrReturnInstruction->getReturnValue();
							assert(retValue != nullptr && "return void");
							if (funcRetType->isIntegerTy())
							{
                                // todo: get the sign bit from type system
								castInst = Builder.CreateIntCast(retValue, funcRetType, true);
							}
							else if (funcRetType->isDoubleTy())
							{
								castInst = Builder.CreateFPCast(retValue, funcRetType);
							}
                            else
                            {
                                castInst = Builder.CreateBitCast(retValue, funcRetType);
                            }
							auto vrIt = boundInfo->virtualRegisterRange.find(retValue);
							if (castInst != nullptr && vrIt != boundInfo->virtualRegisterRange.end())
							{
								boundInfo->virtualRegisterRange.emplace(castInst, vrIt->second);
							}
							ReturnInst::Create(llvmIrReturnInstruction->getContext(),
									   castInst,
									   llvmIrReturnInstruction->getParent());
							llvmIrReturnInstruction->removeFromParent();
						}
					}
					break;
				case Instruction::Switch:
					if (auto llvmIrSwitchInstruction = dyn_cast<SwitchInst>(llvmIrInstruction))
					{
						/*
						 * change the type of label, e.g.
						 *
						 * switch i8 %30, label %231 [
						 *  i8 1, label %213
						 *  i8 2, label %222
						 * ]
						 */
						auto condType = llvmIrSwitchInstruction->getCondition()->getType();
						for (auto caseEle : llvmIrSwitchInstruction->cases())
						{
							auto	 labelValue = caseEle.getCaseValue();
							typeInfo constTypeInfo{labelValue->getType(), false};
							auto	 constLabelValue = dyn_cast<ConstantInt>(labelValue);
							auto	 constValue	 = constLabelValue->getZExtValue();
							auto	 newConstant	 = ConstantInt::get(condType, constValue, false);
							llvmIrSwitchInstruction->replaceUsesOfWith(labelValue, newConstant);
							typeChangedInst.emplace(newConstant, constTypeInfo);
						}
						break;
					}
				case Instruction::Br:
				case Instruction::Select:
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
					break;
				default:
					break;
			}
		}
	}
	return typeChangedInst;
}

void
mergeCast(State * N, Function & llvmIrFunction,
	  std::map<llvm::Value *, std::pair<double, double>> & virtualRegisterRange,
	  std::map<Value *, typeInfo> &			       typeChangedInst)
{
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
	for (BasicBlock & llvmIrBasicBlock : llvmIrFunction)
	{
		for (BasicBlock::iterator itBB = llvmIrBasicBlock.begin(); itBB != llvmIrBasicBlock.end();)
		{
			Instruction * llvmIrInstruction = &*itBB++;
			switch (llvmIrInstruction->getOpcode())
			{
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
				{
					/*
					 * if the operand(0) is equal to operand(1), remove this inst,
					 * except array time cast like
					 * %a = bitcast [100 x i8]* %b to i8*
					 * */
					auto sourceOp = llvmIrInstruction->getOperand(0);
					if (sourceOp->getType() == llvmIrInstruction->getType())
					{
						llvmIrInstruction->replaceAllUsesWith(sourceOp);
						llvmIrInstruction->removeFromParent();
						break;
					}
					auto sourceInst = llvm::dyn_cast<llvm::Instruction>(sourceOp);
					if (sourceInst != nullptr)
					{
						auto siIt = std::find(sourceInstVec.begin(), sourceInstVec.end(), sourceInst);
						if (siIt != sourceInstVec.end())
						{
							/*
							 * substitute destInst to sourceInst
							 * */
							sourceInstVec.erase(siIt);
							Instruction * sourceOperand = llvm::dyn_cast<llvm::Instruction>(sourceInst->getOperand(0));
							IRBuilder<>   Builder(&llvmIrBasicBlock);
							Builder.SetInsertPoint(sourceInst->getNextNode());
							if (compareType(sourceOperand->getType(), llvmIrInstruction->getType()) == 0)
							{
								llvmIrInstruction->replaceAllUsesWith(sourceOperand);
							}
							else
							{
								/*
								 * if llvmIrInstruction in typeChangedInst,
								 * it means llvmIrInstruction is a created castInst.
								 * */
								Value * castInst;
								auto	valueType = llvmIrInstruction->getType();
                                if ((valueType->isFloatTy() || valueType->isDoubleTy()) &&
                                    sourceOperand->getType()->isIntegerTy())
                                {
                                    // float fa = (float)ia;
                                    bool isSigned = sourceInst->getOpcode() == Instruction::SIToFP;
                                    castInst      = isSigned ? Builder.CreateSIToFP(sourceOperand, valueType)
                                                             : Builder.CreateUIToFP(sourceOperand, valueType);
                                }
                                else if (valueType->isIntegerTy() &&
                                         (sourceOperand->getType()->isFloatTy() || sourceOperand->getType()->isDoubleTy()))
                                {
                                    // int iq = (int)fq;
                                    bool isSigned = sourceInst->getOpcode() == Instruction::FPToSI;
                                    castInst      = isSigned ? Builder.CreateFPToSI(sourceOperand, valueType)
                                                             : Builder.CreateFPToUI(sourceOperand, valueType);
                                }
                                else if (valueType->isIntegerTy())
								{
									castInst = Builder.CreateIntCast(sourceOperand, valueType,
													 llvmIrInstruction->getOpcode() == Instruction::SExt);
								}
								else if (valueType->isFloatTy() || valueType->isDoubleTy())
								{
									castInst = Builder.CreateFPCast(sourceOperand, valueType);
								}
                                else
                                {
                                    castInst = Builder.CreateBitCast(sourceOperand, valueType);
                                }
								auto vrIt = virtualRegisterRange.find(sourceOperand);
								if (castInst != nullptr && vrIt != virtualRegisterRange.end())
								{
									virtualRegisterRange.emplace(castInst, vrIt->second);
								}
								auto	 tcIt = typeChangedInst.find(sourceInst);
								typeInfo instPrevTypeInfo;
								instPrevTypeInfo.signFlag = llvmIrInstruction->getOpcode() == Instruction::SExt;
								if (tcIt == typeChangedInst.end())
								{
									/*
									 * It's an original castInst
									 * */
									instPrevTypeInfo.valueType = sourceInst->getType();
								}
								else
								{
									instPrevTypeInfo.valueType = tcIt->second.valueType;
								}
								typeChangedInst.emplace(castInst, instPrevTypeInfo);
								Instruction * newCastInst = llvm::dyn_cast<llvm::Instruction>(castInst);
								llvmIrInstruction->replaceAllUsesWith(newCastInst);
								sourceInstVec.emplace_back(newCastInst);
							}
							llvmIrInstruction->removeFromParent();
						}
						else
						{
							sourceInstVec.emplace_back(llvmIrInstruction);
						}
					}
					break;
				}
				case Instruction::Call:
					if (auto llvmIrCallInstruction = dyn_cast<CallInst>(llvmIrInstruction))
					{
						Function * calledFunction = llvmIrCallInstruction->getCalledFunction();
						if (calledFunction == nullptr || !calledFunction->hasName() || calledFunction->getName().empty())
							break;
						if (calledFunction->getName().startswith("llvm.dbg.value") ||
						    calledFunction->getName().startswith("llvm.dbg.declare"))
						{
							if (!isa<MetadataAsValue>(llvmIrCallInstruction->getOperand(0)))
								break;
							auto firstOperator = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(0));
							if (!isa<ValueAsMetadata>(firstOperator->getMetadata()))
								break;
							auto localVariableAddressAsMetadata = cast<ValueAsMetadata>(firstOperator->getMetadata());
							auto localVariableAddress	    = localVariableAddressAsMetadata->getValue();
							auto siIt			    = std::find(sourceInstVec.begin(), sourceInstVec.end(),
													localVariableAddress);
							if (siIt != sourceInstVec.end())
							{
								sourceInstVec.erase(siIt);
							}
						}
						else
						{
							for (size_t idx = 0; idx < llvmIrCallInstruction->getNumOperands() - 1; idx++)
							{
								auto siIt = std::find(sourceInstVec.begin(), sourceInstVec.end(),
										      llvmIrCallInstruction->getOperand(idx));
								if (siIt != sourceInstVec.end())
								{
									sourceInstVec.erase(siIt);
								}
							}
						}
						break;
					}
				default:
					for (size_t idx = 0; idx < llvmIrInstruction->getNumOperands(); idx++)
					{
						auto siIt = std::find(sourceInstVec.begin(), sourceInstVec.end(),
								      llvmIrInstruction->getOperand(idx));
						if (siIt != sourceInstVec.end())
						{
							sourceInstVec.erase(siIt);
						}
					}
					break;
			}
		}
	}
	return;
}

std::map<uint32_t, uint32_t>
countCastInst(State * N, Function & llvmIrFunction)
{
	std::map<uint32_t, uint32_t> castCountVec{{0, 0}};
	uint32_t		     bbId = 0;
	for (BasicBlock & llvmIrBasicBlock : llvmIrFunction)
	{
		for (Instruction & llvmIrInstruction : llvmIrBasicBlock)
		{
			switch (llvmIrInstruction.getOpcode())
			{
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
					castCountVec[bbId]++;
					break;
				default:
					continue;
			}
		}
		bbId++;
	}
	return castCountVec;
}

using dpLink = std::vector<std::vector<Value *>>;
/*
 * get the index of the node if found, or return "-1"
 * */
int
findNode(const dpLink & dependencyLink, Value * node)
{
	auto dlIt = std::find_if(dependencyLink.begin(), dependencyLink.end(), [node](const auto & v) {
		return std::find(v.begin(), v.end(), node) != v.end();
	});
	if (dlIt != dependencyLink.end())
	{
		return std::distance(dependencyLink.begin(), dlIt);
	}
	else
	{
		return -1;
	}
}

void
insertLink(dpLink & dependencyLink, Value * aimNode, Value * siblingNode)
{
	auto aimPos	= findNode(dependencyLink, aimNode);
	auto siblingPos = findNode(dependencyLink, siblingNode);
	if (aimPos != -1)
	{
		/*aimNode in dependencyLink*/
		if (siblingPos != -1)
		{
			/*
			 * sibling is also in dependencyLink, merge them
			 * */
			auto newPos    = aimPos < siblingPos ? aimPos : siblingPos;
			auto deletePos = aimPos > siblingPos ? aimPos : siblingPos;
			dependencyLink[newPos].insert(dependencyLink[newPos].end(),
						      dependencyLink[deletePos].begin(),
						      dependencyLink[deletePos].end());
			dependencyLink.erase(dependencyLink.begin() + deletePos);
		}
		else
		{
			/*
			 * sibling is not in dependencyLink, insert to aimPos
			 * */
			dependencyLink[aimPos].emplace_back(siblingNode);
		}
	}
	else
	{
		/*aimNode not in dependencyLink*/
		if (siblingPos == -1)
		{
			siblingPos = dependencyLink.capacity();
			dependencyLink.resize(siblingPos + 1);
			dependencyLink[siblingPos].emplace_back(siblingNode);
		}
		dependencyLink[siblingPos].emplace_back(aimNode);
	}
}

std::vector<std::vector<Value *>>
getDependencyLink(State * N, Function & llvmIrFunction)
{
	std::vector<std::vector<Value *>> dependencyLink;
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
						if (calledFunction == nullptr || !calledFunction->hasName() ||
						    calledFunction->getName().empty())
							break;
						if (!calledFunction->getName().startswith("llvm.dbg.value") &&
						    !calledFunction->getName().startswith("llvm.dbg.declare") &&
						    !calledFunction->getName().startswith("llvm.dbg.label"))
						{
							for (size_t idx = 1; idx < llvmIrCallInstruction->getNumOperands() - 1; idx++)
							{
								insertLink(dependencyLink, llvmIrCallInstruction->getOperand(0),
									   llvmIrCallInstruction->getOperand(idx));
							}
							insertLink(dependencyLink, llvmIrCallInstruction->getOperand(0),
								   llvmIrCallInstruction);
						}
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
					/*match binary operands*/
					insertLink(dependencyLink, llvmIrInstruction.getOperand(0),
						   llvmIrInstruction.getOperand(1));
				case Instruction::PHI:
					if (isa<PHINode>(llvmIrInstruction))
					{
						/*match multi operands*/
						for (size_t id = 1; id < llvmIrInstruction.getNumOperands(); id++)
						{
							insertLink(dependencyLink, llvmIrInstruction.getOperand(0),
								   llvmIrInstruction.getOperand(id));
						}
					}
				case Instruction::FNeg:
				case Instruction::Load:
				case Instruction::GetElementPtr:
					/*match dest with operands*/
					insertLink(dependencyLink, llvmIrInstruction.getOperand(0),
						   &llvmIrInstruction);
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
					/*check dest*/
					break;
				case Instruction::Store:
				case Instruction::ICmp:
				case Instruction::FCmp:
					/*match operands*/
					insertLink(dependencyLink, llvmIrInstruction.getOperand(0),
						   llvmIrInstruction.getOperand(1));
				default:
					continue;
			}
		}
	}
	return dependencyLink;
}

/*
 * There are three kinds of instructions in LLVM that are related with signed/unsigned
 *  1. nsw/nuw with Add, Sub, Mul, Shl
 *  2. UDiv/SDiv, URem/SRem, LShr/AShr
 *  3. sgt/ugt, sge/uge, slt/ult, sle/ule in ICmp
 * Note: Sign bit can only change from `signed` to `unsigned` in `type shrinkage`.
 * Remember: We have matched the type of operands before this function.
 *
 * Update: To simplify the problem, we only keep the signed type, so maybe it's not necessary to use this function.
 * */
void
upDateInstSignFlag(State * N, Function & llvmIrFunction,
                   std::map<llvm::Value *, std::pair<double, double>> & virtualRegisterRange,
                   std::map<Value *, typeInfo> & typeChangedInst) {
    for (BasicBlock & llvmIrBasicBlock : llvmIrFunction) {
        for (BasicBlock::iterator itBB = llvmIrBasicBlock.begin(); itBB != llvmIrBasicBlock.end();) {
            Instruction *llvmIrInstruction = &*itBB++;
            if (llvmIrInstruction->getNumOperands() < 2) {
                continue;
            }
            auto lhs = llvmIrInstruction->getOperand(0);
            auto rhs = llvmIrInstruction->getOperand(1);
            auto lhsIt = typeChangedInst.find(lhs);
            auto rhsIt = typeChangedInst.find(rhs);
            if ((lhsIt != typeChangedInst.end() || rhsIt != typeChangedInst.end())) {
                // debug info: to check the range of operands
                auto vrLhsIt = virtualRegisterRange.find(lhs);
                auto vrRhsIt = virtualRegisterRange.find(rhs);
//                assert(vrLhsIt != virtualRegisterRange.end() && vrRhsIt != virtualRegisterRange.end());
                switch (llvmIrInstruction->getOpcode()) {
                    case Instruction::Add:
                    case Instruction::Sub:
                    case Instruction::Mul:
                    case Instruction::Shl:
                    {
                        /*
                         * nsw/nuw
                         * Implement when meet
                         * */
//                        if (lhsIt->second.signFlag || rhsIt->second.signFlag) {
                            if (llvmIrInstruction->hasNoUnsignedWrap()) {
                                /*
                                 * change to `nsw`
                                 * */
                                llvmIrInstruction->setHasNoUnsignedWrap(false);
                            }
//                        } else {
                            if (llvmIrInstruction->hasNoSignedWrap()) {
                                /*
                                 * change to `nuw`
                                 * */
                                llvmIrInstruction->setHasNoSignedWrap(false);
                            }
//                        }
//                        flexprint(N->Fe, N->Fm, N->Fperr,
//                                  "\tupDateInstSignFlag with nsw/nuw: Not Implement!\n");
                        break;
                    }
                    /*
                     * Different inst for signed/unsigned.
                     * Should also care about
                     *  1. the extent.
                     *  2. one operand is signed, the other is unsigned.
                     * Check the LLVM Ref: https://llvm.org/docs/LangRef.html#llvm-language-reference-manual
                     * Implement when meet.
                     * */
                    case Instruction::SDiv:
                    {
                        if (!lhsIt->second.signFlag && !rhsIt->second.signFlag) {
                            IRBuilder<> Builder(&llvmIrBasicBlock);
                            Builder.SetInsertPoint(llvmIrInstruction);
                            auto UDivInst = Builder.CreateUDiv(lhs, rhs);
                            llvmIrInstruction->replaceAllUsesWith(UDivInst);
                            llvmIrInstruction->removeFromParent();
                        }
                        break;
                    }
                    case Instruction::SRem:
                    {
                        if (!lhsIt->second.signFlag && !rhsIt->second.signFlag) {
                            IRBuilder<> Builder(&llvmIrBasicBlock);
                            Builder.SetInsertPoint(llvmIrInstruction);
                            auto URemInst = Builder.CreateURem(lhs, rhs);
                            llvmIrInstruction->replaceAllUsesWith(URemInst);
                            llvmIrInstruction->removeFromParent();
                        }
                        break;
                    }
                    case Instruction::AShr:
                    {
                        if (!lhsIt->second.signFlag && !rhsIt->second.signFlag) {
                            IRBuilder<> Builder(&llvmIrBasicBlock);
                            Builder.SetInsertPoint(llvmIrInstruction);
                            auto LShrInst = Builder.CreateLShr(lhs, rhs);
                            llvmIrInstruction->replaceAllUsesWith(LShrInst);
                            llvmIrInstruction->removeFromParent();
                        }
                        break;
                    }
                    case Instruction::ICmp:
                        if (auto llvmIrICmpInstruction = dyn_cast<ICmpInst>(llvmIrInstruction))
                        {
                            if (llvmIrICmpInstruction->isUnsigned()) {
                                break;
                            }
                            auto lhs  = llvmIrICmpInstruction->getOperand(0);
                            auto rhs = llvmIrICmpInstruction->getOperand(1);
                            /*
                             * If either of the operand is constant,
                             * and the variable operand can only change from `signed` to `unsigned`,
                             * so we only care about when the variable operand is `unsigned`.
                             * Note: here's instruction is signed!
                             *  if the constant operand is negative value, the `scf by range` should simplify it
                             *  if the constant operand is positive value, we can use `unsigned` flag
                             * */
                            if ((isa<llvm::Constant>(lhs) && !isa<llvm::Constant>(rhs)))
                            {
                                llvmIrICmpInstruction->swapOperands();
                                lhs  = llvmIrICmpInstruction->getOperand(0);
                                rhs = llvmIrICmpInstruction->getOperand(1);
                            }
                            if (!isa<llvm::Constant>(lhs) && isa<llvm::Constant>(rhs)) {
                                ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rhs);
                                assert(nullptr != constInt && "ICmp: it's not a const int!!!!!!!!!!!\n");
                                if (constInt->getSExtValue() < 0) {
                                    /*
                                     * the `scf by range` should simplify it
                                     * */
                                    break;
                                }

                                auto originalPred = llvmIrICmpInstruction->getPredicate();
                                llvmIrICmpInstruction->setPredicate(ICmpInst::getUnsignedPredicate(originalPred));
                            } else if (!lhsIt->second.signFlag && !rhsIt->second.signFlag) {
                                /*
                                 * If both of the operands are variable with different sign bit,
                                 * we check the range of them (if we can), e.g.
                                 *
                                 *  %c = icmp slt i16 %a, %b
                                 *
                                 *  if the %a is unsigned, but the max range is less than 32767, we can ignore it.
                                 *  otherwise, it overflows, and we should extend the operands, like,
                                 *
                                 *  %c = sext i16 %a to i32
                                 *  %d = sext i16 %b to i32
                                 *  %e = icmp slt i32 %c, %d
                                 *  %f = trunc i32 %c to i16
                                 *  %g = trunc i32 %d to i16
                                 *
                                 *  Then we replace the `%f`, `%g` to `%a`, `%b`.
                                 *  And also replace the `%e` to the previous icmp result.
                                 * */
                                auto originalPred = llvmIrICmpInstruction->getPredicate();
                                llvmIrICmpInstruction->setPredicate(ICmpInst::getUnsignedPredicate(originalPred));
//                                flexprint(N->Fe, N->Fm, N->Fperr,
//                                          "\tupDateInstSignFlag ICmp with both variable: Not Implement!\n");
                            }
                            break;
                        }
                }
            }
        }
    }
}

void
shrinkType(State * N, BoundInfo * boundInfo, Function & llvmIrFunction)
{
	/*
	 * 1. construct instruction dependency link
	 * 2. work with roll back strategies
	 * */
    std::map<Value *, typeInfo> typeChangedInst = shrinkInstType(N, boundInfo, llvmIrFunction);

	mergeCast(N, llvmIrFunction, boundInfo->virtualRegisterRange, typeChangedInst);

    upDateInstSignFlag(N, llvmIrFunction, boundInfo->virtualRegisterRange, typeChangedInst);
}
}
