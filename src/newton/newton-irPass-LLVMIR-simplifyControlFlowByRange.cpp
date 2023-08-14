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

#include "newton-irPass-LLVMIR-simplifyControlFlowByRange.h"

using namespace llvm;

extern "C"
{

enum CmpRes {
	Depends	    = 1,
	AlwaysTrue  = 2,
	AlwaysFalse = 3,
	Unsupported = 6,
};

CmpRes
compareFCmpWithVariableRange(FCmpInst * llvmIrFCmpInstruction, double leftVariableLowerBound,
			     double leftVariableUpperBound,
			     double rightVariableLowerBound, double rightVariableUpperBound)
{
	switch (llvmIrFCmpInstruction->getPredicate())
	{
		case FCmpInst::FCMP_TRUE:
			return CmpRes::AlwaysTrue;
		case FCmpInst::FCMP_FALSE:
			return CmpRes::AlwaysFalse;
			/*
			 * Ordered means that neither operand is a QNAN while unordered means that either operand may be a QNAN.
			 * More details in https://llvm.org/docs/LangRef.html#fcmp-instruction
			 * */
		case FCmpInst::FCMP_OEQ:
		case FCmpInst::FCMP_UEQ:
			if ((leftVariableLowerBound == rightVariableLowerBound) &&
                (rightVariableLowerBound == leftVariableUpperBound) &&
                (leftVariableUpperBound == rightVariableUpperBound))
			{
				return CmpRes::AlwaysTrue;
			}
            else if (leftVariableLowerBound > rightVariableUpperBound ||
                     leftVariableUpperBound < rightVariableLowerBound)
            {
                return CmpRes::AlwaysFalse;
            }
            else
            {
                return CmpRes::Depends;
            }
        case FCmpInst::FCMP_ONE:
        case FCmpInst::FCMP_UNE:
            if ((leftVariableUpperBound < rightVariableLowerBound) ||
                (leftVariableLowerBound > rightVariableUpperBound))
            {
                return CmpRes::AlwaysTrue;
            }
            else if ((leftVariableLowerBound == rightVariableLowerBound) &&
                     (rightVariableLowerBound == leftVariableUpperBound) &&
                     (leftVariableUpperBound == rightVariableUpperBound))
            {
                return CmpRes::AlwaysFalse;
            }
            else
            {
                return CmpRes::Depends;
            }
		case FCmpInst::FCMP_OGT:
		case FCmpInst::FCMP_UGT:
			if (leftVariableLowerBound > rightVariableUpperBound)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (leftVariableUpperBound <= rightVariableLowerBound)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		case FCmpInst::FCMP_OGE:
		case FCmpInst::FCMP_UGE:
			if (leftVariableLowerBound >= rightVariableUpperBound)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (leftVariableUpperBound < rightVariableLowerBound)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		case FCmpInst::FCMP_OLT:
		case FCmpInst::FCMP_ULT:
			if (leftVariableUpperBound < rightVariableLowerBound)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (leftVariableLowerBound >= rightVariableUpperBound)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		case FCmpInst::FCMP_OLE:
		case FCmpInst::FCMP_ULE:
			if (leftVariableUpperBound <= rightVariableLowerBound)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (leftVariableLowerBound > rightVariableUpperBound)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		default:
			return CmpRes::Unsupported;
	}
}

CmpRes
compareICmpWithVariableRange(ICmpInst * llvmIrICmpInstruction, double leftVariableLowerBound,
			     double leftVariableUpperBound,
			     double rightVariableLowerBound, double rightVariableUpperBound)
{
	switch (llvmIrICmpInstruction->getPredicate())
	{
		case ICmpInst::ICMP_EQ:
			if ((leftVariableLowerBound == rightVariableLowerBound) &&
                (rightVariableLowerBound == leftVariableUpperBound) &&
			    (leftVariableUpperBound == rightVariableUpperBound))
			{
				return CmpRes::AlwaysTrue;
			}
			else if (leftVariableLowerBound > rightVariableUpperBound ||
                leftVariableUpperBound < rightVariableLowerBound)
			{
				return CmpRes::AlwaysFalse;
			}
            else
            {
                return CmpRes::Depends;
            }
		case ICmpInst::ICMP_NE:
			if (leftVariableUpperBound < rightVariableLowerBound || leftVariableLowerBound > rightVariableUpperBound)
			{
				return CmpRes::AlwaysTrue;
			}
			else if ((leftVariableLowerBound == rightVariableLowerBound) &&
                     (rightVariableLowerBound == leftVariableUpperBound) &&
                     (leftVariableUpperBound == rightVariableUpperBound))
			{
				return CmpRes::AlwaysFalse;
			}
            else
            {
                return CmpRes::Depends;
            }
		case ICmpInst::ICMP_UGT:
		case ICmpInst::ICMP_SGT:
			if (leftVariableLowerBound > rightVariableUpperBound)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (leftVariableUpperBound <= rightVariableLowerBound)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		case ICmpInst::ICMP_UGE:
		case ICmpInst::ICMP_SGE:
			if (leftVariableLowerBound >= rightVariableUpperBound)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (leftVariableUpperBound < rightVariableLowerBound)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		case ICmpInst::ICMP_ULT:
		case ICmpInst::ICMP_SLT:
			if (leftVariableUpperBound < rightVariableLowerBound)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (leftVariableLowerBound >= rightVariableUpperBound)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		case ICmpInst::ICMP_ULE:
		case ICmpInst::ICMP_SLE:
			if (leftVariableUpperBound <= rightVariableLowerBound)
			{
				return CmpRes::AlwaysTrue;
			}
			else if (leftVariableLowerBound > rightVariableUpperBound)
			{
				return CmpRes::AlwaysFalse;
			}
			else
			{
				return CmpRes::Depends;
			}
		default:
			return CmpRes::Unsupported;
	}
}

static Type *
GetCompareTy(Value * Op)
{
	return CmpInst::makeCmpResultType(Op->getType());
}

/*
 * For a boolean type or a vector of boolean type, return false or a vector
 * with every element false.
 * */
static llvm::Constant *
getFalse(Type * Ty)
{
	return ConstantInt::getFalse(Ty);
}

/*
 * For a boolean type or a vector of boolean type, return true or a vector
 * with every element true.
 * */
static llvm::Constant *
getTrue(Type * Ty)
{
	return ConstantInt::getTrue(Ty);
}

bool
simplifyControlFlow(State * N, BoundInfo * boundInfo, Function & llvmIrFunction)
{
	bool changed = false;
	for (BasicBlock & llvmIrBasicBlock : llvmIrFunction)
	{
		for (Instruction & llvmIrInstruction : llvmIrBasicBlock)
		{
			switch (llvmIrInstruction.getOpcode())
			{
				case Instruction::ICmp:
					if (auto llvmIrICmpInstruction = dyn_cast<ICmpInst>(&llvmIrInstruction))
					{
						auto leftOperand  = llvmIrICmpInstruction->getOperand(0);
						auto rightOperand = llvmIrICmpInstruction->getOperand(1);
						if ((isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
						{
                            llvmIrICmpInstruction->swapOperands();
                            leftOperand  = llvmIrICmpInstruction->getOperand(0);
                            rightOperand = llvmIrICmpInstruction->getOperand(1);
						}
						else if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tICmp: Expression normalization needed.\n");
						}
						else if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
							auto vrLeftRangeIt  = boundInfo->virtualRegisterRange.find(leftOperand);
							auto vrRightRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
							if (vrLeftRangeIt != boundInfo->virtualRegisterRange.end() &&
							    vrRightRangeIt != boundInfo->virtualRegisterRange.end())
							{
								flexprint(N->Fe, N->Fm, N->Fpinfo,
									  "\tICmp: left operand's lower bound: %f, upper bound: %f\n",
									  vrLeftRangeIt->second.first, vrLeftRangeIt->second.second);
								flexprint(N->Fe, N->Fm, N->Fpinfo,
									  "\tICmp: right operand's lower bound: %f, upper bound: %f\n",
									  vrRightRangeIt->second.first, vrRightRangeIt->second.second);
								CmpRes compareResult = compareICmpWithVariableRange(llvmIrICmpInstruction,
														    vrLeftRangeIt->second.first,
														    vrLeftRangeIt->second.second,
														    vrRightRangeIt->second.first,
														    vrRightRangeIt->second.second);
								flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: the comparison result is %d\n",
									  compareResult);
								/*
								 * Fold trivial predicates.
								 * */
								Type *	retTy	 = GetCompareTy(leftOperand);
								Value * resValue = nullptr;
								if (compareResult == CmpRes::AlwaysTrue)
								{
									resValue = getTrue(retTy);
									llvmIrICmpInstruction->replaceAllUsesWith(resValue);
									changed = true;
								}
								else if (compareResult == CmpRes::AlwaysFalse)
								{
									resValue = getFalse(retTy);
									llvmIrICmpInstruction->replaceAllUsesWith(resValue);
									changed = true;
								}
								else if (compareResult == CmpRes::Unsupported)
								{
									flexprint(N->Fe, N->Fm, N->Fperr,
										  "\tICmp: Current ICmp Predicate is not supported.\n");
								}
							}
						}
						if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							double constValue = 0.0;
							if (ConstantInt * constInt = llvm::dyn_cast<llvm::ConstantInt>(rightOperand))
							{
                                if (llvmIrICmpInstruction->isSigned()) {
                                    constValue = constInt->getSExtValue();
                                } else {
                                    constValue = constInt->getZExtValue();
                                }
							}
							else
							{
								flexprint(N->Fe, N->Fm, N->Fperr, "\tICmp: it's not a const fp!!!!!!!!!!!\n");
							}
							flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: right operand: %f\n", constValue);
							/*
							 * find the variable from the boundInfo->virtualRegisterRange
							 * */
							auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != boundInfo->virtualRegisterRange.end())
							{
								flexprint(N->Fe, N->Fm, N->Fpinfo,
									  "\tICmp: varibale's lower bound: %f, upper bound: %f\n",
									  vrRangeIt->second.first, vrRangeIt->second.second);
								CmpRes compareResult = compareICmpWithVariableRange(llvmIrICmpInstruction,
															 vrRangeIt->second.first,
															 vrRangeIt->second.second,
															 constValue, constValue);
								flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: the comparison result is %d\n",
									  compareResult);
								/*
								 * Fold trivial predicates.
								 * */
								Type *	retTy	 = GetCompareTy(leftOperand);
								Value * resValue = nullptr;
								if (compareResult == CmpRes::AlwaysTrue)
								{
									resValue = getTrue(retTy);
									llvmIrICmpInstruction->replaceAllUsesWith(resValue);
									changed = true;
								}
								else if (compareResult == CmpRes::AlwaysFalse)
								{
									resValue = getFalse(retTy);
									llvmIrICmpInstruction->replaceAllUsesWith(resValue);
									changed = true;
								}
								else if (compareResult == CmpRes::Unsupported)
								{
									flexprint(N->Fe, N->Fm, N->Fperr,
										  "\tICmp: Current ICmp Predicate is not supported.\n");
								}
							}
							else
							{
								flexprint(N->Fe, N->Fm, N->Fperr, "\tICmp: Unknown variable\n");
							}
						}
					}
					break;

				case Instruction::FCmp:
					if (auto llvmIrFCmpInstruction = dyn_cast<FCmpInst>(&llvmIrInstruction))
					{
						auto leftOperand  = llvmIrFCmpInstruction->getOperand(0);
						auto rightOperand = llvmIrFCmpInstruction->getOperand(1);
						if ((isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
						{
                            llvmIrFCmpInstruction->swapOperands();
							flexprint(N->Fe, N->Fm, N->Fperr, "\tFCmp: swap left and right, need to change the type of prediction\n");
						}
						else if (isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							flexprint(N->Fe, N->Fm, N->Fperr, "\tFCmp: Expression normalization needed.\n");
						}
						else if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
						{
							auto vrLeftRangeIt  = boundInfo->virtualRegisterRange.find(leftOperand);
							auto vrRightRangeIt = boundInfo->virtualRegisterRange.find(rightOperand);
							if (vrLeftRangeIt != boundInfo->virtualRegisterRange.end() &&
							    vrRightRangeIt != boundInfo->virtualRegisterRange.end())
							{
								flexprint(N->Fe, N->Fm, N->Fpinfo,
									  "\tICmp: left operand's lower bound: %f, upper bound: %f\n",
									  vrLeftRangeIt->second.first, vrLeftRangeIt->second.second);
								flexprint(N->Fe, N->Fm, N->Fpinfo,
									  "\tICmp: right operand's lower bound: %f, upper bound: %f\n",
									  vrRightRangeIt->second.first, vrRightRangeIt->second.second);
								CmpRes compareResult = compareFCmpWithVariableRange(llvmIrFCmpInstruction,
														    vrLeftRangeIt->second.first,
														    vrLeftRangeIt->second.second,
														    vrRightRangeIt->second.first,
														    vrRightRangeIt->second.second);
								flexprint(N->Fe, N->Fm, N->Fpinfo, "\tICmp: the comparison result is %d\n",
									  compareResult);
								/*
								 * Fold trivial predicates.
								 * */
								Type *	retTy	 = GetCompareTy(leftOperand);
								Value * resValue = nullptr;
								if (compareResult == CmpRes::AlwaysTrue)
								{
									resValue = getTrue(retTy);
									llvmIrFCmpInstruction->replaceAllUsesWith(resValue);
									changed = true;
								}
								else if (compareResult == CmpRes::AlwaysFalse)
								{
									resValue = getFalse(retTy);
									llvmIrFCmpInstruction->replaceAllUsesWith(resValue);
									changed = true;
								}
								else if (compareResult == CmpRes::Unsupported)
								{
									flexprint(N->Fe, N->Fm, N->Fperr,
										  "\tICmp: Current ICmp Predicate is not supported.\n");
								}
							}
						}
						if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
						{
							double constValue = 0.0;
							if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
							{
								/*
								 * both "float" and "double" type can use "convertToDouble"
								 * */
								constValue = (constFp->getValueAPF()).convertToDouble();
							}
							else
							{
								flexprint(N->Fe, N->Fm, N->Fperr, "\tFCmp: it's not a const fp!!!!!!!!!!!\n");
							}
							flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFCmp: right operand: %f\n", constValue);
							/*
							 * find the variable from the boundInfo->virtualRegisterRange
							 * */
							auto vrRangeIt = boundInfo->virtualRegisterRange.find(leftOperand);
							if (vrRangeIt != boundInfo->virtualRegisterRange.end())
							{
								flexprint(N->Fe, N->Fm, N->Fpinfo,
									  "\tFCmp: varibale's lower bound: %f, upper bound: %f\n",
									  vrRangeIt->second.first, vrRangeIt->second.second);
								CmpRes compareResult = compareFCmpWithVariableRange(llvmIrFCmpInstruction,
															 vrRangeIt->second.first,
															 vrRangeIt->second.second,
															 constValue, constValue);
								flexprint(N->Fe, N->Fm, N->Fpinfo, "\tFCmp: the comparison result is %d\n",
									  compareResult);
								/*
								 * Fold trivial predicates.
								 * */
								Type *	retTy	 = GetCompareTy(leftOperand);
								Value * resValue = nullptr;
								if (compareResult == CmpRes::AlwaysTrue)
								{
									resValue = getTrue(retTy);
									llvmIrFCmpInstruction->replaceAllUsesWith(resValue);
									changed = true;
								}
								else if (compareResult == CmpRes::AlwaysFalse)
								{
									resValue = getFalse(retTy);
									llvmIrFCmpInstruction->replaceAllUsesWith(resValue);
									changed = true;
								}
							}
							else
							{
								flexprint(N->Fe, N->Fm, N->Fperr, "\tFCmp: Unknown variable\n");
							}
						}
					}
					break;

				default:
					continue;
			}
		}
	}
	return changed;
}
}