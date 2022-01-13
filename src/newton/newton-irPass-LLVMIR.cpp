/*
	Authored 2021. Nikos Mavrogeorgis.

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

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Pass.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/DebugInfoMetadata.h"

using namespace llvm;


extern "C"
{

#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "version.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "noisy-parser.h"
#include "newton-parser.h"
#include "noisy-lexer.h"
#include "newton-lexer.h"
#include "common-irPass-helpers.h"
#include "common-lexers-helpers.h"
#include "common-irHelpers.h"
#include "common-symbolTable.h"
#include "newton-types.h"
#include "newton-symbolTable.h"
#include "newton-irPass-cBackend.h"
#include "newton-irPass-autoDiff.h"
#include "newton-irPass-estimatorSynthesisBackend.h"
#include "newton-irPass-invariantSignalAnnotation.h"

class PhysicsInfo {
private:
	Physics* physicsType;
	std::vector<PhysicsInfo*> members;
	bool isComposite;
public:
	PhysicsInfo(): physicsType{nullptr}, isComposite{true} {};
	explicit PhysicsInfo(Physics* physics): physicsType{physics}, isComposite{false} {};

	void pushPhysics(Physics* physics) { if (isComposite) members.push_back(new PhysicsInfo(physics)); }
	void pushPhysicsInfo(PhysicsInfo* physics_info) { if (isComposite) members.push_back(physics_info); }

	Physics* get_physics_type() { return physicsType; }
	std::vector<PhysicsInfo *> get_members() { return members; }
	bool get_is_composite() { return isComposite; }
};

std::map<Value*, PhysicsInfo*> virtualRegisterPhysicsTable;

/// Get the physics info of the DIType.
/// If necessary, find the physics name of the subsequent types recursively, e.g. for pointers.
PhysicsInfo*
newtonPhysicsInfo(DIType* debugType, State * N)
{
	if (auto debugInfoDerivedType = dyn_cast<DIDerivedType>(debugType)) {
		switch (debugInfoDerivedType->getTag()) {
			case dwarf::DW_TAG_typedef:
			{
				Physics *physics = newtonPhysicsTablePhysicsForIdentifier(N, N->newtonIrTopScope,
																		  debugInfoDerivedType->getName().data());
				if (!physics)
					return newtonPhysicsInfo(debugInfoDerivedType->getBaseType(), N);
				return new PhysicsInfo{physics};
			}
			case dwarf::DW_TAG_pointer_type:
			case dwarf::DW_TAG_const_type:
			case dwarf::DW_TAG_member:
				return newtonPhysicsInfo(debugInfoDerivedType->getBaseType(), N);
			case dwarf::DW_TAG_structure_type:
			case dwarf::DW_TAG_array_type:
			default:
				errs() << "Unhandled DW_TAG for DIDerivedType\n";
		}
	}
	else if (auto debugInfoCompositeType = dyn_cast<DICompositeType>(debugType)) {
		if (debugInfoCompositeType->getTag() == dwarf::DW_TAG_structure_type) {
			auto physicsInfo = new PhysicsInfo();
			for (auto element: debugInfoCompositeType->getElements())
				if (auto DIMember = dyn_cast<DIDerivedType>(element))
					physicsInfo->pushPhysicsInfo(newtonPhysicsInfo(DIMember, N));
			return physicsInfo;
		}
		else if (debugInfoCompositeType->getTag() == dwarf::DW_TAG_array_type) {
			errs() << "Unhandled DW_TAG for DICompositeType\n";
		}
	}
//	else { TODO
//		errs() << "Unhandled DIType:\n";
//		DebugType->dump();
//		return nullptr;
//	}
	return nullptr;
}

void
printDebugInfoLocation(Instruction* llvmIrInstruction)
{

	auto debugLocation = cast<DILocation>(llvmIrInstruction->getMetadata(0));
	outs() << "Dimension mismatch at: line " << debugLocation->getLine() <<
		", column " << debugLocation->getColumn() << ".\n";
}

Physics*
deepCopyPhysicsNodeWrapper(State *  N, Physics *  physics)
{
	if (physics)
		return deepCopyPhysicsNode(N, physics);
	else
		return nullptr;
}

Physics*
newtonPhysicsAddExponentsWrapper(State *  N, Physics *  left, Physics *  right)
{
	Physics *physicsProduct = deepCopyPhysicsNodeWrapper(N, left);
	if (physicsProduct) {
		newtonPhysicsAddExponents(N, physicsProduct, right);
		return physicsProduct;
	}
	else {
		return nullptr;
	}
}

Physics*
newtonPhysicsSubtractExponentsWrapper(State *  N, Physics *  left, Physics *  right)
{
	Physics *physicsQuotient = deepCopyPhysicsNodeWrapper(N, left);
	if (physicsQuotient) {
		newtonPhysicsSubtractExponents(N, physicsQuotient, right);
		return physicsQuotient;
	}
	else {
		return nullptr;
	}
}

void 
dimensionalityCheck(Function & llvmIrFunction, State * N)
{
	Physics* physics = NULL;

	for (BasicBlock &llvmIrBasicBlock : llvmIrFunction) {

		for (Instruction &llvmIrInstruction : llvmIrBasicBlock) {

			switch (llvmIrInstruction.getOpcode()) {

				case Instruction::Call:
					if (auto llvmIrCallInstruction = dyn_cast<CallInst>(&llvmIrInstruction)) {
						Function *calledFunction = llvmIrCallInstruction->getCalledFunction();
						if (calledFunction->getName().startswith("llvm.dbg.declare")) {
							// Example instruction
							// \code
							// call void @llvm.dbg.declare(metadata double* %accelerationX, metadata !11, metadata !DIExpression()), !dbg !14
							// \endcode

							// Get the 1st operand from llvm.dbg.declare intrinsic.
							// You convert it to `MetadataAsValue` to be able to process it with LLVM `Value` API.
							auto firstOperator = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(0));
							// Extract the metadata from the first operand.
							auto localVariableAddressAsMetadata = cast<ValueAsMetadata>(firstOperator->getMetadata());
							// Finally, get the value contained in the metadata (`double* %accelerationX`).
							auto localVariableAddress = localVariableAddressAsMetadata->getValue();

							// Get the 2nd operand from llvm.dbg.declare intrinsic.
							// You convert it to `MetadataAsValue` as explained above.
							auto secondOperator = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(1));
							// Extract the metadata from the second operand.
							auto debugInfoVariable = cast<DIVariable>(secondOperator->getMetadata());
							// Get the type from the anonymous metadata instance.
							// E.g., from `!11 = !DILocalVariable(name: "accelerationX", scope: !7, file: !1, line: 14, type: !12)`.
							// you get `!12` which looks like:
							// `!12 = !DIDerivedType(tag: DW_TAG_typedef, name: "signalAccelerationX", file: !1, line: 7, baseType: !13)`.
							// Finally, Type->getName() will give "signalAccelerationX", which is the Newton signal type.
							// You also need to convert it to `char *` (Newton does not work with llvm::StringRef).
							if (auto physicsInfo = newtonPhysicsInfo(debugInfoVariable->getType(), N)) {
								// Add the PhysicsInfo to our mapping.
								virtualRegisterPhysicsTable[localVariableAddress] = physicsInfo;
							}
						}
					}
					break;

				case Instruction::Add:
				case Instruction::FAdd:
				case Instruction::Sub:
				case Instruction::FSub:
				case Instruction::And:
				case Instruction::Or:
				case Instruction::Xor:
				case Instruction::ICmp:
				case Instruction::FCmp: {
					PhysicsInfo *leftTerm = virtualRegisterPhysicsTable[llvmIrInstruction.getOperand(0)];
					PhysicsInfo *rightTerm = virtualRegisterPhysicsTable[llvmIrInstruction.getOperand(1)];
					Physics *physicsSum;
					if (!leftTerm) {
						if (!rightTerm) {
							break;
						}
						physicsSum = rightTerm->get_physics_type();
					} else if (!rightTerm) {
						physicsSum = leftTerm->get_physics_type();
					} else {
						if (!areTwoPhysicsEquivalent(N, leftTerm->get_physics_type(),
													 rightTerm->get_physics_type())) {
							printDebugInfoLocation(&llvmIrInstruction);
							exit(1);
						}
						physicsSum = leftTerm->get_physics_type();
					}
					virtualRegisterPhysicsTable[&llvmIrInstruction] = new PhysicsInfo{physicsSum};
					break;
				}

				case Instruction::Mul:
				case Instruction::FMul:
					if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction)) {
						PhysicsInfo *leftTerm = virtualRegisterPhysicsTable[llvmIrBinaryOperator->getOperand(0)];
						PhysicsInfo *rightTerm = virtualRegisterPhysicsTable[llvmIrBinaryOperator->getOperand(1)];
						Physics* physicsProduct;
						if (!leftTerm) {
							if (!rightTerm) {
								break;
							}
							physicsProduct = rightTerm->get_physics_type();
						}
						else if (!rightTerm) {
							physicsProduct = leftTerm->get_physics_type();
						}
						else {
							physicsProduct = newtonPhysicsAddExponentsWrapper(N,
																			  leftTerm->get_physics_type(),
																			  rightTerm->get_physics_type());
						}
						 // Store the result to the destination virtual register.
						virtualRegisterPhysicsTable[llvmIrBinaryOperator] = new PhysicsInfo{physicsProduct};
					}
					break;

				case Instruction::SDiv:
				case Instruction::FDiv:
				case Instruction::UDiv:
				case Instruction::URem:
				case Instruction::SRem:
				case Instruction::FRem:
					if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction)) {
						PhysicsInfo *leftTerm = virtualRegisterPhysicsTable[llvmIrBinaryOperator->getOperand(0)];
						PhysicsInfo *rightTerm = virtualRegisterPhysicsTable[llvmIrBinaryOperator->getOperand(1)];
						Physics* physicsProduct;
						if (!leftTerm) {
							if (!rightTerm) {
								break;
							}
							physicsProduct = rightTerm->get_physics_type();
						}
						else if (!rightTerm) {
							physicsProduct = leftTerm->get_physics_type();
						}
						else {
							physicsProduct = newtonPhysicsSubtractExponentsWrapper(N,
																				   leftTerm->get_physics_type(),
																				   rightTerm->get_physics_type());
						}
						// Store the result to the destination virtual register.
						virtualRegisterPhysicsTable[llvmIrBinaryOperator] = new PhysicsInfo{physicsProduct};
					}
					break;

				case Instruction::Load: // TODO: not all loads should have this
					if (auto llvmIrLoadInstruction = dyn_cast<LoadInst>(&llvmIrInstruction)) {
						virtualRegisterPhysicsTable[llvmIrLoadInstruction] = virtualRegisterPhysicsTable[llvmIrLoadInstruction->getOperand(0)];
					}
					break;

				case Instruction::Store:
					if (auto llvmIrStoreInstruction = dyn_cast<StoreInst>(&llvmIrInstruction)) {
						Value *leftTerm = llvmIrStoreInstruction->getOperand(0);
						Value *rightTerm = llvmIrStoreInstruction->getOperand(1);
						PhysicsInfo *leftPhysicsInfo = virtualRegisterPhysicsTable[leftTerm];
						PhysicsInfo *rightPhysicsInfo = virtualRegisterPhysicsTable[rightTerm];
						if (!leftPhysicsInfo) // E.g. in number assignment to a newton signal
							break;
						if (!rightPhysicsInfo) {
							virtualRegisterPhysicsTable[rightTerm] = virtualRegisterPhysicsTable[leftTerm];
							break;
						}
						if (!areTwoPhysicsEquivalent(N, leftPhysicsInfo->get_physics_type(), rightPhysicsInfo->get_physics_type())) {
							printDebugInfoLocation(llvmIrStoreInstruction);
							exit(1);
						}
					}
					break;

				case Instruction::GetElementPtr:
					if (auto llvmIrGetElementPointerInstruction = dyn_cast<GetElementPtrInst>(&llvmIrInstruction)) {
						uint64_t Index;
						if (auto llvmIrConstantInt = dyn_cast<ConstantInt>(llvmIrGetElementPointerInstruction->getOperand(2))) {
							Index = llvmIrConstantInt->getZExtValue();
						}
						auto physicsInfo = virtualRegisterPhysicsTable[llvmIrGetElementPointerInstruction->getPointerOperand()]->get_members()[Index];
						virtualRegisterPhysicsTable[llvmIrGetElementPointerInstruction] = physicsInfo;
					}
					break;

				case Instruction::SExt:
				case Instruction::ZExt:
				case Instruction::AShr:
				case Instruction::LShr:
				case Instruction::Shl:
				case Instruction::Trunc:
				case Instruction::SIToFP:
				case Instruction::FNeg:
				case Instruction::FPToUI:
				case Instruction::FPToSI:
				case Instruction::UIToFP:
				case Instruction::FPTrunc:
				case Instruction::FPExt:
				case Instruction::PtrToInt:
				case Instruction::IntToPtr:
				case Instruction::BitCast:
				case Instruction::AddrSpaceCast:
				case Instruction::ExtractElement:
					virtualRegisterPhysicsTable[&llvmIrInstruction] = virtualRegisterPhysicsTable[llvmIrInstruction.getOperand(0)];
					break;

				case Instruction::PHI:
				case Instruction::Select: {
					PhysicsInfo *leftTerm = virtualRegisterPhysicsTable[llvmIrInstruction.getOperand(0)];
					PhysicsInfo *rightTerm = virtualRegisterPhysicsTable[llvmIrInstruction.getOperand(1)];
					Physics* physicsPhiNode;
					if (!leftTerm) {
						if (!rightTerm) {
							break;
						}
						physicsPhiNode = rightTerm->get_physics_type();
					}
					else if (!rightTerm) {
						physicsPhiNode = leftTerm->get_physics_type();
					}
					else {
						if (!areTwoPhysicsEquivalent(N, leftTerm->get_physics_type(),
													 rightTerm->get_physics_type())) {

							auto debugLocation = cast<DILocation>(llvmIrInstruction.getMetadata(0));
							errs() << "Warning, cannot deduce physics type at: line " << debugLocation->getLine() <<
								   ", column " << debugLocation->getColumn() << ".\n";
						}
						physicsPhiNode = leftTerm->get_physics_type();
					}
					virtualRegisterPhysicsTable[&llvmIrInstruction] = new PhysicsInfo{physicsPhiNode};
				}
					break;

				// https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/IR/Instruction.def

				// vector/aggregate related
				case Instruction::InsertElement:
				case Instruction::ShuffleVector:
				case Instruction::ExtractValue:
				case Instruction::InsertValue:
					llvmIrInstruction.dump();
					errs() << "Unsupported LLVM IR Instruction!\n";

				// Terminator Instructions
				// These instructions are used to terminate a basic block of the program.
				// Every basic block must end with one of these instructions for it to be a well-formed basic block.
				case Instruction::Ret:
				case Instruction::Br:
				case Instruction::Switch:
				case Instruction::IndirectBr:
				case Instruction::Invoke:
				case Instruction::Resume:
				case Instruction::Unreachable:
				case Instruction::CleanupRet:
				case Instruction::CatchRet:
				case Instruction::CatchSwitch:
				case Instruction::CallBr: // A call-site terminator

				// Memory operators
				case Instruction::Alloca:
				case Instruction::Fence:
				case Instruction::AtomicCmpXchg:
				case Instruction::AtomicRMW:

				// Other operators
				case Instruction::LandingPad:

				default:
					continue;
			}
		}
	}
}


void
irPassLLVMIR(State * N)
{
	if (N->llvmIR == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Please specify the LLVM IR input file\n");
		fatal(N, Esanity);
	}

	SMDiagnostic Err;
	LLVMContext Context;
	std::unique_ptr<Module> Mod(parseIRFile(N->llvmIR, Err, Context));
	if (!Mod) {
		flexprint(N->Fe, N->Fm, N->Fperr, "Error: Couldn't parse IR file.");
		fatal(N, Esanity);
	}

	for (Module::iterator mi = Mod->begin(); mi != Mod->end(); mi++) {
		if (mi->getName().str() == std::string("calc_humidity"))
			dimensionalityCheck(*mi, N);
	}
}

}