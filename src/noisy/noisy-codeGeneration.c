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
#include <llvm-c/Core.h>


typedef struct {
         LLVMContextRef theContext;
         LLVMBuilderRef theBuilder;
         LLVMModuleRef  theModule;
} CodeGenState;

LLVMTypeRef getLLVMTypeFromTypeExpr(State *, IrNode *);

/*
*       Takes a basicType IrNode and returns the LLVMTypeRef type.
*       Assumes that basicType->type == kNoisyIrNodeType_PbasicType.
*/
LLVMTypeRef 
getLLVMTypeFromNoisyType(IrNode * basicType)
{
        LLVMTypeRef llvmType;
        if (L(basicType)->type == kNoisyIrNodeType_Tbool)
        {
                llvmType = LLVMInt1Type();
        }
        else if (L(basicType)->type == kNoisyIrNodeType_PintegerType)
        {
                /*
                *       LLVM does not make distintion on signed and unsigned values on its typesystem.
                *       However it can differentiate between them during the operations (e.g signed addition).
                */
                switch (LL(basicType)->type)
                {
                case kNoisyIrNodeType_Tint4:
                case kNoisyIrNodeType_Tnat4:
                        llvmType = LLVMIntType(4);
                        break;
                case kNoisyIrNodeType_Tint8:
                case kNoisyIrNodeType_Tnat8:
                        llvmType = LLVMInt8Type();
                        break;
                case kNoisyIrNodeType_Tint16:
                case kNoisyIrNodeType_Tnat16:
                        llvmType = LLVMInt16Type();
                        break;
                case kNoisyIrNodeType_Tint32:
                case kNoisyIrNodeType_Tnat32:
                        llvmType = LLVMInt32Type();
                        break;
                case kNoisyIrNodeType_Tint64:
                case kNoisyIrNodeType_Tnat64:
                        llvmType = LLVMInt64Type();
                        break;
                case kNoisyIrNodeType_Tint128:
                case kNoisyIrNodeType_Tnat128:
                        llvmType = LLVMInt128Type();
                        break;
                default:
                        break;
                }
        }
        else if (L(basicType)->type == kNoisyIrNodeType_PrealType)
        {
                switch (LL(basicType)->type)
                {
                case kNoisyIrNodeType_Tfloat16:
                        llvmType = LLVMHalfType();
                        break;
                case kNoisyIrNodeType_Tfloat32:
                        llvmType = LLVMFloatType();
                        break;
                case kNoisyIrNodeType_Tfloat64:
                        llvmType = LLVMDoubleType();
                        break;
                case kNoisyIrNodeType_Tfloat128:
                        llvmType = LLVMFP128Type();
                        break;
                default:
                        break;
                }
        }        
        else if(L(basicType)->type == kNoisyIrNodeType_Tstring)
        {
                llvmType = LLVMPointerType(LLVMInt8Type(),0);
        }
        return llvmType;
}

/*
*       Takes an arrayType IrNode and returns the corresponding LLVMTypeRef type
*       Assumes that arrayTypeNode->type == kNoisyIrNodeType_ParrayType.
*       Arrays are passed as arguments by reference like C.
*/
LLVMTypeRef
getLLVMArrayTypeFromNoisy(State * N,IrNode * arrayTypeNode,int firstTime)
{
        if (firstTime == 1)
        {
                return LLVMPointerType(getLLVMArrayTypeFromNoisy(N,R(arrayTypeNode),0),0) ;
        }

        if (L(arrayTypeNode)->type == kNoisyIrNodeType_PtypeExpr)
        {
                return getLLVMTypeFromTypeExpr(N,L(arrayTypeNode));
        }

        int arrayLength = L(arrayTypeNode)->token->integerConst;
        
        LLVMTypeRef elementType = getLLVMArrayTypeFromNoisy(N,R(arrayTypeNode),0);

        return LLVMArrayType (elementType, arrayLength);
}

/*
*       Takes the state N (needed for symbolTable search) and a typeNameNode
*       and returns the corresponding LLVMTypeRef.
*/
LLVMTypeRef
getLLVMTypeFromTypeSymbol(State * N,IrNode * typeNameNode)
{
        Symbol * typeSymbol = commonSymbolTableSymbolForIdentifier(N,NULL,L(typeNameNode)->tokenString);

        IrNode * typeTree = typeSymbol->typeTree;

        if (RL(typeTree)->type == kNoisyIrNodeType_PbasicType)
        {
                return getLLVMTypeFromNoisyType(RL(typeTree));
        }
        else if (RL(typeTree)->type == kNoisyIrNodeType_PanonAggregateType)
        {
                return getLLVMArrayTypeFromNoisy(N,RL(typeTree),1);
        }
        else if (RL(typeTree)->type == kNoisyIrNodeType_PtypeName)
        {
                return getLLVMTypeFromTypeSymbol(N,RL(typeTree));
        }
        else
        {
                return NULL;
        }
}

/*
*       Takes the state N and a TypeExpr node and returns the corresponding
*       LLVMTypeRef. If it fails returns NULL.
*/
LLVMTypeRef
getLLVMTypeFromTypeExpr(State * N, IrNode * typeExpr)
{
        if (L(typeExpr)->type == kNoisyIrNodeType_PbasicType)
        {
                return getLLVMTypeFromNoisyType(L(typeExpr));
        }
        else if (L(typeExpr)->type == kNoisyIrNodeType_PanonAggregateType)
        {
                IrNode * arrayType = LL(typeExpr);
                if (arrayType->type == kNoisyIrNodeType_ParrayType)
                {
                        return getLLVMArrayTypeFromNoisy(N,arrayType,1);
                }
                /*
                *       Lists and other non aggregate types are not supported
                */
                else
                {
                        return NULL;
                }
        }
        else if (L(typeExpr)->type == kNoisyIrNodeType_PtypeName)
        {
                return getLLVMTypeFromTypeSymbol(N,L(typeExpr));
        }
        return NULL;
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
                 // bool typeIsComplete = true;

                IrNode * inputSignature = RL(noisyModuleTypeNameDeclNode);
                IrNode * outputSignature = RR(noisyModuleTypeNameDeclNode);

                int parameterCount = 0;

                if (LL(inputSignature)->type != kNoisyIrNodeType_Tnil)
                {
                        for  (IrNode * iter = LR(inputSignature); iter != NULL; iter = R(iter))
                        {
                                parameterCount++;
                        }
                        /*
                        *       We need to save parameterCount so we can allocate memory for the
                        *       parameters of the generated function.
                        */
                }
                /*
                *       If type == nil then parameterCount = 0
                */

                Symbol * functionSymbol = commonSymbolTableSymbolForIdentifier(N, NULL, L(noisyModuleTypeNameDeclNode)->tokenString);
                functionSymbol->parameterNum = parameterCount;

                LLVMTypeRef * paramArray = (LLVMTypeRef *) malloc(functionSymbol->parameterNum * sizeof(LLVMTypeRef));
        
                if (LL(inputSignature)->type != kNoisyIrNodeType_Tnil)
                {
                        int paramIndex = 0;
                        for  (IrNode * iter = LR(inputSignature); iter != NULL; iter = R(iter))
                        {
                                LLVMTypeRef llvmType = getLLVMTypeFromTypeExpr(N,L(iter));
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
                if (LL(outputSignature)->type != kNoisyIrNodeType_Tnil)
                {
                        outputBasicType = LRL(outputSignature);
                        returnType = getLLVMTypeFromTypeExpr(N,outputBasicType);
                }
                else
                {
                        returnType = LLVMVoidType();
                }

                if (returnType != NULL)
                {
                        LLVMTypeRef funcType = LLVMFunctionType(returnType,paramArray,functionSymbol->parameterNum,0);
                        LLVMAddFunction(S->theModule,functionSymbol->identifier,funcType);
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
        /*
        *       TODO!
        */
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