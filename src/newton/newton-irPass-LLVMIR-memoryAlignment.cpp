//
// Created by stephen on 15/02/23.
//

/*
	Authored 2022. Stephen Huang.
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

#include "newton-irPass-LLVMIR-memoryAlignment.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/MDBuilder.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DerivedTypes.h"

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

void
memoryAlignment(State * N, BoundInfo * boundInfo, llvm::Function & llvmIrFunction)
{
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
//	printf("<<<<<<<<<< Memeory Alignment >>>>>>>>>>\n\n");
	llvm::Module *module = llvmIrFunction.getParent();
	auto dataLayout = module->getDataLayout();

//	llvmIrFunction.print(llvm::outs());
//	printf("\n");
	for (BasicBlock & llvmIrBasicBlock : llvmIrFunction)
	{
		for (BasicBlock::iterator itBB = llvmIrBasicBlock.begin(); itBB != llvmIrBasicBlock.end();)
		{
			Instruction * llvmIrInstruction = &*itBB++;
			switch (llvmIrInstruction->getOpcode())
			{
//				case Instruction::CmpXchg:
//				case Instruction::Va_Arg:
//				case Instruction::Phi:
				case Instruction::Store:
				{
//					printf("\n> Load\n");
					// cast the general llvm instruction to a specific instruction
					llvm::StoreInst* storeInst = llvm::dyn_cast<llvm::StoreInst>(llvmIrInstruction);

					if (storeInst)
					{
						llvm::Value *storedValue = storeInst->getValueOperand();
						llvm::Type *resultType = storedValue->getType();

						unsigned align = storeInst->getAlignment();
						// if align > 0, that means no align parameter
						if(align > 0)
						{
							// The result type could not be a void type
							if (!resultType->isVoidTy())
							{
								unsigned     resultAlignment = dataLayout.getABITypeAlignment(resultType);

								// if original alignment is not equal to the result alignment, that means it is not correctly aligned
								if (resultAlignment != align)
								{
									// reset the alignment of the instruction
									storeInst->setAlignment(llvm::Align(resultAlignment));
								}
							}

						}
					}

					break;

				}
				case Instruction::Load:
				{
//					printf("\n> Load\n");
					auto vrIt = boundInfo->virtualRegisterRange.find(llvmIrInstruction);
					if (vrIt == boundInfo->virtualRegisterRange.end())
					{
//						printf(">> load break!\n");
						break;
					}

					if(llvmIrInstruction->hasMetadata()){
//						printf(">>> Has MetaData!\n");

						// cast the general llvm instruction to a specific instruction
						llvm::LoadInst* loadInstr = llvm::dyn_cast<llvm::LoadInst>(llvmIrInstruction);
						if (loadInstr)
						{
							unsigned align = loadInstr->getAlignment();
							llvm::Type * resultType	= loadInstr->getType();

							if(align > 0 && !resultType->isVoidTy())
							{
								unsigned     resultAlignment = dataLayout.getABITypeAlignment(resultType);

								// if original alignment is not equal to the result alignment, that means it is not correctly aligned
								if (resultAlignment != align)
								{
									// reset the alignment of the instruction
									loadInstr->setAlignment(llvm::Align(resultAlignment));
								}
							}

						}
						break;

					}
				}

				case Instruction::Alloca:
				{
					llvmIrInstruction->print(llvm::outs());
					llvm::AllocaInst* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(llvmIrInstruction);
					llvm::Type *type = allocaInst->getAllocatedType();
					if (isa<ArrayType>(type)){
						break;
					}
					else if(isa<StructType>(type)){
						StructType *strucTy = dyn_cast<StructType>(type);
						unsigned alignment = dataLayout.getABITypeAlignment(strucTy);
						allocaInst->setAlignment(llvm::Align(alignment));
					}
					break;
				}

				default:
					break;

			}
		}
	}
}
	}




