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
                        return isTypeExprComplete(N,LRL(typeExpr->irLeftChild));
                }
                return false;
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

bool
noisyMatchTypeExpr(IrNode * typeExpr1, IrNode * typeExpr2)
{
        return true;
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

                        if (!noisyMatchTypeExpr(RL(iter),RL(declIter)))
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
noisyFunctionDefnTypeCheck(State * N,IrNode * noisyFunctionDefnNode)
{
        Symbol * functionSymbol = commonSymbolTableSymbolForIdentifier(N, N->moduleScopes, noisyFunctionDefnNode->irLeftChild->tokenString);

        noisySignatureIsMatching(N, RL(noisyFunctionDefnNode),L(functionSymbol->typeTree));
        noisySignatureIsMatching(N, RRL(noisyFunctionDefnNode),R(functionSymbol->typeTree));        


        /*
        *       For local functions we need to typeCheck their declaration.
        */
        if (functionSymbol == NULL)
        {
                noisyDeclareFunctionTypeCheck(N,noisyFunctionDefnNode->irLeftChild->tokenString,RL(noisyFunctionDefnNode),RRL(noisyFunctionDefnNode));
        }
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