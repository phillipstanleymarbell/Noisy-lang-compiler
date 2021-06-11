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
#include "common-irHelpers.h"
#include "noisy-typeCheck.h"




/*
*       If we have templated function declaration, returns false.
*       We should invoke code generation with the load operator.
*/
bool
isTypeExprComplete(State * N,IrNode * typeExpr)
{

        if (L(typeExpr)->type == kNoisyIrNodeType_PbasicType)
        {
                return true;
        }
        else if (L(typeExpr)->type == kNoisyIrNodeType_PtypeName)
        {
                Symbol * typeSymbol = commonSymbolTableSymbolForIdentifier(N,N->noisyIrTopScope,LL(typeExpr)->tokenString);
                if (typeSymbol->symbolType == kNoisySymbolTypeModuleParameter)
                {
                        return false;
                }
                else
                {
                        return isTypeExprComplete(N,typeSymbol->typeTree->irRightChild);
                }
        }
        else if (L(typeExpr)->type == kNoisyIrNodeType_PanonAggregateType)
        {
                if (LL(typeExpr)->type == kNoisyIrNodeType_ParrayType)
                {
                        IrNode * iter;
                        for (iter = LL(typeExpr); iter != NULL; iter = R(iter))
                        {
                                if (iter->irLeftChild->type == kNoisyIrNodeType_PtypeExpr)
                                {
                                        break;
                                }
                        }
                        return isTypeExprComplete(N,L(iter));
                }
                return false;
        }
        return false;
}

NoisyType
getNoisyTypeFromBasicType(IrNode * basicType)
{
        NoisyType noisyType;
        if (L(basicType)->type == kNoisyIrNodeType_Tbool)
        {
                noisyType.basicType = noisyBool;
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
                        noisyType.basicType = noisyInt4;
                        break;
                case kNoisyIrNodeType_Tnat4:
                        noisyType.basicType = noisyNat4;
                        break;
                case kNoisyIrNodeType_Tint8:
                        noisyType.basicType = noisyInt8;
                        break;
                case kNoisyIrNodeType_Tnat8:
                        noisyType.basicType = noisyNat8;
                        break;
                case kNoisyIrNodeType_Tint16:
                        noisyType.basicType = noisyInt16;
                        break;
                case kNoisyIrNodeType_Tnat16:
                        noisyType.basicType = noisyNat16;
                        break;
                case kNoisyIrNodeType_Tint32:
                        noisyType.basicType = noisyInt32;
                        break;
                case kNoisyIrNodeType_Tnat32:
                        noisyType.basicType = noisyNat32;
                        break;
                case kNoisyIrNodeType_Tint64:
                        noisyType.basicType = noisyInt64;
                        break;
                case kNoisyIrNodeType_Tnat64:
                        noisyType.basicType = noisyNat64;
                        break;
                case kNoisyIrNodeType_Tint128:
                        noisyType.basicType = noisyInt128;
                        break;
                case kNoisyIrNodeType_Tnat128:
                        noisyType.basicType = noisyNat128;
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
                        noisyType.basicType = noisyFloat16;
                        break;
                case kNoisyIrNodeType_Tfloat32:
                        noisyType.basicType = noisyFloat32;
                        break;
                case kNoisyIrNodeType_Tfloat64:
                        noisyType.basicType = noisyFloat64;
                        break;
                case kNoisyIrNodeType_Tfloat128:
                        noisyType.basicType = noisyFloat128;
                        break;
                default:
                        break;
                }
        }
        else if(L(basicType)->type == kNoisyIrNodeType_Tstring)
        {
                noisyType.basicType = noisyString;
        }
        return noisyType;
}

/*
*       Takes an arrayType IrNode and returns the corresponding NoisyType
*       Assumes that arrayTypeNode->type == kNoisyIrNodeType_ParrayType.
*/
NoisyType
getNoisyTypeFromArrayNode(State * N,IrNode * arrayTypeNode)
{
        NoisyType noisyType;

        noisyType.basicType = noisyArrayType;
        noisyType.dimensions = 0;

        for (IrNode * iter = arrayTypeNode; iter != NULL; iter = R(iter))
        {
                if (L(iter)->type != kNoisyIrNodeType_PtypeExpr)
                {
                        noisyType.dimensions++;
                }
                else
                {
                        noisyType.arrayType = getNoisyTypeFromTypeExpr(N,L(iter)).basicType;
                }
        }

        noisyType.sizeOfDimension = calloc(noisyType.dimensions,sizeof(int));

        int i = 0;
        for (IrNode * iter = arrayTypeNode; iter != NULL; iter = R(iter))
        {
                if (L(iter)->type != kNoisyIrNodeType_PtypeExpr)
                {
                        noisyType.sizeOfDimension[i] = L(iter)->token->integerConst;
                }
                i++;
        }

        return noisyType;
}

/*
*       Takes the state N (needed for symbolTable search) and a typeNameNode
*       and returns the corresponding NoisyType.
*/
NoisyType
getNoisyTypeFromTypeSymbol(State * N,IrNode * typeNameNode)
{
        NoisyType noisyType;
        Symbol * typeSymbol = commonSymbolTableSymbolForIdentifier(N,NULL,L(typeNameNode)->tokenString);

        if (typeSymbol == NULL)
        {
                typeSymbol = commonSymbolTableSymbolForIdentifier(N,N->noisyIrTopScope,L(typeNameNode)->tokenString);
                noisyType.basicType = noisyTypeError;
                return noisyType;
        }

        IrNode * typeTree = typeSymbol->typeTree;

        if (RL(typeTree)->type == kNoisyIrNodeType_PbasicType)
        {
                return getNoisyTypeFromBasicType(RL(typeTree));
        }
        else if (RL(typeTree)->type == kNoisyIrNodeType_PanonAggregateType)
        {
                return getNoisyTypeFromArrayNode(N,RL(typeTree));
        }
        else if (RL(typeTree)->type == kNoisyIrNodeType_PtypeName)
        {
                return getNoisyTypeFromTypeSymbol(N,RL(typeTree));
        }
        else
        {
                noisyType.basicType = noisyTypeError;
                return noisyType;
        }
}

/*
*       Takes the state N and a TypeExpr node and returns the corresponding
*       NoisyType. If it fails the returned basic type is noisyTypeError.
*/
NoisyType
getNoisyTypeFromTypeExpr(State * N, IrNode * typeExpr)
{
        NoisyType noisyType;
        if (L(typeExpr)->type == kNoisyIrNodeType_PbasicType)
        {
                return getNoisyTypeFromBasicType(L(typeExpr));
        }
        else if (L(typeExpr)->type == kNoisyIrNodeType_PanonAggregateType)
        {
                IrNode * arrayType = LL(typeExpr);
                if (arrayType->type == kNoisyIrNodeType_ParrayType)
                {
                        return getNoisyTypeFromArrayNode(N,arrayType);
                }
                /*
                *       Lists and other non aggregate types are not supported
                */
                else
                {
                        noisyType.basicType = noisyTypeError;
                        return noisyType;
                }
        }
        else if (L(typeExpr)->type == kNoisyIrNodeType_PtypeName)
        {
                return getNoisyTypeFromTypeSymbol(N,L(typeExpr));
        }

        noisyType.basicType = noisyTypeError;
        return noisyType;
}

bool
noisyTypeEquals(NoisyType typ1, NoisyType typ2)
{
        if (typ1.basicType == typ2.basicType)
        {
                if (typ1.basicType == noisyArrayType)
                {
                        if (typ1.arrayType == typ2.arrayType)
                        {
                                if (typ1.dimensions == typ2.dimensions)
                                {
                                        for (int i = 0; i < typ1.dimensions; i++)
                                        {
                                                if (typ1.sizeOfDimension[i] != typ2.sizeOfDimension[i])
                                                {
                                                        return false;
                                                }
                                        }
                                        return true;
                                }
                                return false;
                        }
                        return false;
                }
                return true;
        }
        return false;
}

/*
*       Checks if the function is typeComplete and also counts its parameters as preparation
*       for the code generation.
*/
void
noisyDeclareFunctionTypeCheck(State * N, const char * functionName,IrNode * inputSignature, IrNode * outputSignature)
{
        Symbol * functionSymbol = commonSymbolTableSymbolForIdentifier(N, N->moduleScopes, functionName);

        int parameterCount = 0;

        if (L(inputSignature)->type != kNoisyIrNodeType_Tnil)
        {
                for  (IrNode * iter = inputSignature; iter != NULL; iter = RR(iter))
                {
                        parameterCount++;

                        functionSymbol->isTypeComplete = isTypeExprComplete(N,RL(iter));
                        
                }
                /*
                *       We need to save parameterCount so we can allocate memory for the
                *       parameters of the generated function.
                */
        }
        /*
        *       If type == nil then parameterCount = 0
        */

        functionSymbol->parameterNum = parameterCount;

        functionSymbol->isTypeComplete = functionSymbol->isTypeComplete && isTypeExprComplete(N,RL(outputSignature));
}


void
noisyModuleTypeNameDeclTypeCheck(State * N, IrNode * noisyModuleTypeNameDeclNode)
{
        /*
        *       We do not need to typecheck constant definitions.
        */
        if (R(noisyModuleTypeNameDeclNode)->type == kNoisyIrNodeType_PtypeDecl 
                || R(noisyModuleTypeNameDeclNode)->type == kNoisyIrNodeType_PtypeAnnoteDecl )
        {
                /*
                *       Probably we need to type check if type annotations of units and signals match
                *       through Newton API.
                */
                return ;
        }
        else if (R(noisyModuleTypeNameDeclNode)->type == kNoisyIrNodeType_PfunctionDecl)
        {
                IrNode * inputSignature = RLL(noisyModuleTypeNameDeclNode);
                IrNode * outputSignature = RRL(noisyModuleTypeNameDeclNode);
                noisyDeclareFunctionTypeCheck(N,L(noisyModuleTypeNameDeclNode)->tokenString,inputSignature,outputSignature);
        }
}

void
noisyModuleDeclBodyTypeCheck(State * N, IrNode * noisyModuleDeclBodyNode)
{
        for (IrNode * currentNode = noisyModuleDeclBodyNode; currentNode != NULL; currentNode = currentNode->irRightChild)
        {
                noisyModuleTypeNameDeclTypeCheck(N, currentNode->irLeftChild);
        }
}


/*
*       We need this for every noisyType that we return so we can deallocate its sizeOfDimension array.
*       Otherwise we create garbage. Unfortunately we need accompany each final call of getNoisyType
*       with that function.
*/
void
deallocateNoisyType(NoisyType * typ)
{
        if (typ->basicType == noisyArrayType)
        {
                free(typ->sizeOfDimension);
        }
        return ;
}

bool
noisyMatchTypeExpr(State * N,IrNode * typeExpr1, IrNode * typeExpr2)
{
        NoisyType typ1, typ2;
        typ1 = getNoisyTypeFromTypeExpr(N,typeExpr1);
        typ2 = getNoisyTypeFromTypeExpr(N,typeExpr2);
        bool res = noisyTypeEquals(typ1,typ2);
        deallocateNoisyType(&typ1);
        deallocateNoisyType(&typ2);
        return res;
}

bool
noisySignatureIsMatching(State * N, IrNode * definitionSignature, IrNode * declarationSignature)
{
        if (L(definitionSignature)-> type == kNoisyIrNodeType_Tnil || L(declarationSignature)->type == kNoisyIrNodeType_Tnil)
        {
                if (L(definitionSignature)-> type == L(declarationSignature)-> type)
                {
                        return true;
                }
                else
                {
                        return false;
                }
        }
        else
        {
                IrNode * declIter = declarationSignature;
                for (IrNode * iter = definitionSignature ; iter != NULL; iter = RR(iter))
                {
                        /*
                        *       If one signature has less members than the other.
                        */
                        if (declIter == NULL)
                        {
                                return false;
                        }

                        if (strcmp(L(iter)->tokenString,L(declIter)->tokenString))
                        {
                                return false;
                        }                      

                        if (!noisyMatchTypeExpr(N,RL(iter),RL(declIter)))
                        {
                                return false;
                        }

                        declIter = RR(declIter);
                }

                /*
                *       If one signature has more members than the other.
                */
                if (declIter != NULL)
                {
                        return false;
                }
        }

        return true;
}


void
noisyAssignmentStatementTypeCheck(State * N, IrNode * noisyAssignmentStatementNode)
{
        /*
        *       If type is xseq it means that noisyAssignmentNode is not a declaration but an actual assignment.
        */
        if (R(noisyAssignmentStatementNode)->type == kNoisyIrNodeType_Xseq)
        {
                for (IrNode * iter = L(noisyAssignmentStatementNode); iter != NULL; iter = R(iter))
                {
                        if (LL(iter)->type == kNoisyIrNodeType_Tnil)
                        {
                                /*
                                *       We do not need to type check an assignment to nil.
                                */
                        }
                        else if (LL(iter)->type == kNoisyIrNodeType_PqualifiedIdentifier)
                        {
                                /*
                                *       TODO; Get type from expression and compare with typeExpr.
                                */
                                NoisyType typ1 = getNoisyTypeFromTypeExpr(N,LLL(iter)->symbol->typeTree);
                                if (typ1.basicType == noisyTypeError)
                                {
                                        deallocateNoisyType(&typ1);
                                        return;
                                }
                        }
                }
        }
}

void
noisyStatementTypeCheck(State * N, IrNode * noisyStatementNode)
{
        switch (L(noisyStatementNode)->type)
        {
        case kNoisyIrNodeType_PassignmentStatement:
                noisyAssignmentStatementTypeCheck(N,L(noisyStatementNode));
                break;
        // case kNoisyIrNodeType_PmatchStatement:
        //         noisyMatchStatementTypeCheck(N,S,L(noisyStatementNode));
        //         break;
        // case kNoisyIrNodeType_PiterateStatement:
        //         noisyIterateStatementTypeCheck(N,S,L(noisyStatementNode));
        //         break;
        // case kNoisyIrNodeType_PsequenceStatement:
        //         noisySequenceStatementTypeCheck(N,S,L(noisyStatementNode));
        //         break;
        // case kNoisyIrNodeType_PscopedStatementList:
        //         noisyStatementListTypeCheck(N,S,LL(noisyStatementNode));
        //         break;
        // case kNoisyIrNodeType_PoperatorToleranceDecl:
        //         noisyOperatorToleranceDeclTypeCheck(N,S,L(noisyStatementNode));
        //         break;
        // case kNoisyIrNodeType_PreturnStatement:
        //         noisyReturnStatementTypeCheck(N,S,L(noisyStatementNode));
        //         break;
        default:
                // flexprint(N->Fe, N->Fm, N->Fperr, "Code generation for that statement is not supported");
                // fatal(N,"Code generation Error\n");
                break;
        }
}

void
noisyStatementListTypeCheck(State * N, IrNode * statementListNode)
{
        for (IrNode * iter = statementListNode; iter != NULL; iter=R(iter))
        {
                if (L(iter) != NULL)
                {
                        noisyStatementTypeCheck(N,L(iter));
                }
        }
}


void
noisyFunctionDefnTypeCheck(State * N,IrNode * noisyFunctionDefnNode)
{
        Symbol * functionSymbol = commonSymbolTableSymbolForIdentifier(N, N->moduleScopes, noisyFunctionDefnNode->irLeftChild->tokenString);

        noisySignatureIsMatching(N, RL(noisyFunctionDefnNode),L(functionSymbol->typeTree));
        noisySignatureIsMatching(N, RRL(noisyFunctionDefnNode),R(functionSymbol->typeTree));        


        /*
        *       For local functions we need to typeCheck their declaration.
        *       We cannot skip this step because this function initializes parameterNum and isComplete variables
        *       for function symbols.
        */
        if (functionSymbol == NULL)
        {
                noisyDeclareFunctionTypeCheck(N,noisyFunctionDefnNode->irLeftChild->tokenString,RL(noisyFunctionDefnNode),RRL(noisyFunctionDefnNode));
        }

        noisyStatementListTypeCheck(N,RR(noisyFunctionDefnNode)->irRightChild->irLeftChild);
}

void
noisyModuleDeclTypeCheck(State * N, IrNode * noisyModuleDeclNode)
{
        /*
        *       We do not need to typecheck module parameters.
        */
        noisyModuleDeclBodyTypeCheck(N, RR(noisyModuleDeclNode));
}

void
noisyProgramTypeCheck(State * N,IrNode * noisyProgramNode)
{
        for (IrNode * currentNode = noisyProgramNode; currentNode != NULL; currentNode = currentNode->irRightChild)
        {
                if (currentNode->irLeftChild->type == kNoisyIrNodeType_PmoduleDecl)
                {
                        noisyModuleDeclTypeCheck(N, currentNode->irLeftChild);
                }
                else if (currentNode->irLeftChild->type == kNoisyIrNodeType_PfunctionDefn)
                {
                        noisyFunctionDefnTypeCheck(N,currentNode->irLeftChild);
                }
        }
}

void
noisyTypeCheck(State * N)
{
        noisyProgramTypeCheck(N,N->noisyIrRoot);
}