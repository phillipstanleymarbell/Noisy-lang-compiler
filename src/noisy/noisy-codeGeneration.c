#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include<float.h>
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
#include <llvm-c/BitWriter.h>
#include <llvm-c/Analysis.h>
#include <llvm-c/Transforms/Coroutines.h>
#include <llvm-c/Transforms/InstCombine.h>
#include <llvm-c/Transforms/PassManagerBuilder.h>



typedef struct FrameListNode {
        LLVMValueRef frameValue;
        struct FrameListNode * next;
} FrameListNode;

typedef FrameListNode * FrameList;

typedef struct {
        LLVMContextRef          theContext;
        LLVMBuilderRef          theBuilder;
        LLVMModuleRef           theModule;
        LLVMValueRef            currentFunction;
        LLVMPassManagerRef      thePassManager;
        FrameList               frameList;
        LLVMBasicBlockRef       suspendBB;
        LLVMBasicBlockRef       cleanupBB;
} CodeGenState;

FrameList
noisyAddFrameToList(FrameList list,LLVMValueRef frame)
{
        FrameListNode * newFrameNode = (FrameListNode*) malloc(sizeof(FrameListNode));
        newFrameNode->frameValue = frame;
        newFrameNode->next= list;
        list = newFrameNode;
        return list;
}


FrameList
noisyRemoveFrameFromList(FrameList list)
{
        if (list == NULL)
        {
                return NULL;
        }
        else
        {
                FrameListNode * del = list;
                list = del->next;
                free(del);
        }
        return list;
}

LLVMValueRef
noisyGetFrameFromList(FrameList list)
{
        if (list == NULL)
        {
                return NULL;
        }
        return list->frameValue;
}

void
noisyDestroyCoroutineFrames(CodeGenState * S)
{
        while (S->frameList != NULL)
        {
                int argNum = 1;
                LLVMValueRef args[1];
                args[0] = noisyGetFrameFromList(S->frameList);
                S->frameList = noisyRemoveFrameFromList(S->frameList);
                LLVMValueRef callFunc = LLVMGetNamedFunction(S->theModule,"llvm.coro.destroy");
                LLVMBuildCall2(S->theBuilder,LLVMGetElementType(LLVMTypeOf(callFunc)),callFunc,args,argNum,"");
        }
}


LLVMTypeRef getLLVMTypeFromTypeExpr(State *, IrNode *);
void noisyStatementListCodeGen(State * N, CodeGenState * S,IrNode * statementListNode);
LLVMValueRef noisyExpressionCodeGen(State * N,CodeGenState * S, IrNode * noisyExpressionNode);
LLVMValueRef noisyFunctionDefnCodeGen(State * N, CodeGenState * S,IrNode * noisyFunctionDefnNode);

/*
*       This functions declares all coroutine intrinsic functions so we can use them in our program.
*/
void
noisyDeclareCoroutineIntrinsics(CodeGenState * S)
{
        /*
        *       Declare token llvm.coro.id(i32,i8*,i8*,i8*)
        */
        LLVMTypeRef returnType = LLVMTokenTypeInContext(S->theContext);
        LLVMTypeRef paramTypes[4];
        int parameterNums = 4;
        paramTypes[0] = LLVMInt32TypeInContext(S->theContext);
        paramTypes[1] = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
        paramTypes[2] = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
        paramTypes[3] = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
        LLVMTypeRef functionType = LLVMFunctionType(returnType,paramTypes,parameterNums,false);
        LLVMAddFunction(S->theModule,"llvm.coro.id",functionType);

        /*
        *       Declare i1 @llvm.coro.alloc(token)
        */
        returnType = LLVMInt1TypeInContext(S->theContext);
        parameterNums = 1;
        paramTypes[0] = LLVMTokenTypeInContext(S->theContext);
        functionType = LLVMFunctionType(returnType,paramTypes,parameterNums,false);
        LLVMAddFunction(S->theModule,"llvm.coro.alloc",functionType);

        /*
        *       Declare i32 @llvm.coro.size.i32()
        */
       returnType = LLVMInt32TypeInContext(S->theContext);
       parameterNums = 0;
       functionType = LLVMFunctionType(returnType,paramTypes,parameterNums,false);
       LLVMAddFunction(S->theModule,"llvm.coro.size.i32",functionType);

       /*
       *        Declare i8* @llvm.coro.begin(token,i8*)
       */
        returnType = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
        parameterNums = 2;
        paramTypes[0] = LLVMTokenTypeInContext(S->theContext);
        paramTypes[1] = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
        functionType = LLVMFunctionType(returnType,paramTypes,parameterNums,false);
        LLVMAddFunction(S->theModule,"llvm.coro.begin",functionType);

        /*
        *        Declare i8  @llvm.coro.suspend(token, i1)
        */
        returnType = LLVMInt8TypeInContext(S->theContext);
        parameterNums = 2;
        paramTypes[0] = LLVMTokenTypeInContext(S->theContext);
        paramTypes[1] = LLVMInt1TypeInContext(S->theContext);
        functionType = LLVMFunctionType(returnType,paramTypes,parameterNums,false);
        LLVMAddFunction(S->theModule,"llvm.coro.suspend",functionType);

        /*
        *        Declare i8* @llvm.coro.free(token, i8*)
        */
        returnType = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
        parameterNums = 2;
        paramTypes[0] = LLVMTokenTypeInContext(S->theContext);
        paramTypes[1] = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
        functionType = LLVMFunctionType(returnType,paramTypes,parameterNums,false);
        LLVMAddFunction(S->theModule,"llvm.coro.free",functionType);

        /*
        *       Declare i1 @llvm.coro.end(i8*, i1)
        */
        returnType = LLVMInt1TypeInContext(S->theContext);
        parameterNums = 2;
        paramTypes[0] = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
        paramTypes[1] = LLVMInt1TypeInContext(S->theContext);
        functionType = LLVMFunctionType(returnType,paramTypes,parameterNums,false);
        LLVMAddFunction(S->theModule,"llvm.coro.end",functionType);

        /*
        *       Declare void @llvm.coro.resume(i8*)
        */
        returnType = LLVMVoidTypeInContext(S->theContext);
        parameterNums = 1;
        paramTypes[0] = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
        functionType = LLVMFunctionType(returnType,paramTypes,parameterNums,false);
        LLVMAddFunction(S->theModule,"llvm.coro.resume",functionType);

        /*
        *       Declare i8* @llvm.coro.promise(i8*, i32, i1)
        */
        returnType = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
        parameterNums = 3;
        paramTypes[0] = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
        paramTypes[1] = LLVMInt32TypeInContext(S->theContext);
        paramTypes[2] = LLVMInt1TypeInContext(S->theContext);
        functionType = LLVMFunctionType(returnType,paramTypes,parameterNums,false);
        LLVMAddFunction(S->theModule,"llvm.coro.promise",functionType);

        /*
        *       Declare void @llvm.coro.destroy(i8*)
        */
        returnType = LLVMVoidTypeInContext(S->theContext);
        parameterNums = 1;
        paramTypes[0] = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
        functionType = LLVMFunctionType(returnType,paramTypes,parameterNums,false);
        LLVMAddFunction(S->theModule,"llvm.coro.destroy",functionType);
}

NoisyType
findConstantNoisyType(IrNode * constantNode)
{
        if (constantNode->noisyType.basicType > noisyInitType
        && constantNode->noisyType.basicType != noisyRealConstType
        && constantNode->noisyType.basicType != noisyIntegerConstType
        && constantNode->noisyType.basicType != noisyArrayType)
        {
                return constantNode->noisyType;
        }
        else if (constantNode->noisyType.basicType == noisyArrayType)
        {
                NoisyType retType;
                retType.basicType = constantNode->noisyType.arrayType;
                return retType;
        }
        else if (constantNode->type == kNoisyIrNodeType_PassignmentStatement)
        {
                return RRL(constantNode)->noisyType;
        }
        else
        {
                return findConstantNoisyType(constantNode->irParent);
        }
}

LLVMTypeRef
getLLVMTypeFromNoisyType(CodeGenState * S,NoisyType noisyType,bool byRef,int limit)
{
        LLVMTypeRef llvmType;
        NoisyType basicTypeHelper;
        switch (noisyType.basicType)
        {
        case noisyBool:
                llvmType = LLVMInt1TypeInContext(S->theContext);
                break;
        case noisyInt4:
        case noisyNat4:
                llvmType = LLVMIntTypeInContext(S->theContext,4);
                break;
        case noisyInt8:
        case noisyNat8:
                llvmType = LLVMInt8TypeInContext(S->theContext);
                break;
        case noisyInt16:
        case noisyNat16:
                llvmType = LLVMInt16TypeInContext(S->theContext);
                break;
        case noisyInt32:
        case noisyIntegerConstType:
        case noisyNat32:
                llvmType = LLVMInt32TypeInContext(S->theContext);
                break;
        case noisyInt64:
        case noisyNat64:
                llvmType = LLVMInt64TypeInContext(S->theContext);
                break;
        case noisyInt128:
        case noisyNat128:
                llvmType = LLVMInt128TypeInContext(S->theContext);
                break;
        case noisyFloat16:
                llvmType = LLVMHalfTypeInContext(S->theContext);
                break;
        case noisyFloat32:
        case noisyRealConstType:
                llvmType = LLVMFloatTypeInContext(S->theContext);
                break;
        case noisyFloat64:
                llvmType = LLVMDoubleTypeInContext(S->theContext);
                break;
        case noisyFloat128:
                llvmType = LLVMFP128TypeInContext(S->theContext);
                break;
        case noisyString:
                llvmType = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
                break;
        case noisyArrayType:
                if (byRef)
                {
                        basicTypeHelper.basicType = noisyType.arrayType;
                        llvmType = getLLVMTypeFromNoisyType(S,basicTypeHelper,false,0);
                        for (int i = noisyType.dimensions-1; i >= 1+limit; i--)
                        {
                                llvmType = LLVMArrayType(llvmType,noisyType.sizeOfDimension[i]);
                        }
                        llvmType = LLVMPointerType(llvmType,0);
                }
                else
                {
                        basicTypeHelper.basicType = noisyType.arrayType;
                        llvmType = getLLVMTypeFromNoisyType(S,basicTypeHelper,false,0);
                        for (int i = noisyType.dimensions-1; i >= limit; i--)
                        {
                                llvmType = LLVMArrayType(llvmType,noisyType.sizeOfDimension[i]);
                        }
                }
                break;
        default:
                break;
        }
        return llvmType;
}

LLVMValueRef
noisyDeclareFunction(State * N, CodeGenState * S,IrNode * funcNameNode,IrNode * inputSignature, IrNode * outputSignature)
{
        Symbol * functionSymbol = funcNameNode->symbol;

        if (!functionSymbol->isTypeComplete)
        {
                return NULL;
        }

        IrNode * outputBasicType;
        LLVMTypeRef returnType;
        NoisyType returnNoisyType;
        /*
        *       Currently we only permit one return argument for functions
        *       just like the C convention.
        */
        bool returnsArray = false;
        if (functionSymbol->isChannel)
        {
                /*
                *       Coroutines return their coroutine frame which is an i8* value.
                */
                returnType = LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0);
        }
        else if (L(outputSignature)->type != kNoisyIrNodeType_Tnil)
        {
                outputBasicType = RL(outputSignature);
                returnNoisyType = getNoisyTypeFromTypeExpr(N,outputBasicType);
                returnType = getLLVMTypeFromNoisyType(S,returnNoisyType,true,0);
                if (returnNoisyType.basicType == noisyArrayType)
                {
                        returnType = LLVMVoidTypeInContext(S->theContext);
                        returnsArray = true;
                }
        }
        else
        {
                returnType = LLVMVoidTypeInContext(S->theContext);
        }

        /*
        *       TODO; Change parameter type for input channel of a function.
        */
        int parameterNumber = functionSymbol->parameterNum;
        if (returnsArray)
        {
                parameterNumber++;
        }
        LLVMTypeRef * paramArray = (LLVMTypeRef *) malloc(parameterNumber * sizeof(LLVMTypeRef));

        if (L(inputSignature)->type != kNoisyIrNodeType_Tnil)
        {
                int paramIndex = 0;
                for  (IrNode * iter = inputSignature; iter != NULL; iter = RR(iter))
                {
                        NoisyType typ = getNoisyTypeFromTypeExpr(N,RL(iter));
                        LLVMTypeRef llvmType = getLLVMTypeFromNoisyType(S,typ,true,0);

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
        if (returnsArray)
        {
                paramArray[parameterNumber-1] = getLLVMTypeFromNoisyType(S,returnNoisyType,true,0);
        }

        LLVMValueRef func;

        if (returnType != NULL)
        {
                LLVMTypeRef funcType = LLVMFunctionType(returnType,paramArray,parameterNumber,0);
                if (!strcmp(functionSymbol->identifier,"init"))
                {
                        func =  LLVMAddFunction(S->theModule,"main",funcType);
                }
                else
                {
                        func =  LLVMAddFunction(S->theModule,functionSymbol->identifier,funcType);
                }
        }
        else
        {
                flexprint(N->Fe, N->Fm, N->Fperr, "Code generation for that type is not supported");
                fatal(N,"Code generation Error\n");
        }

        /*
        *       TODO; This creates errors when we return an input array parameter.
        */
        // if (returnsArray)
        // {
        //         char * retAttr = "sret";
        //         unsigned kindId = LLVMGetEnumAttributeKindForName (retAttr,strlen(retAttr));
        //         LLVMAttributeRef attrRef = LLVMCreateTypeAttribute(S->theContext,kindId,getLLVMTypeFromNoisyType(S,returnNoisyType,true,0));
        //         LLVMAddAttributeAtIndex(func,parameterNumber,attrRef);
        // }

        /*
        *       TODO; Maybe deallocation happens elsewhere.
        */
        functionSymbol->llvmPointer = func;
        free(paramArray);
        return func;
}

LLVMValueRef
noisyGetArrayPositionPointer(State * N,CodeGenState * S, Symbol * arraySym,IrNode * noisyQualifiedIdentifierNode)
{
        LLVMTypeRef arrayType;
        LLVMValueRef arrayPtr = arraySym->llvmPointer;
        int lim = 0;
        bool firstTime = true;
        /*
        *       Field select handling.
        */
        for (IrNode * iter = R(noisyQualifiedIdentifierNode); iter != NULL; iter = R(iter))
        {
                LLVMValueRef idxValue = noisyExpressionCodeGen(N,S,LR(iter));
                LLVMValueRef idxValueList[] = {LLVMConstInt(LLVMInt32TypeInContext(S->theContext),0,false) ,idxValue};
                idxValueList[1] = idxValue;
                arrayType = getLLVMTypeFromNoisyType(S,arraySym->noisyType,false,lim);
                if (firstTime && arraySym->symbolType == kNoisySymbolTypeParameter)
                {
                        arrayType = getLLVMTypeFromNoisyType(S,arraySym->noisyType,true,lim);
                        LLVMValueRef loadArrayValue = LLVMBuildLoad2(S->theBuilder,arrayType,arrayPtr,"");
                        idxValueList[0] = idxValueList[1];

                        /*
                        *       TODO; If we want to throw exception messages for out of bounds indexing, we can create a runtime library
                        *       for different types of runtime errors and link it.
                        */

                        // LLVMValueRef boundCheckValue = LLVMBuildICmp(S->theBuilder,LLVMIntULT,idxValue,LLVMConstInt(LLVMInt32Type(),arraySym->noisyType.sizeOfDimension[lim],false),"k_boundCheck");
                        arrayPtr = LLVMBuildGEP2(S->theBuilder,LLVMGetElementType(arrayType),loadArrayValue,idxValueList,1,"k_arrIdx");
                        // arrayPtr = LLVMBuildSelect(S->theBuilder,boundCheckValue,arrayPtr,LLVMGetPoison(LLVMTypeOf(arrayPtr)),"k_safeGep");
                        firstTime = false;
                }
                else
                {
                        // LLVMValueRef boundCheckValue = LLVMBuildICmp(S->theBuilder,LLVMIntULT,idxValue,LLVMConstInt(LLVMInt32Type(),arraySym->noisyType.sizeOfDimension[lim],false),"k_boundCheck");
                        arrayPtr = LLVMBuildGEP2(S->theBuilder,arrayType,arrayPtr,idxValueList,2,"k_arrIdx");
                        // arrayPtr = LLVMBuildSelect(S->theBuilder,boundCheckValue,arrayPtr,LLVMGetPoison(LLVMTypeOf(arrayPtr)),"k_safeGep");
                }
                lim++;
        }

        if (firstTime && arraySym->symbolType == kNoisySymbolTypeParameter)
        {
                arrayType = getLLVMTypeFromNoisyType(S,arraySym->noisyType,true,lim);
                LLVMValueRef loadArrayValue = LLVMBuildLoad2(S->theBuilder,arrayType,arrayPtr,"");
                arrayPtr = loadArrayValue;
        }
        return arrayPtr;
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
                        constValue = LLVMConstInt(LLVMInt64TypeInContext(S->theContext),noisyConstantDeclNode->token->integerConst,false);
                        globalValue = LLVMAddGlobal (S->theModule, LLVMInt64TypeInContext(S->theContext),  L(noisyModuleTypeNameDeclNode)->tokenString);
                }
                else if (noisyConstantDeclNode->type == kNoisyIrNodeType_TrealConst)
                {
                        constValue = LLVMConstReal(LLVMDoubleTypeInContext(S->theContext),noisyConstantDeclNode->token->realConst);
                        globalValue = LLVMAddGlobal (S->theModule, LLVMDoubleTypeInContext(S->theContext),  L(noisyModuleTypeNameDeclNode)->tokenString);
                }
                else if (noisyConstantDeclNode->type == kNoisyIrNodeType_TboolConst)
                {
                        constValue = LLVMConstInt(LLVMInt1TypeInContext(S->theContext),noisyConstantDeclNode->token->integerConst,false);
                        globalValue = LLVMAddGlobal (S->theModule, LLVMInt1TypeInContext(S->theContext),  L(noisyModuleTypeNameDeclNode)->tokenString);
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
                noisyDeclareFunction(N,S,L(noisyModuleTypeNameDeclNode),inputSignature,outputSignature);
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

LLVMValueRef
noisyFactorCodeGen(State * N,CodeGenState * S,IrNode * noisyFactorNode)
{
        if (L(noisyFactorNode)->type == kNoisyIrNodeType_TintegerConst)
        {
                /*
                *       TODO; find a way to change integer and float constant type.
                */
                noisyFactorNode->noisyType = findConstantNoisyType(L(noisyFactorNode));
                return LLVMConstInt(getLLVMTypeFromNoisyType(S,noisyFactorNode->noisyType,0,false),L(noisyFactorNode)->token->integerConst,true);
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_TrealConst)
        {
                noisyFactorNode->noisyType = findConstantNoisyType(L(noisyFactorNode));
                return LLVMConstReal(getLLVMTypeFromNoisyType(S,noisyFactorNode->noisyType,0,false), L(noisyFactorNode)->token->realConst);
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_TstringConst)
        {
                return LLVMConstStringInContext (S->theContext,L(noisyFactorNode)->token->stringConst,strlen(L(noisyFactorNode)->token->stringConst), false);
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_TboolConst)
        {
                return LLVMConstInt(LLVMInt1TypeInContext(S->theContext),L(noisyFactorNode)->token->integerConst,false);
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_PqualifiedIdentifier)
        {
                Symbol * identifierSymbol = LL(noisyFactorNode)->symbol;
                if (identifierSymbol->symbolType == kNoisySymbolTypeParameter)
                {
                        if (identifierSymbol->noisyType.basicType != noisyArrayType
                        && identifierSymbol->noisyType.basicType != noisyString)
                        {
                                return LLVMGetParam(S->currentFunction,identifierSymbol->paramPosition);
                        }
                }
                if (identifierSymbol->noisyType.basicType != noisyArrayType && identifierSymbol->noisyType.basicType != noisyNamegenType)
                {
                        char * name;
                        asprintf(&name,"val_%s",identifierSymbol->identifier);
                        return LLVMBuildLoad2(S->theBuilder,getLLVMTypeFromNoisyType(S,identifierSymbol->noisyType,false,0),identifierSymbol->llvmPointer,name);
                }
                else if (identifierSymbol->noisyType.basicType == noisyArrayType)
                {
                        LLVMValueRef arrayPtr = noisyGetArrayPositionPointer(N,S,identifierSymbol,L(noisyFactorNode));
                        char * name;
                        asprintf(&name,"val_%s",identifierSymbol->identifier);
                        NoisyType retType = identifierSymbol->noisyType;

                        if (LR(noisyFactorNode) != NULL)
                        {
                                retType.basicType = identifierSymbol->noisyType.arrayType;
                                return LLVMBuildLoad2(S->theBuilder,getLLVMTypeFromNoisyType(S,retType,false,0),arrayPtr,name);
                        }
                        else if (identifierSymbol->symbolType == kNoisySymbolTypeParameter)
                        {
                                return arrayPtr;
                        }
                        else
                        {
                                LLVMValueRef idxValueList[] = {LLVMConstInt(LLVMInt32TypeInContext(S->theContext),0,false),LLVMConstInt(LLVMInt32TypeInContext(S->theContext),0,false)};
                                return LLVMBuildGEP2(S->theBuilder,getLLVMTypeFromNoisyType(S,identifierSymbol->noisyType,false,0),arrayPtr,idxValueList,2,"k_arrayDecay");
                        }
                }
                else if (identifierSymbol->noisyType.basicType == noisyNamegenType)
                {
                        Symbol * channelSymbol = identifierSymbol->noisyType.functionDefinition;
                        if (channelSymbol->isChannel)
                        {
                                /*
                                *       We return the coroutine stack frame.
                                */
                                return identifierSymbol->llvmPointer;
                        }
                }
                /*
                *       Noisy namegen type is handled on load expression and on namegen invoke shorthand.
                *       TODO; Add channel implementation.
                */
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_PnamegenInvokeShorthand)
        {
                Symbol * functionSymbol = LL(noisyFactorNode)->symbol;
                if (functionSymbol->noisyType.basicType == noisyNamegenType)
                {
                        functionSymbol = functionSymbol->noisyType.functionDefinition;
                }
                IrNode * inputSignature = L(functionSymbol->typeTree);
                IrNode * outputSignature = R(functionSymbol->typeTree);

                if (inputSignature->type == kNoisyIrNodeType_PwriteTypeSignature)
                {
                        inputSignature = L(inputSignature);
                }

                if (outputSignature->type == kNoisyIrNodeType_PreadTypeSignature)
                {
                        outputSignature = L(outputSignature);
                }

                LLVMValueRef * args;
                LLVMTypeRef * argTyp;

                LLVMTypeRef retType;
                NoisyType noisyReturnType;
                bool returnsArray = false;
                int parameterNumber = functionSymbol->parameterNum;

                if (outputSignature->irLeftChild->type != kNoisyIrNodeType_Tnil)
                {
                        noisyReturnType = outputSignature->irLeftChild->symbol->noisyType;
                        if (noisyReturnType.basicType == noisyArrayType)
                        {
                                retType = LLVMVoidTypeInContext(S->theContext);
                                returnsArray = true;
                                parameterNumber++;
                        }
                        else
                        {
                                retType = getLLVMTypeFromNoisyType(S,noisyReturnType,true,0);
                        }
                }
                else
                {
                        retType = LLVMVoidTypeInContext(S->theContext);
                }

                if (parameterNumber == 0)
                {
                        args = NULL;
                        argTyp = NULL;
                }
                else
                {
                        args = calloc(parameterNumber,sizeof(LLVMValueRef));
                        argTyp = calloc(parameterNumber,sizeof(LLVMTypeRef));

                }

                for (IrNode * iter = LR(noisyFactorNode); iter != NULL; iter = RR(iter))
                {
                        IrNode * argName = L(iter);
                        IrNode * expr = RL(iter);

                        int pos = 0;
                        for (IrNode * iter2 = inputSignature; iter2 != NULL; iter2 = RR(iter2))
                        {
                                if (!strcmp(L(iter2)->tokenString,argName->tokenString))
                                {
                                        break;
                                }
                                pos++;
                        }
                        LLVMValueRef expressionValue = noisyExpressionCodeGen(N,S,expr);
                        /*
                        *       Passing an array as an argument of a function.
                        */
                        if (expr->noisyType.basicType == noisyArrayType)
                        {
                                /*
                                *       When we pass an array as argument of a function we create a copy of the original array and we pass it by reference
                                *       to the function. This way functions dont mutate the parameter arrays.
                                *
                                *       TODO; Maybe we need to allocate the copies on the heap and not on the stack because for every function call
                                *       with arrays we double the space occupied in the stack.
                                */
                                LLVMValueRef copyArrayAddress = LLVMBuildAlloca(S->theBuilder,getLLVMTypeFromNoisyType(S,expr->noisyType,false,0),"k_copyArr");

                                LLVMValueRef oneVal[] = {LLVMConstInt(LLVMInt64TypeInContext(S->theContext),1,false)};
                                LLVMTypeRef arrayType = getLLVMTypeFromNoisyType(S,expr->noisyType,false,0);
                                LLVMValueRef sizeOfExprVal = LLVMBuildGEP2(S->theBuilder,arrayType,LLVMConstPointerNull(LLVMPointerType(arrayType,0)),oneVal,1,"");

                                sizeOfExprVal = LLVMBuildPtrToInt(S->theBuilder,sizeOfExprVal,LLVMInt64TypeInContext(S->theContext),"k_sizeOfT");
                                expressionValue = LLVMBuildMemCpy(S->theBuilder,copyArrayAddress,0,expressionValue,0,sizeOfExprVal);

                                LLVMValueRef idxValueList[] = {LLVMConstInt(LLVMInt32TypeInContext(S->theContext),0,false),LLVMConstInt(LLVMInt32TypeInContext(S->theContext),0,false)};
                                expressionValue = LLVMBuildGEP2(S->theBuilder,getLLVMTypeFromNoisyType(S,expr->noisyType,false,0),copyArrayAddress,idxValueList,2,"k_arrayDecay");
                        }
                        args[pos] = expressionValue;
                }

                LLVMValueRef retArrayAddress;
                if (returnsArray)
                {
                        retArrayAddress = LLVMBuildAlloca(S->theBuilder,getLLVMTypeFromNoisyType(S,noisyReturnType,false,0),"k_retAlloc");
                        LLVMValueRef idxValueList[] = {LLVMConstInt(LLVMInt32TypeInContext(S->theContext),0,false),LLVMConstInt(LLVMInt32TypeInContext(S->theContext),0,false)};
                        LLVMValueRef retArray = LLVMBuildGEP2(S->theBuilder,getLLVMTypeFromNoisyType(S,noisyReturnType,false,0),retArrayAddress,idxValueList,2,"k_arrayDecay");
                        args[parameterNumber-1] = retArray;
                }

                int i = 0;
                if (parameterNumber != 0)
                {
                        for (IrNode * iter2 = inputSignature; iter2 != NULL; iter2 = RR(iter2))
                        {
                                argTyp[i] = getLLVMTypeFromNoisyType(S,L(iter2)->symbol->noisyType,true,0);
                                i++;
                        }
                }

                if (returnsArray)
                {
                        argTyp[i++] = getLLVMTypeFromNoisyType(S,noisyReturnType,true,0); 
                }

                LLVMTypeRef funcType = LLVMFunctionType(retType,argTyp,i,false);
                LLVMValueRef callValue = LLVMBuildCall2(S->theBuilder,funcType,functionSymbol->llvmPointer,args,parameterNumber,"");

                if (returnsArray)
                {
                        return retArrayAddress;
                }
                return callValue;
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_Pexpression)
        {
                return noisyExpressionCodeGen(N,S,L(noisyFactorNode));
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_PtypeMaxExpr)
        {
                NoisyType exprType = getNoisyTypeFromBasicType(LL(noisyFactorNode));
                switch (exprType.basicType)
                {
                case noisyBool:
                        return LLVMConstInt(LLVMInt1TypeInContext(S->theContext),1,false);
                        break;
                case noisyInt4:
                        return LLVMConstInt(LLVMIntType(4),7,true);
                        break;
                case noisyInt8:
                        return LLVMConstInt(LLVMInt8TypeInContext(S->theContext),127,true);
                        break;
                case noisyInt16:
                        return LLVMConstInt(LLVMInt16TypeInContext(S->theContext),32767,true);
                        break;
                case noisyInt32:
                        return LLVMConstInt(LLVMInt32TypeInContext(S->theContext),2147483647,true);
                        break;
                case noisyInt64:
                        return LLVMConstInt(LLVMInt64TypeInContext(S->theContext),9223372036854775807,true);
                        break;
                // case noisyInt128:
                //         return LLVMConstInt(LLVMInt128Type(),0x7fffffffffffffffffffffffffffffff,true);
                //         break;
                case noisyNat4:
                        return LLVMConstInt(LLVMIntType(4),15,false);
                        break;
                case noisyNat8:
                        return LLVMConstInt(LLVMInt8TypeInContext(S->theContext),255,false);
                        break;
                case noisyNat16:
                        return LLVMConstInt(LLVMInt16TypeInContext(S->theContext),65535,false);
                        break;
                case noisyNat32:
                        return LLVMConstInt(LLVMInt32TypeInContext(S->theContext),0xffffffff,false);
                        break;
                case noisyNat64:
                        return LLVMConstInt(LLVMInt64TypeInContext(S->theContext),0xffffffffffffffff,false);
                        break;
                // case noisyNat128:
                //         return LLVMConstInt(LLVMInt128TypeInContext(S->theContext),0xffffffffffffffffffffffffffffffffLL,false);
                //         break;
                case noisyFloat16:
                        /*
                        *       TODO; May be wrong for half float.
                        */
                        return LLVMConstReal(LLVMHalfTypeInContext(S->theContext),65.504f);
                        break;
                case noisyFloat32:
                        return LLVMConstReal(LLVMFloatTypeInContext(S->theContext),FLT_MAX);
                        break;
                case noisyFloat64:
                        return LLVMConstReal(LLVMDoubleTypeInContext(S->theContext),DBL_MAX);
                        break;
                case noisyFloat128:
                        return LLVMConstReal(LLVMFP128TypeInContext(S->theContext),LDBL_MAX);
                        break;
                default:
                        break;
                }
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_PtypeMinExpr)
        {
                NoisyType exprType = getNoisyTypeFromBasicType(LL(noisyFactorNode));
                switch (exprType.basicType)
                {
                case noisyBool:
                        return LLVMConstInt(LLVMInt1TypeInContext(S->theContext),0,false);
                        break;
                case noisyInt4:
                        return LLVMConstInt(LLVMIntType(4),-8,true);
                        break;
                case noisyInt8:
                        return LLVMConstInt(LLVMInt8TypeInContext(S->theContext),-128,true);
                        break;
                case noisyInt16:
                        return LLVMConstInt(LLVMInt16TypeInContext(S->theContext),-32768,true);
                        break;
                case noisyInt32:
                        return LLVMConstInt(LLVMInt32TypeInContext(S->theContext),-2147483648,true);
                        break;
                case noisyInt64:
                        return LLVMConstInt(LLVMInt64TypeInContext(S->theContext),0x1000000000000000LL,true);
                        break;
                // case noisyInt128:
                //         return LLVMConstInt(LLVMInt128TypeInContext(S->theContext),0x10000000000000000000000000000000,true);
                //         break;
                case noisyNat4:
                        return LLVMConstInt(LLVMIntType(4),0,false);
                        break;
                case noisyNat8:
                        return LLVMConstInt(LLVMInt8TypeInContext(S->theContext),0,false);
                        break;
                case noisyNat16:
                        return LLVMConstInt(LLVMInt16TypeInContext(S->theContext),0,false);
                        break;
                case noisyNat32:
                        return LLVMConstInt(LLVMInt32TypeInContext(S->theContext),0,false);
                        break;
                case noisyNat64:
                        return LLVMConstInt(LLVMInt64TypeInContext(S->theContext),0,false);
                        break;
                case noisyNat128:
                        return LLVMConstInt(LLVMInt128TypeInContext(S->theContext),0,false);
                        break;
                case noisyFloat16:
                        /*
                        *       TODO; May be wrong for half float.
                        */
                        return LLVMConstReal(LLVMHalfTypeInContext(S->theContext),-65.504f);
                        break;
                case noisyFloat32:
                        return LLVMConstReal(LLVMFloatTypeInContext(S->theContext),FLT_MIN);
                        break;
                case noisyFloat64:
                        return LLVMConstReal(LLVMDoubleTypeInContext(S->theContext),DBL_MIN);
                        break;
                case noisyFloat128:
                        return LLVMConstReal(LLVMFP128TypeInContext(S->theContext),LDBL_MIN);
                        break;
                default:
                        break;
                }
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_Tnil)
        {
                LLVMTypeRef nilType = getLLVMTypeFromNoisyType(S,findConstantNoisyType(noisyFactorNode),false,0);
                return LLVMConstNull(nilType);
        }
        /*
        *       TODO; We reach here when we call channel ops.
        */
        return LLVMConstNull(LLVMInt32TypeInContext(S->theContext));
}

LLVMValueRef
noisyUnaryOpCodeGen(State * N, CodeGenState * S,IrNode * noisyUnaryOpNode, LLVMValueRef termVal, IrNode * noisyFactorNode)
{
        NoisyType factorNoisyType = noisyFactorNode->noisyType;
        LLVMTypeRef factorType = getLLVMTypeFromNoisyType(S,factorNoisyType,0,false);

        switch (L(noisyUnaryOpNode)->type)
        {
        /*
        *       case kNoisyIrNodeType_Tplus: Does not do anything.
        */
        case kNoisyIrNodeType_Tminus:
                if (noisyIsOfType(noisyFactorNode->noisyType,noisyIntegerConstType))
                {
                        /*
                        *       We use sub instruction for integer neg.
                        */
                        return LLVMBuildSub(S->theBuilder,LLVMConstInt(factorType,0,true),termVal,"k_negRes");
                }
                else
                {
                        return LLVMBuildFNeg(S->theBuilder,termVal,"k_negRes");
                }
                break;
        case kNoisyIrNodeType_Ttilde:
                /*
                *       We use term XOR 1 for bitwise negation.
                */
                return LLVMBuildXor(S->theBuilder,termVal,LLVMConstInt(factorType,1,false),"k_notRes");
                break;
        case kNoisyIrNodeType_PunaryBoolOp:
                /*
                *       We only have 1 unary boolean operator the "!"-negation operator.
                */
                return LLVMBuildXor(S->theBuilder,termVal,LLVMConstInt(LLVMInt1TypeInContext(S->theContext),1,false),"k_notRes");
                break;
        case kNoisyIrNodeType_Tlength:
                return LLVMConstInt(getLLVMTypeFromNoisyType(S,findConstantNoisyType(noisyFactorNode),false,0),factorNoisyType.sizeOfDimension[factorNoisyType.dimensions-1],false);
                break;
        case kNoisyIrNodeType_TchannelOperator:
                /*
                *       If the factor has noisyNamegen type it means we read from the output channel of a coroutine.
                */
                if (factorNoisyType.basicType == noisyNamegenType)
                {
                        int argNum = 1;
                        LLVMValueRef args[3];
                        args[0] = termVal;
                        LLVMValueRef callFunc = LLVMGetNamedFunction(S->theModule,"llvm.coro.resume");
                        LLVMBuildCall2(S->theBuilder,LLVMGetElementType(LLVMTypeOf(callFunc)),callFunc,args,argNum,"");

                        NoisyType outputNoisyType = RL(factorNoisyType.functionDefinition->typeTree)->symbol->noisyType;
                        LLVMTypeRef outputType = getLLVMTypeFromNoisyType(S,outputNoisyType,false,0);
                        argNum = 3;
                        args[0] = termVal;
                        args[1] = LLVMConstInt(LLVMInt32TypeInContext(S->theContext),0,false);
                        args[2] = LLVMConstInt(LLVMInt1TypeInContext(S->theContext),0,false);
                        callFunc = LLVMGetNamedFunction(S->theModule,"llvm.coro.promise");
                        LLVMValueRef promiseAddr = LLVMBuildCall2(S->theBuilder,LLVMGetElementType(LLVMTypeOf(callFunc)),callFunc,args,argNum,"k_promiseAddrRaw");
                        promiseAddr = LLVMBuildBitCast(S->theBuilder,promiseAddr,LLVMPointerType(outputType,0),"k_promiseAddr");
                        return LLVMBuildLoad2(S->theBuilder,outputType,promiseAddr,"k_promiseVal");

                }
                return termVal;
                break;
        default:
                /*
                *       TODO; length sort reverse (library calls probably) and channel operator
                */
                return termVal;
                break;
        }
}


LLVMValueRef
noisyTermCodeGen(State * N,CodeGenState * S,IrNode * noisyTermNode)
{
        IrNode * factorNode = NULL;
        IrNode * unaryOpNode = NULL;
        bool typeCast = false;

        /*
        *       This flag is needed because the form of the tree is different based on whether a prefix exists
        *       on the term expression.
        */
        bool prefixExists = false;

        if (L(noisyTermNode)->type == kNoisyIrNodeType_PbasicType)
        {
                typeCast = true;
                prefixExists = true;
        }

        if (L(noisyTermNode)->type == kNoisyIrNodeType_Pfactor)
        {
                factorNode = L(noisyTermNode);
        }
        else if (R(noisyTermNode)->type == kNoisyIrNodeType_Pfactor)
        {
                factorNode = R(noisyTermNode);
                if (L(noisyTermNode)->type == kNoisyIrNodeType_PunaryOp)
                {
                        unaryOpNode = L(noisyTermNode);
                        prefixExists = true;
                }
        }
        else if (RR(noisyTermNode)->type == kNoisyIrNodeType_Pfactor)
        {

                factorNode = RR(noisyTermNode);
                if (RL(noisyTermNode)->type == kNoisyIrNodeType_PunaryOp)
                {
                        unaryOpNode = RL(noisyTermNode);
                        prefixExists = true;
                }
        }

        LLVMValueRef termVal = noisyFactorCodeGen(N,S,factorNode);

        for (IrNode * iter = prefixExists ? R(factorNode) : R(noisyTermNode) ; iter != NULL; iter = RR(iter))
        {
                LLVMValueRef factorIterVal = noisyFactorCodeGen(N,S,RL(iter));

                switch (LL(iter)->type)
                {
                case kNoisyIrNodeType_Tasterisk:
                        if (noisyIsOfType(noisyTermNode->noisyType,noisyIntegerConstType))
                        {
                                termVal = LLVMBuildMul(S->theBuilder,termVal,factorIterVal,"k_mulRes");
                        }
                        else
                        {
                                termVal = LLVMBuildFMul(S->theBuilder,termVal,factorIterVal,"k_mulRes");
                        }
                        break;
                case kNoisyIrNodeType_Tdivide:
                        if (noisyIsOfType(noisyTermNode->noisyType,noisyIntegerConstType))
                        {
                                if (noisyIsSigned(noisyTermNode->noisyType))
                                {
                                        termVal = LLVMBuildSDiv(S->theBuilder,termVal,factorIterVal,"k_divRes");
                                }
                                else
                                {
                                        termVal = LLVMBuildUDiv(S->theBuilder,termVal,factorIterVal,"k_divRes");
                                }
                        }
                        else
                        {
                                termVal = LLVMBuildFDiv(S->theBuilder,termVal,factorIterVal,"k_divRes"); 
                        }
                        break;
                case kNoisyIrNodeType_Tpercent:
                        if (noisyIsOfType(noisyTermNode->noisyType,noisyIntegerConstType))
                        {
                                if (noisyIsSigned(noisyTermNode->noisyType))
                                {
                                        termVal = LLVMBuildSRem(S->theBuilder,termVal,factorIterVal,"k_modRes");
                                }
                                else
                                {
                                        termVal = LLVMBuildURem(S->theBuilder,termVal,factorIterVal,"k_modRes");
                                }
                        }
                        else
                        {
                                /*
                                *       TODO; I think that type system does not permit us to use remainder for floats.
                                */
                                termVal = LLVMBuildFRem(S->theBuilder,termVal,factorIterVal,"k_modRes"); 
                        }
                        break;
                case kNoisyIrNodeType_Tand:
                        termVal = LLVMBuildAnd(S->theBuilder,termVal,factorIterVal,"k_andRes");
                        break;
                case kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp:
                        switch (LLL(iter)->type)
                        {
                        case kNoisyIrNodeType_TlogicalAnd:
                                termVal = LLVMBuildAnd(S->theBuilder,termVal,factorIterVal,"k_andRes");
                                /* code */
                                break;
                        case kNoisyIrNodeType_Txor:
                                termVal = LLVMBuildXor(S->theBuilder,termVal,factorIterVal,"k_xorRes");
                                break;
                        default:
                                break;
                        }
                        break;
                default:
                        break;
                }
        }

        if (unaryOpNode != NULL)
        {
                termVal = noisyUnaryOpCodeGen(N,S,unaryOpNode,termVal,factorNode);
        }

        if (typeCast && factorNode->noisyType.basicType != noisyIntegerConstType && factorNode->noisyType.basicType != noisyRealConstType)
        {
                NoisyType factorType = factorNode->noisyType;
                NoisyType termType = noisyTermNode->noisyType;
                LLVMTypeRef destType = getLLVMTypeFromNoisyType(S,termType,false,0);
                if (noisyIsOfType(factorType,noisyIntegerConstType))
                {
                        if (noisyIsOfType(termType,noisyIntegerConstType))
                        {
                                if (noisyIsSigned(factorType))
                                {
                                        if (noisyIsSigned(termType))
                                        {
                                                /*
                                                *       Convert from signed to signed.
                                                */
                                                if (factorType.basicType <= termType.basicType)
                                                {
                                                        /*
                                                        *       Convert from smaller to bigger signed integer.
                                                        */
                                                        termVal = LLVMBuildSExtOrBitCast(S->theBuilder,termVal,destType,"k_typeCastRes");
                                                }
                                                else if (factorType.basicType > termType.basicType)
                                                {
                                                        /*
                                                        *       Convert from bigger integer to smaller.
                                                        */
                                                        termVal = LLVMBuildTrunc(S->theBuilder,termVal,destType,"k_typeCastRes");
                                                }
                                        }
                                        else
                                        {
                                                /*
                                                *       Convert from signed to unsigned.
                                                *       This condition checks if the factorType has less or more bits than termType.
                                                */
                                                if (factorType.basicType + 6 < termType.basicType)
                                                {
                                                        termVal = LLVMBuildZExt(S->theBuilder,termVal,destType,"k_typeCastRes");
                                                }
                                                else if (factorType.basicType + 6 > termType.basicType)
                                                {
                                                        /*
                                                        *       Convert from bigger integer to smaller (from signed to unsigned)
                                                        */
                                                        termVal = LLVMBuildTrunc(S->theBuilder,termVal,destType,"k_typeCastRes");
                                                }
                                        }
                                }
                                else
                                {
                                        if (noisyIsSigned(termType))
                                        {
                                                /*
                                                *       Convert from unsigned to signed.
                                                */
                                                if (factorType.basicType - 6 < termType.basicType)
                                                {
                                                        /*
                                                        *       Convert from smaller to bigger signed integer.
                                                        */
                                                        termVal = LLVMBuildSExtOrBitCast(S->theBuilder,termVal,destType,"k_typeCastRes");
                                                }
                                                else if (factorType.basicType - 6 > termType.basicType)
                                                {
                                                        /*
                                                        *       Convert from bigger integer to smaller.
                                                        */
                                                        termVal = LLVMBuildTrunc(S->theBuilder,termVal,destType,"k_typeCastRes");
                                                }
                                        }
                                        else
                                        {
                                                /*
                                                *       Convert from unsigned to unsigned.
                                                */
                                                if (factorType.basicType <= termType.basicType)
                                                {
                                                        termVal = LLVMBuildZExtOrBitCast(S->theBuilder,termVal,destType,"k_typeCastRes");
                                                }
                                                else if (factorType.basicType > termType.basicType)
                                                {
                                                        /*
                                                        *       Convert from bigger integer to smaller (from unsigned to unsigned)
                                                        */
                                                        termVal = LLVMBuildTrunc(S->theBuilder,termVal,destType,"k_typeCastRes");
                                                }
                                        }

                                }
                        }
                        else if (noisyIsOfType(termType,noisyRealConstType))
                        {
                                /*
                                *       Convert from integer to float
                                */
                                if (noisyIsSigned(factorType))
                                {
                                        termVal = LLVMBuildSIToFP(S->theBuilder,termVal,destType,"k_typeCastRes");
                                }
                                else
                                {
                                        termVal = LLVMBuildUIToFP(S->theBuilder,termVal,destType,"k_typeCastRes");
                                }
                        }
                }
                else if (noisyIsOfType(factorType,noisyRealConstType))
                {
                        if (noisyIsOfType(termType,noisyIntegerConstType))
                        {
                                if (noisyIsSigned(termType))
                                {
                                        termVal = LLVMBuildFPToSI(S->theBuilder,termVal,destType,"k_typeCastRes");
                                }
                                else
                                {
                                        termVal = LLVMBuildFPToUI(S->theBuilder,termVal,destType,"k_typeCastRes");
                                }
                        }
                        else if (noisyIsOfType(termType,noisyRealConstType))
                        {
                                if (factorType.basicType < termType.basicType)
                                {
                                        termVal = LLVMBuildFPExt(S->theBuilder,termVal,destType,"k_typeCastRes");
                                }
                                else
                                {
                                        termVal = LLVMBuildFPTrunc(S->theBuilder,termVal,destType,"k_typeCastRes");
                                }
                        }
                }
        }
        return termVal;
}


LLVMValueRef
noisyExpressionCodeGen(State * N,CodeGenState * S, IrNode * noisyExpressionNode)
{
        if (L(noisyExpressionNode)->type == kNoisyIrNodeType_Pterm)
        {
                LLVMValueRef exprVal =  noisyTermCodeGen(N,S,L(noisyExpressionNode));

                for (IrNode * iter = R(noisyExpressionNode); iter != NULL; iter = RR(iter))
                {
                        IrNode * operatorNode = L(iter);
                        IrNode * termNode = RL(iter);
                        LLVMValueRef termIterVal = noisyTermCodeGen(N,S,termNode);

                        switch (L(operatorNode)->type)
                        {
                        case kNoisyIrNodeType_Tplus:
                                if (noisyIsOfType(termNode->noisyType,noisyIntegerConstType))
                                {
                                        exprVal = LLVMBuildAdd(S->theBuilder,exprVal,termIterVal,"k_sumRes");
                                }
                                else
                                {
                                        exprVal = LLVMBuildFAdd(S->theBuilder,exprVal,termIterVal,"k_sumRes");
                                }
                                break;
                        case kNoisyIrNodeType_Tminus:
                                if (noisyIsOfType(termNode->noisyType,noisyIntegerConstType))
                                {
                                        exprVal = LLVMBuildSub(S->theBuilder,exprVal,termIterVal,"k_subRes");
                                }
                                else
                                {
                                        exprVal = LLVMBuildFSub(S->theBuilder,exprVal,termIterVal,"k_subRes");
                                }
                                break;
                        case kNoisyIrNodeType_TrightShift:
                                exprVal = LLVMBuildLShr(S->theBuilder,exprVal,termIterVal,"k_rShiftRes");
                                break;
                        case kNoisyIrNodeType_TleftShift:
                                exprVal = LLVMBuildShl(S->theBuilder,exprVal,termIterVal,"k_lShiftRes");
                                break;
                        case kNoisyIrNodeType_TbitwiseOr:
                                exprVal = LLVMBuildOr(S->theBuilder,exprVal,termIterVal,"k_bitwiseOrRes");
                                break;
                        case kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp:
                                /*
                                *       We only have the "or" low precedence binary op.
                                */
                                exprVal = LLVMBuildOr(S->theBuilder,exprVal,termIterVal,"k_bitwiseOrRes");
                                break;
                        case kNoisyIrNodeType_PcmpOp:
                                switch (LL(operatorNode)->type )
                                {
                                case kNoisyIrNodeType_Tequals:
                                        if (noisyIsOfType(termNode->noisyType,noisyRealConstType))
                                        {
                                                exprVal = LLVMBuildFCmp(S->theBuilder,LLVMRealOEQ,exprVal,termIterVal,"k_equalRes");
                                        }
                                        else
                                        {
                                                exprVal = LLVMBuildICmp(S->theBuilder,LLVMIntEQ,exprVal,termIterVal,"k_equalRes");
                                        }
                                        break;
                                case kNoisyIrNodeType_TnotEqual:
                                        if (noisyIsOfType(termNode->noisyType,noisyRealConstType))
                                        {
                                                exprVal = LLVMBuildFCmp(S->theBuilder,LLVMRealONE,exprVal,termIterVal,"k_notEqualRes");
                                        }
                                        else
                                        {
                                                exprVal = LLVMBuildICmp(S->theBuilder,LLVMIntNE,exprVal,termIterVal,"k_notEqualRes");
                                        }
                                        break;
                                case kNoisyIrNodeType_TgreaterThan:
                                        if (noisyIsOfType(termNode->noisyType,noisyRealConstType))
                                        {
                                                exprVal = LLVMBuildFCmp(S->theBuilder,LLVMRealOGT,exprVal,termIterVal,"k_gtRes");
                                        }
                                        else
                                        {
                                                if (noisyIsSigned(termNode->noisyType))
                                                {
                                                        exprVal = LLVMBuildICmp(S->theBuilder,LLVMIntSGT,exprVal,termIterVal,"k_gtRes");
                                                }
                                                else
                                                {
                                                        exprVal = LLVMBuildICmp(S->theBuilder,LLVMIntUGT,exprVal,termIterVal,"k_gtRes");
                                                }
                                        }
                                        break;
                                case kNoisyIrNodeType_TgreaterThanEqual:
                                        if (noisyIsOfType(termNode->noisyType,noisyRealConstType))
                                        {
                                                exprVal = LLVMBuildFCmp(S->theBuilder,LLVMRealOGE,exprVal,termIterVal,"k_geRes");
                                        }
                                        else
                                        {
                                                if (noisyIsSigned(termNode->noisyType))
                                                {
                                                        exprVal = LLVMBuildICmp(S->theBuilder,LLVMIntSGE,exprVal,termIterVal,"k_geRes");
                                                }
                                                else
                                                {
                                                        exprVal = LLVMBuildICmp(S->theBuilder,LLVMIntUGE,exprVal,termIterVal,"k_geRes");
                                                }
                                        }
                                        break;
                                case kNoisyIrNodeType_TlessThan:
                                        if (noisyIsOfType(termNode->noisyType,noisyRealConstType))
                                        {
                                                exprVal = LLVMBuildFCmp(S->theBuilder,LLVMRealOLT,exprVal,termIterVal,"k_ltRes");
                                        }
                                        else
                                        {
                                                if (noisyIsSigned(termNode->noisyType))
                                                {
                                                        exprVal = LLVMBuildICmp(S->theBuilder,LLVMIntSLT,exprVal,termIterVal,"k_ltRes");
                                                }
                                                else
                                                {
                                                        exprVal = LLVMBuildICmp(S->theBuilder,LLVMIntULT,exprVal,termIterVal,"k_ltRes");
                                                }
                                        }
                                        break;
                                case kNoisyIrNodeType_TlessThanEqual:
                                        if (noisyIsOfType(termNode->noisyType,noisyRealConstType))
                                        {
                                                exprVal = LLVMBuildFCmp(S->theBuilder,LLVMRealOLE,exprVal,termIterVal,"k_leRes");
                                        }
                                        else
                                        {
                                                if (noisyIsSigned(termNode->noisyType))
                                                {
                                                        exprVal = LLVMBuildICmp(S->theBuilder,LLVMIntSLE,exprVal,termIterVal,"k_leRes");
                                                }
                                                else
                                                {
                                                        exprVal = LLVMBuildICmp(S->theBuilder,LLVMIntULE,exprVal,termIterVal,"k_leRes");
                                                }
                                        }
                                        break;
                                default:
                                        break;
                                }
                                break;
                        default:
                                break;
                        }

                }
                return exprVal;
        }
        else if (L(noisyExpressionNode)->type == kNoisyIrNodeType_PanonAggrCastExpr)
        {
                if (LL(noisyExpressionNode)->type == kNoisyIrNodeType_ParrayCastExpr)
                {
                        /*
                        *       TODO; Add support for non-constant expressions. It would also need to change the assignment code
                        *       since we use the memcpy intrinsic in the assignment codegen.
                        */
                        if (LLL(noisyExpressionNode)->type == kNoisyIrNodeType_PinitList)
                        {
                                int size = noisyExpressionNode->noisyType.sizeOfDimension[0];
                                LLVMValueRef * elemValArr = calloc(size,sizeof(LLVMValueRef));

                                int i = 0;
                                for (IrNode * iter = LLL(noisyExpressionNode); iter != NULL; iter = R(iter))
                                {
                                        LLVMValueRef exprVal = noisyExpressionCodeGen(N,S,LL(iter));

                                        if (LLVMIsConstant(exprVal))
                                        {
                                                elemValArr[i] = exprVal;
                                                i++;
                                        }
                                        else
                                        {
                                                char *	details;

                                                asprintf(&details, "Expressions in array cast expression need to be constant\n");
                                                noisySemanticError(N,LL(iter),details);
                                                noisySemanticErrorRecovery(N);
                                        }
                                }
                                LLVMValueRef constArrVal = LLVMConstArray(getLLVMTypeFromNoisyType(S,LLL(noisyExpressionNode)->irLeftChild->irLeftChild->noisyType,false,0),elemValArr,size);
                                return constArrVal;
                        }
                        else if (LLL(noisyExpressionNode)->type == kNoisyIrNodeType_TintegerConst)
                        {
                                int size = noisyExpressionNode->noisyType.sizeOfDimension[0];
                                LLVMValueRef * elemValArr = calloc(size,sizeof(LLVMValueRef));

                                int i = 0;
                                IrNode * iter;
                                for (iter = LLR(noisyExpressionNode); iter != NULL; iter = R(iter))
                                {
                                        if (L(iter)->type == kNoisyIrNodeType_Tasterisk)
                                        {
                                                break;
                                        }

                                        LLVMValueRef exprVal = noisyExpressionCodeGen(N,S,LL(iter));

                                        if (LLVMIsConstant(exprVal))
                                        {
                                                elemValArr[i] = exprVal;
                                                i++;
                                        }
                                        else
                                        {
                                                char *	details;

                                                asprintf(&details, "Expressions in array cast expression need to be constant\n");
                                                noisySemanticError(N,LL(iter),details);
                                                noisySemanticErrorRecovery(N);
                                        }
                                }

                                if (iter != NULL)
                                {
                                        LLVMValueRef exprVal = noisyExpressionCodeGen(N,S,RLL(iter));
                                        if (LLVMIsConstant(exprVal))
                                        {
                                                for (int j = i;j < size;j++)
                                                {
                                                        elemValArr[j] = exprVal;
                                                }
                                        }
                                        else
                                        {
                                                char *	details;

                                                asprintf(&details, "Expressions in array cast expression need to be constant\n");
                                                noisySemanticError(N,LL(iter),details);
                                                noisySemanticErrorRecovery(N);
                                        }
                                }
                                LLVMValueRef constArrVal = LLVMConstArray(getLLVMTypeFromNoisyType(S,LLR(noisyExpressionNode)->irLeftChild->irLeftChild->noisyType,false,0),elemValArr,size);
                                return constArrVal;
                        }
                }
        }
        else if (L(noisyExpressionNode)->type == kNoisyIrNodeType_PloadExpr)
        {
                LLVMValueRef functionVal = LLVMGetNamedFunction(S->theModule,LR(noisyExpressionNode)->symbol->identifier);
                if (functionVal == NULL)
                {
                        LLVMBasicBlockRef currentBlock = LLVMGetInsertBlock(S->theBuilder);
                        IrNode * funcDefn = noisyExpressionNode->noisyType.functionDefinition->functionDefinition;
                        L(funcDefn)->symbol->isTypeComplete = true;
                        LLVMValueRef prevFunc = S->currentFunction;
                        functionVal = noisyFunctionDefnCodeGen(N,S,noisyExpressionNode->noisyType.functionDefinition->functionDefinition);
                        L(funcDefn)->symbol->isTypeComplete = false;
                        S->currentFunction =  prevFunc;
                        LLVMPositionBuilderAtEnd(S->theBuilder,currentBlock);
                }
                /*
                *       If we load a channel
                */
                Symbol * funcSymbol = noisyExpressionNode->noisyType.functionDefinition;
                if (funcSymbol->isChannel)
                {
                        if (funcSymbol->parameterNum == 0)
                        {
                                functionVal = LLVMBuildCall2(S->theBuilder,LLVMGetElementType(LLVMTypeOf(functionVal)),functionVal,NULL,0,"k_hdl");
                                S->frameList =  noisyAddFrameToList(S->frameList,functionVal);
                        }
                }
                return functionVal;
        }
        /*
        *       Unreachable.
        */
        return LLVMConstInt(LLVMInt32TypeInContext(S->theContext),42,true);
}

void
noisyAssignmentStatementCodeGen(State * N,CodeGenState * S, IrNode * noisyAssignmentStatementNode)
{
        if (R(noisyAssignmentStatementNode)->type == kNoisyIrNodeType_Xseq)
        {
                /*
                *       If it is an actual assignment and not a declaration
                */
                /*
                *      Eval expression. Store the result.
                */
                LLVMValueRef exprVal = noisyExpressionCodeGen(N,S,RRL(noisyAssignmentStatementNode));

                for (IrNode * iter = L(noisyAssignmentStatementNode); iter != NULL; iter = R(iter))
                {
                        if (LL(iter)->type == kNoisyIrNodeType_Tnil)
                        {
                                /*
                                *       When we assign to nil nothing happens(?).
                                */
                        }
                        else if (LL(iter)->type == kNoisyIrNodeType_PqualifiedIdentifier)
                        {
                                Symbol * lvalSym = LLL(iter)->symbol;
                                LLVMTypeRef lvalType;
                                LLVMValueRef lvalVal;
                                char * name;
                                switch (RLL(noisyAssignmentStatementNode)->type)
                                {
                                case kNoisyIrNodeType_TcolonAssign:
                                        if (lvalSym->noisyType.basicType != noisyNamegenType)
                                        {
                                                char * name;
                                                asprintf(&name,"var_%s",lvalSym->identifier);
                                                lvalSym->llvmPointer = LLVMBuildAlloca(S->theBuilder,getLLVMTypeFromNoisyType(S,lvalSym->noisyType,false,0),name);
                                        }
                                        break;
                                case kNoisyIrNodeType_TplusAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(S,lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        if (noisyIsOfType(lvalSym->noisyType,noisyIntegerConstType))
                                        {
                                                exprVal = LLVMBuildAdd(S->theBuilder,lvalVal,exprVal,"k_sumRes");
                                        }
                                        else
                                        {
                                                exprVal = LLVMBuildFAdd(S->theBuilder,lvalVal,exprVal,"k_sumRes");
                                        }
                                        break;
                                case kNoisyIrNodeType_TminusAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(S,lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        if (noisyIsOfType(lvalSym->noisyType,noisyIntegerConstType))
                                        {
                                                exprVal = LLVMBuildSub(S->theBuilder,lvalVal,exprVal,"k_subRes");
                                        }
                                        else
                                        {
                                                exprVal = LLVMBuildFSub(S->theBuilder,lvalVal,exprVal,"k_subRes");
                                        }
                                        break;
                                case kNoisyIrNodeType_TasteriskAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(S,lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        if (noisyIsOfType(lvalSym->noisyType,noisyIntegerConstType))
                                        {
                                                exprVal = LLVMBuildMul(S->theBuilder,lvalVal,exprVal,"k_mulRes");
                                        }
                                        else
                                        {
                                                exprVal = LLVMBuildFMul(S->theBuilder,lvalVal,exprVal,"k_mulRes");
                                        }
                                        break;
                                case kNoisyIrNodeType_TdivideAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(S,lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        if (noisyIsOfType(lvalSym->noisyType,noisyIntegerConstType))
                                        {
                                                if (noisyIsSigned(lvalSym->noisyType))
                                                {
                                                        exprVal = LLVMBuildSDiv(S->theBuilder,lvalVal,exprVal,"k_divRes");
                                                }
                                                else
                                                {
                                                        exprVal = LLVMBuildUDiv(S->theBuilder,lvalVal,exprVal,"k_divRes");
                                                }
                                        }
                                        else
                                        {
                                                exprVal = LLVMBuildFDiv(S->theBuilder,lvalVal,exprVal,"k_divRes");
                                        }
                                        break;
                                case kNoisyIrNodeType_TpercentAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(S,lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        if (noisyIsOfType(lvalSym->noisyType,noisyIntegerConstType))
                                        {
                                                if (noisyIsSigned(lvalSym->noisyType))
                                                {
                                                        exprVal = LLVMBuildSRem(S->theBuilder,lvalVal,exprVal,"k_modRes");
                                                }
                                                else
                                                {
                                                        exprVal = LLVMBuildURem(S->theBuilder,lvalVal,exprVal,"k_modRes");
                                                }
                                        }
                                        break;
                                case kNoisyIrNodeType_TorAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(S,lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        exprVal = LLVMBuildOr(S->theBuilder,lvalVal,exprVal,"k_orRes");
                                        break;
                                case kNoisyIrNodeType_TandAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(S,lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        exprVal = LLVMBuildAnd(S->theBuilder,lvalVal,exprVal,"k_andRes");
                                        break;
                                case kNoisyIrNodeType_TxorAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(S,lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        exprVal = LLVMBuildXor(S->theBuilder,lvalVal,exprVal,"k_xorRes");
                                        break;
                                case kNoisyIrNodeType_TleftShiftAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(S,lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        exprVal = LLVMBuildShl(S->theBuilder,lvalVal,exprVal,"k_lShiftRes");
                                        break;
                                case kNoisyIrNodeType_TrightShiftAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(S,lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        exprVal = LLVMBuildLShr(S->theBuilder,lvalVal,exprVal,"k_lShiftRes");
                                        break;
                                case kNoisyIrNodeType_TchannelOperatorAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        /*
                                        *       If we write to the ouptput channel of a coroutine.
                                        */
                                        if (lvalSym->symbolType == kNoisySymbolTypeReturnParameter)
                                        {
                                                LLVMBuildStore(S->theBuilder,exprVal,lvalSym->llvmPointer);
                                                int argNum = 2;
                                                LLVMValueRef args[2];
                                                args[0] = LLVMConstNull(LLVMTokenTypeInContext(S->theContext));
                                                args[1] = LLVMConstInt(LLVMInt1TypeInContext(S->theContext),0,false);
                                                LLVMValueRef funcCall = LLVMGetNamedFunction(S->theModule,"llvm.coro.suspend");
                                                LLVMValueRef suspend = LLVMBuildCall2(S->theBuilder,LLVMGetElementType(LLVMTypeOf(funcCall)),funcCall,args,argNum,"");
                                                LLVMBasicBlockRef resumeBB = LLVMInsertBasicBlockInContext(S->theContext,LLVMGetNextBasicBlock(LLVMGetInsertBlock(S->theBuilder)),"coroResume");

                                                LLVMValueRef switchVal = LLVMBuildSwitch(S->theBuilder,suspend,S->suspendBB,2);
                                                LLVMAddCase(switchVal,LLVMConstInt(LLVMInt8TypeInContext(S->theContext),0,false),resumeBB);
                                                LLVMAddCase(switchVal,LLVMConstInt(LLVMInt8TypeInContext(S->theContext),1,false),S->cleanupBB);

                                                LLVMPositionBuilderAtEnd(S->theBuilder,resumeBB);
                                                return;
                                        }
                                        break;
                                default:
                                        break;
                                }

                                if (RRL(noisyAssignmentStatementNode)->noisyType.basicType == noisyArrayType)
                                {
                                        /*
                                        *       When we have an array cast on the rval of an assignment.
                                        */
                                        LLVMValueRef oneVal[] = {LLVMConstInt(LLVMInt64TypeInContext(S->theContext),1,false)};

                                        /*
                                        *       Solution on how to implement sizeof of a type
                                        *       https://stackoverflow.com/questions/14608250/how-can-i-find-the-size-of-a-type
                                        */

                                        LLVMValueRef sizeOfExprVal;

                                        LLVMValueRef srcArrayValue;
                                        LLVMValueRef dstPtrVal;
                                        if (lvalSym->symbolType == kNoisySymbolTypeParameter)
                                        {
                                                dstPtrVal = LLVMBuildLoad2(S->theBuilder,getLLVMTypeFromNoisyType(S,lvalSym->noisyType,true,0),lvalSym->llvmPointer,"k_loadParam");
                                        }
                                        else
                                        {
                                                dstPtrVal = lvalSym->llvmPointer;
                                        }
                                        if (RRL(noisyAssignmentStatementNode)->irLeftChild->type == kNoisyIrNodeType_PanonAggrCastExpr)
                                        {
                                                sizeOfExprVal= LLVMBuildGEP2(S->theBuilder,LLVMTypeOf(exprVal),LLVMConstPointerNull(LLVMPointerType(LLVMTypeOf(exprVal),0)),oneVal,1,"");
                                                sizeOfExprVal = LLVMBuildPtrToInt(S->theBuilder,sizeOfExprVal,LLVMInt64TypeInContext(S->theContext),"k_sizeOfT");
                                                srcArrayValue = LLVMAddGlobal(S->theModule,LLVMTypeOf(exprVal),"k_arrConst");
                                                LLVMSetInitializer(srcArrayValue,exprVal);
                                                LLVMSetGlobalConstant(srcArrayValue,true);
                                                dstPtrVal = LLVMBuildBitCast(S->theBuilder,dstPtrVal,LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0),"");
                                        }
                                        else
                                        {
                                                LLVMTypeRef arrayType = getLLVMTypeFromNoisyType(S,RRL(noisyAssignmentStatementNode)->noisyType,false,0);
                                                sizeOfExprVal= LLVMBuildGEP2(S->theBuilder,arrayType,LLVMConstPointerNull(LLVMPointerType(arrayType,0)),oneVal,1,"");
                                                sizeOfExprVal = LLVMBuildPtrToInt(S->theBuilder,sizeOfExprVal,LLVMInt64TypeInContext(S->theContext),"k_sizeOfT");
                                                srcArrayValue = exprVal;
                                                dstPtrVal = dstPtrVal;
                                        }

                                        LLVMBuildMemCpy(S->theBuilder,dstPtrVal,0,srcArrayValue,0,sizeOfExprVal);
                                }
                                else if (lvalSym->noisyType.basicType == noisyArrayType)
                                {
                                        /*
                                        *       When we have an array on lval of an assignment.
                                        */
                                        LLVMBuildStore(S->theBuilder,exprVal,noisyGetArrayPositionPointer(N,S,lvalSym,LL(iter)));
                                }
                                else if (lvalSym->symbolType == kNoisySymbolTypeReturnParameter)
                                {
                                        /*
                                        *       TODO; This is placeholder to avoid segafault for fib.n. It needs to change when we implement channels.
                                        */
                                        ;
                                }
                                else if (lvalSym->noisyType.basicType != noisyNamegenType)
                                {
                                        LLVMBuildStore(S->theBuilder,exprVal,lvalSym->llvmPointer);
                                }
                                else
                                {
                                        lvalSym->llvmPointer = exprVal;
                                }
                        }
                }
        }
        else
        {
                for (IrNode * iter = L(noisyAssignmentStatementNode); iter != NULL; iter = R(iter))
                {
                        if (LL(iter)->type == kNoisyIrNodeType_PqualifiedIdentifier)
                        {
                                Symbol * identifierSymbol = LLL(iter)->symbol;
                                if (identifierSymbol->noisyType.basicType != noisyNamegenType)
                                {
                                        char * name;
                                        asprintf(&name,"var_%s",identifierSymbol->identifier);
                                        identifierSymbol->llvmPointer = LLVMBuildAlloca(S->theBuilder,getLLVMTypeFromNoisyType(S,identifierSymbol->noisyType,false,0),name);
                                }
                        }
                }
        }
}

void
noisyMatchStatementCodeGen(State * N,CodeGenState * S, IrNode * matchNode)
{
        if (L(matchNode)->type == kNoisyIrNodeType_Tmatchseq)
        {
                LLVMBasicBlockRef thenBlock;
                LLVMBasicBlockRef afterBlock = LLVMAppendBasicBlock(S->currentFunction,"after");
                LLVMBasicBlockRef elseBlock, prevThenBlock;

                for (IrNode * iter = R(matchNode); iter != NULL; iter = RR(iter))
                {
                        LLVMValueRef condVal = noisyExpressionCodeGen(N,S,L(iter));

                        thenBlock = LLVMAppendBasicBlock(S->currentFunction,"then");
                        if (RR(iter) != NULL)
                        {
                                elseBlock = LLVMAppendBasicBlock(S->currentFunction,"else");
                        }
                        else
                        {
                                prevThenBlock = thenBlock;
                                elseBlock = afterBlock;
                        }

                        LLVMBuildCondBr(S->theBuilder,condVal,thenBlock,elseBlock);
                        LLVMPositionBuilderAtEnd(S->theBuilder,thenBlock);
                        noisyStatementListCodeGen(N,S,RLL(iter));
                        /*
                        *       TODO; This is questionable. I added it so we can have a return statemnt inside statements that end with branc instruction.
                        *       Probably it works.
                        */
                        LLVMValueRef terminatorValue = LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(S->theBuilder));
                        if (terminatorValue != NULL)
                        {
                                /*
                                *       If we do not return then we add branch normally.
                                */
                                if (LLVMIsAReturnInst(terminatorValue) == NULL)
                                {
                                        LLVMBuildBr(S->theBuilder,afterBlock);
                                }
                        }
                        else
                        {
                                LLVMBuildBr(S->theBuilder,afterBlock);
                        }

                        if (RR(iter) != NULL)
                        {
                                LLVMPositionBuilderAtEnd(S->theBuilder,elseBlock);
                        }
                        thenBlock = elseBlock;
                }
                LLVMMoveBasicBlockAfter(afterBlock,prevThenBlock);
                LLVMPositionBuilderAtEnd(S->theBuilder,afterBlock);
        }
        else
        {
                /*
                *       Match statement. Noisy spec defines that the order of evaluation of the guardedStatements is random.
                *       However, in this implementation we evaluate them in the order they have been written by the programmer.
                */
                LLVMBasicBlockRef thenBlock,afterBlock;
                for (IrNode * iter = R(matchNode); iter != NULL; iter = RR(iter))
                {
                        LLVMValueRef condVal = noisyExpressionCodeGen(N,S,L(iter));
                        thenBlock = LLVMAppendBasicBlock(S->currentFunction,"then");
                        afterBlock = LLVMAppendBasicBlock(S->currentFunction,"after");

                        LLVMBuildCondBr(S->theBuilder,condVal,thenBlock,afterBlock);

                        LLVMPositionBuilderAtEnd(S->theBuilder,thenBlock);
                        noisyStatementListCodeGen(N,S,RLL(iter));

                        LLVMValueRef terminatorValue = LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(S->theBuilder));;
                        if (terminatorValue != NULL)
                        {
                                /*
                                *       If we do not return then we add branch normally.
                                */
                                if (LLVMIsAReturnInst(terminatorValue) == NULL)
                                {
                                        LLVMBuildBr(S->theBuilder,afterBlock);
                                }
                        }
                        else
                        {
                                LLVMBuildBr(S->theBuilder,afterBlock);
                        }
                        LLVMPositionBuilderAtEnd(S->theBuilder,afterBlock);
                }
        }

}

void
noisyIterateStatementCodeGen(State * N,CodeGenState * S, IrNode * iterateNode)
{
        LLVMBasicBlockRef loopBlock = LLVMAppendBasicBlock(S->currentFunction,"loop");
        LLVMBuildBr(S->theBuilder,loopBlock);
        LLVMPositionBuilderAtEnd(S->theBuilder,loopBlock);
        for (IrNode * iter = R(iterateNode); iter != NULL; iter = RR(iter))
        {
                LLVMValueRef condVal = noisyExpressionCodeGen(N,S,L(iter));
                LLVMBasicBlockRef thenBlock = LLVMAppendBasicBlock(S->currentFunction,"then");
                LLVMBasicBlockRef afterBlock = LLVMAppendBasicBlock(S->currentFunction,"after");

                LLVMBuildCondBr(S->theBuilder,condVal,thenBlock,afterBlock);

                LLVMPositionBuilderAtEnd(S->theBuilder,thenBlock);
                noisyStatementListCodeGen(N,S,RLL(iter));
                LLVMBuildBr(S->theBuilder,loopBlock);

                LLVMPositionBuilderAtEnd(S->theBuilder,afterBlock);
        }
}

void
noisySequenceStatementCodeGen(State * N,CodeGenState * S, IrNode * sequenceNode)
{
        noisyAssignmentStatementCodeGen(N,S,LL(sequenceNode));
        LLVMBasicBlockRef condBlock = LLVMAppendBasicBlock(S->currentFunction,"cond");
        LLVMBasicBlockRef loopBlock = LLVMAppendBasicBlock(S->currentFunction,"loop");
        LLVMBasicBlockRef afterBlock = LLVMAppendBasicBlock(S->currentFunction,"after");
        LLVMBuildBr(S->theBuilder,condBlock);

        LLVMPositionBuilderAtEnd(S->theBuilder,condBlock);
        LLVMValueRef condVal = noisyExpressionCodeGen(N,S,LRL(sequenceNode));
        LLVMBuildCondBr(S->theBuilder,condVal,loopBlock,afterBlock);

        LLVMPositionBuilderAtEnd(S->theBuilder,loopBlock);
        noisyStatementListCodeGen(N,S,RL(sequenceNode));
        noisyAssignmentStatementCodeGen(N,S,LRR(sequenceNode)->irLeftChild);

        LLVMValueRef terminatorValue = LLVMGetBasicBlockTerminator(LLVMGetInsertBlock(S->theBuilder));;
        if (terminatorValue != NULL)
        {
                /*
                *       If we do not return then we add branch normally.
                */
                if (LLVMIsAReturnInst(terminatorValue) == NULL)
                {
                        LLVMBuildBr(S->theBuilder,condBlock);
                }
        }
        else
        {
                LLVMBuildBr(S->theBuilder,condBlock);
        }

        LLVMPositionBuilderAtEnd(S->theBuilder,afterBlock);
}

void
noisyOperatorToleranceDeclCodeGen(State * N,CodeGenState * S, IrNode * toleranceDeclNode)
{
        /*
        *       We do not implement that.
        */
}

void
noisyReturnStatementCodeGen(State * N,CodeGenState * S, IrNode * returnNode)
{
        noisyDestroyCoroutineFrames(S);
        LLVMValueRef expressionValue = noisyExpressionCodeGen(N,S,LRL(returnNode));
        if (LRL(returnNode)->noisyType.basicType == noisyArrayType)
        {
                LLVMValueRef oneVal[] = {LLVMConstInt(LLVMInt64TypeInContext(S->theContext),1,false)};
                LLVMTypeRef arrayType = getLLVMTypeFromNoisyType(S,LRL(returnNode)->noisyType,false,0);
                LLVMValueRef sizeOfExprVal= LLVMBuildGEP2(S->theBuilder,arrayType,LLVMConstPointerNull(LLVMPointerType(arrayType,0)),oneVal,1,"");
                sizeOfExprVal = LLVMBuildPtrToInt(S->theBuilder,sizeOfExprVal,LLVMInt64TypeInContext(S->theContext),"k_sizeOfT");
                LLVMValueRef dstValue = LLVMBuildLoad2(S->theBuilder,LLVMGetElementType(LLVMTypeOf(LL(returnNode)->symbol->llvmPointer)),LL(returnNode)->symbol->llvmPointer,"");
                LLVMBuildMemCpy(S->theBuilder,dstValue,0,expressionValue,0,sizeOfExprVal);
                LLVMBuildRetVoid(S->theBuilder);
        }
        else
        {
                LLVMBuildRet(S->theBuilder,expressionValue);
        }
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
                if (L(iter) != NULL && LL(iter) != NULL)
                {
                        noisyStatementCodeGen(N,S,L(iter));
                }
        }
}

LLVMValueRef
noisyFunctionDefnCodeGen(State * N, CodeGenState * S,IrNode * noisyFunctionDefnNode)
{
        LLVMValueRef func;
        static bool isCoroutineDeclared = false;
        if (!strcmp(L(noisyFunctionDefnNode)->tokenString,"init"))
        {
                func = LLVMGetNamedFunction(S->theModule,"main");
        }
        else
        {
                func = LLVMGetNamedFunction(S->theModule,L(noisyFunctionDefnNode)->tokenString);
        }
        /*
        *       Declare local function
        */
        if (func == NULL)
        {
                func = noisyDeclareFunction(N,S,noisyFunctionDefnNode->irLeftChild,RL(noisyFunctionDefnNode),RRL(noisyFunctionDefnNode));
                if (func == NULL)
                {
                        /*
                        *       If func depends on Module parameters we skip its definition until its loaded.
                        */
                        return func;
                }
        }
        S->currentFunction = func;
        L(noisyFunctionDefnNode)->symbol->llvmPointer = func;
        LLVMBasicBlockRef funcEntry = LLVMAppendBasicBlock(func, "entry");
        LLVMPositionBuilderAtEnd(S->theBuilder, funcEntry);
        IrNode * outputSignature = RRL(noisyFunctionDefnNode);

        if (L(noisyFunctionDefnNode)->symbol->isChannel)
        {
                /*
                *       Coroutine Code Gen. For coroutines, we need to allocate memory for the coroutine frame and possible promise value,
                *       before generating user code.
                */
                LLVMValueRef returnPromiseValue;
                if (L(outputSignature)->type != kNoisyIrNodeType_Tnil)
                {
                        LLVMTypeRef returnType = getLLVMTypeFromNoisyType(S,L(outputSignature)->symbol->noisyType,false,0);
                        returnPromiseValue = LLVMBuildAlloca(S->theBuilder,returnType,"k_retPromise");
                        L(outputSignature)->symbol->llvmPointer = returnPromiseValue;
                        returnPromiseValue = LLVMBuildBitCast(S->theBuilder,returnPromiseValue,LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0),"k_retPromisev");
                }
                else
                {
                        returnPromiseValue = LLVMConstNull(LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0));
                }

                if (!isCoroutineDeclared)
                {
                        noisyDeclareCoroutineIntrinsics(S);
                        isCoroutineDeclared = true;
                }
                /*
                *       Create coro token.
                */
                LLVMValueRef args[4];
                int argNum = 4;
                args[0] = LLVMConstInt(LLVMInt32TypeInContext(S->theContext),0,false);
                args[1] = returnPromiseValue;
                args[2] = LLVMConstNull(LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0));
                args[3] = LLVMConstNull(LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0));
                LLVMValueRef callFunc = LLVMGetNamedFunction(S->theModule,"llvm.coro.id");
                LLVMValueRef coroToken = LLVMBuildCall2(S->theBuilder,LLVMGetElementType(LLVMTypeOf(callFunc)),callFunc,args,argNum,"k_coroId");

                /*
                *       Check if dynamic memory allocation is needed for coro token.
                */
                args[0] = coroToken;
                argNum = 1;
                callFunc = LLVMGetNamedFunction(S->theModule,"llvm.coro.alloc");
                LLVMValueRef needDynAlloc = LLVMBuildCall2(S->theBuilder,LLVMGetElementType(LLVMTypeOf(callFunc)),callFunc,args,argNum,"k_needDynAlloc");

                LLVMBasicBlockRef dynAlloc = LLVMAppendBasicBlockInContext(S->theContext,S->currentFunction,"dynAlloc");
                LLVMBasicBlockRef coroBegin = LLVMAppendBasicBlockInContext(S->theContext,S->currentFunction,"coroBegin");

                LLVMBuildCondBr(S->theBuilder,needDynAlloc,dynAlloc,coroBegin);
                LLVMPositionBuilderAtEnd(S->theBuilder,dynAlloc);

                /*
                *       If dynamic allocation is need then calculate the size needed, allocate the memory and branc to coro begin.
                */
                argNum = 0;
                callFunc = LLVMGetNamedFunction(S->theModule,"llvm.coro.size.i32");
                LLVMValueRef size = LLVMBuildCall2(S->theBuilder,LLVMGetElementType(LLVMTypeOf(callFunc)),callFunc,args,argNum,"k_size");

                LLVMValueRef mallocedMem = LLVMBuildArrayMalloc(S->theBuilder,LLVMInt1TypeInContext(S->theContext),size,"k_alloc");
                mallocedMem = LLVMBuildBitCast(S->theBuilder,mallocedMem,LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0),"");

                LLVMBuildBr(S->theBuilder,coroBegin);
                LLVMPositionBuilderAtEnd(S->theBuilder,coroBegin);

                /*
                *       At coro begin we have a phi node indicating if we malloc'ed memory or if it is stack allocated.
                *       Then we call coro.begin that return the coro frame,essential for the coroutine handling.
                *       After calling coro.begin we immediately call coro.suspend. The semantics should be that when the
                *       users calls a loadExpr we allocate memory for the coroFrame. After that every time that the function
                *       is invoked via a channel call we actually invoke the resume function of the coroutine.
                */
                LLVMValueRef allocPhi = LLVMBuildPhi(S->theBuilder,LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0),"k_allocPhi");
                LLVMValueRef phiValues[2];
                phiValues[0] = LLVMConstNull(LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0));
                phiValues[1] = mallocedMem;
                LLVMBasicBlockRef blockList[2];
                blockList[0] = funcEntry;
                blockList[1] = dynAlloc;
                LLVMAddIncoming(allocPhi,phiValues,blockList,2);

                argNum = 2;
                args[0] = coroToken;
                args[1] = allocPhi;
                callFunc = LLVMGetNamedFunction(S->theModule,"llvm.coro.begin");
                LLVMValueRef hdl = LLVMBuildCall2(S->theBuilder,LLVMGetElementType(LLVMTypeOf(callFunc)),callFunc,args,argNum,"hdl");

                argNum = 2;
                args[0] = LLVMConstNull(LLVMTokenTypeInContext(S->theContext));
                args[1] = LLVMConstInt(LLVMInt1TypeInContext(S->theContext),0,false);
                callFunc = LLVMGetNamedFunction(S->theModule,"llvm.coro.suspend");
                LLVMValueRef suspend = LLVMBuildCall2(S->theBuilder,LLVMGetElementType(LLVMTypeOf(callFunc)),callFunc,args,argNum,"sus");

                LLVMBasicBlockRef suspendBB= LLVMAppendBasicBlockInContext(S->theContext,S->currentFunction,"suspend");
                LLVMBasicBlockRef cleanupBB = LLVMInsertBasicBlockInContext(S->theContext,suspendBB,"cleanup");
                LLVMBasicBlockRef userCodeBB = LLVMInsertBasicBlockInContext(S->theContext,cleanupBB,"usercode");

                LLVMValueRef switchVal = LLVMBuildSwitch(S->theBuilder,suspend,suspendBB,2);
                LLVMAddCase(switchVal,LLVMConstInt(LLVMInt8TypeInContext(S->theContext),0,false),userCodeBB);
                LLVMAddCase(switchVal,LLVMConstInt(LLVMInt8TypeInContext(S->theContext),1,false),cleanupBB);

                LLVMPositionBuilderAtEnd(S->theBuilder,cleanupBB);
                /*
                *       At cleanup we check if we need to deallocate the memory or if it is stack allocated.
                */
                argNum = 2;
                args[0] = coroToken;
                args[1] = hdl;
                callFunc = LLVMGetNamedFunction(S->theModule,"llvm.coro.free");
                LLVMValueRef mem = LLVMBuildCall2(S->theBuilder,LLVMGetElementType(LLVMTypeOf(callFunc)),callFunc,args,argNum,"k_mem");

                LLVMBasicBlockRef dynFreeBB = LLVMInsertBasicBlockInContext(S->theContext,suspendBB,"dynFree");
                LLVMValueRef needDynFree = LLVMBuildICmp(S->theBuilder,LLVMIntNE,mem,LLVMConstNull(LLVMPointerType(LLVMInt8TypeInContext(S->theContext),0)),"k_needDynFree");
                LLVMBuildCondBr(S->theBuilder,needDynFree,dynFreeBB,suspendBB);

                LLVMPositionBuilderAtEnd(S->theBuilder,dynFreeBB);

                /*
                *       On dynamic free we free the malloc'ed memory and we branc to suspend.
                */
                LLVMBuildFree(S->theBuilder,mem);
                LLVMBuildBr(S->theBuilder,suspendBB);

                LLVMPositionBuilderAtEnd(S->theBuilder,suspendBB);

                /*
                *       On suspend we destory the coroutine stack frame and we return.
                */
                argNum = 2;
                args[0] = hdl;
                args[1] = LLVMConstInt(LLVMInt1TypeInContext(S->theContext),0,false);
                callFunc = LLVMGetNamedFunction(S->theModule,"llvm.coro.end");
                LLVMBuildCall2(S->theBuilder,LLVMGetElementType(LLVMTypeOf(callFunc)),callFunc,args,argNum,"sus");

                LLVMBuildRet(S->theBuilder,hdl);

                LLVMPositionBuilderAtEnd(S->theBuilder,userCodeBB);
                S->cleanupBB = cleanupBB;
                S->suspendBB = suspendBB;

                noisyStatementListCodeGen(N,S,RR(noisyFunctionDefnNode)->irRightChild->irLeftChild);

                S->cleanupBB = NULL;
                S->suspendBB = NULL;
                LLVMValueRef lastInst = LLVMGetLastInstruction(LLVMGetLastBasicBlock(S->currentFunction));
                if (lastInst == NULL || LLVMIsATerminatorInst(lastInst) == NULL)
                {
                        LLVMBuildBr(S->theBuilder,cleanupBB);
                }

        }
        else
        {
                /*
                *       Normal function code generation.
                */
                /*
                *       Array arguments need the following processing before any other code generation.
                */
                for  (IrNode * iter = RL(L(noisyFunctionDefnNode)->symbol->functionDefinition); iter != NULL; iter = RR(iter))
                {
                        Symbol * identifierSymbol;
                        if (L(iter)->type == kNoisyIrNodeType_Tnil)
                        {
                        break;
                        }
                        else
                        {
                                identifierSymbol = L(iter)->symbol;
                                // identifierSymbol->noisyType = getNoisyTypeFromTypeExpr(N,identifierSymbol->typeTree);
                        }

                        if (identifierSymbol->noisyType.basicType == noisyArrayType)
                        {
                                LLVMValueRef paramValue = LLVMGetParam(S->currentFunction,identifierSymbol->paramPosition);
                                LLVMValueRef arrayAddrValue = LLVMBuildAlloca(S->theBuilder,LLVMTypeOf(paramValue),"k_arrAddr");
                                LLVMBuildStore(S->theBuilder,paramValue,arrayAddrValue);
                                identifierSymbol->llvmPointer = arrayAddrValue;
                        }
                }

                if (L(outputSignature)->type != kNoisyIrNodeType_Tnil)
                {
                        Symbol * identifierSymbol = L(outputSignature)->symbol;
                        if (identifierSymbol->noisyType.basicType == noisyArrayType)
                        {
                                LLVMValueRef paramValue = LLVMGetLastParam(S->currentFunction);
                                LLVMValueRef arrayAddrValue = LLVMBuildAlloca(S->theBuilder,LLVMTypeOf(paramValue),"k_retAddr");
                                LLVMBuildStore(S->theBuilder,paramValue,arrayAddrValue);
                                identifierSymbol->llvmPointer = arrayAddrValue;
                        }
                }

                noisyStatementListCodeGen(N,S,RR(noisyFunctionDefnNode)->irRightChild->irLeftChild);
                /*
                *       If the functions returns void the final instruction should be the Ret Void.
                */
                if (L(outputSignature)->type == kNoisyIrNodeType_Tnil)
                {
                        noisyDestroyCoroutineFrames(S);
                        LLVMBuildRetVoid(S->theBuilder);
                }
        }

        LLVMVerifyFunction(func,LLVMPrintMessageAction);
        return func;
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
        S->thePassManager = LLVMCreatePassManager();

        noisyProgramCodeGen(N,S,N->noisyIrRoot);

        // LLVMAddCoroEarlyPass(S->thePassManager);
        // LLVMAddCoroSplitPass(S->thePassManager);
        // LLVMAddCoroElidePass(S->thePassManager);
        // LLVMAddCoroCleanupPass(S->thePassManager);

        LLVMRunPassManager(S->thePassManager,S->theModule);

        /*
        *       We need to dispose LLVM structures in order to avoid leaking memory. Free code gen state.
        */
        char * fileName;
        char * fileName2 = (char*)calloc(strlen(N->fileName)-1,sizeof(char));
        strncpy(fileName2,N->fileName,strlen(N->fileName)-2);
        // fileName2[strlen(N->fileName)-2]='\0';
        asprintf(&fileName,"%s.bc",fileName2);
        char * msg;
        LLVMVerifyModule(S->theModule,LLVMPrintMessageAction,&msg);
        LLVMDisposeMessage(msg);
        LLVMWriteBitcodeToFile(S->theModule,fileName);

        LLVMDisposePassManager(S->thePassManager);
        LLVMDisposeBuilder(S->theBuilder);
        LLVMDisposeModule(S->theModule);
        LLVMContextDispose(S->theContext);
        free(S);
        return ;

}