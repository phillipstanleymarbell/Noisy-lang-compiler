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
bool noisyIsOfType(NoisyType typ1,NoisyBasicType typeSuperSet);
void noisyStatementListTypeCheck(State * N, IrNode * statementListNode, Scope * currentScope);
void noisyFunctionDefnTypeCheck(State * N,IrNode * noisyFunctionDefnNode,Scope * currentScope);



extern char *		gNoisyProductionDescriptions[];
extern const char *	gNoisyTokenDescriptions[];

static char		kNoisyErrorTokenHtmlTagOpen[]	= "<span style=\"background-color:#FFCC00; color:#FF0000;\">";
static char		kNoisyErrorTokenHtmlTagClose[]	= "</span>";
static char		kNoisyErrorDetailHtmlTagOpen[]	= "<span style=\"background-color:#A9E9FF; color:#000000;\">";
static char		kNoisyErrorDetailHtmlTagClose[]	= "</span>";


void
noisySemanticErrorPre(State *  N, IrNode * currentlyParsingNode,
	const char *  string1, const char *  string2, const char *  string3, const char *  string4)
{
	flexprint(N->Fe, N->Fm, N->Fperr, "\n-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --\n");
	if (N->mode & kCommonModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "<b>");
	}

	if (N->mode & kCommonModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\t%s, line %d position %d, %s ",
						string1,
						currentlyParsingNode->sourceInfo->lineNumber,
						currentlyParsingNode->sourceInfo->columnNumber,
						kNoisyErrorTokenHtmlTagOpen);
		flexprint(N->Fe, N->Fm, N->Fperr, " %s %s %s.<br><br>%s%s",
			kNoisyErrorTokenHtmlTagClose,
			string2,
			(currentlyParsingNode->type > kNoisyIrNodeType_TMax ?
				gNoisyProductionDescriptions[currentlyParsingNode->type] :
				gNoisyTokenDescriptions[currentlyParsingNode->type]),
			kNoisyErrorDetailHtmlTagOpen,
			string3);
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n\t%s, %s line %d position %d,",
						string1,
						currentlyParsingNode->sourceInfo->fileName,
						currentlyParsingNode->sourceInfo->lineNumber,
						currentlyParsingNode->sourceInfo->columnNumber);
		flexprint(N->Fe, N->Fm, N->Fperr, " %s %s.\n\n\t%s",
			string2,
			(currentlyParsingNode->type > kNoisyIrNodeType_TMax ?
				gNoisyProductionDescriptions[currentlyParsingNode->type] :
				gNoisyTokenDescriptions[currentlyParsingNode->type]),
			string3);
	}	
}

void
noisySemanticErrorPost(State *  N)
{
	if (N->mode & kCommonModeCGI)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s</b>", kNoisyErrorDetailHtmlTagClose);
	}

	flexprint(N->Fe, N->Fm, N->Fperr, "\n-- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - --\n\n");	
}


void
noisySemanticError(State *  N, IrNode * currentlyParsingNode, char *  details)
{
	noisySemanticErrorPre(N, currentlyParsingNode, EsemanticsA, EsemanticsB, details, EsemanticsD);
	noisySemanticErrorPost(N);
}


void
noisySemanticErrorRecovery(State *  N)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParserErrorRecovery);

	if (N->verbosityLevel & kCommonVerbosityDebugParser)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "In noisySemanticErrorRecovery(), about to discard tokens...\n");
	}

	if ((N != NULL) && (N->jmpbufIsValid))
	{
		//fprintf(stderr, "doing longjmp");

		/*
		 *	See issue #291.
		 */
		longjmp(N->jmpbuf, 0);
	}

	/*
	 *	Not reached if N->jmpbufIsValid
	 */
	consolePrintBuffers(N);

	exit(EXIT_SUCCESS);
}
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

bool
isMeasurementType(State * N, IrNode * typeExpr)
{
        if (R(typeExpr) != NULL && RL(typeExpr) != NULL)
        {
                IrNode * typeAnnoteList = RL(typeExpr);
                for (IrNode * iter = typeAnnoteList; iter != NULL; iter = iter->irRightChild)
                {
                        if (LL(iter)->type == kNoisyIrNodeType_PtimeseriesDesignation && LLL(iter)->type == kNoisyIrNodeType_Tmeasurement)
                        {
                                return true;
                        }
                }
                return false;
        }
        return false;
}

void
noisyInitNoisyType(NoisyType * typ)
{
        typ->basicType = noisyBasicTypeInit;
        typ->dimensions = 0;
        typ->arrayType = noisyBasicTypeInit;
}

bool
noisyTypeEquals(NoisyType typ1, NoisyType typ2)
{
        if (typ1.basicType == typ2.basicType)
        {
                if (typ1.basicType == noisyBasicTypeArrayType)
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
        else if (typ1.basicType == noisyBasicTypeIntegerConstType)
        {
                return noisyIsOfType(typ2,noisyBasicTypeIntegerConstType);
        }
        else if (typ2.basicType == noisyBasicTypeIntegerConstType)
        {
                return noisyIsOfType(typ1,noisyBasicTypeIntegerConstType);        
        }
        else if (typ1.basicType == noisyBasicTypeRealConstType)
        {
                return noisyIsOfType(typ2,noisyBasicTypeRealConstType);          
        }
        else if (typ2.basicType == noisyBasicTypeRealConstType)
        {
                return noisyIsOfType(typ1,noisyBasicTypeRealConstType);          
        }
        return false;
}

bool
noisyIsOfType(NoisyType typ1,NoisyBasicType typeSuperSet)
{
        if (typeSuperSet == noisyBasicTypeIntegerConstType)
        {
                switch (typ1.basicType)
                {
                case noisyBasicTypeInt4:
                case noisyBasicTypeInt8:
                case noisyBasicTypeInt16:
                case noisyBasicTypeInt32:
                case noisyBasicTypeInt64:
                case noisyBasicTypeInt128:
                case noisyBasicTypeIntegerConstType:
                case noisyBasicTypeNat4:
                case noisyBasicTypeNat8:
                case noisyBasicTypeNat16:
                case noisyBasicTypeNat32:
                case noisyBasicTypeNat64:
                case noisyBasicTypeNat128:
                        return true;
                        break;
                default:
                        return false;
                        break;
                }
        }
        else if (typeSuperSet == noisyBasicTypeRealConstType)
        {
                switch (typ1.basicType)
                {
                case noisyBasicTypeFloat16:
                case noisyBasicTypeFloat32:
                case noisyBasicTypeFloat64:
                case noisyBasicTypeFloat128:
                case noisyBasicTypeRealConstType:
                        return true;
                        break;
                default:
                        return false;
                        break;
                }
        }
        else if (typeSuperSet == noisyBasicTypeArithType)
        {
                return noisyIsOfType(typ1,noisyBasicTypeIntegerConstType) || noisyIsOfType(typ1,noisyBasicTypeRealConstType);
        }
        return false;
}

bool
noisyIsSigned(NoisyType typ)
{
        return (typ.basicType > noisyBasicTypeInit && typ.basicType <= noisyBasicTypeInt128);
}
/*
*       Takes two NoisyTypes arguments, compares their basicType
*       and returns the most specific type. For example if we have
*       a noisyIntegerConst and a noisyBasicTypeInt32 it returns the noisyBasicTypeInt32 type.
*/
NoisyType
noisyGetMoreSpecificType(NoisyType typ1, NoisyType typ2)
{
        return typ1.basicType < typ2.basicType ? typ1 : typ2;
}

/*
*       Takes to NoisyTypes and checks if we can convert from type fromType to type
*       toType.
*/
bool
noisyCanTypeCast(NoisyType fromType,NoisyType toType)
{
        /*
        *       We can convert from integers to other integers, from integers to floats, from floats
        *       to integers, from naturals to integers and from floats to other floats.
        *       We do not permit any other type casts.
        */
        if (fromType.basicType > noisyBasicTypeBool && fromType.basicType <= noisyBasicTypeRealConstType)
        {
                if (toType.basicType > noisyBasicTypeBool && toType.basicType <= noisyBasicTypeRealConstType)
                {
                        // /*
                        // *       We do not permit from integers to naturals
                        // */
                        // if (fromType.basicType > noisyBasicTypeBool && fromType.basicType <= noisyBasicTypeInt128)
                        // {
                        //         if (toType.basicType >= noisyBasicTypeNat4 && toType.basicType <= noisyBasicTypeNat128)
                        //         {
                        //                 return false;
                        //         }
                        // }
                        return true;
                }
        }
        return false;
}

NoisyType
getNoisyTypeFromBasicType(IrNode * basicType)
{
        NoisyType noisyType;
        noisyType.arrayType = noisyBasicTypeInit;
        noisyType.dimensions = 0;

        if (L(basicType)->type == kNoisyIrNodeType_Tbool)
        {
                noisyType.basicType = noisyBasicTypeBool;
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
                        noisyType.basicType = noisyBasicTypeInt4;
                        break;
                case kNoisyIrNodeType_Tnat4:
                        noisyType.basicType = noisyBasicTypeNat4;
                        break;
                case kNoisyIrNodeType_Tint8:
                        noisyType.basicType = noisyBasicTypeInt8;
                        break;
                case kNoisyIrNodeType_Tnat8:
                        noisyType.basicType = noisyBasicTypeNat8;
                        break;
                case kNoisyIrNodeType_Tint16:
                        noisyType.basicType = noisyBasicTypeInt16;
                        break;
                case kNoisyIrNodeType_Tnat16:
                        noisyType.basicType = noisyBasicTypeNat16;
                        break;
                case kNoisyIrNodeType_Tint32:
                        noisyType.basicType = noisyBasicTypeInt32;
                        break;
                case kNoisyIrNodeType_Tnat32:
                        noisyType.basicType = noisyBasicTypeNat32;
                        break;
                case kNoisyIrNodeType_Tint64:
                        noisyType.basicType = noisyBasicTypeInt64;
                        break;
                case kNoisyIrNodeType_Tnat64:
                        noisyType.basicType = noisyBasicTypeNat64;
                        break;
                case kNoisyIrNodeType_Tint128:
                        noisyType.basicType = noisyBasicTypeInt128;
                        break;
                case kNoisyIrNodeType_Tnat128:
                        noisyType.basicType = noisyBasicTypeNat128;
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
                        noisyType.basicType = noisyBasicTypeFloat16;
                        break;
                case kNoisyIrNodeType_Tfloat32:
                        noisyType.basicType = noisyBasicTypeFloat32;
                        break;
                case kNoisyIrNodeType_Tfloat64:
                        noisyType.basicType = noisyBasicTypeFloat64;
                        break;
                case kNoisyIrNodeType_Tfloat128:
                        noisyType.basicType = noisyBasicTypeFloat128;
                        break;
                default:
                        break;
                }
        }
        else if(L(basicType)->type == kNoisyIrNodeType_Tstring)
        {
                noisyType.basicType = noisyBasicTypeString;
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

        noisyType.basicType = noisyBasicTypeArrayType;
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
        // Symbol * typeSymbol = commonSymbolTableSymbolForIdentifier(N,N->noisyIrTopScope,L(typeNameNode)->tokenString);
        Symbol * typeSymbol = typeNameNode->symbol;

        if (typeSymbol == NULL)
        {
                /*
                *       Type symbol does not exist error?
                */
                typeSymbol = commonSymbolTableSymbolForIdentifier(N,N->noisyIrTopScope,L(typeNameNode)->tokenString);
                noisyType.basicType = noisyBasicTypeErrorType;
                return noisyType;
        }

        IrNode * typeTree = typeSymbol->typeTree;

        if (typeTree == NULL)
        {
                noisyType.basicType = noisyBasicTypeErrorType;
                return noisyType;
        }

        /*
        *       This case is for loading a templated function. The symbols type tree should be a typeExpr
        *       which we assign to it.
        */
        if (typeTree->type == kNoisyIrNodeType_PtypeExpr)
        {
                return getNoisyTypeFromTypeExpr(N,typeTree);
        }

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
                noisyType.basicType = noisyBasicTypeErrorType;
                return noisyType;
        }
}

/*
*       Takes the state N and a TypeExpr node and returns the corresponding
*       NoisyType. If it fails the returned basic type is noisyBasicTypeErrorType.
*/
NoisyType
getNoisyTypeFromTypeExpr(State * N, IrNode * typeExpr)
{
        NoisyType noisyType;
        if (typeExpr->type == kNoisyIrNodeType_PconstantDecl)
        {
                NoisyType noisyType;
                noisyType.arrayType = noisyBasicTypeInit;
                noisyType.dimensions = 0;
                switch (L(typeExpr)->type)
                {
                case kNoisyIrNodeType_TintegerConst:
                        noisyType.basicType = noisyBasicTypeIntegerConstType;
                        break;
                case kNoisyIrNodeType_TrealConst:
                        noisyType.basicType = noisyBasicTypeRealConstType;
                        break;
                case kNoisyIrNodeType_TboolConst:
                        noisyType.basicType = noisyBasicTypeBool;
                        break;
                default:
                        noisyType.basicType = noisyBasicTypeErrorType;
                        break;
                }
                return noisyType;
        }
        else
        {
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
                                /*
                                *       Not supported types error.
                                */
                        char *	details;

                                asprintf(&details, "Unsupported non Aggregate Type\n");
                                noisySemanticError(N,L(typeExpr),details);
                                noisySemanticErrorRecovery(N);
                        }
                }
                else if (L(typeExpr)->type == kNoisyIrNodeType_PtypeName)
                {
                        return getNoisyTypeFromTypeSymbol(N,L(typeExpr));
                }
        }
        

        noisyType.basicType = noisyBasicTypeErrorType;
        return noisyType;
}

/*
*       Takes an argument name, an expression and an inputSIgnature of a function. Checks if the argument name and type matches
*       with one of the signature's arguments. In that case it returns true. Otherwise, it returns false.
*/
bool
noisyArgumentMatchesSignature(State * N, IrNode * argName,IrNode * expr,IrNode * inputSignature, Scope * currentScope, bool isLoaded)
{
        for (IrNode * iter = inputSignature; iter != NULL; iter = RR(iter))
        {
                if (!strcmp(L(iter)->tokenString,argName->tokenString))
                {
                        NoisyType typeExprType = getNoisyTypeFromTypeExpr(N,RL(iter));
                        if (noisyTypeEquals(typeExprType,getNoisyTypeFromExpression(N,expr,currentScope)))
                        {
                                L(iter)->symbol->noisyType = typeExprType;
                                expr->noisyType = typeExprType;
                                return true;
                        }
                }
        }
        /*
        *       If we search all arguments and they dont match then we return false.
        */
        return false;               
}

/*
*       Takes a noisyFactor IrNode and a the currentScope and returns the NoisyType of the factor.
*       If every type check is correct returns the type else it returns noisyBasicTypeErrorType.
*/
NoisyType
getNoisyTypeFromFactor(State * N, IrNode * noisyFactorNode, Scope * currentScope)
{
        NoisyType factorType;

        if (L(noisyFactorNode)->type == kNoisyIrNodeType_TintegerConst)
        {
                factorType.basicType = noisyBasicTypeIntegerConstType;
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_TrealConst)
        {
                factorType.basicType = noisyBasicTypeRealConstType;
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_TstringConst)
        {
                factorType.basicType = noisyBasicTypeString;
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_TboolConst)
        {
                factorType.basicType = noisyBasicTypeBool;
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_PqualifiedIdentifier)
        {
                Symbol * identifierSymbol = LL(noisyFactorNode)->symbol;

                if (identifierSymbol == NULL)
                {
                        factorType.basicType = noisyBasicTypeErrorType;
                }

                if (identifierSymbol->typeTree != NULL)
                {
                        factorType = getNoisyTypeFromTypeExpr(N,identifierSymbol->typeTree);
                        identifierSymbol->noisyType = factorType;
                }
                else
                {
                        factorType = identifierSymbol->noisyType;
                }
                
                if (factorType.basicType == noisyBasicTypeArrayType)
                {
                        int dims = 0;
                        for (IrNode * iter = LR(noisyFactorNode); iter != NULL; iter = R(iter))
                        {
                                if (! noisyIsOfType(getNoisyTypeFromExpression(N,LR(iter),currentScope), noisyBasicTypeIntegerConstType))
                                {
                                        /*
                                        *       Indexes are not integers error.
                                        */
                                        char *	details;

                                        asprintf(&details, "Indexing \"%s\" array with a non-integer expression\n",identifierSymbol->identifier);
                                        noisySemanticError(N,iter,details);
                                        noisySemanticErrorRecovery(N);
                                        // factorType.basicType = noisyBasicTypeErrorType;
                                }
                                dims++;
                        }

                        factorType.dimensions -= dims;
                        /*
                        *       If there are no type errors on array indexing we the arrayType of the array.
                        *       e.g. when we index an array of int32 the factor we return has type int32.
                        */
                        if (factorType.dimensions == 0)
                        {
                                factorType.basicType = factorType.arrayType;
                        }
                        else if (factorType.dimensions < 0 )
                        {
                                /*
                                *       Indexing dimension error.
                                */
                                char *	details;

                                asprintf(&details, "Dimensions of array \"%s\" dont match\n",identifierSymbol->identifier);
                                noisySemanticError(N,noisyFactorNode,details);
                                noisySemanticErrorRecovery(N);
                        }     
                }
                else
                {
                        if (LR(noisyFactorNode) != NULL)
                        {
                                factorType.basicType = noisyBasicTypeErrorType;
                        }
                }
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_PnamegenInvokeShorthand)
        {
                /*
                *       We search for functionName in the top scope (local functions) and if it fails it searches on the module scope.
                */
                Symbol * functionNameSymbol = commonSymbolTableSymbolForIdentifier(N, N->noisyIrTopScope, LL(noisyFactorNode)->tokenString);   
                bool loadedFunction = false;

                if (functionNameSymbol == NULL)
                {
                        functionNameSymbol = commonSymbolTableSymbolForIdentifier(N,currentScope,LL(noisyFactorNode)->tokenString);
                        if (functionNameSymbol->noisyType.basicType != noisyBasicTypeNamegenType)
                        {
                                char * details;

                                asprintf(&details, "Unknown namegen invocation \"%s\"\n",LL(noisyFactorNode)->tokenString);
                                noisySemanticError(N,noisyFactorNode,details);
                                noisySemanticErrorRecovery(N);
                        }
                        else
                        {
                                loadedFunction = true;
                                functionNameSymbol = functionNameSymbol->noisyType.functionDefinition;
                        }

                }


                bool paramCorrect = true;
                int paramCount = 0;

                IrNode * inputSignature =L(functionNameSymbol->typeTree);
                if (inputSignature->type == kNoisyIrNodeType_PwriteTypeSignature)
                {
                        inputSignature = L(inputSignature);
                }
                /*
                *       Check if inputSignature is nil. Else typeCheck every argument.
                */
                if (L(inputSignature)->type == kNoisyIrNodeType_Tnil)
                {
                        if (LR(noisyFactorNode) == NULL)
                        {
                                factorType.basicType = noisyBasicTypeNilType;        
                        }
                        else
                        {
                                char * details;
                                asprintf(&details, "Using arguments for nil function \"%s\"\n",functionNameSymbol->identifier);
                                noisySemanticError(N,noisyFactorNode,details);
                                noisySemanticErrorRecovery(N);
                        }
                }
                else
                {
                        for (IrNode * iter = LR(noisyFactorNode); iter != NULL; iter = RR(iter))
                        {
                                IrNode * argName = L(iter);
                                IrNode * expr = RL(iter);

                                paramCount++;
                                paramCorrect = paramCorrect && noisyArgumentMatchesSignature(N,argName,expr,inputSignature,currentScope,loadedFunction);

                                if (!paramCorrect)
                                {
                                        char * details;
                                        asprintf(&details, "Argument type mismatch for argument \"%s\"\n",argName->tokenString);
                                        noisySemanticError(N,noisyFactorNode,details);
                                        noisySemanticErrorRecovery(N);
                                }

                        }

                        if (paramCount != functionNameSymbol->parameterNum)
                        {
                                char * details;
                                asprintf(&details, "Wrong number of arguments for function \"%s\"\n",functionNameSymbol->identifier);
                                noisySemanticError(N,noisyFactorNode,details);
                                noisySemanticErrorRecovery(N);
                        }
                }
                

                IrNode * outputSignature = R(functionNameSymbol->typeTree);
                if (outputSignature->type == kNoisyIrNodeType_PreadTypeSignature)
                {
                        outputSignature = L(outputSignature);
                }

                /*
                *       TypeCheck output signature. The type returned is the return type of the function.
                */
                if (L(outputSignature)->type ==kNoisyIrNodeType_Tnil)
                {
                        factorType.basicType = noisyBasicTypeNilType;
                }
                else
                {
                        factorType = getNoisyTypeFromTypeExpr(N,RL(outputSignature));
                        L(outputSignature)->symbol->noisyType = factorType;
                }
                
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_Pexpression)
        {
                factorType = getNoisyTypeFromExpression(N,L(noisyFactorNode),currentScope);
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_PtypeMinExpr
                || L(noisyFactorNode)->type == kNoisyIrNodeType_PtypeMaxExpr)
        {
                factorType = getNoisyTypeFromBasicType(LL(noisyFactorNode));
        }
        else if (L(noisyFactorNode)->type == kNoisyIrNodeType_Tnil)
        {
                factorType.basicType = noisyBasicTypeNilType;
        }
	// FIX: factorType can be uninitalized
        noisyFactorNode->noisyType = factorType;
        return factorType;
}

/*
*       Take a unaryOp node and a factorType and type checks if the operator can be
*       applied on the specific factorType.
*/
NoisyType
noisyUnaryOpTypeCheck(State * N,IrNode * noisyUnaryOpNode,NoisyType factorType)
{
        NoisyType returnType = factorType;

        if (L(noisyUnaryOpNode)->type == kNoisyIrNodeType_Tplus ||
            L(noisyUnaryOpNode)->type == kNoisyIrNodeType_Tminus)
        {
                if (!noisyIsOfType(factorType,noisyBasicTypeArithType))
                {
                        char * details;
                        asprintf(&details, "Unary operator and operand mismatch \"%s\"\n",L(noisyUnaryOpNode)->tokenString);
                        noisySemanticError(N,noisyUnaryOpNode,details);
                        noisySemanticErrorRecovery(N);
                }
        }
        else if (L(noisyUnaryOpNode)->type == kNoisyIrNodeType_Ttilde)
        {
                if (!noisyIsOfType(factorType,noisyBasicTypeIntegerConstType))
                {
                        char * details;
                        asprintf(&details, "Unary operator and operand mismatch \"%s\"\n",L(noisyUnaryOpNode)->tokenString);
                        noisySemanticError(N,noisyUnaryOpNode,details);
                        noisySemanticErrorRecovery(N);
                }
        }
        else if (L(noisyUnaryOpNode)->type == kNoisyIrNodeType_PunaryBoolOp)
        {
                if (factorType.basicType != noisyBasicTypeBool)
                {
                        char * details;
                        asprintf(&details, "Unary operator and operand mismatch \"%s\"\n",LL(noisyUnaryOpNode)->tokenString);
                        noisySemanticError(N,noisyUnaryOpNode,details);
                        noisySemanticErrorRecovery(N);
                }
        }
        else if (L(noisyUnaryOpNode)->type == kNoisyIrNodeType_Tlength
                || L(noisyUnaryOpNode)->type == kNoisyIrNodeType_Tsort
                || L(noisyUnaryOpNode)->type == kNoisyIrNodeType_Treverse)
        {
                if (factorType.basicType != noisyBasicTypeArrayType && factorType.basicType != noisyBasicTypeString)
                {
                        char * details;
                        asprintf(&details, "Unary operator and operand mismatch \"%s\"\n",L(noisyUnaryOpNode)->tokenString);
                        noisySemanticError(N,noisyUnaryOpNode,details);
                        noisySemanticErrorRecovery(N);
                }
                if (L(noisyUnaryOpNode)->type == kNoisyIrNodeType_Tlength)
                {
                        NoisyType ret;
                        ret.basicType = noisyBasicTypeIntegerConstType;
                        returnType = ret;
                }
        }
        else if (L(noisyUnaryOpNode)->type == kNoisyIrNodeType_TchannelOperator)
        {
                if (factorType.basicType != noisyBasicTypeNamegenType)
                {
                        IrNode * qualifiedIdentifierNode = RL(noisyUnaryOpNode->irParent);
                        if (qualifiedIdentifierNode->type == kNoisyIrNodeType_PqualifiedIdentifier)
                        {
                                if (qualifiedIdentifierNode->irLeftChild->symbol->symbolType != kNoisySymbolTypeParameter)
                                {
                                        char * details;
                                        asprintf(&details, "Cannot read from a non channel factor!\n");
                                        noisySemanticError(N,noisyUnaryOpNode,details);
                                        noisySemanticErrorRecovery(N);
                                }
                                N->currentFunction->isChannel = true;
                                returnType = qualifiedIdentifierNode->irLeftChild->symbol->noisyType;
                        }
                        else
                        {
                                char * details;
                                asprintf(&details, "Cannot read from a non channel factor!\n");
                                noisySemanticError(N,noisyUnaryOpNode,details);
                                noisySemanticErrorRecovery(N);
                        }
                }
                else
                {
                        factorType.functionDefinition->isChannel = true;
                        IrNode * factorTypeExpr;
                        if (factorType.functionDefinition->typeTree->type == kNoisyIrNodeType_PfunctionDecl)
                        {
                                factorTypeExpr = L(RLR(factorType.functionDefinition->typeTree));
                        }
                        else
                        {
                                factorTypeExpr = RRL(factorType.functionDefinition->typeTree);
                        }
                        returnType = getNoisyTypeFromTypeExpr(N,factorTypeExpr);
                }

                /*
                *       After applying the channel operator on a namegen, the return value has the type of the
                *       namegen's return value.
                */
        }
        else
        {
                returnType = factorType;
        }
        return returnType;
}

/*
*       Takes a noisyTerNode typechecks it and everything is correct it returns the NoisyType of the term.
*       TODO; Might need more work.
*/
NoisyType
getNoisyTypeFromTerm(State * N, IrNode * noisyTermNode, Scope * currentScope)
{
        NoisyType basicType,termType;
        IrNode * factorNode = NULL;
        IrNode * unaryOpNode = NULL;
        noisyInitNoisyType(&basicType);

        /*
        *       This flag is needed because the form of the tree is different based on whether a prefix exists
        *       on the term expression.
        */
        bool prefixExists = false;

        if (L(noisyTermNode)->type == kNoisyIrNodeType_PbasicType)
        {
                basicType = getNoisyTypeFromBasicType(noisyTermNode->irLeftChild);
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

        termType = getNoisyTypeFromFactor(N,factorNode,currentScope);

        if (R(noisyTermNode) != NULL)
        {
                if (RL(noisyTermNode)->type == kNoisyIrNodeType_TplusPlus || RL(noisyTermNode)->type == kNoisyIrNodeType_TminusMinus)
                {
                        factorNode = R(factorNode);
                }
        }

        for (IrNode * iter = prefixExists ? R(factorNode) : R(noisyTermNode) ; iter != NULL; iter = RR(iter))
        {
                NoisyType factorIterType;

                factorIterType = getNoisyTypeFromFactor(N,RL(iter),currentScope);

                if (!noisyTypeEquals(factorIterType,termType))
                {
                        /*
                        *       Operands type mismatch.
                        */
                        char *	details;

                        asprintf(&details, "Operands type mismatch\n");
                        noisySemanticError(N,factorNode,details);
                        noisySemanticErrorRecovery(N);
                        break;
                }

                if (LL(iter)->type == kNoisyIrNodeType_Tasterisk
                || LL(iter)->type == kNoisyIrNodeType_Tdivide)
                {
                        if (!noisyIsOfType(termType,noisyBasicTypeArithType))
                        {
                                /*
                                *       Operator and operand mismatch.
                                */
                               char *	details;

                                asprintf(&details, "Operator and operands type mismatch\n");
                                noisySemanticError(N,factorNode,details);
                                noisySemanticErrorRecovery(N);
                                break;

                        }
                }
                else if (LL(iter)->type == kNoisyIrNodeType_Tpercent
                || LL(iter)->type == kNoisyIrNodeType_TarithmeticAnd)
                {
                        if (!noisyIsOfType(termType,noisyBasicTypeIntegerConstType))
                        {
                                char *	details;

                                asprintf(&details, "Operator and operands type mismatch\n");
                                noisySemanticError(N,factorNode,details);
                                noisySemanticErrorRecovery(N);
                                break;
                        }
                }
                else if (LL(iter)->type == kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp)
                {
                        NoisyType boolType = {noisyBasicTypeBool,0,0};
                        if (!noisyTypeEquals(termType,boolType))
                        {
                                /*
                                *       Operator and operand mismatch.
                                */
                                char *	details;

                                asprintf(&details, "Operator and operands type mismatch\n");
                                noisySemanticError(N,factorNode,details);
                                noisySemanticErrorRecovery(N);
                        }
                }
                else
                {
                        /*
                        *       Unsupported operators
                        */
                        char *	details;

                        asprintf(&details, "Unsupported binary operator\n");
                        noisySemanticError(N,factorNode,details);
                        noisySemanticErrorRecovery(N);
                }
                termType = noisyGetMoreSpecificType(termType,factorIterType);
        }

        if (unaryOpNode != NULL)
        {
                termType = noisyUnaryOpTypeCheck(N,unaryOpNode,termType);
        }

        if (basicType.basicType != noisyBasicTypeInit)
        {
                /*
                *       The basic type, typecasts the factor expression.
                */
                if (noisyCanTypeCast(termType,basicType))
                {
                        termType = basicType;
                }
                else
                {
                        char *	details;

                        asprintf(&details, "Cannot convert types!\n");
                        noisySemanticError(N,noisyTermNode,details);
                        noisySemanticErrorRecovery(N);
                }
        }

        noisyTermNode->noisyType = termType;
        return termType;
}

/*
*       Takes a noisyExpressionNode and returns its NoisyType after typeChecking it.
*/
NoisyType
getNoisyTypeFromExpression(State * N, IrNode * noisyExpressionNode, Scope * currentScope)
{
        NoisyType typ1,returnType;
        noisyInitNoisyType(&returnType);

        if (L(noisyExpressionNode)->type == kNoisyIrNodeType_Pterm)
        {
                typ1 = getNoisyTypeFromTerm(N,L(noisyExpressionNode), currentScope);

                returnType = typ1;

                for (IrNode * iter = R(noisyExpressionNode); iter != NULL; iter = RR(iter))
                {
                        IrNode * operatorNode = L(iter);
                        IrNode * termNode = RL(iter);

                        NoisyType termTyp = getNoisyTypeFromTerm(N,termNode,currentScope);

                        if (noisyTypeEquals(returnType,termTyp))
                        {
                                switch (L(operatorNode)->type)
                                {
                                case kNoisyIrNodeType_Tplus:
                                case kNoisyIrNodeType_Tminus:
                                        if (!noisyIsOfType(returnType,noisyBasicTypeArithType))
                                        {
                                                if (L(operatorNode)->type == kNoisyIrNodeType_Tplus && returnType.basicType == noisyBasicTypeString)
                                                {
                                                        /*
                                                        *       We use it for emphasis. "+" operator works for strings as well.
                                                        */
                                                }
                                                else
                                                {
                                                        char *	details;

                                                        asprintf(&details, "Operator \"%s\" and operands type mismatch\n",L(operatorNode)->tokenString);
                                                        noisySemanticError(N,L(noisyExpressionNode),details);
                                                        noisySemanticErrorRecovery(N);
                                                }
                                        }
                                        /*
                                        *       returnType = typ1;
                                        */
                                        break;
                                case kNoisyIrNodeType_TrightShift:
                                case kNoisyIrNodeType_TleftShift:
                                case kNoisyIrNodeType_TbitwiseOr:
                                        if (!noisyIsOfType(returnType,noisyBasicTypeIntegerConstType))
                                        {
                                                char *	details;

                                                asprintf(&details, "Operator \"%s\" and operands type mismatch\n",L(operatorNode)->tokenString);
                                                noisySemanticError(N,L(noisyExpressionNode),details);
                                                noisySemanticErrorRecovery(N);
                                        }
                                        break;
                                case kNoisyIrNodeType_PcmpOp:
                                        if (LL(operatorNode)->type == kNoisyIrNodeType_Tequals
                                        || LL(operatorNode)->type == kNoisyIrNodeType_TnotEqual)
                                        {
                                                if (returnType.basicType == noisyBasicTypeArrayType)
                                                {
                                                        char *	details;

                                                        asprintf(&details, "Operator \"%s\" and operands type mismatch\n",LL(operatorNode)->tokenString);
                                                        noisySemanticError(N,L(noisyExpressionNode),details);
                                                        noisySemanticErrorRecovery(N);
                                                }
                                                /*
                                                *       This type assignment happens here so we can find constants' type.
                                                */
                                                RL(iter)->noisyType = returnType;
                                                returnType.basicType = noisyBasicTypeBool;
                                        }
                                        else if (LL(operatorNode)->type == kNoisyIrNodeType_TgreaterThan
                                                || LL(operatorNode)-> type == kNoisyIrNodeType_TgreaterThanEqual
                                                || LL(operatorNode)-> type == kNoisyIrNodeType_TlessThan
                                                || LL(operatorNode)-> type == kNoisyIrNodeType_TlessThanEqual)
                                        {
                                                if (noisyIsOfType(returnType,noisyBasicTypeArithType))
                                                {
                                                        /*
                                                        *       This type assignment happens here so we can find constants' type.
                                                        */
                                                        RL(iter)->noisyType = returnType;
                                                        returnType.basicType = noisyBasicTypeBool; 
                                                }
                                                else
                                                {
                                                        char *	details;

                                                        asprintf(&details, "Operator \"%s\" and operands type mismatch\n",LL(operatorNode)->tokenString);
                                                        noisySemanticError(N,L(noisyExpressionNode),details);
                                                        noisySemanticErrorRecovery(N);  
                                                }

                                        }
                                        else
                                        {
                                                char *	details;

                                                asprintf(&details, "Unsupported CmpOp \"%s\"\n",L(operatorNode)->tokenString);
                                                noisySemanticError(N,L(noisyExpressionNode),details);
                                                noisySemanticErrorRecovery(N);
                                        }
                                        break;
                                case kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp:
                                        if (returnType.basicType != noisyBasicTypeBool)
                                        {
                                                char *	details;

                                                        asprintf(&details, "Operator \"%s\" and operands type mismatch\n",L(operatorNode)->tokenString);
                                                        noisySemanticError(N,L(noisyExpressionNode),details);
                                                        noisySemanticErrorRecovery(N);
                                        }
                                        break;
                                default:
                                        break;
                                }
                        }
                        else
                        {
                                /*
                                *       Operands type mismatch error.
                                */
                                char *	details;

                                asprintf(&details, "Operands type mismatch\n");
                                noisySemanticError(N,L(noisyExpressionNode),details);
                                noisySemanticErrorRecovery(N);
                        }
                        returnType = noisyGetMoreSpecificType(returnType,termTyp);
                }
        }
        else if (L(noisyExpressionNode)->type == kNoisyIrNodeType_PanonAggrCastExpr)
        {
                if (LL(noisyExpressionNode)->type == kNoisyIrNodeType_ParrayCastExpr)
                {
                        returnType.basicType = noisyBasicTypeArrayType;
                        if (LLL(noisyExpressionNode)->type == kNoisyIrNodeType_PinitList)
                        {
                                int sizeOfDim = 0;

                                NoisyType elemType;
                                noisyInitNoisyType(&elemType);
                                for (IrNode * iter = LLL(noisyExpressionNode); iter != NULL; iter = R(iter))
                                {
                                        if (elemType.basicType == noisyBasicTypeInit)
                                        {
                                               elemType = getNoisyTypeFromExpression(N,LL(iter),currentScope);
                                        }
                                        else
                                        {
                                                if (!noisyTypeEquals(elemType,getNoisyTypeFromExpression(N,LL(iter),currentScope)))
                                                {
                                                        /*
                                                        *       Elements type dont match in array.
                                                        */
                                                        char *	details;

                                                        asprintf(&details, "Elements of arrayInitList don't match\n");
                                                        noisySemanticError(N,L(iter),details);
                                                        noisySemanticErrorRecovery(N);
                                                }
                                        }

                                        sizeOfDim++;
                                }

                                if (elemType.basicType != noisyBasicTypeArrayType)
                                {
                                        returnType.dimensions = 1;
                                        returnType.sizeOfDimension[0] = sizeOfDim;
                                        returnType.arrayType = elemType.basicType;
                                }
                                else
                                {
                                        returnType.dimensions = elemType.dimensions + 1;
                                        int i;
                                        for (i = 0; i < elemType.dimensions; i++)
                                        {
                                                returnType.sizeOfDimension[i] = elemType.sizeOfDimension[i];
                                        }
                                        returnType.sizeOfDimension[i] = sizeOfDim;
                                        returnType.arrayType = elemType.arrayType;
                                }
                        }
                        else if (LLL(noisyExpressionNode)->type == kNoisyIrNodeType_TintegerConst)
                        {
                                int sizeOfDim = LLL(noisyExpressionNode)->token->integerConst;

                                NoisyType elemType;
                                noisyInitNoisyType(&elemType);

                                for (IrNode * iter = LLR(noisyExpressionNode); iter != NULL; iter = R(iter))
                                {
                                        IrNode * exprNode = LL(iter);
                                        if (L(iter)->type == kNoisyIrNodeType_Tasterisk)
                                        {
                                                exprNode = RLL(iter);
                                        }
                                        if (elemType.basicType == noisyBasicTypeInit)
                                        {
                                               elemType = getNoisyTypeFromExpression(N,exprNode,currentScope);
                                        }
                                        else
                                        {
                                                if (!noisyTypeEquals(elemType,getNoisyTypeFromExpression(N,exprNode,currentScope)))
                                                {
                                                        /*
                                                        *       Elements type dont match in array.
                                                        */
                                                        char *	details;

                                                        asprintf(&details, "Elements of arrayInitList don't match\n");
                                                        noisySemanticError(N,L(iter),details);
                                                        noisySemanticErrorRecovery(N);
                                                }
                                        }
                                }

                                if (elemType.basicType != noisyBasicTypeArrayType)
                                {
                                        returnType.dimensions = 1;
                                        returnType.sizeOfDimension[0] = sizeOfDim;
                                        returnType.arrayType = elemType.basicType;
                                }
                                else
                                {
                                        returnType.dimensions = elemType.dimensions + 1;
                                        int i;
                                        for (i = 1; i < returnType.dimensions; i++)
                                        {
                                                returnType.sizeOfDimension[i] = elemType.sizeOfDimension[i-1];
                                        }
                                        returnType.sizeOfDimension[0] = sizeOfDim;
                                        returnType.arrayType = elemType.arrayType;
                                }
                        }
                }
                else
                {
                        /*
                        *       Unsupported expression.
                        */
                        char *	details;

                        asprintf(&details, "This non aggregate cast expression is not supported\n");
                        noisySemanticError(N,LL(noisyExpressionNode),details);
                        noisySemanticErrorRecovery(N);
                }
        }
        else if (L(noisyExpressionNode)->type == kNoisyIrNodeType_PloadExpr)
        {
                static int loadCount = 0;

                IrNode * moduleNameIrNode = LL(noisyExpressionNode);
                IrNode * funcNameIrNode;
                IrNode * tupleTypeNode;
                // IrNode * typeNameNode;

                if (LR(noisyExpressionNode)->type == kNoisyIrNodeType_Tidentifier)
                {
                        funcNameIrNode = LR(noisyExpressionNode);
                        if (L(funcNameIrNode)->type == kNoisyIrNodeType_PtupleType)
                        {
                                tupleTypeNode = L(funcNameIrNode);
                                // typeNameNode = RL(funcNameIrNode);
                        }
                        else
                        {
                                tupleTypeNode = NULL;
                                // typeNameNode = L(funcNameIrNode);
                        }

                }
                else
                {
                        funcNameIrNode = NULL;
                        if (LRL(noisyExpressionNode)->type == kNoisyIrNodeType_PtupleType)
                        {
                                tupleTypeNode = LRL(noisyExpressionNode);
                                // typeNameNode = LRR(noisyExpressionNode)->irLeftChild;
                        }
                        else
                        {
                                tupleTypeNode = NULL;
                                // typeNameNode = LRL(noisyExpressionNode);
                        }

                }

                Symbol * moduleSymbol = commonSymbolTableSymbolForIdentifier(N,N->moduleScopes,moduleNameIrNode->tokenString);

                if (moduleSymbol == NULL || moduleSymbol->symbolType != kNoisySymbolTypeModule)
                {
                        char *	details;

                        asprintf(&details, "Could not find a module named \"%s\"\n",moduleNameIrNode->tokenString);
                        noisySemanticError(N,moduleNameIrNode,details);
                        noisySemanticErrorRecovery(N);
                }

                IrNode * typeParameterListNodeIter = RL(moduleSymbol->typeTree->irParent->irParent);
                for (IrNode * iter = (tupleTypeNode); iter != NULL; iter = R(iter))
                {
                        /*
                        *       Assign the type expression to the module parameters.
                        */
                        if (typeParameterListNodeIter == NULL)
                        {
                                char *	details;

                                asprintf(&details, "Type parameters mismatch when loading from module \"%s\"\n",moduleSymbol->identifier);
                                noisySemanticError(N,moduleNameIrNode,details);
                                noisySemanticErrorRecovery(N);
                        }

                        L(typeParameterListNodeIter)->symbol->typeTree = L(iter);
                        typeParameterListNodeIter = R(typeParameterListNodeIter);
                }

                if (typeParameterListNodeIter != NULL)
                {
                        if (typeParameterListNodeIter->irLeftChild != NULL)
                        {
                                char *	details;

                                asprintf(&details, "Type parameters mismatch when loading from module \"%s\"\n",moduleSymbol->identifier);
                                noisySemanticError(N,moduleNameIrNode,details);
                                noisySemanticErrorRecovery(N);
                        }
                }

                Symbol * funcSymbol = NULL;
                if (funcNameIrNode != NULL)
                {
                        funcSymbol = commonSymbolTableSymbolForIdentifier(N,moduleSymbol->scope->firstChild,funcNameIrNode->tokenString);
                        if (funcSymbol == NULL
                        || (funcSymbol->symbolType != kNoisySymbolTypeNamegenDeclaration
                        && funcSymbol->symbolType != kNoisySymbolTypeNamegenDefinition) )
                        {
                                char *	details;

                                asprintf(&details, "Could not find a function named \"%s\" in module \"%s\"\n",funcNameIrNode->tokenString,moduleNameIrNode->tokenString);
                                noisySemanticError(N,moduleNameIrNode,details);
                                noisySemanticErrorRecovery(N);
                        }
                        /*
                        *       We use this for templated functions. Templated functions are not typeComplete. Therefore
                        *       when we load them with a specific type they become typeComplete and then we can typeCheck them
                        *       with noisyFunctionDefnTypeCheck.
                        */
                        if (!funcSymbol->isTypeComplete)
                        {
                                /*
                                *       For templated functions we create a new symbol table entry. They have new name (their
                                *       name with an "_integer" appended) as well as deep copies of the their tree regarding typeTree
                                *       and function definition nodes. Before entering here we have assigned basic types to the module
                                *       parameters. Then we typeCheck the function definition with the types assigned and then we deep
                                *       copy the tree of the function so we can keep all the information we need. During the deepCopy
                                *       we also copy the symbol nodes. This way we do minimal changes to the whole code for templated
                                *       functions.
                                *       TODO; Might need changes if we see that code generation does not have all the necessary information.
                                */
                                funcSymbol->isTypeComplete = true;

                                Token * t = calloc(1,sizeof(Token));
                                t->sourceInfo = funcSymbol->sourceInfo;

				// FIX: NoisySymbolType to IrNodeType without a cast
                                t->type = kNoisySymbolTypeNamegenDefinition;
                                asprintf(&t->identifier,"%s_%d",funcSymbol->identifier,loadCount);

                                Symbol * newFunctionSym = commonSymbolTableAddOrLookupSymbolForToken(N,N->noisyIrTopScope->firstChild,t);
                                newFunctionSym->functionDefinition = deepCopyIrNode(N,funcSymbol->functionDefinition,loadCount);
                                noisyFunctionDefnTypeCheck(N,newFunctionSym->functionDefinition,N->noisyIrTopScope->firstChild);
                                newFunctionSym->parameterNum = funcSymbol->parameterNum;
                                newFunctionSym->isTypeComplete = funcSymbol->isTypeComplete;
                                IrNode * newTypeTree = calloc(1,sizeof(IrNode));
                                newTypeTree->irLeftChild = RL(newFunctionSym->functionDefinition);
                                newTypeTree->irRightChild = RRL(newFunctionSym->functionDefinition);
                                newFunctionSym->typeTree = newTypeTree;

                                returnType.functionDefinition = newFunctionSym;
                                funcSymbol->isTypeComplete = false;
                        }
                        else
                        {
                                /*
                                *       If we dont have templated code we just pass the funcSymbol pointer.
                                */
                                returnType.functionDefinition = funcSymbol;
                        }
                }
                else
                {
                        char *	details;

                        asprintf(&details, "We do not support loadExpr with module names only.\n");
                        noisySemanticError(N,moduleNameIrNode,details);
                        noisySemanticErrorRecovery(N);
                }

                /*
                *       If we actually changed a typeTre for type parameters of module we need to revert them back to their initial
                *       stage.
                */
                if (tupleTypeNode != NULL)
                {
                        for (IrNode * typeParameterListNodeIter = RL(moduleSymbol->typeTree->irParent->irParent); typeParameterListNodeIter != NULL; typeParameterListNodeIter = R(typeParameterListNodeIter))
                        {
                                /*
                                *       We need to revert what we did before for the next load expressions.
                                *       Each load does not modify the module permanently but it specialises it and loads it.
                                */
                                L(typeParameterListNodeIter)->symbol->typeTree = NULL;
                        }
                }

                returnType.basicType = noisyBasicTypeNamegenType;
                loadCount++;
        }
        noisyExpressionNode->noisyType = returnType;
        return returnType;
}

/*
*       Checks if the function is typeComplete and also counts its parameters as preparation
*       for the code generation.
*/
void
noisyDeclareFunctionTypeCheck(State * N, const char * functionName,IrNode * inputSignature, IrNode * outputSignature,Scope * currentScope)
{
        Symbol * functionSymbol = commonSymbolTableSymbolForIdentifier(N, currentScope, functionName);
        /*
        *       By default functions are not channels. We check if they use channel operators and then we decide they are channels.
        */
        functionSymbol->isChannel = false;
        functionSymbol->isTypeComplete = false;

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
        if (parameterCount == 0)
        {
                functionSymbol->isTypeComplete = true;
        }

        if (L(outputSignature)->type != kNoisyIrNodeType_Tnil)
        {
                functionSymbol->isTypeComplete = functionSymbol->isTypeComplete && isTypeExprComplete(N,RL(outputSignature));
                if (isMeasurementType(N,RL(outputSignature)) && parameterCount == 0)
                {
                        functionSymbol->isSensorChannel = true;
                }
        }
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


bool
noisyMatchTypeExpr(State * N,IrNode * typeExpr1, IrNode * typeExpr2)
{
        NoisyType typ1, typ2;
        typ1 = getNoisyTypeFromTypeExpr(N,typeExpr1);
        typ2 = getNoisyTypeFromTypeExpr(N,typeExpr2);
        bool res = noisyTypeEquals(typ1,typ2);
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

                        L(iter)->symbol->noisyType = getNoisyTypeFromTypeExpr(N,RL(iter));
                        L(declIter)->symbol->noisyType = getNoisyTypeFromTypeExpr(N,RL(iter));
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
*       TypeChecks an assignment statement.
*/
void
noisyAssignmentStatementTypeCheck(State * N, IrNode * noisyAssignmentStatementNode, Scope * currentScope)
{
        /*
        *       If type is xseq it means that noisyAssignmentNode is not a declaration but an actual assignment.
        *       If it is declaration then R(noisyAssignmentStatementNode)->type ==  kNoisyIrNodeType_PtypeEpxr.
        */
        if (R(noisyAssignmentStatementNode)->type == kNoisyIrNodeType_Xseq)
        {
                if (RLL(noisyAssignmentStatementNode)->type != kNoisyIrNodeType_TcolonAssign)
                {
                        NoisyType lValueType, rValueType, prevLVal,expectedRvalType;
                        rValueType = getNoisyTypeFromExpression(N,RRL(noisyAssignmentStatementNode),currentScope);
                        bool firstTime = true;
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
                                        prevLVal = lValueType;
                                        Symbol * lvalSymbol = LLL(iter)->symbol;
                                        if (lvalSymbol->typeTree != NULL)
                                        {
                                                lValueType = getNoisyTypeFromTypeExpr(N,lvalSymbol->typeTree);
                                        }
                                        else
                                        {
                                                lValueType = lvalSymbol->noisyType;
                                        }
                                        

                                        if (lValueType.basicType == noisyBasicTypeArrayType)
                                        {
                                                // arrayLvalType = lValueType;
                                                int dims = 0;
                                                for (IrNode * iter2 = LLR(iter); iter2 != NULL; iter2 = R(iter2))
                                                {
                                                        if (! noisyIsOfType(getNoisyTypeFromExpression(N,LR(iter2),currentScope), noisyBasicTypeIntegerConstType))
                                                        {
                                                                /*
                                                                *       Indexes are not integers error.
                                                                */
                                                                char *	details;

                                                                asprintf(&details, "Indexing \"%s\" array with a non-integer expression\n",LLL(iter)->symbol->identifier);
                                                                noisySemanticError(N,iter,details);
                                                                noisySemanticErrorRecovery(N);
                                                                // factorType.basicType = noisyBasicTypeErrorType;
                                                        }
                                                        dims++;
                                                }

                                                lValueType.dimensions -= dims;
                                                /*
                                                *       If there are no type errors on array indexing we the arrayType of the array.
                                                *       e.g. when we index an array of int32 the factor we return has type int32.
                                                */
                                                if (lValueType.dimensions == 0)
                                                {
                                                        lValueType.basicType = lValueType.arrayType;
                                                }
                                                else if (lValueType.dimensions < 0 )
                                                {
                                                        /*
                                                        *       Indexing dimension error.
                                                        */
                                                        char *	details;

                                                        asprintf(&details, "Dimensions of array \"%s\" dont match\n",LLL(iter)->symbol->identifier);
                                                        noisySemanticError(N,LL(iter),details);
                                                        noisySemanticErrorRecovery(N);
                                                }
                                        }
                                        if (!firstTime && !noisyTypeEquals(lValueType,prevLVal))
                                        {
                                                char *	details;

                                                asprintf(&details, "Cannot have different lvalue types on assignment\n");
                                                noisySemanticError(N,LLL(iter),details);
                                                noisySemanticErrorRecovery(N);
                                        }
                                        firstTime = false;
                                        if (noisyTypeEquals(lValueType,rValueType))
                                        {
                                                if (RLL(noisyAssignmentStatementNode)->type != kNoisyIrNodeType_Tassign
                                                && RLL(noisyAssignmentStatementNode)->type != kNoisyIrNodeType_TchannelOperatorAssign)
                                                {
                                                        if (RLL(noisyAssignmentStatementNode)->type == kNoisyIrNodeType_TplusAssign
                                                        || RLL(noisyAssignmentStatementNode)->type == kNoisyIrNodeType_TminusAssign
                                                        || RLL(noisyAssignmentStatementNode)->type == kNoisyIrNodeType_TasteriskAssign
                                                        || RLL(noisyAssignmentStatementNode)->type == kNoisyIrNodeType_TdivideAssign)
                                                        {
                                                                /*
                                                                *       For "/=", "*=", "-=", "+="
                                                                *       operators, values need to have arithmetic type.
                                                                */
                                                                if (!noisyIsOfType(lValueType,noisyBasicTypeArithType))
                                                                {
                                                                        char *	details;

                                                                        asprintf(&details, "Type operator and operand mismatch on assignment\n");
                                                                        noisySemanticError(N,LL(iter),details);
                                                                        noisySemanticErrorRecovery(N);
                                                                }
                                                        }
                                                        else
                                                        {
                                                                /*
                                                                *       For "^=", "|=", "&=", "%=", ">>=", "<<=" 
                                                                *       operators, values need to have integer type.
                                                                */
                                                                if (!noisyIsOfType(lValueType,noisyBasicTypeIntegerConstType))
                                                                {
                                                                        char *	details;

                                                                        asprintf(&details, "Type operator and operand mismatch on assignment\n");
                                                                        noisySemanticError(N,LL(iter),details);
                                                                        noisySemanticErrorRecovery(N);
                                                                }
                                                        }
                                                }
                                                else if (RLL(noisyAssignmentStatementNode)->type == kNoisyIrNodeType_TchannelOperatorAssign)
                                                {
                                                        /*
                                                        *       If the lval is kNoisySymbolTypeReturnParameter then it means we write
                                                        *       to the output channel of a function.
                                                        */
                                                        if (lvalSymbol->symbolType == kNoisySymbolTypeReturnParameter)
                                                        {
                                                                N->currentFunction->isChannel = true;
                                                        }
                                                }
                                        }
                                        else if (RLL(noisyAssignmentStatementNode)->type == kNoisyIrNodeType_TchannelOperatorAssign)
                                        {
                                                if (lValueType.basicType == noisyBasicTypeNamegenType)
                                                {
                                                        /*
                                                        *       Else if lval has noisyBasicTypeNamegenType it means we write to the input channel
                                                        *       of a channel function.
                                                        */
                                                        if (lValueType.functionDefinition->parameterNum != 1)
                                                        {
                                                                char *	details;

                                                                asprintf(&details, "Cannot write to channel with multiple or zero inputs!\n");
                                                                noisySemanticError(N,LL(iter),details);
                                                                noisySemanticErrorRecovery(N);
                                                        }

                                                        lvalSymbol->noisyType.functionDefinition->isChannel = true;
                                                        expectedRvalType = getNoisyTypeFromTypeExpr(N,LRL(lValueType.functionDefinition->typeTree));
                                                        if (!noisyTypeEquals(expectedRvalType,rValueType))
                                                        {
                                                                char *	details;

                                                                asprintf(&details, "Channel and value type mismatch!\n");
                                                                noisySemanticError(N,LL(iter),details);
                                                                noisySemanticErrorRecovery(N);
                                                        }
                                                        expectedRvalType = noisyGetMoreSpecificType(expectedRvalType,rValueType);
                                                }
                                                else
                                                {
                                                        char *	details;

                                                        asprintf(&details, "Cannot write to a non channel!\n");
                                                        noisySemanticError(N,LL(iter),details);
                                                        noisySemanticErrorRecovery(N);
                                                }
                                        }
                                        else if (rValueType.basicType == noisyBasicTypeNilType)
                                        {
                                                /*
                                                *       We can assign nil to any type.
                                                */
                                                ;
                                        }
                                        else
                                        {
                                                char *	details;

                                                asprintf(&details, "Type mismatch on assignment\n");
                                                noisySemanticError(N,LL(iter),details);
                                                noisySemanticErrorRecovery(N);
                                        }
                                        /*
                                        *       We need to save the symbol type only on declarations of variables.
                                        *       For assignments we only save the lvalType on the rval expression node
                                        *       so we can use it as  a way to find integer and float constant types during code gen.
                                        */

                                        /*
                                        *       We assign to the expression the noisyType so we can find the appropriate type for constants.
                                        */
                                        if (lValueType.basicType != noisyBasicTypeNamegenType)
                                        {
                                                RRL(noisyAssignmentStatementNode)->noisyType = lValueType;
                                        }
                                        else
                                        {
                                                /*
                                                *       For channel assignment lval and rval do not have the same noisyType.
                                                *       Lval has noisyBasicTypeNamegenType and rval should have the channel's input type.
                                                */
                                                RRL(noisyAssignmentStatementNode)->noisyType = expectedRvalType;
                                        }
                                }
                        }
                }
                else
                {
                        NoisyType rValueType = getNoisyTypeFromExpression(N,RRL(noisyAssignmentStatementNode),currentScope);
                        for (IrNode * iter = L(noisyAssignmentStatementNode); iter != NULL; iter = R(iter))
                        {
                                if (LL(iter)->type == kNoisyIrNodeType_Tnil)
                                {
                                        char *	details;

                                        asprintf(&details, "We cannot assign type to nil\n");
                                        noisySemanticError(N,LL(iter),details);
                                        noisySemanticErrorRecovery(N);
                                }
                                else if (LL(iter)->type == kNoisyIrNodeType_PqualifiedIdentifier)
                                {
                                        LLL(iter)->symbol->noisyType = rValueType;
                                }
                        }
                }
        }
        else
        {
                NoisyType typeExprType = getNoisyTypeFromTypeExpr(N,R(noisyAssignmentStatementNode));
                for (IrNode * iter = L(noisyAssignmentStatementNode); iter != NULL; iter = R(iter))
                {
                        if (LL(iter)->type == kNoisyIrNodeType_Tnil)
                        {
                                char *	details;

                                asprintf(&details, "We cannot assign type to nil\n");
                                noisySemanticError(N,LL(iter),details);
                                noisySemanticErrorRecovery(N);
                        }
                        else if (LL(iter)->type == kNoisyIrNodeType_PqualifiedIdentifier)
                        {
                                LLL(iter)->symbol->noisyType = typeExprType;
                        }
                }
        }
}

void
noisyGuardedStatementTypeCheck(State * N, IrNode * noisyGuardedStatementNode, Scope * currentScope)
{
        Scope * nextScope = currentScope;
        for (IrNode * iter = noisyGuardedStatementNode; iter != NULL; iter = RR(iter))
        {
                /*
                *       TODO; ChanEvent.
                */
                NoisyType exprType = getNoisyTypeFromExpression(N,L(iter),currentScope);

                if (exprType.basicType != noisyBasicTypeBool)
                {
                        char *	details;

                        asprintf(&details, "Not boolean expression on a match statement\n");
                        noisySemanticError(N,L(iter),details);
                        noisySemanticErrorRecovery(N);
                }
                /*
                *       For scoped statementLists we move the scopes and we parse the statementList.
                */
                noisyStatementListTypeCheck(N,RLL(iter),nextScope);
                nextScope = nextScope->next;
        }
}


void
noisyMatchStatementTypeCheck(State * N,IrNode * noisyMatchStatementNode,Scope * currentScope)
{
        noisyGuardedStatementTypeCheck(N,R(noisyMatchStatementNode),currentScope->firstChild);
}

void
noisyIterateStatementTypeCheck(State * N, IrNode * noisyIterateStatementNode, Scope * currentScope)
{
        noisyGuardedStatementTypeCheck(N,R(noisyIterateStatementNode),currentScope->firstChild->firstChild);
}

void
noisyOrderingHeadTypeCheck(State * N,IrNode * orderingHeadNode,Scope * currentScope)
{
        noisyAssignmentStatementTypeCheck(N,L(orderingHeadNode),currentScope);
        NoisyType exprType = getNoisyTypeFromExpression(N,RL(orderingHeadNode),currentScope);
        if (exprType.basicType != noisyBasicTypeBool)
        {
                char *	details;

                asprintf(&details, "Not boolean expression on the termination condition of a sequence statement\n");
                noisySemanticError(N,RL(orderingHeadNode),details);
                noisySemanticErrorRecovery(N);
        }
        noisyAssignmentStatementTypeCheck(N,RRL(orderingHeadNode),currentScope);
}

void
noisySequenceStatementTypeCheck(State * N,IrNode * noisySequenceStatementNode,Scope * currentScope)
{
        noisyOrderingHeadTypeCheck(N,L(noisySequenceStatementNode),currentScope->parent);
        noisyStatementListTypeCheck(N,RL(noisySequenceStatementNode),currentScope);
}

void
noisyReturnStatementTypeCheck(State * N,IrNode * noisyReturnStatementNode,Scope * currentScope)
{
        /*
        *       This should work for multiple return variables but currently we support only one returnType.
        */
        for (IrNode * iter = L(noisyReturnStatementNode); iter != NULL; iter = RR(iter))
        {
                Symbol * argSymbol = commonSymbolTableSymbolForIdentifier(N,currentScope,L(iter)->tokenString);
                if (argSymbol == NULL || argSymbol->symbolType != kNoisySymbolTypeReturnParameter)
                {
                        char *	details;

                        asprintf(&details, "Unknown return variable\n");
                        noisySemanticError(N,L(iter),details);
                        noisySemanticErrorRecovery(N);
                }
                /*
                *       Since arguments have type in their signature we assume that the typeTree exists.
                */
                NoisyType argType = getNoisyTypeFromTypeExpr(N,argSymbol->typeTree);
                NoisyType exprType = getNoisyTypeFromExpression(N,RL(iter),currentScope);
                if (!noisyTypeEquals(argType,exprType))
                {
                        char *	details;

                        asprintf(&details, "Return expression and variable mismatch\n");
                        noisySemanticError(N,RL(iter),details);
                        noisySemanticErrorRecovery(N);
                }
                RL(iter)->noisyType = noisyGetMoreSpecificType(argType,exprType);
        }
}


/*
*       TODO; Check scopings.
*/
void
noisyStatementTypeCheck(State * N, IrNode * noisyStatementNode, Scope * currentScope)
{
        switch (L(noisyStatementNode)->type)
        {
        case kNoisyIrNodeType_PassignmentStatement:
                noisyAssignmentStatementTypeCheck(N,L(noisyStatementNode), currentScope);
                break;
        case kNoisyIrNodeType_PmatchStatement:
                noisyMatchStatementTypeCheck(N,L(noisyStatementNode),currentScope);
                break;
        case kNoisyIrNodeType_PiterateStatement:
                noisyIterateStatementTypeCheck(N,L(noisyStatementNode), currentScope);
                break;
        case kNoisyIrNodeType_PsequenceStatement:
                noisySequenceStatementTypeCheck(N,L(noisyStatementNode),currentScope);
                break;
        case kNoisyIrNodeType_PscopedStatementList:
                noisyStatementListTypeCheck(N,LL(noisyStatementNode),currentScope);
                break;
        // case kNoisyIrNodeType_PoperatorToleranceDecl:
        //         noisyOperatorToleranceDeclTypeCheck(N,S,L(noisyStatementNode));
        //         break;
        case kNoisyIrNodeType_PreturnStatement:
                noisyReturnStatementTypeCheck(N,L(noisyStatementNode),currentScope);
                break;
        default:
                noisySemanticError(N,L(noisyStatementNode),"This statement is not supported!\n");
                noisySemanticErrorRecovery(N);
                break;
        }
}

void
noisyStatementListTypeCheck(State * N, IrNode * statementListNode, Scope * currentScope)
{
        Scope * nextScope = currentScope;
        Scope * sequenceScope = currentScope->firstChild;
        for (IrNode * iter = statementListNode; iter != NULL; iter=R(iter))
        {
                if (L(iter) != NULL && LL(iter) != NULL)
                {
                        if (LL(iter)->type == kNoisyIrNodeType_PscopedStatementList)
                        {
                                noisyStatementTypeCheck(N,L(iter),nextScope);
                                nextScope = currentScope->next;
                        }
                        else if (LL(iter)->type == kNoisyIrNodeType_PsequenceStatement
                                || LL(iter)->type == kNoisyIrNodeType_PmatchStatement)
                        {
                                noisyStatementTypeCheck(N,L(iter),sequenceScope);
                                sequenceScope = sequenceScope->next;
                        }
                        else
                        {
                                noisyStatementTypeCheck(N,L(iter),currentScope);
                        }
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

        if (functionSymbol->isTypeComplete)
        {
                N->currentFunction = functionSymbol;
                noisyStatementListTypeCheck(N,RR(noisyFunctionDefnNode)->irRightChild->irLeftChild,commonSymbolTableGetScopeWithName(N,currentScope,functionSymbol->identifier)->firstChild);
        }
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
