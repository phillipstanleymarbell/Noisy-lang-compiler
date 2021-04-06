#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "version.h"
#include "noisy-timeStamps.h"
#include "common-errors.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "noisy-codeGeneration.h"
#include "common-irHelpers.h"
#include <llvm-c/Core.h>


typedef struct {
         LLVMContextRef theContext;
         LLVMBuilderRef theBuilder;
         LLVMModuleRef  theModule;
} CodeGenState;


void
noisyModuleTypeNameDeclCodeGen(State * N, CodeGenState * S,IrNode * noisyModuleTypeNameDeclNode)
{
        
        if (R(noisyModuleTypeNameDeclNode)->type == kNoisyIrNodeType_PconstantDecl)
        {
                IrNode * noisyConstantDeclNode = RL(noisyModuleTypeNameDeclNode);
                LLVMValueRef constValue;
                LLVMValueRef globalValue;

                if (noisyConstantDeclNode->type == kNoisyIrNodeType_TintegerConst)
                {
                        constValue = LLVMConstInt(LLVMInt64Type(),noisyConstantDeclNode->token->integerConst,true);
                        globalValue = LLVMAddGlobal (S->theModule, LLVMInt64Type(),  L(noisyModuleTypeNameDeclNode)->tokenString);
                }
                else if (noisyConstantDeclNode->type == kNoisyIrNodeType_TrealConst)
                {
                        constValue = LLVMConstReal(LLVMDoubleType(),noisyConstantDeclNode->token->realConst);
                        globalValue = LLVMAddGlobal (S->theModule, LLVMDoubleType(),  L(noisyModuleTypeNameDeclNode)->tokenString);
                }
                else if (noisyConstantDeclNode->type == kNoisyIrNodeType_TboolConst)
                {
                        constValue = LLVMConstInt(LLVMInt1Type(),noisyConstantDeclNode->token->integerConst,false);
                        globalValue = LLVMAddGlobal (S->theModule, LLVMInt1Type(),  L(noisyModuleTypeNameDeclNode)->tokenString);
                }
                LLVMSetInitializer (globalValue ,constValue);
                LLVMSetGlobalConstant (globalValue, true);

        }
        
}

void
noisyTypeParameterListCodeGen(State * N, CodeGenState * S,IrNode * noisyTypeParameterListNode)
{
        ;
}

void
noisyModuleDeclBodyCodeGen(State * N, CodeGenState * S,IrNode * noisyModuleDeclBodyNode)
{
        for (IrNode * currentNode = noisyModuleDeclBodyNode; currentNode != NULL; currentNode = currentNode->irRightChild)
        {
                noisyModuleTypeNameDeclCodeGen(N, S, currentNode->irLeftChild);
        }
}

void
noisyModuleDeclCodeGen(State * N, CodeGenState * S,IrNode * noisyModuleDeclNode)
{
        /*
        *       The first module declaration gives its name to the LLVM module we are going to create.
        */
        static int firstTime = 1;
        if (firstTime)
        {
                S->theModule = LLVMModuleCreateWithNameInContext(noisyModuleDeclNode->irLeftChild->symbol->identifier,S->theContext);
                firstTime = 0;
        }
        /*
        *       TODO: Add code for multiple Module declarations.
        */

        IrNode * noisyTypeParameterListNode = RL(noisyModuleDeclNode);
        IrNode * noisyModuleDeclBodyNode = RR(noisyModuleDeclNode);

        noisyTypeParameterListCodeGen(N, S, noisyTypeParameterListNode);
        noisyModuleDeclBodyCodeGen(N, S, noisyModuleDeclBodyNode);
        

}

void 
noisyFunctionDefnCodeGen(State * N, IrNode * noisyFunctionDefnNode)
{
        ;
}


void
noisyProgramCodeGen(State * N, CodeGenState * S,IrNode * noisyProgramNode)
{
        noisyModuleDeclCodeGen(N, S, noisyProgramNode->irLeftChild);

        for (IrNode * currentNode = noisyProgramNode; currentNode->irRightChild != NULL; currentNode = currentNode->irRightChild)
        {
                if (currentNode->irLeftChild->type == kNoisyIrNodeType_PmoduleDecl)
                {
                        noisyModuleDeclCodeGen(N, S, currentNode->irLeftChild);
                }
                else if (currentNode->irLeftChild->type == kNoisyIrNodeType_PfunctionDefn)
                {
                        noisyFunctionDefnCodeGen(N, currentNode->irLeftChild);
                }
                else
                {
                        flexprint(N->Fe, N->Fm, N->Fperr, "Code generation for that is not supported");
                }
        }
}

void
noisyCodeGen(State * N)
{
        /*
        *       Declare the basic code generation state and the necessary data structures for LLVM.
        */
        CodeGenState * S = (CodeGenState *)malloc(sizeof(CodeGenState));
        S->theContext = LLVMContextCreate();
        S->theBuilder = LLVMCreateBuilderInContext(S->theContext);

        noisyProgramCodeGen(N,S,N->noisyIrRoot);

        /*
        *       We need to dispose LLVM structures in order to avoid leaking memory. Free code gen state.
        */

        flexprint(N->Fe,N->Fm,N->Fpg,LLVMPrintModuleToString(S->theModule));
        LLVMDisposeModule(S->theModule);
        LLVMDisposeBuilder(S->theBuilder);
        LLVMContextDispose(S->theContext);
        free(S);
        return ;

}