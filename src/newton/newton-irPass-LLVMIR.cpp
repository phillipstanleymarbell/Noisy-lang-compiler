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

std::map<Value*, PhysicsInfo*> vreg_physics_table;

/// Get the physics info of the DIType.
/// If necessary, find the physics name of the subsequent types recursively, e.g. for pointers.
PhysicsInfo*
newtonPhysicsInfo(DIType* DebugType, State * N)
{
    if (auto Type = dyn_cast<DIDerivedType>(DebugType)) {
        switch (Type->getTag()) {
            case dwarf::DW_TAG_typedef:
            {
                Physics *physics = newtonPhysicsTablePhysicsForIdentifier(N, N->newtonIrTopScope,
                                                                          Type->getName().data());
                if (!physics)
                    return newtonPhysicsInfo(Type->getBaseType(), N);
                return new PhysicsInfo{physics};
            }
            case dwarf::DW_TAG_pointer_type:
            case dwarf::DW_TAG_const_type:
            case dwarf::DW_TAG_member:
                return newtonPhysicsInfo(Type->getBaseType(), N);
            case dwarf::DW_TAG_structure_type:
            default:
                errs() << "Unhandled DW_TAG\n";
        }
    }
    else if (auto CType = dyn_cast<DICompositeType>(DebugType)) {
        if (CType->getTag() == dwarf::DW_TAG_structure_type) {
            auto physicsInfo = new PhysicsInfo();
            for (auto i: CType->getElements())
                if (auto DIMember = dyn_cast<DIDerivedType>(i))
                    physicsInfo->pushPhysicsInfo(newtonPhysicsInfo(DIMember, N));
            return physicsInfo;
        }
    }
//    else { TODO
//        errs() << "Unhandled DIType:\n";
//        DebugType->dump();
//        return nullptr;
//    }
    return nullptr;
}

void 
dimensionalityCheck(Function & F, State * N)
{
	Physics* physics = NULL;

	for (BasicBlock &BB : F) {

		for (Instruction &I : instructions(F)) {

            outs() << I << "\n";
            switch (I.getOpcode()) {

                case Instruction::Call:
                    if (auto CI = dyn_cast<CallInst>(&I)) {
                        Function *CalledFunc = CI->getCalledFunction();
                        if (CalledFunc->getName().startswith("llvm.dbg.declare")) {
                            // Example instruction
                            // \code
                            // call void @llvm.dbg.declare(metadata double* %accelerationX, metadata !11, metadata !DIExpression()), !dbg !14
                            // \endcode

                            // Get the 1st operand from llvm.dbg.declare intrinsic.
                            // You convert it to `MetadataAsValue` to be able to process it with LLVM `Value` API.
                            auto FirstOp = cast<MetadataAsValue>(CI->getOperand(0));
                            // Extract the metadata from the first operand.
                            auto LocalVarAddrAsMetadata = cast<ValueAsMetadata>(FirstOp->getMetadata());
                            // Finally, get the value contained in the metadata (`double* %accelerationX`).
                            auto LocalVarAddr = LocalVarAddrAsMetadata->getValue();

                            // Get the 2nd operand from llvm.dbg.declare intrinsic.
                            // You convert it to `MetadataAsValue` as explained above.
                            auto SecondOp = cast<MetadataAsValue>(CI->getOperand(1));
                            // Extract the metadata from the second operand.
                            auto DIVar = cast<DIVariable>(SecondOp->getMetadata());
                            // Get the type from the anonymous metadata instance.
                            // E.g., from `!11 = !DILocalVariable(name: "accelerationX", scope: !7, file: !1, line: 14, type: !12)`.
                            // you get `!12` which looks like:
                            // `!12 = !DIDerivedType(tag: DW_TAG_typedef, name: "signalAccelerationX", file: !1, line: 7, baseType: !13)`.
                            // Finally, Type->getName() will give "signalAccelerationX", which is the Newton signal type.
                            // You also need to convert it to `char *` (Newton does not work with llvm::StringRef).
                            if (auto physicsInfo = newtonPhysicsInfo(DIVar->getType(), N)) {
                                // Add the PhysicsInfo to our mapping.
                                vreg_physics_table[LocalVarAddr] = physicsInfo;
                            }
                        }
                    }
                    break;

                case Instruction::FAdd:
                    if (auto BO = dyn_cast<BinaryOperator>(&I)) {
                        Value *leftTerm = BO->getOperand(0);
                        Value *rightTerm = BO->getOperand(1);
                        if (!areTwoPhysicsEquivalent(N, vreg_physics_table[leftTerm]->get_physics_type(),
                                                     vreg_physics_table[rightTerm]->get_physics_type())) {
                            outs() << "Dimension mismatch in addition operands.\n";
                            exit(1);
                        }
                        else
                            vreg_physics_table[BO] = vreg_physics_table[leftTerm];
                    }
                    break;

                case Instruction::FMul:
                    if (auto BO = dyn_cast<BinaryOperator>(&I)) {
                        Value *leftTerm = BO->getOperand(0);
                        Value *rightTerm = BO->getOperand(1);
                        // `newtonPhysicsAddExponents1 adds the right argument to the left,
                        // so we first create a new copy for our new Physics type.
                        Physics *physicsProduct = deepCopyPhysicsNode(N, vreg_physics_table[leftTerm]->get_physics_type());
                        newtonPhysicsAddExponents(N, physicsProduct, vreg_physics_table[rightTerm]->get_physics_type());
                        // Store the result to the destination virtual register.
                        vreg_physics_table[BO] = new PhysicsInfo{physicsProduct};
                    }
                    break;

                case Instruction::FDiv:
                    if (auto BO = dyn_cast<BinaryOperator>(&I)) {
                        Value *leftTerm = BO->getOperand(0);
                        Value *rightTerm = BO->getOperand(1);
                        // `newtonPhysicsSubtractExponents1 adds the right argument from the left,
                        // so we first create a new copy for our new Physics type.
                        Physics *physicsProduct = deepCopyPhysicsNode(N, vreg_physics_table[leftTerm]->get_physics_type());
                        newtonPhysicsSubtractExponents(N, physicsProduct, vreg_physics_table[rightTerm]->get_physics_type());
                    }
                    break;

                case Instruction::Load: // TODO: not all loads should have this
                    if (auto LI = dyn_cast<LoadInst>(&I)) {
                        vreg_physics_table[LI] = vreg_physics_table[LI->getOperand(0)];
                    }
                    break;

                case Instruction::Store:
                    if (auto StoreI = dyn_cast<StoreInst>(&I)) {
                        Value *leftTerm = StoreI->getOperand(0);
                        Value *rightTerm = StoreI->getOperand(1);
                        PhysicsInfo *leftPhysicsInfo = vreg_physics_table[leftTerm];
                        PhysicsInfo *rightPhysicsInfo = vreg_physics_table[rightTerm];
                        if (!leftPhysicsInfo) // E.g. in number assignment to a newton signal
                            break;
                        if (!rightPhysicsInfo) {
                            vreg_physics_table[rightTerm] = vreg_physics_table[leftTerm];
                            break;
                        }
                        if (!areTwoPhysicsEquivalent(N, leftPhysicsInfo->get_physics_type(), rightPhysicsInfo->get_physics_type())) {
                            outs() << "Dimension mismatch in assignment.\n";
                            exit(1);
                        }
                    }
					break;

                case Instruction::GetElementPtr:
                    if (auto GEPI = dyn_cast<GetElementPtrInst>(&I)) {
                        uint64_t Idx;
                        if (auto CI = dyn_cast<ConstantInt>(GEPI->getOperand(2))) {
                            Idx = CI->getZExtValue();
                        }
                        auto physicsInfo = vreg_physics_table[GEPI->getPointerOperand()]->get_members()[Idx];
                        vreg_physics_table[GEPI] = physicsInfo;
                    }
                    break;


                // https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/IR/Instruction.def

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

                // Standard unary operators
                case Instruction::FNeg:

                // Standard binary operators
                case Instruction::Add:
                case Instruction::Sub:
                case Instruction::FSub:
                case Instruction::Mul:
                case Instruction::UDiv:
                case Instruction::SDiv:
                case Instruction::URem:
                case Instruction::SRem:
                case Instruction::FRem:

                // Logical operators (integer operands)
                case Instruction::Shl:
                case Instruction::LShr:
                case Instruction::AShr:
                case Instruction::And:
                case Instruction::Or:
                case Instruction::Xor:

                // Memory operators...
                case Instruction::Alloca:
                case Instruction::Fence:
                case Instruction::AtomicCmpXchg:
                case Instruction::AtomicRMW:

                // Cast operators ...
                case Instruction::Trunc:
                case Instruction::ZExt:
                case Instruction::SExt:
                case Instruction::FPToUI:
                case Instruction::FPToSI:
                case Instruction::UIToFP:
                case Instruction::SIToFP:
                case Instruction::FPTrunc:
                case Instruction::FPExt:
                case Instruction::PtrToInt:
                case Instruction::IntToPtr:
                case Instruction::BitCast:
                case Instruction::AddrSpaceCast:

                // Other operators...
                case Instruction::ICmp:
                case Instruction::FCmp:
                case Instruction::PHI:
                case Instruction::Select:
                case Instruction::UserOp1:
                case Instruction::UserOp2:
                case Instruction::VAArg:
                case Instruction::ExtractElement:
                case Instruction::InsertElement:
                case Instruction::ShuffleVector:
                case Instruction::ExtractValue:
                case Instruction::InsertValue:
                case Instruction::LandingPad:
				default:
					continue;
			}
		}
	}
}


void 
getAllVariables(Function & F, State * N) 
{
	for (Function::iterator BB = F.begin(), E = F.end(); BB!=E; ++BB) {

		for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {

			//get the Metadata declared in the llvm intrinsic functions such as llvm.dbg.declare()
			if (CallInst* CI = dyn_cast<CallInst>(I)) {

				if (Function *F = CI->getCalledFunction()) {

					if (F->getName().startswith("llvm.")) {

						outs() << "===========================================================\n";

						MetadataAsValue *MAV = cast<MetadataAsValue>(CI->getOperand(1));
						DIVariable *Var = cast<DIVariable>(MAV->getMetadata());
						//DIDerivedType *Type = cast<DIDerivedType>(Var->getType());
						//DIType *BaseType = Type->getBaseType();

						DIType *Type = Var->getType();
						std::string temp(Type->getName());
						char *cstr = &temp[0];

						Signal * signal = NULL;
						attachSignalsToParameterNodes(N);
						signal = findKthSignalByIdentifier(N, cstr, 0);

						if (signal)
							outs() << "Found signal: " << cstr << " as: " 
								<< signal->invariantExpressionIdentifier << " with physics id:"
								<< signal->baseNode->physics << " in invariant\n"; // TODO
					}
				}
			}
			else if (BinaryOperator* BO = dyn_cast<BinaryOperator>(I)) {
				outs() << *(BO->getOperand(1)) << "\n";
				outs() << BO->getOperand(1)->getName() << "\n";
			}
		}
	}
}


void 
getAllMDNFunc(Function & F) 
{
	for (Function::iterator BB = F.begin(), E = F.end(); BB!=E; ++BB) {

		for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ++I) {

			SmallVector<std::pair<unsigned, MDNode*>, 4> MDForInst;

			//Get all the mdnodes attached to each instruction
			I->getAllMetadata(MDForInst);
			for (auto &MD : MDForInst) {
				outs() << "\n+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++\n";
				if (MDNode *N = MD.second) {
					outs() << *N << "\n";
				}
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
//		getAllVariables(*mi, N); // not needed for now
//        if (mi->getName().str() == std::string("calc_humidity"))
            dimensionalityCheck(*mi, N);
	}
}

}
