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

#include "newton-irPass-LLVMIR-constantSubstitution.h"

using namespace llvm;

extern "C"
{
/*
 * Steps of constantSubstitution:
 *  1. for each instruction (that is the case statement), get the range of current instruction from boundInfo
 *  2. check if the lower range and upper range is the same value, then it means this is a constant value instruction
 *  3. get the type of current constant value instruction, mainly float/double/integer (with different bits)
 *  4. use llvm API to create a new constant value
 *  5. substitute current instruction with the constant value
 * */
void constantSubstitution(State * N, BoundInfo * boundInfo, llvm::Function & llvmIrFunction) {
	/*
	 * Some special instructions that need to pay attention:
	 * %i = alloca type, the type of this instruction is "type*"
	 * %i = call retType @func_name (type %p1, ...)
	 * call void @llvm.dbg.declare/value (metadata type %p, ...)
	 * %i = load type, type* %op, the type of this instruction is "type"
	 * %i = gep type, type1* %op1, type2 %op2, (type3 %op3)
	 * %i = castInst type1 %op1 to type2
	 * store type %op1, type* %op2
	 * %.i = phi type [%op1, %bb1], [%op2, %bb2], ...
	 * %i = binary type %op1, %op2
	 * %i = unary type %op
	 * */
	for (BasicBlock & llvmIrBasicBlock : llvmIrFunction) {
		for (BasicBlock::iterator itBB = llvmIrBasicBlock.begin(); itBB != llvmIrBasicBlock.end();) {
			Instruction *llvmIrInstruction = &*itBB++;
			switch (llvmIrInstruction->getOpcode()) {
				case Instruction::Call:
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
				case Instruction::FNeg:
				{
					/*
					 * todo: binary operator is the easiest case, start from here
					 * */
					flexprint(N->Fe, N->Fm, N->Fpinfo, "\tconstantSubstitution::binaryOp: start from here!\n");
					auto vrIt = boundInfo->virtualRegisterRange.find(llvmIrInstruction);
					if (vrIt == boundInfo->virtualRegisterRange.end()) {
						break;
					}
					auto lowerBound = vrIt->second.first;
					auto upperBound = vrIt->second.second;
					/*
					 * if it's a constant
					 * */
					if (fabs(lowerBound - upperBound) < DBL_EPSILON) {
						/*
						 * check the type of instruction
						 * */
						Value* newConstant = nullptr;
						switch (llvmIrInstruction->getType()->getTypeID()) {
							case Type::IntegerTyID:
								/*
								 * todo: further check the bits
								 * */
								break;
							case Type::FloatTyID:
							case Type::DoubleTyID:
								newConstant = ConstantFP::get(llvmIrInstruction->getType(), lowerBound);
								break;
							default:
								break;
						}
						if (newConstant != nullptr) {
							llvmIrInstruction->replaceAllUsesWith(newConstant);
							llvmIrInstruction->removeFromParent();
						}
					}
				}
					break;
				case Instruction::Load:
					break;
				case Instruction::GetElementPtr:
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
					break;
				case Instruction::Store:
					break;
				case Instruction::ICmp:
				case Instruction::FCmp:
					break;
				case Instruction::PHI:
					break;
				case Instruction::Ret:
					break;
				case Instruction::Switch:
					break;
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
