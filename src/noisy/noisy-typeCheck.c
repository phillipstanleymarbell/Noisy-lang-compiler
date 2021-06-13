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

NoisyType getNoisyTypeFromExpression(State * N, IrNode * noisyExpressionNode, Scope * currentScope);


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
                /*
                *       TODO; Scope here needs fixing. Typenames need to be resolved.
                */
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

void
noisyInitNoisyType(NoisyType * typ)
{
        typ->basicType = noisyInitType;
        typ->dimensions = 0;
        typ->sizeOfDimension = NULL;
        typ->arrayType = noisyInitType;
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

bool
noisyIsOfType(NoisyType typ1,NoisyBasicType typeSuperSet)
{
        if (typeSuperSet == noisyIntegerConstType)
        {
                switch (typ1.basicType)
                {
                case noisyInt4:
                case noisyInt8:
                case noisyInt16:
                case noisyInt32:
                case noisyInt64:
                case noisyInt128:
                case noisyIntegerConstType:
                case noisyNat4:
                case noisyNat8:
                case noisyNat16:
                case noisyNat32:
                case noisyNat64:
                case noisyNat128:
                        return true;
                        break;
                default:
                        return false;
                        break;
                }
        }
        else if (typeSuperSet == noisyRealConstType)
        {
                switch (typ1.basicType)
                {
                case noisyFloat16:
                case noisyFloat32:
                case noisyFloat64:
                case noisyFloat128:
                case noisyRealConstType:
                        return true;
                        break;
                default:
                        return false;
                        break;
                }
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

NoisyType
getNoisyTypeFromFactor(State * N, IrNode * noisyFactorNode, Scope * currentScope)
{
        NoisyType factorType;

        if (L(noisyFactorNode)->type == kNoisyIrNodeType_TintegerConst)
        {
                factorType.basicType = noisyIntegerConstType;
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_TrealConst)
        {
                factorType.basicType = noisyRealConstType;
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_TstringConst)
        {
                factorType.basicType = noisyString;
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_TboolConst)
        {
                factorType.basicType = noisyBool;
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_PqualifiedIdentifier)
        {
                Symbol * identifierSymbol = commonSymbolTableSymbolForIdentifier(N, currentScope, LL(noisyFactorNode)->tokenString);

                if (identifierSymbol == NULL)
                {
                        factorType.basicType = noisyTypeError;
                }

                factorType = getNoisyTypeFromTypeExpr(N,identifierSymbol->typeTree);
                if (factorType.basicType == noisyArrayType)
                {
                        int dims = 0;
                        for (IrNode * iter = LR(noisyFactorNode); iter != NULL; iter = R(iter))
                        {
                                if (! noisyIsOfType(getNoisyTypeFromExpression(N,LR(iter),currentScope), noisyIntegerConstType))
                                {
                                        factorType.basicType = noisyTypeError;
                                }
                                dims++;
                        }

                        if (dims != factorType.dimensions)
                        {
                                factorType.basicType = noisyTypeError;
                        }
                        /*
                        *       If there are no type errors on array indexing we the arrayType of the array.
                        *       e.g. when we index an array of int32 the factor we return has type int32.
                        */
                        if (factorType.basicType != noisyTypeError)
                        {
                                factorType.basicType = factorType.arrayType;
                        }
                }
                else
                {
                        if (LR(noisyFactorNode) != NULL)
                        {
                                factorType.basicType = noisyTypeError;
                        }
                }
        }
        return factorType;
}


NoisyType
noisyUnaryOpTypeCheck(IrNode * noisyUnaryOpNode,NoisyType factorType)
{
        NoisyType returnType;

        if (L(noisyUnaryOpNode)->type == kNoisyIrNodeType_Tplus ||
            L(noisyUnaryOpNode)->type == kNoisyIrNodeType_Tminus)
        {
                switch (factorType.basicType)
                {
                case noisyBool:
                case noisyString:
                case noisyInitType:
                case noisyArrayType:
                case noisyTypeError:
                        returnType.basicType = noisyTypeError;
                        break;
                default:
                        returnType = factorType;
                        break;
                }
        }
        /*
        *       "~" and "<-" operator we do not type check so far.
        */
        else
        {
                returnType = factorType;
        }
        return returnType;
}

/*
*       TODO; Not completed.
*/
NoisyType
getNoisyTypeFromTerm(State * N, IrNode * noisyTermNode, Scope * currentScope)
{
        NoisyType basicType,factorType, unaryType;
        IrNode * factorNode = NULL;
        IrNode * unaryOpNode = NULL;
        noisyInitNoisyType(&basicType);
        noisyInitNoisyType(&unaryType);


        if (L(noisyTermNode)->type == kNoisyIrNodeType_PbasicType)
        {
                basicType = getNoisyTypeFromBasicType(noisyTermNode->irLeftChild);
        }

        if (L(noisyTermNode)->type == kNoisyIrNodeType_Pfactor)
        {
                factorNode = L(noisyTermNode);
        }
        else if (RL(noisyTermNode)->type == kNoisyIrNodeType_Pfactor)
        {
                factorNode = RL(noisyTermNode);
                if (L(noisyTermNode)->type == kNoisyIrNodeType_PunaryOp)
                {
                        unaryOpNode = L(noisyTermNode);
                }
        }
        else if (RR(noisyTermNode)->type == kNoisyIrNodeType_Pfactor)
        {

                factorNode = RR(noisyTermNode);
                if (RL(noisyTermNode)->type == kNoisyIrNodeType_PunaryOp)
                {
                        unaryOpNode = RL(noisyTermNode);
                }
        }

        factorType = getNoisyTypeFromFactor(N,factorNode,currentScope);
        if (unaryOpNode != NULL)
        {
                unaryType = noisyUnaryOpTypeCheck(unaryOpNode,factorType);
        }

        if (unaryType.basicType != noisyInitType)
        {
                factorType = unaryType;
        }

        if (basicType.basicType != noisyInitType)
        {
                if (!noisyTypeEquals(basicType,factorType))
                {
                        factorType.basicType = noisyTypeError;
                }
        }

        /*
        *       We need to check highprecedencebinaryoperator and possible second factor.
        */

        return factorType;
}

/*
*       TODO; Not completed.
*/
NoisyType
getNoisyTypeFromExpression(State * N, IrNode * noisyExpressionNode, Scope * currentScope)
{
        NoisyType typ1;

        if (L(noisyExpressionNode)->type == kNoisyIrNodeType_Pterm)
        {
                typ1 = getNoisyTypeFromTerm(N,L(noisyExpressionNode), currentScope);
        }

        return typ1;
}

/*
*       Checks if the function is typeComplete and also counts its parameters as preparation
*       for the code generation.
*/
void
noisyDeclareFunctionTypeCheck(State * N, const char * functionName,IrNode * inputSignature, IrNode * outputSignature,Scope * currentScope)
{
        Symbol * functionSymbol = commonSymbolTableSymbolForIdentifier(N, currentScope, functionName);

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
noisyModuleTypeNameDeclTypeCheck(State * N, IrNode * noisyModuleTypeNameDeclNode, Scope * currentScope)
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
                noisyDeclareFunctionTypeCheck(N,L(noisyModuleTypeNameDeclNode)->tokenString,inputSignature,outputSignature,currentScope);
        }
}

void
noisyModuleDeclBodyTypeCheck(State * N, IrNode * noisyModuleDeclBodyNode,Scope * currentScope)
{
        for (IrNode * currentNode = noisyModuleDeclBodyNode; currentNode != NULL; currentNode = currentNode->irRightChild)
        {
                noisyModuleTypeNameDeclTypeCheck(N, currentNode->irLeftChild,currentScope);
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

/*
*       TODO; Not completed.
*/
void
noisyAssignmentStatementTypeCheck(State * N, IrNode * noisyAssignmentStatementNode, Scope * currentScope)
{
        /*
        *       If type is xseq it means that noisyAssignmentNode is not a declaration but an actual assignment.
        */
        if (R(noisyAssignmentStatementNode)->type == kNoisyIrNodeType_Xseq)
        {
                NoisyType lValuetype, rValueType;
                rValueType = getNoisyTypeFromExpression(N,RRL(noisyAssignmentStatementNode),currentScope);
                for (IrNode * iter = L(noisyAssignmentStatementNode); iter != NULL; iter = R(iter))
                {
                        if (LL(iter)->type == kNoisyIrNodeType_Tnil)
                        {
                                /*
                                *       We do not need to type check lValueType of an assignment to nil.
                                */
                        }
                        else if (LL(iter)->type == kNoisyIrNodeType_PqualifiedIdentifier)
                        {
                                /*
                                *       TODO; Get type from expression and compare with typeExpr.
                                */
                                lValuetype = getNoisyTypeFromTypeExpr(N,LLL(iter)->symbol->typeTree);
                                if (lValuetype.basicType == noisyTypeError || rValueType.basicType == noisyTypeError)
                                {
                                        deallocateNoisyType(&lValuetype);
                                        return;
                                }
                        }
                }
        }
}

void
noisyStatementTypeCheck(State * N, IrNode * noisyStatementNode, Scope * currentScope)
{
        switch (L(noisyStatementNode)->type)
        {
        case kNoisyIrNodeType_PassignmentStatement:
                noisyAssignmentStatementTypeCheck(N,L(noisyStatementNode), currentScope);
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
noisyStatementListTypeCheck(State * N, IrNode * statementListNode, Scope * currentScope)
{
        for (IrNode * iter = statementListNode; iter != NULL; iter=R(iter))
        {
                if (L(iter) != NULL)
                {
                        noisyStatementTypeCheck(N,L(iter),currentScope);
                }
        }
}


void
noisyFunctionDefnTypeCheck(State * N,IrNode * noisyFunctionDefnNode,Scope * currentScope)
{
        Symbol * functionSymbol = commonSymbolTableSymbolForIdentifier(N, N->moduleScopes, noisyFunctionDefnNode->irLeftChild->tokenString);

        noisySignatureIsMatching(N, RL(noisyFunctionDefnNode),L(functionSymbol->typeTree));
        noisySignatureIsMatching(N, RRL(noisyFunctionDefnNode),R(functionSymbol->typeTree));        

        /*
        *       For local functions we need to typeCheck their declaration.
        *       We cannot skip this step because this function initializes parameterNum and isComplete variables
        *       for function symbols.
        *       If we have a local definition of a function its scope is the TopScope.
        */

        if (functionSymbol->scope == N->noisyIrTopScope)
        {
                noisyDeclareFunctionTypeCheck(N,noisyFunctionDefnNode->irLeftChild->tokenString,RL(noisyFunctionDefnNode),RRL(noisyFunctionDefnNode),currentScope);
        }

        noisyStatementListTypeCheck(N,RR(noisyFunctionDefnNode)->irRightChild->irLeftChild,commonSymbolTableGetScopeWithName(N,currentScope,functionSymbol->identifier));
}

void
noisyModuleDeclTypeCheck(State * N, IrNode * noisyModuleDeclNode,Scope * currentScope)
{
        /*
        *       We do not need to typecheck module parameters.
        */
        noisyModuleDeclBodyTypeCheck(N, RR(noisyModuleDeclNode),currentScope);
}

void
noisyProgramTypeCheck(State * N,IrNode * noisyProgramNode,Scope * currentScope)
{
        for (IrNode * currentNode = noisyProgramNode; currentNode != NULL; currentNode = currentNode->irRightChild)
        {
                if (currentNode->irLeftChild->type == kNoisyIrNodeType_PmoduleDecl)
                {
                        noisyModuleDeclTypeCheck(N, currentNode->irLeftChild,N->moduleScopes);
                }
                else if (currentNode->irLeftChild->type == kNoisyIrNodeType_PfunctionDefn)
                {
                        noisyFunctionDefnTypeCheck(N,currentNode->irLeftChild,currentScope->firstChild);
                }
        }
}

void
noisyTypeCheck(State * N)
{
        noisyProgramTypeCheck(N,N->noisyIrRoot,N->noisyIrTopScope);
}