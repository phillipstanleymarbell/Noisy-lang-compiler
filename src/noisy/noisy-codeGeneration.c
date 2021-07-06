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


typedef struct {
        LLVMContextRef  theContext;
        LLVMBuilderRef  theBuilder;
        LLVMModuleRef   theModule;
        LLVMValueRef    currentFunction;
} CodeGenState;

LLVMTypeRef getLLVMTypeFromTypeExpr(State *, IrNode *);
void noisyStatementListCodeGen(State * N, CodeGenState * S,IrNode * statementListNode);
LLVMValueRef noisyExpressionCodeGen(State * N,CodeGenState * S, IrNode * noisyExpressionNode);

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
getLLVMTypeFromNoisyType(NoisyType noisyType,bool byRef,int limit)
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
        case noisyIntegerConstType:
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
        case noisyRealConstType:
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
                if (byRef)
                {
                        basicTypeHelper.basicType = noisyType.arrayType;
                        llvmType = getLLVMTypeFromNoisyType(basicTypeHelper,false,0);
                        for (int i = noisyType.dimensions-1; i >= 1+limit; i--)
                        {
                                llvmType = LLVMArrayType(llvmType,noisyType.sizeOfDimension[i]);
                        }
                        llvmType = LLVMPointerType(llvmType,0);
                }
                else
                {
                        basicTypeHelper.basicType = noisyType.arrayType;
                        llvmType = getLLVMTypeFromNoisyType(basicTypeHelper,false,0);
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
                        LLVMTypeRef llvmType = getLLVMTypeFromNoisyType(typ,true,0);
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
                returnType = getLLVMTypeFromNoisyType(typ,true,0);
        }
        else
        {
                returnType = LLVMVoidType();
        }

        LLVMValueRef func;

        if (returnType != NULL)
        {
                LLVMTypeRef funcType = LLVMFunctionType(returnType,paramArray,functionSymbol->parameterNum,0);
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
        *       TODO; Maybe deallocation happens elsewhere.
        */
        free(paramArray);
        return func;
}

LLVMValueRef
noisyGetArrayPositionPointer(State * N,CodeGenState * S, Symbol * arraySym,IrNode * noisyQualifiedIdentifierNode)
{
        LLVMTypeRef arrayType;
        LLVMValueRef arrayPtr = arraySym->llvmPointer;
        int lim = 0;
        for (IrNode * iter = R(noisyQualifiedIdentifierNode); iter != NULL; iter = R(iter))
        {
                LLVMValueRef idxValue = noisyExpressionCodeGen(N,S,LR(iter));
                LLVMValueRef idxValueList[] = {LLVMConstInt(LLVMInt32Type(),0,false) ,idxValue};
                idxValueList[1] = idxValue;
                arrayType = getLLVMTypeFromNoisyType(arraySym->noisyType,false,lim);
                arrayPtr = LLVMBuildGEP2(S->theBuilder,arrayType,arrayPtr,idxValueList,2,"k_arrIdx");
                lim++;
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
                        constValue = LLVMConstInt(LLVMInt64Type(),noisyConstantDeclNode->token->integerConst,false);
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

LLVMValueRef
noisyFactorCodeGen(State * N,CodeGenState * S,IrNode * noisyFactorNode)
{
        if (L(noisyFactorNode)->type == kNoisyIrNodeType_TintegerConst)
        {
                /*
                *       TODO; find a way to change integer and float constant type.
                */
                return LLVMConstInt(getLLVMTypeFromNoisyType(findConstantNoisyType(L(noisyFactorNode)),0,false),L(noisyFactorNode)->token->integerConst,true);
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_TrealConst)
        {
                return LLVMConstReal(getLLVMTypeFromNoisyType(findConstantNoisyType(L(noisyFactorNode)),0,false), L(noisyFactorNode)->token->realConst);
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_TstringConst)
        {
                return LLVMConstStringInContext (S->theContext,L(noisyFactorNode)->token->stringConst,strlen(L(noisyFactorNode)->token->stringConst), false);
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_TboolConst)
        {
                return LLVMConstInt(LLVMInt1Type(),L(noisyFactorNode)->token->integerConst,false);
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_PqualifiedIdentifier)
        {
                Symbol * identifierSymbol = LL(noisyFactorNode)->symbol;
                if (identifierSymbol->symbolType == kNoisySymbolTypeParameter)
                {
                        return LLVMGetParam(S->currentFunction,identifierSymbol->paramPosition);
                }
                else if (identifierSymbol->noisyType.basicType != noisyArrayType && identifierSymbol->noisyType.basicType != noisyNamegenType)
                {
                        char * name;
                        asprintf(&name,"val_%s",identifierSymbol->identifier);
                        return LLVMBuildLoad2(S->theBuilder,getLLVMTypeFromNoisyType(identifierSymbol->noisyType,false,0),identifierSymbol->llvmPointer,name);
                }
                else if (identifierSymbol->noisyType.basicType == noisyArrayType)
                {
                        LLVMValueRef arrayPtr = noisyGetArrayPositionPointer(N,S,identifierSymbol,L(noisyFactorNode));
                        char * name;
                        asprintf(&name,"val_%s",identifierSymbol->identifier);
                        NoisyType retType = identifierSymbol->noisyType;
                        retType.basicType = identifierSymbol->noisyType.arrayType;
                        return LLVMBuildLoad2(S->theBuilder,getLLVMTypeFromNoisyType(retType,false,0),arrayPtr,name);
                }
                /*
                *       TODO; NamegenType.
                */
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_PnamegenInvokeShorthand)
        {
                Symbol * functionSymbol = LL(noisyFactorNode)->symbol;
                IrNode * inputSignature = L(functionSymbol->typeTree);
                IrNode * outputSignature = R(functionSymbol->typeTree);

                LLVMValueRef * args;
                LLVMTypeRef * argTyp;
                if (functionSymbol->parameterNum == 0)
                {
                        args = NULL;
                        argTyp = NULL;
                }
                else
                {
                        args = calloc(functionSymbol->parameterNum,sizeof(LLVMValueRef));
                        argTyp = calloc(functionSymbol->parameterNum,sizeof(LLVMTypeRef));

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
                        args[pos] = noisyExpressionCodeGen(N,S,expr);
                }
                int i = 0;
                if (functionSymbol->parameterNum != 0)
                {
                        for (IrNode * iter2 = inputSignature; iter2 != NULL; iter2 = RR(iter2))
                        {
                                argTyp[i] = getLLVMTypeFromNoisyType(L(iter2)->symbol->noisyType,false,0);
                                i++;
                        }
                }
                LLVMTypeRef retType = (outputSignature->irLeftChild->type == kNoisyIrNodeType_Tnil) ? 
                                                                                retType = LLVMVoidType()
                                                                                : getLLVMTypeFromNoisyType(outputSignature->irLeftChild->symbol->noisyType,false,0);

                LLVMTypeRef funcType = LLVMFunctionType(retType,argTyp,i,false);

                return LLVMBuildCall2(S->theBuilder,funcType,functionSymbol->llvmPointer,args,functionSymbol->parameterNum,"");
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
                        return LLVMConstInt(LLVMInt1Type(),1,false);
                        break;
                case noisyInt4:
                        return LLVMConstInt(LLVMIntType(4),7,true);
                        break;
                case noisyInt8:
                        return LLVMConstInt(LLVMInt8Type(),127,true);
                        break;
                case noisyInt16:
                        return LLVMConstInt(LLVMInt16Type(),32767,true);
                        break;
                case noisyInt32:
                        return LLVMConstInt(LLVMInt32Type(),2147483647,true);
                        break;
                case noisyInt64:
                        return LLVMConstInt(LLVMInt64Type(),9223372036854775807,true);
                        break;
                // case noisyInt128:
                //         return LLVMConstInt(LLVMInt128Type(),0x7fffffffffffffffffffffffffffffff,true);
                //         break;
                case noisyNat4:
                        return LLVMConstInt(LLVMIntType(4),15,false);
                        break;
                case noisyNat8:
                        return LLVMConstInt(LLVMInt8Type(),255,false);
                        break;
                case noisyNat16:
                        return LLVMConstInt(LLVMInt16Type(),65535,false);
                        break;
                case noisyNat32:
                        return LLVMConstInt(LLVMInt32Type(),0xffffffff,false);
                        break;
                case noisyNat64:
                        return LLVMConstInt(LLVMInt64Type(),0xffffffffffffffff,false);
                        break;
                // case noisyNat128:
                //         return LLVMConstInt(LLVMInt128Type(),0xffffffffffffffffffffffffffffffffLL,false);
                //         break;
                case noisyFloat16:
                        /*
                        *       TODO; May be wrong for half float.
                        */
                        return LLVMConstReal(LLVMHalfType(),65.504f);
                        break;
                case noisyFloat32:
                        return LLVMConstReal(LLVMFloatType(),FLT_MAX);
                        break;
                case noisyFloat64:
                        return LLVMConstReal(LLVMDoubleType(),DBL_MAX);
                        break;
                case noisyFloat128:
                        return LLVMConstReal(LLVMFP128Type(),LDBL_MAX);
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
                        return LLVMConstInt(LLVMInt1Type(),0,false);
                        break;
                case noisyInt4:
                        return LLVMConstInt(LLVMIntType(4),-8,true);
                        break;
                case noisyInt8:
                        return LLVMConstInt(LLVMInt8Type(),-128,true);
                        break;
                case noisyInt16:
                        return LLVMConstInt(LLVMInt16Type(),-32768,true);
                        break;
                case noisyInt32:
                        return LLVMConstInt(LLVMInt32Type(),-2147483648,true);
                        break;
                case noisyInt64:
                        return LLVMConstInt(LLVMInt64Type(),0x1000000000000000LL,true);
                        break;
                // case noisyInt128:
                //         return LLVMConstInt(LLVMInt128Type(),0x10000000000000000000000000000000,true);
                //         break;
                case noisyNat4:
                        return LLVMConstInt(LLVMIntType(4),0,false);
                        break;
                case noisyNat8:
                        return LLVMConstInt(LLVMInt8Type(),0,false);
                        break;
                case noisyNat16:
                        return LLVMConstInt(LLVMInt16Type(),0,false);
                        break;
                case noisyNat32:
                        return LLVMConstInt(LLVMInt32Type(),0,false);
                        break;
                case noisyNat64:
                        return LLVMConstInt(LLVMInt64Type(),0,false);
                        break;
                case noisyNat128:
                        return LLVMConstInt(LLVMInt128Type(),0,false);
                        break;
                case noisyFloat16:
                        /*
                        *       TODO; May be wrong for half float.
                        */
                        return LLVMConstReal(LLVMHalfType(),-65.504f);
                        break;
                case noisyFloat32:
                        return LLVMConstReal(LLVMFloatType(),FLT_MIN);
                        break;
                case noisyFloat64:
                        return LLVMConstReal(LLVMDoubleType(),DBL_MIN);
                        break;
                case noisyFloat128:
                        return LLVMConstReal(LLVMFP128Type(),LDBL_MIN);
                        break;
                default:
                        break;
                }
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_Tnil)
        {
                LLVMTypeRef nilType = getLLVMTypeFromNoisyType(findConstantNoisyType(noisyFactorNode),false,0);
                return LLVMConstNull(nilType);
        }
        /*
        *       TODO; We reach here when we call channel ops.
        */
        return LLVMConstNull(LLVMInt32Type());
}

LLVMValueRef
noisyUnaryOpCodeGen(State * N, CodeGenState * S,IrNode * noisyUnaryOpNode, LLVMValueRef termVal, IrNode * noisyFactorNode)
{
        LLVMTypeRef factorType = getLLVMTypeFromNoisyType(findConstantNoisyType(noisyFactorNode),0,false);
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
                return LLVMBuildXor(S->theBuilder,termVal,LLVMConstInt(LLVMInt1Type(),1,false),"k_notRes");
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
                LLVMTypeRef destType = getLLVMTypeFromNoisyType(termType,false,0);
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
                /*
                *       TODO; Change 3rd argument. It's dummy value.
                */
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
                                int size = noisyExpressionNode->noisyType.sizeOfDimension[noisyExpressionNode->noisyType.dimensions-1];
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
                                LLVMValueRef constArrVal = LLVMConstArray(getLLVMTypeFromNoisyType(LLL(noisyExpressionNode)->irLeftChild->irLeftChild->noisyType,false,0),elemValArr,size);
                                return constArrVal;
                        }
                        else if (LLL(noisyExpressionNode)->type == kNoisyIrNodeType_TintegerConst)
                        {
                                int size = noisyExpressionNode->noisyType.sizeOfDimension[noisyExpressionNode->noisyType.dimensions-1];
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

                                LLVMValueRef constArrVal = LLVMConstArray(getLLVMTypeFromNoisyType(LLR(noisyExpressionNode)->irLeftChild->irLeftChild->noisyType,false,0),elemValArr,size);
                                return constArrVal;
                        }
                }
        }
        else if (L(noisyExpressionNode)->type == kNoisyIrNodeType_PloadExpr)
        {

        }
        return LLVMConstInt(LLVMInt32Type(),42,true);
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
                                                lvalSym->llvmPointer = LLVMBuildAlloca(S->theBuilder,getLLVMTypeFromNoisyType(lvalSym->noisyType,false,0),name);
                                        }
                                        break;
                                case kNoisyIrNodeType_TplusAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(lvalSym->noisyType,false,0);
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
                                        lvalType = getLLVMTypeFromNoisyType(lvalSym->noisyType,false,0);
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
                                        lvalType = getLLVMTypeFromNoisyType(lvalSym->noisyType,false,0);
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
                                        lvalType = getLLVMTypeFromNoisyType(lvalSym->noisyType,false,0);
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
                                        lvalType = getLLVMTypeFromNoisyType(lvalSym->noisyType,false,0);
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
                                        lvalType = getLLVMTypeFromNoisyType(lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        exprVal = LLVMBuildOr(S->theBuilder,lvalVal,exprVal,"k_orRes");
                                        break;
                                case kNoisyIrNodeType_TandAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        exprVal = LLVMBuildAnd(S->theBuilder,lvalVal,exprVal,"k_andRes");
                                        break;
                                case kNoisyIrNodeType_TxorAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        exprVal = LLVMBuildXor(S->theBuilder,lvalVal,exprVal,"k_xorRes");
                                        break;
                                case kNoisyIrNodeType_TleftShiftAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        exprVal = LLVMBuildShl(S->theBuilder,lvalVal,exprVal,"k_lShiftRes");
                                        break;
                                case kNoisyIrNodeType_TrightShiftAssign:
                                        asprintf(&name,"val_%s",lvalSym->identifier);
                                        lvalType = getLLVMTypeFromNoisyType(lvalSym->noisyType,false,0);
                                        lvalVal = LLVMBuildLoad2(S->theBuilder,lvalType,lvalSym->llvmPointer,name);
                                        exprVal = LLVMBuildLShr(S->theBuilder,lvalVal,exprVal,"k_lShiftRes");
                                        break;
                                /*
                                *       TODO; Channel operator assign.
                                */
                                default:
                                        break;
                                }

                                if (RRL(noisyAssignmentStatementNode)->noisyType.basicType == noisyArrayType)
                                {
                                        /*
                                        *       When we have an array cast on the rval of an assignment.
                                        */
                                        LLVMValueRef oneVal[] = {LLVMConstInt(LLVMInt64Type(),1,false)};

                                        /*
                                        *       Solution on how to implement sizeof of a type
                                        *       https://stackoverflow.com/questions/14608250/how-can-i-find-the-size-of-a-type
                                        */

                                        LLVMValueRef sizeOfExprVal = LLVMBuildGEP2(S->theBuilder,LLVMTypeOf(exprVal),LLVMConstPointerNull(LLVMPointerType(LLVMTypeOf(exprVal),0)),oneVal,1,"");

                                        sizeOfExprVal = LLVMBuildPtrToInt(S->theBuilder,sizeOfExprVal,LLVMInt64Type(),"k_sizeOfT");

                                        LLVMValueRef globalVar = LLVMAddGlobal(S->theModule,LLVMTypeOf(exprVal),"k_arrConst");
                                        LLVMSetInitializer(globalVar,exprVal);
                                        LLVMSetGlobalConstant(globalVar,true);

                                        LLVMValueRef dstPtrVal = LLVMBuildBitCast(S->theBuilder,lvalSym->llvmPointer,LLVMPointerType(LLVMInt8Type(),0),"");

                                        LLVMBuildMemCpy(S->theBuilder,dstPtrVal,0,globalVar,0,sizeOfExprVal); 
                                }
                                else if (lvalSym->noisyType.basicType == noisyArrayType)
                                {
                                        /*
                                        *       When we have an array on lval of an assignment.
                                        */
                                        LLVMBuildStore(S->theBuilder,exprVal,noisyGetArrayPositionPointer(N,S,lvalSym,LL(iter)));
                                }
                                else if (lvalSym->noisyType.basicType != noisyNamegenType)
                                {
                                        LLVMBuildStore(S->theBuilder,exprVal,lvalSym->llvmPointer);
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
                                        identifierSymbol->llvmPointer = LLVMBuildAlloca(S->theBuilder,getLLVMTypeFromNoisyType(identifierSymbol->noisyType,false,0),name);
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
                        LLVMBuildBr(S->theBuilder,afterBlock);
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
                        LLVMBuildBr(S->theBuilder,afterBlock);

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
        LLVMBuildBr(S->theBuilder,condBlock);

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
        LLVMBuildRet(S->theBuilder, noisyExpressionCodeGen(N,S,LRL(returnNode)));
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

void 
noisyFunctionDefnCodeGen(State * N, CodeGenState * S,IrNode * noisyFunctionDefnNode)
{
        LLVMValueRef func;
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
                func = noisyDeclareFunction(N,S,noisyFunctionDefnNode->irLeftChild->tokenString,RL(noisyFunctionDefnNode),RRL(noisyFunctionDefnNode));
                if (func == NULL)
                {
                        /*
                        *       If func depends on Module parameters we skip its definition until its loaded.
                        */
                        return ;
                }
        }
        S->currentFunction = func;
        L(noisyFunctionDefnNode)->symbol->llvmPointer = func;
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
        char * fileName;
        char * fileName2 = (char*)calloc(strlen(N->fileName)-1,sizeof(char));
        strncpy(fileName2,N->fileName,strlen(N->fileName)-2);
        // fileName2[strlen(N->fileName)-2]='\0';
        asprintf(&fileName,"%s.bc",fileName2);
        LLVMWriteBitcodeToFile(S->theModule,fileName);

        LLVMDisposeBuilder(S->theBuilder);
        LLVMDisposeModule(S->theModule);
        LLVMContextDispose(S->theContext);
        free(S);
        return ;

}