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

std::map<StringRef, Physics*> vreg_physics_table;

void 
iterateInstructions(Function & F, State * N) 
{
	Physics* physics = NULL;
	N->newtonIrTopScope = commonSymbolTableAllocScope(N);

	for (BasicBlock &BB : F) {

		for (Instruction &I : instructions(F)) {

			switch (I.getOpcode()) {

				case Instruction::Call:

					if (CallInst* CI = dyn_cast<CallInst>(&I)) {

						Function *F = CI->getCalledFunction();
						if (F->getName().startswith("llvm.dbg.declare")) {

							MetadataAsValue *MAV = cast<MetadataAsValue>(CI->getOperand(1));
							DIVariable *Var = cast<DIVariable>(MAV->getMetadata());
							DIDerivedType *Type = cast<DIDerivedType>(Var->getType());

							physics = newtonPhysicsTableAddPhysicsForToken(N, N->newtonIrTopScope, NULL);
							vreg_physics_table[Var->getName()] = physics;
						}
					}
					break;

				case Instruction::FAdd:

					if (BinaryOperator* BO = dyn_cast<BinaryOperator>(&I)) {

						StringRef leftTerm = BO->getOperand(1)->getName();
						StringRef rightTerm = BO->getOperand(2)->getName();

						if (!areTwoPhysicsEquivalent(N, vreg_physics_table[leftTerm], vreg_physics_table[rightTerm]))
							outs() << "leftTerm and rightTerm do not have the same dimensions.\n";
					}
					break;
				case Instruction::FMul:
					if (BinaryOperator* BO = dyn_cast<BinaryOperator>(&I)) {

						StringRef leftTerm = BO->getOperand(1)->getName();
						StringRef rightTerm = BO->getOperand(2)->getName();
						//newtonPhysicsAddExponents(N, termRoot->physics, leftFactor->physics);
					}
					break;
				case Instruction::Load:
					//shallowCopyPhysicsNode
					break;
				case Instruction::Store:
					// Check type of store
					// Check dimensioality
					if (StoreInst *StoreI = dyn_cast<StoreInst>(&I))
						outs() << StoreI->isSimple() << "\n";
					break;
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
						DIDerivedType *Type = cast<DIDerivedType>(Var->getType());
						DIType *BaseType = Type->getBaseType();

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
		getAllVariables(*mi, N);
		iterateInstructions(*mi, N);
	}
}

}
