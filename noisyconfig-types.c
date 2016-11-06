/*
	Authored 2015. Phillip Stanley-Marbell.

	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions
	are met:

	*	Redistributions of source code must retain the above
		copyright notice, this list of conditions and the following
		disclaimer.

	*	Redistributions in binary form must reproduce the above
		copyright notice, this list of conditions and the following
		disclaimer in the documentation and/or other materials
		provided with the distribution.

	*	Neither the name of the author nor the names of its
		contributors may be used to endorse or promote products
		derived from this software without specific prior written
		permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisyconfig-errors.h"
#include "version.h"
#include "noisyconfig.h"
#include "noisyconfig-irPass-helpers.h"
#include "noisyconfig-symbolTable.h"
#include "noisyconfig-types.h"

extern const char	gNoisyConfigTypeNodeSignatures[];
extern const char	gNoisyConfigAstNodeStrings[];
extern const char * gReservedTokenDescriptions[];

/*
 * Currently parser does not set NoisyConfigSymbol.
 */
void 
noisyConfigIdentifierTypeInferencePass(NoisyConfigState * N, NoisyConfigIrNode * irNode) 
{
    if (irNode == NULL) 
    {
        return;
    }

    checkAllNodeTypes(N, irNode);
    if (L(irNode) == irNode || R(irNode) == irNode)
    {
    	NoisyConfigFatal(N, "Immediate cycle in Ir, seen noisyIrPassProtobufAstSerializeWalk()!!\n");
    }

    // set parent pointers of the nodes
    if (L(irNode) != NULL) 
        L(irNode)->irParent = irNode;
    if (R(irNode) != NULL) 
        R(irNode)->irParent = irNode;
    NoisyConfigIrPassTypeChecker(N, L(irNode));
    NoisyConfigIrPassTypeChecker(N, R(irNode));
}

void 
noisyConfigIrPassTypeChecker(NoisyConfigState * N, NoisyConfigIrNode * irNode) 
{
    if (irNode == NULL) 
    {
        return;
    }

    checkAllNodeTypes(N, irNode);
    if (L(irNode) == irNode || R(irNode) == irNode)
    {
    	NoisyConfigFatal(N, "Immediate cycle in Ir, seen NoisyConfigIrPassProtobufAstSerializeWalk()!!\n");
    }

    // set parent pointers of the nodes
    if (L(irNode) != NULL) 
        L(irNode)->irParent = irNode;
    if (R(irNode) != NULL) 
        R(irNode)->irParent = irNode;
    NoisyConfigIrPassTypeChecker(N, L(irNode));
    NoisyConfigIrPassTypeChecker(N, R(irNode));
}

void 
checkAllNodeTypes(NoisyConfigState * N, NoisyConfigIrNode * node)
{
    switch(node->type) 
    {
        case kNoisyConfigIrNodeType_Tidentifier:
            if (!isValidIdentifier(N, node))
            {
                NoisyConfigFatal(N, "An identifier failed type checking!! \n");
            }
        case kNoisyConfigIrNodeType_PlowPrecedenceBinaryOp:
            break;
        case kNoisyConfigIrNodeType_Tminus:
        case kNoisyConfigIrNodeType_Tplus:
            checkPlus(N, node);
            break;
      
        case kNoisyConfigIrNodeType_PassignOp:
            break;
        default:
            break;
    }
}

/*
 * input: kNoisyConfigIrNodeType_Pterm nodes when binop detected (and maybe more)
 * do a post order walk, and propagate types up from leaf nodes
 *
 * output: basic type of the kNoisyConfigIrNodeType_Pterm
 */
NoisyConfigIrNodeType 
postOrderWalkBinOp(NoisyConfigState * N, NoisyConfigIrNode * node) 
{
    
    NoisyConfigIrNodeType left = kNoisyConfigIrNodeType_Tnone; // a flag enum value for None
    NoisyConfigIrNodeType right = kNoisyConfigIrNodeType_Tnone;

    // base case: leaf node that only has nil nodes as children
    if (L(node) == NULL && R(node) == NULL)
    {
        if (node->type == kNoisyConfigIrNodeType_Tidentifier) {
            NoisyConfigSymbol * tmp = NoisyConfigSymbolTableSymbolForIdentifier(N, node->currentScope, node->tokenString);
            return tmp->symbolType;
        }
        return node->type;
    }

    // recurse
    if (L(node) != NULL)
        left = postOrderWalkBinOp(N, L(node));
    if (R(node) != NULL)
        right = postOrderWalkBinOp(N, R(node));

    // if we need to look up the node in symbol table to return the correct type
    // e.g.) kNoisyConfigIrNodeType_Tidentifier
    if (left == kNoisyConfigIrNodeType_Tidentifier)
        left = NoisyConfigSymbolTableSymbolForIdentifier(N, node->currentScope, L(node)->tokenString)->symbolType; 
    if (right == kNoisyConfigIrNodeType_Tidentifier)
        right = NoisyConfigSymbolTableSymbolForIdentifier(N, node->currentScope, R(node)->tokenString)->symbolType; 

    // sanity checks
    if (left == kNoisyConfigIrNodeType_Tidentifier && right == kNoisyConfigIrNodeType_Tidentifier) 
        NoisyConfigFatal(N, "postOrder failed: both children are identifiers\n");
    if (left == kNoisyConfigIrNodeType_Tnone && right == kNoisyConfigIrNodeType_Tnone)
        NoisyConfigFatal(N, "postOrder failed: left and right are -1\n");
    
    // evaluate the result
    if (node->type == kNoisyConfigIrNodeType_Tidentifier) // if we need to evaluate the current node
    {
        NoisyConfigIrNodeType current = NoisyConfigSymbolTableSymbolForIdentifier(N, node->currentScope, node->tokenString)->symbolType; 
        if (isTokenToIgnoreBinOp(left) || left == kNoisyConfigIrNodeType_Tnone) 
            return areSameTypes(current, right); // identifier node doesn't need to propagate up kNoisyConfigIrNodeType_Tint
        if (isTokenToIgnoreBinOp(right) || right == kNoisyConfigIrNodeType_Tnone)
            return areSameTypes(current, left);
        if (!areSameTypes(left, current) || !areSameTypes(current, right)) // left, current, right must all be same
            NoisyConfigFatal(N, "postOrder failed\n");
        return left; // same as right
    }
    else 
    {
        if (isTokenToIgnoreBinOp(left) || left == kNoisyConfigIrNodeType_Tnone) 
            return right; // identifier node doesn't need to propagate up kNoisyConfigIrNodeType_Tint
        if (isTokenToIgnoreBinOp(right) || right == kNoisyConfigIrNodeType_Tnone)
            return left;
        if (!areSameTypes(left, right))
            NoisyConfigFatal(N, "postOrder failed\n");
        return left; // same as right
    }
}

void 
checkPlus(NoisyConfigState * N, NoisyConfigIrNode * node) 
{ 
    NoisyConfigIrNode * term = lookupNodeInParents(node, kNoisyConfigIrNodeType_Pterm);
    if (term == NULL)
        NoisyConfigFatal(N, "there is no kNoisyConfigIrNodeType_Pterm!\n");
    NoisyConfigIrNodeType type = postOrderWalkBinOp(N, term);

    if (type != kNoisyConfigIrNodeType_Tint && type != kNoisyConfigIrNodeType_TintConst)
    {
        NoisyConfigFatal(N, "checkplus has slightly failed\n");
    }
}

/*
 * Perform type inference at declaration 
 * Input: PXXX_Declaration nodes
 */
void
noisyConfigInferIdentifierTypeInDeclaration(NoisyConfigState * N, NoisyConfigScope * scope, NoisyConfigIrNode * node)
{
   NoisyConfigIrNodeType type = node->type;
   if (type == kNoisyConfigIrNodeType_PprogtypeDeclaration ||
       type == kNoisyConfigIrNodeType_PprogtypeTypenameDeclaration ||
       type == kNoisyConfigIrNodeType_PconstantDeclaration ||
       type == kNoisyConfigIrNodeType_PnamegenDeclaration ||
       type == kNoisyConfigIrNodeType_PtypeDeclaration ||
       type == kNoisyConfigIrNodeType_PadtTypeDeclaration
      )
   {
       
   }
   else 
   {
      NoisyConfigFatal(N, "This function should not have been called with a non declaration node");
   }
   // TODO
   // post order walk on some depth level down and then insert to symtab an identifier
}


/*
 * Perform type inference at declaration for statement nodes only
 * Input: declaration tokens like defineAs Token nodes
 * For each declaration token such as defineAs, find the statement ancestor
 * the statement node's left child identifier will be inserted with the correct node type
 */
void
noisyConfigInferIdentifierTypeInStatement(NoisyConfigState * N, NoisyConfigScope * scope, NoisyConfigIrNode * node)
{
    NoisyConfigIrNode * statement = lookupNodeInParents(node, kNoisyConfigIrNodeType_Pstatement);
    if (statement == NULL)
    {
        NoisyConfigFatal(N, "There is no statement in the parent!!\n"); 
    }
    NoisyConfigIrNode * identifierNode = statement->irLeftChild;
    if (identifierNode == NULL)
    {
        NoisyConfigFatal(N, "Left child of statement isn't identifier!!\n");
    }
    // NoisyConfigSymbol * symbol = NoisyConfigSymbolTableAddOrLookupSymbolForToken(N, scope, identifierNode->token);
    // symbol->type = postOrderWalkBinOp(N, node);
}

/*
 * this function accounts for the fact that sometimes
 * different types are indeed the same thing
 * e.g.) kNoisyConfigIrNodeType_Tint and kNoisyConfigIrNodeType_TintConst
 */
bool
areSameTypes(NoisyConfigIrNodeType type1, NoisyConfigIrNodeType type2)
{
    if ((type1 == kNoisyConfigIrNodeType_Tint && type2 == kNoisyConfigIrNodeType_TintConst) || 
        (type2 == kNoisyConfigIrNodeType_Tint && type1 == kNoisyConfigIrNodeType_TintConst))
        return true;
    return type1 == type2;
}

bool
isTokenToIgnoreBinOp(NoisyConfigIrNodeType targetType) 
{
    return (targetType == kNoisyConfigIrNodeType_Tplus ||
            targetType == kNoisyConfigIrNodeType_Tminus ||
            targetType == kNoisyConfigIrNodeType_Tgets); // TODO: fill in more of these
}


NoisyConfigIrNode *
lookupNodeInParents(NoisyConfigIrNode * node, NoisyConfigIrNodeType targetType)
{
    NoisyConfigIrNode * current = node;
    while (current != NULL && current->type != targetType)
    {
        current = current->irParent;
    }
    return current;
}

/* in a subtree, returns the first node of targetType it finds.
 */
NoisyConfigIrNode *
lookupNodeInSubtree(NoisyConfigIrNode * node, NoisyConfigIrNodeType targetType)
{
    if (node == NULL)
        return NULL;
    if (node->type == targetType) 
        return node;

    NoisyConfigIrNode * leftResult = lookupNodeInSubtree(node->irLeftChild, targetType);
    if (leftResult != NULL)
        return leftResult;
    NoisyConfigIrNode * rightResult = lookupNodeInSubtree(node->irRightChild, targetType);
    if (rightResult != NULL)
        return rightResult;
    return NULL; // if children are nil
}



// TODO after plus, generalize to some binary op
void
checkBinOps(NoisyConfigState * N, NoisyConfigIrNode * node)
{
    flexprint(N->Fe, N->Fm, N->Fpinfo, "inside binops %s\n", node->tokenString);
}

bool 
isNumber(char c)
{
    return c >= '0' && c <= '9';
}

bool 
isValidIdChar(char * string) 
{
    // check string is not "true" or "false"
    if (!(strcmp(string, "true") || (strcmp(string, "false")))) 
    {
        return false;
    }

    // check string is not one of reserved tokens
    for (int i = 0; i < kNoisyConfigIrNodeTypeMax; i++)
    {
        if ((gReservedTokenDescriptions[i] != NULL) && !strcmp(gReservedTokenDescriptions[i], string))
        {
            return false;
        }
    } 
    return true;
}

bool 
isValidIdentifier(NoisyConfigState * N, NoisyConfigIrNode * node) 
{
    if (!isNumber(node->tokenString[0]) && isValidIdChar(node->tokenString)) 
    {
        return true;
    }
    else 
    {
        flexprint(N->Fe, N->Fm, N->Fperr, "Not a valid identifier %s!", node->tokenString);
        return false;
    }
}


NoisyConfigIrNode *
noisyConfigTypeValidateIrSubtree(NoisyConfigState *  N, NoisyConfigIrNode *  subtree)
{
	return NULL;
}


bool
noisyConfigTypeEqualsSubtreeTypes(NoisyConfigState *  N, NoisyConfigIrNode *  subtreeA, NoisyConfigIrNode *  subtreeB)
{
	return false;
}


char * 
noisyConfigTypeMakeTypeSignature(NoisyConfigState *  N, NoisyConfigIrNode *  subtree)
{
	char *	signature;
	char *	leftSignature;
	char *	rightSignature;


	/*
	 *	Type string is a list of chars representing nodes seen on a
	 *	post-order walk of tree rooted at n.  The possible node types
	 *	that will be seen and their signature chars are defined in m.m.
	 */
	if (subtree == NULL)
	{
		return strdup("");
	}

	char s = gNoisyConfigTypeNodeSignatures[subtree->type];
	if (s == 0)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s, node type is %d (%s)\n", EcannotFindTypeSignatureForNodeType, subtree->type, gNoisyConfigAstNodeStrings[subtree->type]);
		NoisyConfigFatal(N, Esanity);
	}

	leftSignature	= NoisyConfigTypeMakeTypeSignature(N, subtree->irLeftChild);
	rightSignature	= NoisyConfigTypeMakeTypeSignature(N, subtree->irRightChild);

	signature = calloc(strlen(leftSignature) + strlen(rightSignature) + 2, sizeof(char));
	if (signature == NULL)
	{
		NoisyConfigFatal(N, Emalloc);
	}

	strcpy(signature, leftSignature);
	strcpy(&signature[strlen(leftSignature)], rightSignature);
	signature[strlen(leftSignature) + strlen(rightSignature)] = s;
	signature[strlen(leftSignature) + strlen(rightSignature) + 1] = '\0';

	free(leftSignature);
	free(rightSignature);


	return signature;
}
