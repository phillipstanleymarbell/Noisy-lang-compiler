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


typedef struct {
         LLVMContextRef theContext;
         LLVMBuilderRef theBuilder;
         LLVMModuleRef  theModule;
         LLVMValueRef   currentFunction;
} CodeGenState;

LLVMTypeRef getLLVMTypeFromTypeExpr(State *, IrNode *);
void noisyStatementListCodeGen(State * N, CodeGenState * S,IrNode * statementListNode);
LLVMValueRef noisyExpressionCodeGen(State * N,CodeGenState * S, IrNode * noisyExpressionNode);

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

LLVMValueRef
noisyFactorCodeGen(State * N,CodeGenState * S,IrNode * noisyFactorNode, LLVMTypeRef nilType)
{
        if (L(noisyFactorNode)->type == kNoisyIrNodeType_TintegerConst)
        {
                return LLVMConstInt(LLVMInt32Type(),L(noisyFactorNode)->token->integerConst,true);
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_TrealConst)
        {
                return LLVMConstReal(LLVMFloatType(), L(noisyFactorNode)->token->realConst);
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
                        LLVMTypeRef arrayType;
                        LLVMValueRef arrayPtr = identifierSymbol->llvmPointer;
                        int lim = 0;
                        for (IrNode * iter = LR(noisyFactorNode); iter != NULL; iter = R(iter))
                        {
                                LLVMValueRef idxValue = noisyExpressionCodeGen(N,S,LR(iter));
                                LLVMValueRef idxValueList[] = {LLVMConstInt(LLVMInt32Type(),0,false) ,idxValue};
                                idxValueList[1] = idxValue;
                                arrayType = getLLVMTypeFromNoisyType(identifierSymbol->noisyType,false,lim);
                                arrayPtr = LLVMBuildGEP2(S->theBuilder,arrayType,arrayPtr,idxValueList,2,"k_arrIdx");   
                                lim++;
                        }
                        char * name;
                        asprintf(&name,"val_%s",identifierSymbol->identifier);
                        NoisyType retType;
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
                for (IrNode * iter2 = inputSignature; iter2 != NULL; iter2 = RR(iter2))
                {
                        argTyp[i] = getLLVMTypeFromNoisyType(L(iter2)->symbol->noisyType,false,0);
                        i++;
                }
                LLVMTypeRef retType = getLLVMTypeFromNoisyType(outputSignature->irLeftChild->symbol->noisyType,false,0);
                LLVMTypeRef funcType = LLVMFunctionType(retType,argTyp,i,false);

                return LLVMBuildCall2(S->theBuilder,funcType,functionSymbol->llvmPointer,args,functionSymbol->parameterNum,"k_callFn");
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
                        return LLVMConstInt(LLVMIntType(4),-7,true);
                        break;
                case noisyInt8:
                        return LLVMConstInt(LLVMInt8Type(),-127,true);
                        break;
                case noisyInt16:
                        return LLVMConstInt(LLVMInt16Type(),-32767,true);
                        break;
                case noisyInt32:
                        return LLVMConstInt(LLVMInt32Type(),-2147483647,true);
                        break;
                case noisyInt64:
                        return LLVMConstInt(LLVMInt64Type(),-9223372036854775807,true);
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
                return LLVMConstNull(nilType);
        }
        return LLVMConstInt(LLVMInt32Type(),42,true);
}

LLVMValueRef
noisyTermCodeGen(State * N,CodeGenState * S,IrNode * noisyTermNode)
{
        IrNode * factorNode = NULL;
        // IrNode * unaryOpNode = NULL;

        /*
        *       This flag is needed because the form of the tree is different based on whether a prefix exists
        *       on the term expression.
        */
        // bool prefixExists = false;

        if (L(noisyTermNode)->type == kNoisyIrNodeType_PbasicType)
        {
                // prefixExists = true;
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
                        // unaryOpNode = L(noisyTermNode);
                        // prefixExists = true;
                }
        }
        else if (RR(noisyTermNode)->type == kNoisyIrNodeType_Pfactor)
        {

                factorNode = RR(noisyTermNode);
                if (RL(noisyTermNode)->type == kNoisyIrNodeType_PunaryOp)
                {
                        // unaryOpNode = RL(noisyTermNode);
                        // prefixExists = true;
                }
        }

        /*
        *       TODO; Change 3rd argument.
        */
        return noisyFactorCodeGen(N,S,factorNode,LLVMInt32Type());
}


LLVMValueRef
noisyExpressionCodeGen(State * N,CodeGenState * S, IrNode * noisyExpressionNode)
{
        if (L(noisyExpressionNode)->type == kNoisyIrNodeType_Pterm)
        {
                return noisyTermCodeGen(N,S,L(noisyExpressionNode));
        }
        else if (L(noisyExpressionNode)->type == kNoisyIrNodeType_PanonAggrCastExpr)
        {

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
                if (RLL(noisyAssignmentStatementNode)->type != kNoisyIrNodeType_TcolonAssign)
                {
                        /*
                        *      Eval expression. Store the result.
                        */
                        noisyExpressionCodeGen(N,S,RRL(noisyAssignmentStatementNode));
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

        flexprint(N->Fe,N->Fm,N->Fpg,LLVMPrintModuleToString(S->theModule));
        LLVMDisposeModule(S->theModule);
        LLVMDisposeBuilder(S->theBuilder);
        LLVMContextDispose(S->theContext);
        free(S);
        return ;

}