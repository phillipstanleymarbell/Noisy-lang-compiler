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

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>

#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/Support/Path.h"
#include "llvm/Support/JSON.h"

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


enum DefaultConstantsE {
	kMaxDimensions			= 1024
} DefaultConstants;

class PhysicsInfo {
private:
	Physics *	physicsType;
	std::vector<PhysicsInfo *> members;
	bool composite;
public:
	PhysicsInfo(): physicsType{nullptr}, composite{true} {};
	explicit PhysicsInfo(Physics *  physics): physicsType{physics}, composite{false} {};

	void pushPhysicsInfo(PhysicsInfo *  physics_info) { if (composite) { members.push_back(physics_info); } }
	void insertPhysicsInfoAt(PhysicsInfo *  physics_info, uint64_t index) { if (composite) { members.at(index) = physics_info; } }

	bool isComposite() const { return composite; }
	Physics* getPhysicsType() { return physicsType; }
	std::vector<PhysicsInfo *> get_members() { return members; }
};

std::map<Value *, PhysicsInfo *> virtualRegisterPhysicsTable;
std::map<Value *, Value *> virtualRegisterIdentifier;
std::map<StringRef, PhysicsInfo *> sourceVariablePhysicsTable;

/*
 *	Get the physics info of the DIType.
 *	If necessary, find the physics name of the subsequent types recursively, e.g. for pointers.
 */
PhysicsInfo *
newtonPhysicsInfo(DIType *  debugType, State *  N)
{
	if (auto debugInfoDerivedType = dyn_cast<DIDerivedType>(debugType))
	{
		switch (debugInfoDerivedType->getTag())
		{
			case dwarf::DW_TAG_typedef:
			{
				Physics *	physics = newtonPhysicsTablePhysicsForIdentifier(N, N->newtonIrTopScope,
																		  debugInfoDerivedType->getName().data());
				if (!physics)
				{
					return newtonPhysicsInfo(debugInfoDerivedType->getBaseType(), N);
				}
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
	else if (auto debugInfoCompositeType = dyn_cast<DICompositeType>(debugType))
	{
		if (debugInfoCompositeType->getTag() == dwarf::DW_TAG_structure_type)
		{
			auto	physicsInfo = new PhysicsInfo();
			for (auto element: debugInfoCompositeType->getElements())
			{
				if (auto DIMember = dyn_cast<DIDerivedType>(element))
				{
					physicsInfo->pushPhysicsInfo(newtonPhysicsInfo(DIMember, N));
				}
			}
			return physicsInfo;
		}
		else if (debugInfoCompositeType->getTag() == dwarf::DW_TAG_array_type)
		{
			/*
			 * Example:
			 * !12 = !DICompositeType(tag: DW_TAG_array_type, baseType: !13, size: 128, elements: !14)
			 * !13 = !DIBasicType(name: "double", size: 64, encoding: DW_ATE_float)
			 * !14 = !{!15}
			 * !15 = !DISubrange(count: 2)
			 */
			auto	physicsInfo = new PhysicsInfo();
			auto	arrayPhysicsInfo = newtonPhysicsInfo(debugInfoCompositeType->getBaseType(), N);
			auto	element = debugInfoCompositeType->getElements()[0];
			if (auto debugInfoSubrange = dyn_cast<DISubrange>(element))
			{
				auto	countPointerUnion = debugInfoSubrange->getCount();
				auto	countConstantIntPointer = countPointerUnion.get<ConstantInt*>();
				auto	elementCount = countConstantIntPointer->getZExtValue();

				for (unsigned index = 0; index < elementCount; index++)
				{
					physicsInfo->pushPhysicsInfo(arrayPhysicsInfo);
				}
			}
			return physicsInfo;
		}
	}
	return nullptr;
}

void
printDebugInfoLocation(Instruction *  llvmIrInstruction, Physics *  left, Physics *  right)
{
	auto	debugLocation = cast<DILocation>(llvmIrInstruction->getMetadata(0));
	outs() << "Dimension mismatch at: line " << debugLocation->getLine() <<
		", column " << debugLocation->getColumn() << ".\n";
	outs() << "Left-hand side: " << left->identifier << "\n";
	outs() << "Right-hand side: " << right->identifier << "\n";
}

Physics *
deepCopyPhysicsNodeWrapper(State *  N, Physics *  physics)
{
	if (physics)
	{
		return deepCopyPhysicsNode(N, physics);
	}
	else
	{
		return nullptr;
	}
}

Physics *
newtonPhysicsAddExponentsWrapper(State *  N, Physics *  left, Physics *  right)
{
	Physics *	physicsProduct = deepCopyPhysicsNodeWrapper(N, left);
	if (physicsProduct)
	{
		newtonPhysicsAddExponents(N, physicsProduct, right);
		return physicsProduct;
	}
	else
	{
		return nullptr;
	}
}

Physics *
newtonPhysicsSubtractExponentsWrapper(State *  N, Physics *  left, Physics *  right)
{
	Physics *	physicsQuotient = deepCopyPhysicsNodeWrapper(N, left);
	if (physicsQuotient)
	{
		newtonPhysicsSubtractExponents(N, physicsQuotient, right);
		return physicsQuotient;
	}
	else
	{
		return nullptr;
	}
}

void
dumpPhysicsInfoJSON(json::OStream &jsonOStream, StringRef name, PhysicsInfo* physicsInfo)
{
	if (physicsInfo->isComposite()) {
		jsonOStream.attributeBegin(name);
		jsonOStream.arrayBegin();
		for (auto &member : physicsInfo->get_members()) {
			dumpPhysicsInfoJSON(jsonOStream, "", member);
		}
		jsonOStream.arrayEnd();
		jsonOStream.attributeEnd();
	}
	else if (!name.empty())
		jsonOStream.attribute(name, physicsInfo->getPhysicsType()->identifier);
	else
		jsonOStream.value(physicsInfo->getPhysicsType()->identifier);
}

void
dimensionalityCheck(Function &  llvmIrFunction, State *  N, FunctionCallee  runtimeCheckFunction, FunctionCallee initNewtonRuntime, FunctionCallee newtonInsert, FunctionCallee newtonCheckDimensions)
{
	if (!llvmIrFunction.empty())
	{
		Instruction *	firstIrInstruction = llvmIrFunction.begin()->getFirstNonPHI();
		IRBuilder<> 	builder(firstIrInstruction);
		builder.CreateCall(initNewtonRuntime, {});
	}

	for (BasicBlock &  llvmIrBasicBlock : llvmIrFunction)
	{
		for (Instruction & 	llvmIrInstruction : llvmIrBasicBlock)
		{
			switch (llvmIrInstruction.getOpcode())
			{
				/*
				 *	This is where metadata are connected to the virtual registers.
				 *	Using metadata we can get the typedef information related to Newton signals.
			     *	Example instruction
			     *	\code
			     *	call void @llvm.dbg.declare(metadata double* %accelerationX, metadata !11, metadata !DIExpression()), !dbg !14
			     *	\endcode
			     *	Get the 1st operand from llvm.dbg.declare intrinsic.
			     *	Convert it to `MetadataAsValue` to be able to process it with LLVM `Value` API.
			     *	Extract the metadata from the first operand.
			     *	Get the value contained in the metadata (`double* %accelerationX`).
			     *	Get the 2nd operand from llvm.dbg.declare intrinsic.
			     *	You convert it to `MetadataAsValue` as explained above.
			     *	Extract the metadata from the second operand.
			     *	Get the type from the anonymous metadata instance.
			     *	E.g., from `!11 = !DILocalVariable(name: "accelerationX", scope: !7, file: !1, line: 14, type: !12)`.
			     *	you get `!12` which looks like:
			     *	`!12 = !DIDerivedType(tag: DW_TAG_typedef, name: "signalAccelerationX", file: !1, line: 7, baseType: !13)`.
			     *	Finally, Type->getName() will give "signalAccelerationX", which is the Newton signal type.
			     *	You also need to convert it to `char *` (Newton does not work with llvm::StringRef).
			     *	Finally, add the PhysicsInfo to our mapping.
				 */
				case Instruction::Call:
					if (auto llvmIrCallInstruction = dyn_cast<CallInst>(&llvmIrInstruction))
					{
						Function *	calledFunction = llvmIrCallInstruction->getCalledFunction();
						if (calledFunction->getName().startswith("llvm.dbg.declare"))
						{
							auto	firstOperator = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(0));
							auto	localVariableAddressAsMetadata = cast<ValueAsMetadata>(firstOperator->getMetadata());
							auto	localVariableAddress = localVariableAddressAsMetadata->getValue();

							auto	secondOperator = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(1));
							auto	debugInfoVariable = cast<DIVariable>(secondOperator->getMetadata());

							if (auto  physicsInfo = newtonPhysicsInfo(debugInfoVariable->getType(), N))
							{
								virtualRegisterPhysicsTable[localVariableAddress] = physicsInfo;
								sourceVariablePhysicsTable[debugInfoVariable->getName()] = physicsInfo;

								if (!physicsInfo->isComposite())
								{
									auto	dimensionIterator = physicsInfo->getPhysicsType()->dimensions;
									IRBuilder<> 	builder(&llvmIrInstruction);
									Type *	argumentType = runtimeCheckFunction.getFunctionType()->getParamType(0);
									Type *	pointerType = runtimeCheckFunction.getFunctionType()->getParamType(1);
									Type *	Int64Type = llvm::IntegerType::getInt64Ty(builder.getContext());

									auto element_size = llvm::ConstantInt::get(Int64Type, sizeof(int64_t));
									auto array_size = llvm::ConstantInt::get(Int64Type, kMaxDimensions);
									auto alloc_size = llvm::ConstantExpr::getMul(element_size, array_size);

									Instruction* Malloc = CallInst::CreateMalloc(&llvmIrInstruction,
																				 Int64Type, Int64Type->getPointerTo(), alloc_size,
																				 nullptr, nullptr, "");
									auto	pointerIndex = builder.CreatePointerCast(Malloc, pointerType);
									Value *	locationPointer;
									int64_t exponent;
									for (int64_t i = 0; dimensionIterator; dimensionIterator = dimensionIterator->next, i++)
									{
										exponent = (int64_t)dimensionIterator->exponent;
										if (exponent)
										{
											locationPointer = builder.CreateGEP(pointerIndex, llvm::ConstantInt::get(Int64Type, i));
											builder.CreateStore(llvm::ConstantInt::get(Int64Type, exponent), locationPointer);
										}
									}
									auto	symbolNumber = builder.CreateCall(newtonInsert, {pointerIndex});
									virtualRegisterIdentifier[localVariableAddress] = symbolNumber;
								}
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
					PhysicsInfo *	leftTerm = virtualRegisterPhysicsTable[llvmIrInstruction.getOperand(0)];
					PhysicsInfo *	rightTerm = virtualRegisterPhysicsTable[llvmIrInstruction.getOperand(1)];
					Value *		leftIdentifier = virtualRegisterIdentifier[llvmIrInstruction.getOperand(0)];
					Value *		rightIdentifier = virtualRegisterIdentifier[llvmIrInstruction.getOperand(1)];
					Physics *	physicsSum;
					if (leftIdentifier && rightIdentifier)
					{
						IRBuilder<> 	builder(&llvmIrInstruction);
						Type *	argumentType = newtonCheckDimensions.getFunctionType()->getParamType(0);
						auto	leftIndex = builder.CreateIntCast(leftIdentifier, argumentType, true);
						auto	rightIndex = builder.CreateIntCast(rightIdentifier, argumentType, true);
						builder.CreateCall(newtonCheckDimensions, {leftIndex, rightIndex});
					}
					if (!leftTerm)
					{
						if (!rightTerm)
						{
							break;
						}
						physicsSum = rightTerm->getPhysicsType();
					}
					else if (!rightTerm)
					{
						physicsSum = leftTerm->getPhysicsType();
					}
					else
					{
						if (!areTwoPhysicsEquivalent(N, leftTerm->getPhysicsType(),
													 rightTerm->getPhysicsType()))
						{
							printDebugInfoLocation(&llvmIrInstruction,
												   leftTerm->getPhysicsType(), rightTerm->getPhysicsType());
							exit(1);
						}
						physicsSum = leftTerm->getPhysicsType();
					}
					virtualRegisterPhysicsTable.insert({&llvmIrInstruction, new PhysicsInfo{physicsSum}});
					break;
				}

				case Instruction::Mul:
				case Instruction::FMul:
					if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
					{
						PhysicsInfo *	leftTerm = virtualRegisterPhysicsTable[llvmIrBinaryOperator->getOperand(0)];
						PhysicsInfo *	rightTerm = virtualRegisterPhysicsTable[llvmIrBinaryOperator->getOperand(1)];
						Physics * 	physicsProduct;
						if (!leftTerm)
						{
							if (!rightTerm)
							{
								break;
							}
							physicsProduct = rightTerm->getPhysicsType();
						}
						else if (!rightTerm)
						{
							physicsProduct = leftTerm->getPhysicsType();
						}
						else
						{
							physicsProduct = newtonPhysicsAddExponentsWrapper(N,
																			  leftTerm->getPhysicsType(),
																			  rightTerm->getPhysicsType());
						}
						/*
						 *	Store the result to the destination virtual register.
						 */
						virtualRegisterPhysicsTable.insert({llvmIrBinaryOperator, new PhysicsInfo{physicsProduct}});
					}
					break;

				case Instruction::SDiv:
				case Instruction::FDiv:
				case Instruction::UDiv:
				case Instruction::URem:
				case Instruction::SRem:
				case Instruction::FRem:
					if (auto llvmIrBinaryOperator = dyn_cast<BinaryOperator>(&llvmIrInstruction))
					{
						PhysicsInfo *	leftTerm = virtualRegisterPhysicsTable[llvmIrBinaryOperator->getOperand(0)];
						PhysicsInfo *	rightTerm = virtualRegisterPhysicsTable[llvmIrBinaryOperator->getOperand(1)];
						Physics * 	physicsProduct;
						if (!leftTerm)
						{
							if (!rightTerm)
							{
								break;
							}
							physicsProduct = rightTerm->getPhysicsType();
						}
						else if (!rightTerm)
						{
							physicsProduct = leftTerm->getPhysicsType();
						}
						else
						{
							physicsProduct = newtonPhysicsSubtractExponentsWrapper(N,
																				   leftTerm->getPhysicsType(),
																				   rightTerm->getPhysicsType());
						}
						/*
						 *	Store the result to the destination virtual register.
						 */
						virtualRegisterPhysicsTable.insert({llvmIrBinaryOperator, new PhysicsInfo{physicsProduct}});
					}
					break;

				/*
				 * 	Some load instruction may not need this.
				 * 	Need to examine that. TODO
				 */
				case Instruction::Load:
					if (auto llvmIrLoadInstruction = dyn_cast<LoadInst>(&llvmIrInstruction))
					{
						virtualRegisterPhysicsTable.insert({llvmIrLoadInstruction, virtualRegisterPhysicsTable[llvmIrLoadInstruction->getOperand(0)]});
						virtualRegisterIdentifier.insert({llvmIrLoadInstruction, virtualRegisterIdentifier[llvmIrLoadInstruction->getOperand(0)]});
					}
					break;

				case Instruction::Store:
					if (auto llvmIrStoreInstruction = dyn_cast<StoreInst>(&llvmIrInstruction))
					{
						Value *			leftTerm = llvmIrStoreInstruction->getOperand(0);
						Value *			rightTerm = llvmIrStoreInstruction->getOperand(1);
						PhysicsInfo *	leftPhysicsInfo = virtualRegisterPhysicsTable[leftTerm];
						PhysicsInfo *	rightPhysicsInfo = virtualRegisterPhysicsTable[rightTerm];
						/*
						 *	This case arises when we assign a number to a Newton signal.
						 */
						if (!leftPhysicsInfo)
						{
							break;
						}
						if (!rightPhysicsInfo)
						{
							/*
							 * If the lvalue refers to a struct or array element and has no physics type,
							 * we must update its physicsInfo with the assigned type.
							 * If it had a type then we wouldn't be in this case,
							 * since the `GetElementPtr` case would have assigned the `rightTerm` that physics type.
							 * Example:
							 * \code
							 * %7 = getelementptr inbounds [2 x double], [2 x double]* %2, i64 0, i64 0, !dbg !27
							 * store double %6, double* %7, align 16, !dbg !28
							 * \endcode
							 */
							if (auto llvmIrGetElementPointerInstruction = dyn_cast<GetElementPtrInst>(rightTerm))
							{
								uint64_t	index;
								if (auto llvmIrConstantInt = dyn_cast<ConstantInt>(llvmIrGetElementPointerInstruction->getOperand(2)))
								{
									Value *			structurePointer = llvmIrGetElementPointerInstruction->getPointerOperand();
									PhysicsInfo	*	structurePointerPhysicsInfo = virtualRegisterPhysicsTable[structurePointer];

									index = llvmIrConstantInt->getZExtValue();
									structurePointerPhysicsInfo->insertPhysicsInfoAt(virtualRegisterPhysicsTable[leftTerm], index);
								}
							}
							virtualRegisterPhysicsTable.insert({rightTerm, virtualRegisterPhysicsTable[leftTerm]});
							virtualRegisterIdentifier.insert({rightTerm, virtualRegisterIdentifier[leftTerm]});
							break;
						}
						if (!areTwoPhysicsEquivalent(N, leftPhysicsInfo->getPhysicsType(),
													 rightPhysicsInfo->getPhysicsType()))
						{
							printDebugInfoLocation(&llvmIrInstruction,
												   leftPhysicsInfo->getPhysicsType(),
												   rightPhysicsInfo->getPhysicsType());
							exit(1);
						}
					}
					break;

				case Instruction::GetElementPtr:
					if (auto llvmIrGetElementPointerInstruction = dyn_cast<GetElementPtrInst>(&llvmIrInstruction))
					{
						uint8_t operandsNumber = llvmIrGetElementPointerInstruction->getNumOperands();
						uint64_t index;

						/*
						 * TODO: handle all cases with arrays and pointers.
						 */
						if (operandsNumber != 3)
						{
							continue;
						}

						/*
						 * For one-dimensional arrays the 3rd operand is the index of the expression.
						 */
						Value *		indexOperand = llvmIrGetElementPointerInstruction->getOperand(2);
						if (auto llvmIrConstantInt = dyn_cast<ConstantInt>(indexOperand))
						{
							Value *			structurePointer = llvmIrGetElementPointerInstruction->getPointerOperand();
							PhysicsInfo	*	structurePointerPhysicsInfo = virtualRegisterPhysicsTable[structurePointer];
							PhysicsInfo * 	physicsInfo;

							index = llvmIrConstantInt->getZExtValue();
							physicsInfo = structurePointerPhysicsInfo->get_members()[index];
							virtualRegisterPhysicsTable.insert({llvmIrGetElementPointerInstruction, physicsInfo});

							if (physicsInfo) {
								auto dimensionIterator = physicsInfo->getPhysicsType()->dimensions;
								IRBuilder<> builder(&llvmIrInstruction);
								Type *argumentType = runtimeCheckFunction.getFunctionType()->getParamType(0);
								Type *pointerType = runtimeCheckFunction.getFunctionType()->getParamType(1);
								Type *Int64Type = llvm::IntegerType::getInt64Ty(builder.getContext());

								auto element_size = llvm::ConstantInt::get(Int64Type, sizeof(int64_t));
								auto array_size = llvm::ConstantInt::get(Int64Type, kMaxDimensions);
								auto alloc_size = llvm::ConstantExpr::getMul(element_size, array_size);

								Instruction *Malloc = CallInst::CreateMalloc(&llvmIrInstruction,
																			 Int64Type, Int64Type->getPointerTo(),
																			 alloc_size,
																			 nullptr, nullptr, "");
								auto pointerIndex = builder.CreatePointerCast(Malloc, pointerType);
								Value *locationPointer;
								int64_t exponent;
								for (int64_t i = 0; dimensionIterator; dimensionIterator = dimensionIterator->next, i++) {
									exponent = (int64_t) dimensionIterator->exponent;
									if (exponent) {
										locationPointer = builder.CreateGEP(pointerIndex,
																			llvm::ConstantInt::get(Int64Type, i));
										builder.CreateStore(llvm::ConstantInt::get(Int64Type, exponent),
															locationPointer);
									}
								}
								auto symbolNumber = builder.CreateCall(newtonInsert, {pointerIndex});
								virtualRegisterIdentifier.insert({llvmIrGetElementPointerInstruction, symbolNumber});
							}
						}
						else
						{
							IRBuilder<> 	builder(&llvmIrInstruction);
							llvm::Type *i64Type = llvm::IntegerType::getInt64Ty(builder.getContext());

							Type *	argumentType = runtimeCheckFunction.getFunctionType()->getParamType(0);
							Type *	pointerType = runtimeCheckFunction.getFunctionType()->getParamType(1);

							auto element_size = llvm::ConstantInt::get(i64Type, 8);
							auto array_size = llvm::ConstantInt::get(i64Type, 100);
							auto alloc_size = llvm::ConstantExpr::getMul(element_size, array_size);

							Instruction* Malloc = CallInst::CreateMalloc(&llvmIrInstruction,
																		 i64Type, i64Type->getPointerTo(), alloc_size,
																		 nullptr, nullptr, "");

							auto	variableIndex = builder.CreateIntCast(indexOperand, argumentType, true);
							auto	pointerIndex = builder.CreatePointerCast(Malloc, pointerType);
							builder.CreateCall(runtimeCheckFunction, {variableIndex, pointerIndex});
						}
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
					virtualRegisterPhysicsTable.insert({&llvmIrInstruction, virtualRegisterPhysicsTable[llvmIrInstruction.getOperand(0)]});
					break;

				case Instruction::PHI:
				case Instruction::Select:
				{
					PhysicsInfo *	leftTerm = virtualRegisterPhysicsTable[llvmIrInstruction.getOperand(0)];
					PhysicsInfo *	rightTerm = virtualRegisterPhysicsTable[llvmIrInstruction.getOperand(1)];
					Physics *	 	physicsPhiNode;
					if (!leftTerm)
					{
						if (!rightTerm)
						{
							break;
						}
						physicsPhiNode = rightTerm->getPhysicsType();
					}
					else if (!rightTerm)
					{
						physicsPhiNode = leftTerm->getPhysicsType();
					}
					else
					{
						if (!areTwoPhysicsEquivalent(N, leftTerm->getPhysicsType(),
													 rightTerm->getPhysicsType()))
						{

							auto debugLocation = cast<DILocation>(llvmIrInstruction.getMetadata(0));
							errs() << "Warning, cannot deduce physics type at: line " << debugLocation->getLine() <<
								   ", column " << debugLocation->getColumn() << ".\n";
						}
						physicsPhiNode = leftTerm->getPhysicsType();
					}
					virtualRegisterPhysicsTable.insert({&llvmIrInstruction, new PhysicsInfo{physicsPhiNode}});
				}
					break;

				/*
				 *	More information on all the LLVM IR instructions can be found at:
				 *	https://github.com/llvm/llvm-project/blob/main/llvm/include/llvm/IR/Instruction.def
				 */

				/*
				 *	vector/aggregate related
				 *	Unsupported for now.
				 */
				case Instruction::InsertElement:
				case Instruction::ShuffleVector:
				case Instruction::ExtractValue:
				case Instruction::InsertValue:
					errs() << llvmIrInstruction << "\n";
					errs() << "Unsupported LLVM IR Instruction!\n";

				/*
				 *	Terminator Instructions
				 *	These instructions are used to terminate a basic block of the program.
				 *	Every basic block must end with one of these instructions for it to be a well-formed basic block.
				 *	So far, they are not interesting to handle for Newton-related information.
				 */
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
				case Instruction::CallBr:

				/*
				 *	Memory operators
				 */
				case Instruction::Alloca:
				case Instruction::Fence:
				case Instruction::AtomicCmpXchg:
				case Instruction::AtomicRMW:

				/*
				 *	Other operators
				 */
				case Instruction::LandingPad:

				default:
					continue;
			}
		}
	}
}


void
irPassLLVMIRDimensionCheck(State *  N)
{
	if (N->llvmIR == nullptr)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Please specify the LLVM IR input file\n");
		fatal(N, Esanity);
	}

	StringRef	filePath(N->llvmIR);
	std::string	filePathStem;
	std::string	modifiedIRFilePath;
	std::string	JSONFilePath;

	std::error_code errorCode(errno,std::generic_category());

	filePathStem = std::string(sys::path::parent_path(filePath)) + "/" + std::string(sys::path::stem(filePath));

	modifiedIRFilePath = filePathStem + ".bc";
	raw_fd_ostream modifiedIROutputFile(modifiedIRFilePath, errorCode);

	JSONFilePath = filePathStem + ".json";
	raw_fd_ostream JSONOutputFile(JSONFilePath, errorCode);
	int JSONPrettyPrinting = true;
	json::OStream jsonOStream(JSONOutputFile, JSONPrettyPrinting);

	SMDiagnostic 	Err;
	LLVMContext 	Context;
	std::unique_ptr<Module>	Mod(parseIRFile(N->llvmIR, Err, Context));
	if (!Mod)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Error: Couldn't parse IR file.");
		fatal(N, Esanity);
	}

	Type *VoidType = Type::getVoidTy(Context);
	Type *Int64Type = Type::getInt64Ty(Context);
	Type *Int64PointerType = Type::getInt64PtrTy(Context);

	FunctionCallee	arrayDimensionalityCheck = Mod->getOrInsertFunction("__array_dimensionality_check", /* return type */ VoidType, Int64Type, Int64PointerType);
	FunctionCallee	initNewtonRuntime = Mod->getOrInsertFunction("__newtonInit", /* return type */ VoidType);
	FunctionCallee	newtonInsert = Mod->getOrInsertFunction("__newtonInsert", /* return type */ Int64Type, Int64PointerType);
	FunctionCallee	newtonCheckDimensions = Mod->getOrInsertFunction("__newtonCheckDimensions", /* return type */ Int64Type, Int64Type, Int64Type);

	for (auto & mi : *Mod)
	{
		dimensionalityCheck(mi, N, arrayDimensionalityCheck, initNewtonRuntime, newtonInsert, newtonCheckDimensions);
	}

	WriteBitcodeToFile(*Mod, modifiedIROutputFile);

	jsonOStream.object([&] {
		for (auto &iter: sourceVariablePhysicsTable) {
			dumpPhysicsInfoJSON(jsonOStream, iter.first, iter.second);
		}
	});
}

}
