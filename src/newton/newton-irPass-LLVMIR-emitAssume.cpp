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

#include "newton-irPass-LLVMIR-emitAssume.h"

using namespace llvm;

extern "C" {
/*
 * emit a `llvm.assume` after each instruction if its range can be analyzed
 * */
void
emitAssume(State * N, BoundInfo * boundInfo, llvm::Function & llvmIrFunction)
{
	for (BasicBlock & llvmIrBasicBlock : llvmIrFunction)
	{
		for (BasicBlock::iterator itBB = llvmIrBasicBlock.begin(); itBB != llvmIrBasicBlock.end();)
		{
			Instruction * llvmIrInstruction = &*itBB++;
			switch (llvmIrInstruction->getOpcode())
			{
                /*
                 * only emit after the instruction that is not supported by builtin_assume
                 * */
				case Instruction::Call:
                    break;
//				case Instruction::Add:
//				case Instruction::FAdd:
//				case Instruction::Sub:
//				case Instruction::FSub:
//				case Instruction::Mul:
//				case Instruction::FMul:
//				case Instruction::SDiv:
//				case Instruction::FDiv:
//				case Instruction::UDiv:
//				case Instruction::URem:
//				case Instruction::SRem:
//				case Instruction::FRem:
//				case Instruction::Shl:
//				case Instruction::LShr:
//				case Instruction::AShr:
//				case Instruction::And:
//				case Instruction::Or:
//				case Instruction::Xor:
//				case Instruction::FNeg:
//				case Instruction::FPToUI:
//				case Instruction::FPToSI:
//				case Instruction::SIToFP:
//				case Instruction::UIToFP:
				case Instruction::ZExt:
				case Instruction::SExt:
				case Instruction::FPExt:
				case Instruction::Trunc:
				case Instruction::FPTrunc:
				case Instruction::BitCast:
				case Instruction::Load:
				case Instruction::GetElementPtr:
				case Instruction::PHI:
                {
                    auto vrIt = boundInfo->virtualRegisterRange.find(llvmIrInstruction);
                    if (vrIt == boundInfo->virtualRegisterRange.end()) {
                        break;
                    }
                    IRBuilder<> Builder(&llvmIrBasicBlock);
                    /*
                     * Phi node should be at the top of block,
                     * so move back the insert point until it's not a Phi node.
                     * */
                    auto insertPoint = llvmIrInstruction->getNextNode();
                    while (isa<PHINode>(insertPoint))
                    {
                        insertPoint = insertPoint->getNextNode();
                    }
                    Builder.SetInsertPoint(insertPoint);

                    Function* assumeIntrinsic = Intrinsic::getDeclaration(
                            llvmIrFunction.getParent(), Intrinsic::assume
                            );
                    /*
                     * 1. get lower/upper bound from map
                     * 2. check lower bound <= upper bound
                     * 3. icmp for INT comparison; fcmp for FP comparison
                     * */
                    auto lowerBound = vrIt->second.first;
                    auto upperBound = vrIt->second.second;
                    if (lowerBound > upperBound) {
                        /*if this happens, there might be a bug*/
                        break;
                    }

                    /*
                     * The excepted code is:
                     * block_a:
                     * %cmp1 = cmpInst
                     * br i1 %cmp1, label %block_b, label %block_c
                     *
                     * block_b:
                     * %cmp2 = cmpInst
                     * br label %block_c
                     *
                     * block_c:
                     * %v = phi i1 [false, %block_a], [%cmp2, %block_b]
                     * call void @llvm.assume(i1 %v)
                     * */
                    Value *assumeLowerCond, *assumeUpperCond;
                    Type *instType = llvmIrInstruction->getType();
                    /*
                     * skip pointer type
                     * */
                    if (instType->isPointerTy()) {
                        break;
                    }

                    if (std::isnan(lowerBound) || std::isnan(upperBound)) {
                        break;
                    }

                    if (std::isinf(lowerBound) || std::isinf(upperBound)) {
                        break;
                    }

                    if (instType->isFloatTy() || instType->isDoubleTy()) {
                        assumeLowerCond = Builder.CreateFCmpOGE(llvmIrInstruction,
                                                                ConstantFP::get(instType, lowerBound));
                        assumeUpperCond = Builder.CreateFCmpOLE(llvmIrInstruction,
                                                                ConstantFP::get(instType, upperBound));
                    } else {
                        if (lowerBound < 0) {
                            assumeLowerCond = Builder.CreateICmpSGE(llvmIrInstruction,
                                                                    ConstantInt::get(instType, (int)lowerBound, true));
                            assumeUpperCond = Builder.CreateICmpSLE(llvmIrInstruction,
                                                                    ConstantInt::get(instType, (int)upperBound, true));
                        } else {
                            assumeLowerCond = Builder.CreateICmpUGE(llvmIrInstruction,
                                                                    ConstantInt::get(instType, (int)lowerBound, false));
                            assumeUpperCond = Builder.CreateICmpULE(llvmIrInstruction,
                                                                    ConstantInt::get(instType, (int)upperBound, false));
                        }
                    }
                    auto assumeCond = Builder.CreateLogicalAnd(assumeLowerCond, assumeUpperCond);
                    Value* assumeInst = Builder.CreateCall(assumeIntrinsic, assumeCond);
                }
                    break;
				case Instruction::Store:
				case Instruction::ICmp:
				case Instruction::FCmp:
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
