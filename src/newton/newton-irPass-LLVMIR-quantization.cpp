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

#include "newton-irPass-LLVMIR-quantization.h"

using namespace llvm;

#define FRAC_Q			16
#define FRAC_BASE		(1<<FRAC_Q)
#define BIT_WIDTH        32

extern "C"
{
void setQuantizedType(Value * inValue, Type * quantizedType) {
    auto valueType = inValue->getType();
    unsigned pointerAddr;
    bool	 isPointer = false;
    if (valueType != nullptr)
    {
        if (valueType->isPointerTy()) {
            isPointer = true;
            pointerAddr = valueType->getPointerAddressSpace();
            valueType = valueType->getPointerElementType();
        }
        if (valueType->isDoubleTy() || valueType->isFloatTy())
        {
            if (isPointer) {
                inValue->mutateType(quantizedType->getPointerTo(pointerAddr));
            } else {
                inValue->mutateType(quantizedType);
            }
        }
    }
}

void setQuantizedPointerType(Value * inValue, Type * quantizedType, unsigned pointerAddr) {
    auto valueType = inValue->getType();
    if (valueType != nullptr)
    {
        if (valueType->isDoubleTy() || valueType->isFloatTy())
        {
            inValue->mutateType(quantizedType->getPointerTo(pointerAddr));
        }
    }
}

void quantizeConstant(Instruction * inInstruction, Type * quantizedType) {
    for (size_t idx = 0; idx < inInstruction->getNumOperands(); idx++) {
        Value * inValue = inInstruction->getOperand(idx);

        if (!isa<llvm::ConstantFP>(inValue)) {
            continue;
        }

        ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(inValue);
        Value * newValue = nullptr;
        if (inValue->getType()->isFloatTy())
        {
            float constValue = constFp->getValueAPF().convertToFloat();
            constValue *= FRAC_BASE;
            newValue	 = ConstantInt::get(quantizedType, round(constValue), true);
        }
        else if (inValue->getType()->isDoubleTy())
        {
            double constValue = constFp->getValueAPF().convertToDouble();
            constValue *= FRAC_BASE;
            newValue	 = ConstantInt::get(quantizedType, round(constValue), true);
        }
        else
        {
            assert(false && "unknown floating type");
        }

        inInstruction->replaceUsesOfWith(inValue, newValue);
    }
}

void simplifyConstant(Instruction * inInstruction, Type * quantizedType) {
    auto checkDecimal = [](float decimalNum) {
        int digits = 0;
        /*
         * Since the max value of `int16` is 32767,
         * we maximum multiply with 1,000 to make sure it won't exceed max_int16
         * */
        do {
            decimalNum *= 10;
            digits++;
        } while ((int)round(decimalNum) % 1 != 0 && digits < 4);
        return decimalNum;
    };

    auto compensateFP = [inInstruction, quantizedType](float quantizedNum, float decimalNum) {
        /*
         * 3333.3 / 3.3333 = 1000
         * ===>
         * Example 1:
         * a * 3.3333 ~= a * (3333 / 1000) ~= (int)a * 3333 / 1000
         *
         * Example 2:
         * a / 3.3333 ~= a / (3333 / 1000) ~= (int)a * 1000 / 3333
         *
         * Example 3:
         * 3.3333 / a ~= (3333 / 1000) / a ~= 3333 / (int)a / 1000
         * */
        float compensateNum = quantizedNum / decimalNum;
        auto quantizeNumValue = ConstantInt::get(quantizedType, round(quantizedNum), true);
        auto compensateNumValue = ConstantInt::get(quantizedType, round(compensateNum), true);

        IRBuilder<> Builder(inInstruction);
        Instruction * insertPoint = inInstruction->getNextNode();
        Builder.SetInsertPoint(insertPoint);
        Value * newFisrtInst = nullptr;
        Value * newSecondInst = nullptr;
        auto instOpCode = inInstruction->getOpcode();
        auto constOperand = isa<llvm::Constant>(inInstruction->getOperand(0)) ?
                inInstruction->getOperand(0) : inInstruction->getOperand(1);
        auto nonConstOperand = isa<llvm::Constant>(inInstruction->getOperand(0)) ?
                               inInstruction->getOperand(1) : inInstruction->getOperand(0);
        if (instOpCode == Instruction::FMul) {
            newFisrtInst = Builder.CreateMul(nonConstOperand, quantizeNumValue);
            newSecondInst = Builder.CreateSDiv(newFisrtInst, compensateNumValue);
        } else if (instOpCode == Instruction::FDiv && isa<llvm::Constant>(inInstruction->getOperand(1))) {
            newFisrtInst = Builder.CreateMul(nonConstOperand, compensateNumValue);
            newSecondInst = Builder.CreateSDiv(newFisrtInst, quantizeNumValue);
        } else if (instOpCode == Instruction::FDiv && isa<llvm::Constant>(inInstruction->getOperand(0))) {
            newFisrtInst = Builder.CreateSDiv(quantizeNumValue, nonConstOperand);
            newSecondInst = Builder.CreateSDiv(newFisrtInst, compensateNumValue);
        }

        inInstruction->replaceAllUsesWith(newSecondInst);
        inInstruction->removeFromParent();
    };

    for (size_t idx = 0; idx < inInstruction->getNumOperands(); idx++) {
        Value * inValue = inInstruction->getOperand(idx);

        if (!isa<llvm::ConstantFP>(inValue)) {
            continue;
        }

        ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(inValue);
        Value * newValue = nullptr;
        if (inValue->getType()->isFloatTy())
        {
            float constValue = constFp->getValueAPF().convertToFloat();
            compensateFP(checkDecimal(constValue), constValue);
        }
        else if (inValue->getType()->isDoubleTy())
        {
            double constValue = constFp->getValueAPF().convertToDouble();
            compensateFP(checkDecimal(constValue), constValue);
        }
        else
        {
            assert(false && "unknown floating type");
        }
    }
}

llvm::Function * createFixMul(llvm::Function * inFunction, Type * quantizedType, std::vector<llvm::Function*>& functionsToInsert) {
    /*
     * check if this function is exist
     * */
    std::string fixmulFuncName = "fixmul";
    auto irModule = inFunction->getParent();
    for (auto & function : *irModule) {
        if (function.getName() == fixmulFuncName) {
            return &function;
        }
    }

    llvm::FunctionType * funcType = llvm::FunctionType::get(quantizedType, {quantizedType, quantizedType}, false);
    llvm::Function * func = llvm::Function::Create(funcType, llvm::Function::PrivateLinkage, fixmulFuncName, irModule);

    llvm::BasicBlock* entryBB = llvm::BasicBlock::Create(inFunction->getContext(), "entry", func);
    llvm::IRBuilder<> builder(entryBB);
    builder.SetInsertPoint(entryBB);

    /*
     * ((int64_t)x*y)>>FRAC_Q
     *
     * ===========>
     *
     * define private i32 @mulfix(i32 %0, i32 %1) {
     * %3 = sext i32 %0 to i64
     * %4 = sext i32 %1 to i64
     * %5 = mul nsw i64 %3, %4
     * %6 = ashr i64 %5, 8
     * %7 = trunc i64 %6 to i32
     * ret i32 %7
     * }
     * */
    Type* higherQuantizedType;
    switch (BIT_WIDTH) {
        case 8:
            higherQuantizedType = Type::getInt16Ty(inFunction->getContext());
            break;
        case 16:
            higherQuantizedType = Type::getInt32Ty(inFunction->getContext());
            break;
        default:
            higherQuantizedType = Type::getInt64Ty(inFunction->getContext());
            break;
    }

    llvm::Function::arg_iterator arg1 = &*(func->arg_begin());
    llvm::Function::arg_iterator arg2 = &*(++arg1);
    llvm::Value* sext1 = builder.CreateSExt(arg1, higherQuantizedType);
    llvm::Value* sext2 = builder.CreateSExt(arg2, higherQuantizedType);
    llvm::Value* mulInst = builder.CreateMul(sext1, sext2);
    llvm::Value* ashrInst = builder.CreateAShr(mulInst, ConstantInt::get(higherQuantizedType, FRAC_Q));
    llvm::Value* truncInst = builder.CreateTrunc(ashrInst, quantizedType);
    builder.CreateRet(truncInst);

    functionsToInsert.emplace_back(func);

    return func;
}

void substituteHardcodeFunc(Instruction * inInstruction, Type * quantizedType, llvm::Function * func) {
    IRBuilder<> Builder(inInstruction);
    Instruction * insertPoint = inInstruction->getNextNode();
    Builder.SetInsertPoint(insertPoint);
//    Value * newInst = nullptr;

    llvm::CallInst* callInst = Builder.CreateCall(func, {inInstruction->getOperand(0), inInstruction->getOperand(1)});
//    InlineFunctionInfo inlineFuncInfo;
//    llvm::InlineFunction(*callInst, inlineFuncInfo);

    inInstruction->replaceAllUsesWith(callInst);
    inInstruction->removeFromParent();

    int a = 0;
}

CmpInst::Predicate quantizePredict(CmpInst::Predicate predict) {
    switch (predict) {
        case FCmpInst::FCMP_OEQ:
        case FCmpInst::FCMP_UEQ:
            return ICmpInst::ICMP_EQ;
        case FCmpInst::FCMP_OGT:
        case FCmpInst::FCMP_UGT:
            return ICmpInst::ICMP_SGT;
        case FCmpInst::FCMP_OGE:
        case FCmpInst::FCMP_UGE:
            return ICmpInst::ICMP_SGE;
        case FCmpInst::FCMP_OLT:
        case FCmpInst::FCMP_ULT:
            return ICmpInst::ICMP_SLT;
        case FCmpInst::FCMP_OLE:
        case FCmpInst::FCMP_ULE:
            return ICmpInst::ICMP_SLE;
        case FCmpInst::FCMP_ONE:
        case FCmpInst::FCMP_UNE:
            return ICmpInst::ICMP_NE;
    }
}

void quantizeSimpleFPInstruction(Instruction * inInstruction, Type * quantizedType) {
    IRBuilder<> Builder(inInstruction);
    Instruction * insertPoint = inInstruction->getNextNode();
    Builder.SetInsertPoint(insertPoint);
    Value * newInst = nullptr;
    switch (inInstruction->getOpcode())
    {
        case Instruction::FAdd:
        {
            newInst = Builder.CreateAdd(inInstruction->getOperand(0), inInstruction->getOperand(1));
            break;
        }
        case Instruction::FSub:
        {
            newInst = Builder.CreateSub(inInstruction->getOperand(0), inInstruction->getOperand(1));
            break;
        }
        case Instruction::FRem:
        {
            newInst = Builder.CreateSRem(inInstruction->getOperand(0), inInstruction->getOperand(1));
            break;
        }
        case Instruction::FCmp:
        {
            FCmpInst *fcmp_inst = dyn_cast<FCmpInst>(inInstruction);
            newInst = Builder.CreateICmp(quantizePredict(fcmp_inst->getPredicate()),
                                         fcmp_inst->getOperand(0), fcmp_inst->getOperand(1));
            break;
        }
        /*
         * Change fneg(a) to `0-a`.
         * */
        case Instruction::FNeg:
        {
            auto constZero = ConstantInt::get(quantizedType, 0, true);
            newInst = Builder.CreateSub(constZero, inInstruction->getOperand(0));
            break;
        }
        default:
            break;
    }
    inInstruction->replaceAllUsesWith(newInst);
    inInstruction->removeFromParent();
}

void
irPassLLVMIRAutoQuantization(State * N, llvm::Function & llvmIrFunction, std::vector<llvm::Function*>& functionsToInsert)
{
    flexprint(N->Fe, N->Fm, N->Fpinfo, "\tauto quantization.\n");

    Type* quantizedType;
    switch (BIT_WIDTH) {
        case 8:
            quantizedType = Type::getInt8Ty(llvmIrFunction.getContext());
            break;
        case 16:
            quantizedType = Type::getInt16Ty(llvmIrFunction.getContext());
            break;
        case 32:
            quantizedType = Type::getInt32Ty(llvmIrFunction.getContext());
            break;
        case 64:
            quantizedType = Type::getInt64Ty(llvmIrFunction.getContext());
            break;
        default:
            flexprint(N->Fe, N->Fm, N->Fperr, "\tunknown int type.\n");
            return;
    }

    /*
     * generate hardcode function - fixmul and fixdiv
     * */
    llvm::Function * fixmul = createFixMul(&llvmIrFunction, quantizedType, functionsToInsert);

    /*
     * quantize the arguments type
     * */
    for (int idx = 0; idx < llvmIrFunction.arg_size(); idx++)
    {
        auto	 paramOp	 = llvmIrFunction.getArg(idx);
        setQuantizedType(paramOp, quantizedType);
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
                        unsigned addressSpace = llvmIrAllocaInstruction->getType()->getPointerAddressSpace();
                        llvmIrAllocaInstruction->setAllocatedType(quantizedType);
                        setQuantizedPointerType(llvmIrAllocaInstruction, quantizedType, addressSpace);
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
                            for (size_t idx = 0; idx < llvmIrCallInstruction->getNumOperands() - 1; idx++)
                            {
                                setQuantizedType(llvmIrCallInstruction->getOperand(idx), quantizedType);
                            }
                            quantizeConstant(llvmIrCallInstruction, quantizedType);
                        }
                    }
                    break;
                case Instruction::Load:
                case Instruction::GetElementPtr:
                case Instruction::PHI:
                {
                    setQuantizedType(llvmIrInstruction, quantizedType);
                }

                case Instruction::Store:
                {
                    /*
                     * If either of the operands is constant, change it to a int value
                     * */
                    quantizeConstant(llvmIrInstruction, quantizedType);
                }
                break;

                /*
                 * For fmul/fdiv,
                 *
                 * if either one of the operands is a constant value, simplify it by multiplying with 10^n,
                 * then replace the instruction to mul/div;
                 *
                 * else substitute this instruction to a pre-implemented function: mulfix/divfix.
                 * */
                case Instruction::FMul:
                case Instruction::FDiv:
                {
                    if (isa<llvm::Constant>(llvmIrInstruction->getOperand(0)) ||
                            isa<llvm::Constant>(llvmIrInstruction->getOperand(1))) {
                        simplifyConstant(llvmIrInstruction, quantizedType);
                    }
                    else {
                        substituteHardcodeFunc(llvmIrInstruction, quantizedType, fixmul);
                    }
                    break;
                }

                /*
                 * If either one of the operands is a constant value, quantize it,
                 * then replace the instruction to the int version.
                 * */
                case Instruction::FCmp:
                case Instruction::FAdd:
                case Instruction::FSub:
                case Instruction::FRem:
                {
                    quantizeConstant(llvmIrInstruction, quantizedType);
                }
                case Instruction::FNeg:
                {
                    quantizeSimpleFPInstruction(llvmIrInstruction, quantizedType);
                    break;
                }

//                case Instruction::Add:
//                case Instruction::Sub:
//                case Instruction::Mul:
//                case Instruction::UDiv:
//				case Instruction::SDiv:
//                case Instruction::URem:
//                case Instruction::SRem:
//
//				case Instruction::Shl:
//				case Instruction::LShr:
//				case Instruction::AShr:
//				case Instruction::And:
//				case Instruction::Or:
//				case Instruction::Xor:
//
//                case Instruction::ICmp:

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
                    flexprint(N->Fe, N->Fm, N->Fperr, "\tdidn't support it.\n");
                }

				case Instruction::Ret:
				case Instruction::Switch:
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
	return;
}
}
