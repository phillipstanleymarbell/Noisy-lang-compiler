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
#include "common-symbolTable.h"
#include "noisy-codeGeneration.h"
#include "common-irHelpers.h"
#include "noisy-typeCheck.h"
#include <llvm-c/Core.h>


typedef struct {
         LLVMContextRef theContext;
         LLVMBuilderRef theBuilder;
         LLVMModuleRef  theModule;
} CodeGenState;

LLVMTypeRef getLLVMTypeFromTypeExpr(State *, IrNode *);
void noisyStatementListCodeGen(State * N, CodeGenState * S,IrNode * statementListNode);

LLVMTypeRef
getLLVMTypeFromNoisyType(NoisyType noisyType)
{
        LLVMTypeRef llvmType;
        NoisyType basicTypeHelper;
        switch (noisyType.basicType)
        {
        case noisyBool:
                llvmType = LLVMInt1Type();
                break;
        case noisyInt4:
        case noisyNat4:
                llvmType = LLVMIntType(4);
                break;
        case noisyInt8:
        case noisyNat8:
                llvmType = LLVMInt8Type();
                break;
        case noisyInt16:
        case noisyNat16:
                llvmType = LLVMInt16Type();
                break;
        case noisyInt32:
        case noisyNat32:
                llvmType = LLVMInt32Type();
                break;
        case noisyInt64:
        case noisyNat64:
                llvmType = LLVMInt64Type();
                break;
        case noisyInt128:
        case noisyNat128:
                llvmType = LLVMInt128Type();
                break;
        case noisyFloat16:
                llvmType = LLVMHalfType();
                break;
        case noisyFloat32:
                llvmType = LLVMFloatType();
                break;
        case noisyFloat64:
                llvmType = LLVMDoubleType();
                break;
        case noisyFloat128:
                llvmType = LLVMFP128Type();
                break;
        case noisyString:
                llvmType = LLVMPointerType(LLVMInt8Type(),0);
                break;
        case noisyArrayType:
                basicTypeHelper.basicType = noisyType.arrayType;
                llvmType = getLLVMTypeFromNoisyType(basicTypeHelper);
                for (int i = noisyType.dimensions-1; i >= 1; i--)
                {
                        llvmType = LLVMArrayType(llvmType,noisyType.sizeOfDimension[i]);
                }
                llvmType = LLVMPointerType(llvmType,0);
                break;
        default:
                break;
        }
        return llvmType;
}

LLVMValueRef
noisyDeclareFunction(State * N, CodeGenState * S,const char * functionName,IrNode * inputSignature, IrNode * outputSignature)
{
        Symbol * functionSymbol = commonSymbolTableSymbolForIdentifier(N, N->moduleScopes, functionName);

        if (!functionSymbol->isTypeComplete)
        {
                return NULL;
        }

        LLVMTypeRef * paramArray = (LLVMTypeRef *) malloc(functionSymbol->parameterNum * sizeof(LLVMTypeRef));

        if (L(inputSignature)->type != kNoisyIrNodeType_Tnil)
        {
                int paramIndex = 0;
                for  (IrNode * iter = inputSignature; iter != NULL; iter = RR(iter))
                {
                        NoisyType typ = getNoisyTypeFromTypeExpr(N,RL(iter));
                        LLVMTypeRef llvmType = getLLVMTypeFromNoisyType(typ);
                        if (llvmType != NULL)
                        {
                                paramArray[paramIndex] = llvmType;
                        }
                        else
                        {
                                flexprint(N->Fe, N->Fm, N->Fperr, "Code generation for that type is not supported");
                                fatal(N,"Code generation Error\n");
                        }

                        paramIndex++;
                }
        }


        IrNode * outputBasicType;
        LLVMTypeRef returnType;
        /*
        *       Currently we only permit one return argument for functions
        *       just like the C convention.
        */
        if (L(outputSignature)->type != kNoisyIrNodeType_Tnil)
        {
                outputBasicType = RL(outputSignature);
                NoisyType typ = getNoisyTypeFromTypeExpr(N,outputBasicType);
                returnType = getLLVMTypeFromNoisyType(typ);
        }
        else
        {
                returnType = LLVMVoidType();
        }

        LLVMValueRef func;

        if (returnType != NULL)
        {
                LLVMTypeRef funcType = LLVMFunctionType(returnType,paramArray,functionSymbol->parameterNum,0);
                func =  LLVMAddFunction(S->theModule,functionSymbol->identifier,funcType);
        }
        else
        {
                flexprint(N->Fe, N->Fm, N->Fperr, "Code generation for that type is not supported");
                fatal(N,"Code generation Error\n");
        }

        /*
        *       TODO; Maybe deallocation happens elsewhere.
        */
        free(paramArray);
        return func;
}

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
        else if (R(noisyModuleTypeNameDeclNode)->type == kNoisyIrNodeType_PtypeDecl 
                || R(noisyModuleTypeNameDeclNode)->type == kNoisyIrNodeType_PtypeAnnoteDecl )
        {
                /*
                *       Type declarations and type annotations declarations are handled by the parser and
                *       the Noisy's type system and do not generate code.
                */
                return ;
        }
        else if (R(noisyModuleTypeNameDeclNode)->type == kNoisyIrNodeType_PfunctionDecl)
        {
                IrNode * inputSignature = RLL(noisyModuleTypeNameDeclNode);
                IrNode * outputSignature = RRL(noisyModuleTypeNameDeclNode);
                noisyDeclareFunction(N,S,L(noisyModuleTypeNameDeclNode)->tokenString,inputSignature,outputSignature);
        }
}

void
noisyTypeParameterListCodeGen(State * N, CodeGenState * S,IrNode * noisyTypeParameterListNode)
{
        /*
        *       TODO!
        */
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
noisyModuleDeclCodeGen(State * N, CodeGenState * S, IrNode * noisyModuleDeclNode)
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
noisyAssignmentStatementCodeGen(State * N,CodeGenState * S, IrNode * assignmentNode)
{
        ;
}

void
noisyMatchStatementCodeGen(State * N,CodeGenState * S, IrNode * matchNode)
{

}

void
noisyIterateStatementCodeGen(State * N,CodeGenState * S, IrNode * iterateNode)
{

}

void
noisySequenceStatementCodeGen(State * N,CodeGenState * S, IrNode * sequenceNode)
{

}

void
noisyOperatorToleranceDeclCodeGen(State * N,CodeGenState * S, IrNode * toleranceDeclNode)
{

}

void
noisyReturnStatementCodeGen(State * N,CodeGenState * S, IrNode * returnNode)
{

}

void
noisyStatementCodeGen(State * N, CodeGenState * S, IrNode * noisyStatementNode)
{
        switch (L(noisyStatementNode)->type)
        {
        case kNoisyIrNodeType_PassignmentStatement:
                noisyAssignmentStatementCodeGen(N,S,L(noisyStatementNode));
                break;
        case kNoisyIrNodeType_PmatchStatement:
                noisyMatchStatementCodeGen(N,S,L(noisyStatementNode));
                break;
        case kNoisyIrNodeType_PiterateStatement:
                noisyIterateStatementCodeGen(N,S,L(noisyStatementNode));
                break;
        case kNoisyIrNodeType_PsequenceStatement:
                noisySequenceStatementCodeGen(N,S,L(noisyStatementNode));
                break;
        case kNoisyIrNodeType_PscopedStatementList:
                noisyStatementListCodeGen(N,S,LL(noisyStatementNode));
                break;
        case kNoisyIrNodeType_PoperatorToleranceDecl:
                noisyOperatorToleranceDeclCodeGen(N,S,L(noisyStatementNode));
                break;
        case kNoisyIrNodeType_PreturnStatement:
                noisyReturnStatementCodeGen(N,S,L(noisyStatementNode));
                break;
        default:
                flexprint(N->Fe, N->Fm, N->Fperr, "Code generation for that statement is not supported");
                fatal(N,"Code generation Error\n");
                break;
        }        
}

void
noisyStatementListCodeGen(State * N, CodeGenState * S,IrNode * statementListNode)
{
        for (IrNode * iter = statementListNode; iter != NULL; iter=R(iter))
        {
                if (L(iter) != NULL)
                {
                        noisyStatementCodeGen(N,S,L(iter));
                }
        }
}

void 
noisyFunctionDefnCodeGen(State * N, CodeGenState * S,IrNode * noisyFunctionDefnNode)
{
        LLVMValueRef func = LLVMGetNamedFunction(S->theModule,L(noisyFunctionDefnNode)->tokenString);
        /*
        *       Declare local function
        */
        if (func == NULL)
        {
                func = noisyDeclareFunction(N,S,noisyFunctionDefnNode->irLeftChild->tokenString,RL(noisyFunctionDefnNode),RRL(noisyFunctionDefnNode));
                if (func == NULL)
                {
                        /*
                        *       If func depends on Module parameters we skip its definition until its loaded.
                        */
                        return ;
                }
        }

        LLVMBasicBlockRef funcEntry = LLVMAppendBasicBlock(func, "entry");
        LLVMPositionBuilderAtEnd(S->theBuilder, funcEntry);

        noisyStatementListCodeGen(N,S,RR(noisyFunctionDefnNode)->irRightChild->irLeftChild);
}


void
noisyProgramCodeGen(State * N, CodeGenState * S,IrNode * noisyProgramNode)
{
        noisyModuleDeclCodeGen(N, S, noisyProgramNode->irLeftChild);

        for (IrNode * currentNode = R(noisyProgramNode); currentNode != NULL; currentNode = currentNode->irRightChild)
        {
                if (currentNode->irLeftChild->type == kNoisyIrNodeType_PmoduleDecl)
                {
                        noisyModuleDeclCodeGen(N, S, currentNode->irLeftChild);
                }
                else if (currentNode->irLeftChild->type == kNoisyIrNodeType_PfunctionDefn)
                {
                        noisyFunctionDefnCodeGen(N,S,currentNode->irLeftChild);
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