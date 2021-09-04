
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


#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/CFG.h"
//#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include <string>
#include "llvm/IR/ValueSymbolTable.h"


#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h" 
#include "llvm/IR/Instructions.h" 
#include "llvm/IRReader/IRReader.h" 
#include "llvm/Pass.h" 
#include "llvm/Support/SourceMgr.h" 
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/IntrinsicInst.h" 
#include "llvm/IR/DebugInfoMetadata.h" 

#include <vector>
#include <map>
#include <tuple>

using namespace llvm;


#include <iostream>

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


// The function Nikos used
// =======================
void
getVariables(Function &F, State *N){
    for(Function::iterator BB=F.begin(), E=F.end(); BB!=E; ++BB){
        for(BasicBlock::iterator I=BB->begin(), IE=BB->end(); I!=IE; ++I){
		std::cout << dyn_cast<Instruction>(I) << "\n";
	}
    }

}
// -------------------------

std::vector<std::string> NewtonDefinition{"accelerationX",
					"accelerationY",
					"accelerationZ",
					"temp",
					"distanceA",
					"distanceB",
					"distanceC"};

std::vector<std::tuple<llvm::StringRef, llvm::StringRef, unsigned> > newton_var_info; 


std::map<std::tuple<std::string, std::string, std::string>, bool> Newton_rules{{std::make_tuple("accX", "accX", "Add"), true},
                                                                               {std::make_tuple("accX", "accY", "Add"), true},
                                                                               {std::make_tuple("accX", "accZ", "Add"), true},
                                                                               {std::make_tuple("accY", "accX", "Add"), true},
                                                                               {std::make_tuple("accY", "accY", "Add"), true},
                                                                               {std::make_tuple("accY", "accZ", "Add"), true},
                                                                               {std::make_tuple("accZ", "accX", "Add"), true},
                                                                               {std::make_tuple("accZ", "accY", "Add"), true},
                                                                               {std::make_tuple("accZ", "accZ", "Add"), true},
                                                                               {std::make_tuple("accX", "normal", "Add"), true},
                                                                               {std::make_tuple("accY", "normal", "Add"), true},
                                                                               {std::make_tuple("accZ", "normal", "Add"), true},
									       {std::make_tuple("accX", "temperature", "Add"), false},
									       {std::make_tuple("temperature", "accX", "Add"), false},
                                                                               {std::make_tuple("accX", "accY", "Mul"), true},
                                                                               {std::make_tuple("accX", "accZ", "Mul"), true},
                                                                               {std::make_tuple("accY", "accX", "Mul"), true},
                                                                               {std::make_tuple("accY", "accY", "Mul"), true},
                                                                               {std::make_tuple("accY", "accZ", "Mul"), true},
                                                                               {std::make_tuple("accZ", "accX", "Mul"), true},
                                                                               {std::make_tuple("accZ", "accY", "Mul"), true},
                                                                               {std::make_tuple("accZ", "accZ", "Mul"), true},
                                                                               {std::make_tuple("accX", "normal", "Mul"), true},
                                                                               {std::make_tuple("accY", "normal", "Mul"), true},
                                                                               {std::make_tuple("accZ", "normal", "Mul"), true},
                                                                               {std::make_tuple("accX", "temperature", "Mul"), false},
                                                                               {std::make_tuple("temperature", "accX", "Mul"), false},
									       {std::make_tuple("disA", "disA", "Mul"), true},
									       {std::make_tuple("disB", "disB", "Mul"), true},
  									       };

void get_Newton_var(Function &F, std::vector<std::string> &newton_definition=NewtonDefinition, std::vector<std::tuple<llvm::StringRef, llvm::StringRef, unsigned> >& newton_vars=newton_var_info){
    for (BasicBlock &BB : F){
        for(Instruction &I : BB){
	    if(DbgVariableIntrinsic *DbgIntrinsic = dyn_cast<DbgVariableIntrinsic>(&I)){
	        DILocalVariable *DV = dyn_cast<DILocalVariable>(DbgIntrinsic->getVariable());
		if(std::find(newton_definition.begin(), newton_definition.end(), DV->getType()->getName()) != NewtonDefinition.end()){
		    newton_vars.push_back(std::make_tuple(DV->getName(), DV->getType()->getName(), DV->getLine()));
		}
	     }
	}
    }

}



// a list of users of Newton type variables
void touched_variables(std::string varName, Function &F){
    const ValueSymbolTable* symbolTable = F.getValueSymbolTable();
    Value* track_var = symbolTable->lookup(varName);
    std::vector<Value*> workList;
    workList.push_back(track_var);
    errs() << "\n";
    errs() << varName <<  " has following users: \n";
    while(!workList.empty()){
      Value* cur_value = workList.back();  // get the last value of the work-list
      workList.pop_back();  // remove the value from the work-list
      for(User *U : cur_value->users()){  // iterate all users of the cur_value
        workList.push_back(dyn_cast<Value>(U));
	errs() << "    |---> " << dyn_cast<Instruction>(U) << "\t| " << *dyn_cast<Instruction>(U) << "\t\t\t\t|name: " << dyn_cast<Instruction>(U)->getName() << "\t|opcode: " << dyn_cast<Instruction>(U)->getOpcodeName() <<"\n";
//	if(dyn_cast<Instruction>(U)->getOpcode() == Instruction::Store){
//	  errs() << "store: 1st operand: " << dyn_cast<Instruction>(U)->getOperand(0)->getName() << " : " << dyn_cast<Instruction>(U)->getOperand(0) << " | 2nd operand: " << dyn_cast<Instruction>(U)->getOperand(1)->getName() << " : " << dyn_cast<Instruction>(U)->getOperand(1) << "\n";
//	}
//	if(dyn_cast<Instruction>(U)->getOpcode() == Instruction::FAdd){
//	  errs() << "add: 1st operand: " << dyn_cast<Instruction>(U)->getOperand(0) << "| 2nd operand: " << dyn_cast<Instruction>(U)->getOperand(1)->getName() << " : " << dyn_cast<Instruction>(U)->getOperand(1) << "\n";
//	}
      }
    }
    errs() << "\n";
  }



// ========
void init_inst_type_collector(std::string newton_var, Function &F, std::map<Instruction*, std::string> *collector){
    const ValueSymbolTable* symbolTable = F.getValueSymbolTable();
    Value* target_var = symbolTable->lookup(newton_var);
    //errs() << newton_var << "===========" << dyn_cast<Instruction>(target_var)->getName() << " : " << *dyn_cast<Instruction>(target_var) << " : " << dyn_cast<Instruction>(target_var) << "\n";
    (*collector)[dyn_cast<Instruction>(target_var)] = std::string(dyn_cast<Instruction>(target_var)->getName()); 
  }


// check the content of the collector
void print_collector(std::map<Instruction*, std::string>& collector){
    std::map<Instruction*, std::string>::iterator i;
    for(i=collector.begin(); i!=collector.end(); i++){
      errs() << i->first << "\t" << i->second << "\n";
    }
  }


// add instruction to collector if it has Newton operands
void add_to_collector(llvm::Value* operand, std::string type, std::map<Instruction*, std::string>& collector){
    collector[dyn_cast<Instruction>(operand)] = type;
  }


// get names of all Newton varialbes
// e.g. accX, accY, ...
std::vector<llvm::StringRef> newton_vars;
void get_Newton_var_names(std::vector<llvm::StringRef>& newton_vars, std::vector<std::tuple<llvm::StringRef, llvm::StringRef, unsigned> >& newton_var_info = newton_var_info){
  std::vector<std::tuple<llvm::StringRef, llvm::StringRef, unsigned> >::iterator i;
  for(i = newton_var_info.begin(); i != newton_var_info.end(); i++){
    newton_vars.push_back(std::get<0>(*i));
  }
}



// check operand if it is of a Newton type
bool check_operand(llvm::Value* operand, std::map<Instruction*, std::string>& collector){
    if(collector.find(dyn_cast<Instruction>(operand)) != collector.end()){
      //errs() << collector[dyn_cast<Instruction>(operand)] << "  \n";
      return true;      
    }
    return false;
  }



// find key
// check true or false
void valid_add(std::string ty1, std::string ty2, std::map<std::tuple<std::string, std::string, std::string>, bool>& rules=Newton_rules){
    std::tuple<std::string, std::string, std::string> key = std::make_tuple(ty1, ty2, "Add");
    auto it = rules.find(key);
    if(it != rules.end()){
      if(rules[key]){
        std::cout << "\033[1;32mvalid add operation\033[0m\n";
      }
      else{
        std::cout << "\033[1;31minvalid add operation\033[0m\n";
      }
    }
  }

void valid_mul(std::string ty1, std::string ty2, std::map<std::tuple<std::string, std::string, std::string>, bool>& rules=Newton_rules){
    std::tuple<std::string, std::string, std::string> key = std::make_tuple(ty1, ty2, "Mul");
    auto it = rules.find(key);
    if(it != rules.end()){
      if(rules[key]){
        std::cout << "\033[1;32mvalid mul operation\033[0m\n";
      }
      else{
        std::cout << "\033[1;31minvalid mul operation\033[0m\n";
      }
    }
  }


// An example of how to perform dimensional analysis
// ====================================
void check_dimension(Function& F, std::map<Instruction*, std::string>& collector){
    for (BasicBlock &BB : F){
      for(Instruction &I : BB){
	switch (I.getOpcode()){
	  case Instruction::Ret:
	    errs() << I << " is a return\n";
	    break;
	  case Instruction::Add:
	  case Instruction::FAdd:{
	    errs() << I << " is an addition\n";
	    errs() << I.getOperand(0) << " " << I.getOperand(1) << "\n";  

	    if(check_operand(I.getOperand(0), collector) & !check_operand(I.getOperand(1), collector)){
	      valid_add(collector[dyn_cast<Instruction>(I.getOperand(0))], "normal");
	    }
	    if(!check_operand(I.getOperand(0), collector) & check_operand(I.getOperand(1), collector)){
	      valid_add("normal", collector[dyn_cast<Instruction>(I.getOperand(0))]);
	    }
	    if(check_operand(I.getOperand(0), collector) & check_operand(I.getOperand(1), collector)){
	      valid_add(collector[dyn_cast<Instruction>(I.getOperand(0))], collector[dyn_cast<Instruction>(I.getOperand(1))]);
	    }

	    if(check_operand(I.getOperand(0), collector)){
	      errs() << I.getOperand(0) << "   0 -----> Newton\n";
	      //valid_add();
	    }
	    if(check_operand(I.getOperand(1), collector)){
	      errs() << I.getOperand(1) << "   1 -----> Newton\n";
	    }
	    
	    break;
	    } // end case add
	  
	  case Instruction::Mul:
	  case Instruction::FMul:{
	    errs() << "check mul\n";
	    if(check_operand(I.getOperand(0), collector) & !check_operand(I.getOperand(1), collector)){
              valid_mul(collector[dyn_cast<Instruction>(I.getOperand(0))], "normal");
            }
            if(!check_operand(I.getOperand(0), collector) & check_operand(I.getOperand(1), collector)){
              valid_mul("normal", collector[dyn_cast<Instruction>(I.getOperand(0))]);
            }
            if(check_operand(I.getOperand(0), collector) & check_operand(I.getOperand(1), collector)){
              valid_mul(collector[dyn_cast<Instruction>(I.getOperand(0))], collector[dyn_cast<Instruction>(I.getOperand(1))]);
            }

	    break;
				  }  // end case mul

	  case Instruction::Call:
          case Instruction::Invoke:{
	    errs() << "check function call\n";
	    break;
				}  // check call/invoke

          case Instruction::And:{
	    errs() << "check and\n";
	    break;
				}  // check and
	  case Instruction::SDiv:{
	    errs() << "check SDiv\n";
	    break;
				 }  // check SDiv
	  case Instruction::Load:{
	    errs() << I << " is a load\n";
	    //errs() << &I << "      "<< I.getOperand(0) << "\n";
	    //print_collector(collector);
	    if(check_operand(I.getOperand(0), collector)){
	      add_to_collector(&I, collector[dyn_cast<Instruction>(I.getOperand(0))], collector);
	    }

	    break;
				 }  // end case Load
	  case Instruction::Store:
	    errs() << I << " is a store\n";
	    break;

	  default:
	    break;
	}
      }  // Instruction loop
    }  // BB loop
    std::map<Instruction*, std::string>::iterator i;
    for(i=collector.begin(); i!=collector.end(); i++){
      errs() << i->first << " | " << i->second << "\n";
    }
  }  




void 
irPassLLVMDimensionAnalysis(State * N){


  SMDiagnostic Err;
  LLVMContext Context;
  std::unique_ptr<Module>Mod(parseIRFile(N->llvmIR, Err, Context));
  if(!Mod){
      flexprint(N->Fe, N->Fm, N->Fperr, "Error: Couldn't parse IR file.");
      fatal(N, Esanity);
  }


  for(Module::iterator mi=Mod->begin(); mi!=Mod->end(); mi++){
      if (mi->isDeclaration()){
          break;
      }
      
      get_Newton_var(*mi);

      get_Newton_var_names(newton_vars);

      std::map<Instruction*, std::string> inst_type_collector2;

      for(llvm::StringRef name : newton_vars){
	     init_inst_type_collector(std::string(name), *mi, &inst_type_collector2);
      }


      check_dimension(*mi, inst_type_collector2);

      std::cout << "\n----------\n";
      touched_variables("accX", *mi);
  }
  
}  // end of dimensional analysis

}  // end of extern
