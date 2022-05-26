#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include <set>
#include <algorithm>

#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/Path.h"

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

// todo: maybe we can move this to the struct "State" in common-data-structures.h
class IRDumper {
 public:
  IRDumper() = default;

  ~IRDumper() = default;

  void dump(State *N, std::string fileSuffix, std::unique_ptr<Module> Mod) {
	StringRef	filePath(N->llvmIR);
	filePath_ =
			std::string(sys::path::parent_path(filePath)) + "/" +
			std::string(sys::path::stem(filePath)) + "_" + fileSuffix + ".";

	clean();

	flexprint(N->Fe, N->Fm, N->Fpinfo, "Dump IR of: %s\n", filePath_.c_str());
	std::error_code errorCode(errno,std::generic_category());
	raw_fd_ostream dumpedFile(filePath_+"bc", errorCode);
	WriteBitcodeToFile(*Mod, dumpedFile);
	dumpedFile.close();

	disassemble();
  }

 private:
  std::string filePath_;

  void clean() {
	std::string cmd = "rm -f " + filePath_ + "*";
	system(cmd.c_str());
  }

  void disassemble() {
	std::string cmd = "llvm-dis " + filePath_ + "bc" + " -o " + filePath_ + "ll";
	system(cmd.c_str());
  }
};

typedef struct BoundInfo {
	std::map<std::string, std::pair<double, double>> variableBound;
	std::map<std::string, std::pair<double, double>> typeRange;
} BoundInfo;

void
simplifyControlFlow(State * N, BoundInfo * boundInfo, Function & llvmIrFunction)
{
	std::map<Value *, std::pair<double, double>> virtualRegisterRange;
	for (BasicBlock & llvmIrBasicBlock : llvmIrFunction) {
		for (Instruction &llvmIrInstruction : llvmIrBasicBlock) {
			switch (llvmIrInstruction.getOpcode()) {
				case Instruction::Call:
					if (auto llvmIrCallInstruction = dyn_cast<CallInst>(&llvmIrInstruction)) {
						Function *calledFunction = llvmIrCallInstruction->getCalledFunction();
						if (calledFunction->getName().startswith("llvm.dbg.declare")) {
							auto firstOperator = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(0));
							auto localVariableAddressAsMetadata = cast<ValueAsMetadata>(firstOperator->getMetadata());
							auto localVariableAddress = localVariableAddressAsMetadata->getValue();

							auto variableMetadata = cast<MetadataAsValue>(llvmIrCallInstruction->getOperand(1));
							auto debugInfoVariable = cast<DIVariable>(variableMetadata->getMetadata());
							auto variableType = debugInfoVariable->getType();

							// if we find such type in boundInfo->typeRange,
							// we get its range and bind the var with it in boundInfo->variableBound
							// and record it in the virtualRegisterRange
							auto typeRangeIt = boundInfo->typeRange.find(variableType->getName().str());
							if (typeRangeIt != boundInfo->typeRange.end()) {
								boundInfo->variableBound.emplace(debugInfoVariable->getName().str(), typeRangeIt->second);
								virtualRegisterRange.emplace(localVariableAddress, typeRangeIt->second);
							}
						}
					}
					break;

				case Instruction::Load:
					if (auto llvmIrLoadInstruction = dyn_cast<LoadInst>(&llvmIrInstruction))
					{
						auto vrRangeIt = virtualRegisterRange.find(llvmIrLoadInstruction->getOperand(0));
						if (vrRangeIt != virtualRegisterRange.end())
						{
							virtualRegisterRange.emplace(llvmIrLoadInstruction, vrRangeIt->second);
						}
					}
					break;
				case Instruction::ICmp:
				case Instruction::FCmp: {
					auto leftOperand = llvmIrInstruction.getOperand(0);
					auto rightOperand = llvmIrInstruction.getOperand(1);
					/// todo: expression normalization needed, which simpily the "const cmp const" or normalize into the "var cmp const" form
					/// so this if-branch is a debug message, and will be deleted after finishing the expression normalization
					if ((isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand)) ||
							(isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand)))
					{
						flexprint(N->Fe, N->Fm, N->Fperr, "\tExpression normalization needed.\n");
					}

					if (!isa<llvm::Constant>(leftOperand) && !isa<llvm::Constant>(rightOperand))
					{

					}
					else if (!isa<llvm::Constant>(leftOperand) && isa<llvm::Constant>(rightOperand))
					{
						// eg. fcmp ogt double %1, 0
						// get the constant value
						double constValue = 0.0;
						if (ConstantFP * constFp = llvm::dyn_cast<llvm::ConstantFP>(rightOperand))
						{
							// both "float" and "double" type can use "convertToDouble"
							constValue = (constFp->getValueAPF()).convertToDouble();
						}
						flexprint(N->Fe, N->Fm, N->Fpinfo, "\tCmp: right operand: %f\n", constValue);
						// find the variable from the virtualRegisterRange
						auto vrRangeIt = virtualRegisterRange.find(leftOperand);
						if (vrRangeIt != virtualRegisterRange.end())
						{

						}
					}
					break;
				}

				case Instruction::Br:
					if (auto llvmIrBrInstruction = dyn_cast<BranchInst>(&llvmIrInstruction))
					{
						flexprint(N->Fe, N->Fm, N->Fpinfo, "\tInstruction::Br\n");
						if (llvmIrBrInstruction->isConditional())
						{
							flexprint(N->Fe, N->Fm, N->Fpinfo, "\tconditional\n");
							auto condValue = llvmIrBrInstruction->getCondition();
							auto vrRangeIt = virtualRegisterRange.find(condValue);
							if (vrRangeIt != virtualRegisterRange.end())
							{
								// if true, remove the condition
								// if false, remove the whole block
							}
						}
						else
						{
							flexprint(N->Fe, N->Fm, N->Fpinfo, "\tunconditional\n");
						}
					}
					break;

				default:
					continue;
			}
		}
	}
}

void irPassLLVMIRSimplifyControlFlowByRange(State *N) {
	if (N->llvmIR == nullptr)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Please specify the LLVM IR input file\n");
		fatal(N, Esanity);
	}

	SMDiagnostic 	Err;
	LLVMContext 	Context;
	std::unique_ptr<Module>	Mod(parseIRFile(N->llvmIR, Err, Context));
	if (!Mod)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "Error: Couldn't parse IR file.");
		fatal(N, Esanity);
	}

	flexprint(N->Fe, N->Fm, N->Fpinfo, "simplify control flow by range\n");
	auto boundInfo = new BoundInfo();

	// get sensor info, we only concern the id and range here
	if (N->sensorList != NULL)
	{
		for (Modality * currentModality = N->sensorList->modalityList; currentModality != NULL; currentModality = currentModality->next)
		{
			flexprint(N->Fe, N->Fm, N->Fpinfo, "\tModality: %s\n", currentModality->identifier);
			flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\trangeLowerBound: %f\n", currentModality->rangeLowerBound);
			flexprint(N->Fe, N->Fm, N->Fpinfo, "\t\trangeUpperBound: %f\n", currentModality->rangeUpperBound);
			boundInfo->typeRange.emplace(currentModality->identifier, std::make_pair(currentModality->rangeLowerBound, currentModality->rangeUpperBound));
		}
	}

	for (auto& mi : *Mod)
	{
		simplifyControlFlow(N, boundInfo, mi);
	}

	auto dumper = new IRDumper();
	dumper->dump(N, "output", std::move(Mod));

}

}
