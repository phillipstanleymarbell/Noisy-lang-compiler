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

#include <stdbool.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisy-errors.h"
#include "version.h"
#include "noisy-timeStamps.h"
#include "noisy.h"
#include "noisy-irPass-helpers.h"
#include "noisy-symbolTable.h"
#include "noisy-types.h"

extern const char	gNoisyTypeNodeSignatures[];
extern const char	gNoisyAstNodeStrings[];
extern const char * gReservedTokenDescriptions[];

void 
noisyIrPassTypeChecker(NoisyState * N, NoisyIrNode * irNode) 
{
  NoisyTimeStampTraceMacro(kNoisyTimeStampKeyIrPassTypeChecker);
  
  if (irNode == NULL) 
  {
    return;
  }

  checkAllNodeTypes(N, irNode);
  if (L(irNode) == irNode || R(irNode) == irNode)
  {
  	noisyFatal(N, "Immediate cycle in Ir, seen noisyIrPassProtobufAstSerializeWalk()!!\n");
  }

  // set parent pointers of the nodes
  if (L(irNode) != NULL) 
      L(irNode)->irParent = irNode;
  if (R(irNode) != NULL) 
      R(irNode)->irParent = irNode;
  noisyIrPassTypeChecker(N, L(irNode));
  noisyIrPassTypeChecker(N, R(irNode));
}

void 
checkAllNodeTypes(NoisyState * N, NoisyIrNode * node)
{
    switch(node->type) 
    {
        case kNoisyIrNodeType_Tidentifier:
            if (!isValidIdentifier(N, node))
            {
                noisyFatal(N, "An identifier failed type checking!! \n");
            }
            break;
        case kNoisyIrNodeType_PlowPrecedenceBinaryOp:
            break;
        case kNoisyIrNodeType_Tminus:
        case kNoisyIrNodeType_T
        case kNoisyIrNodeType_Tplus:
            checkPlus(N, node);
            break;
      
        case kNoisyIrNodeType_PassignOp:
            break;
        default:
            break;

    }
}

/*
 * input: kNoisyIrNodeType_Pterm nodes when binop detected (and maybe more)
 * do a post order walk, and propagate types up from leaf nodes
 *
 * output: basic type of the kNoisyIrNodeType_Pterm
 */
NoisyIrNodeType postOrderWalkBinOp(NoisyState * N, NoisyIrNode * node) 
{
    
    NoisyIrNodeType left = kNoisyIrNodeType_Tnone; // a flag enum value for None
    NoisyIrNodeType right = kNoisyIrNodeType_Tnone;

    // base case: leaf node that only has nil nodes as children
    if (L(node) == NULL && R(node) == NULL)
    {
        if (node->type == kNoisyIrNodeType_Tidentifier) {
            NoisySymbol * tmp = noisySymbolTableSymbolForIdentifier(N, node->currentScope, node->tokenString);
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
    // e.g.) kNoisyIrNodeType_Tidentifier
    if (left == kNoisyIrNodeType_Tidentifier)
        left = noisySymbolTableSymbolForIdentifier(N, node->currentScope, L(node)->tokenString)->symbolType; 
    if (right == kNoisyIrNodeType_Tidentifier)
        right = noisySymbolTableSymbolForIdentifier(N, node->currentScope, R(node)->tokenString)->symbolType; 

    // sanity checks
    if (left == kNoisyIrNodeType_Tidentifier && right == kNoisyIrNodeType_Tidentifier) 
        noisyFatal(N, "postOrder failed: both children are identifiers\n");
    if (left == kNoisyIrNodeType_Tnone && right == kNoisyIrNodeType_Tnone)
        noisyFatal(N, "postOrder failed: left and right are -1\n");
    
    // evaluate the result
    if (node->type == kNoisyIrNodeType_Tidentifier) // if we need to evaluate the current node
    {
        NoisyIrNodeType current = noisySymbolTableSymbolForIdentifier(N, node->currentScope, node->tokenString)->symbolType; 
        if (isTokenToIgnoreBinOp(left) || left == kNoisyIrNodeType_Tnone) 
            return areSameTypes(current, right); // identifier node doesn't need to propagate up kNoisyIrNodeType_Tint
        if (isTokenToIgnoreBinOp(right) || right == kNoisyIrNodeType_Tnone)
            return areSameTypes(current, left);
        if (!areSameTypes(left, current) || !areSameTypes(current, right)) // left, current, right must all be same
            noisyFatal(N, "postOrder failed\n");
        return left; // same as right
    }
    else 
    {
        if (isTokenToIgnoreBinOp(left) || left == kNoisyIrNodeType_Tnone) 
            return right; // identifier node doesn't need to propagate up kNoisyIrNodeType_Tint
        if (isTokenToIgnoreBinOp(right) || right == kNoisyIrNodeType_Tnone)
            return left;
        if (!areSameTypes(left, right))
            noisyFatal(N, "postOrder failed\n");
        return left; // same as right
    }
}

void 
checkPlus(NoisyState * N, NoisyIrNode * node) 
{ 
    NoisyIrNode * term = lookupNodeInParents(node, kNoisyIrNodeType_Pterm);
    if (term == NULL)
        noisyFatal(N, "there is no kNoisyIrNodeType_Pterm!\n");
    NoisyIrNodeType type = postOrderWalkBinOp(N, term);
    
    if (type != kNoisyIrNodeType_Tint && type != kNoisyIrNodeType_TintConst)
    {
        noisyFatal(N, "checkplus has slightly failed\n");
    }
}

/*
 * Perform type inference at declaration
 * For each declaration token such as defineAs, find the statement ancestor
 * the statement node's left child identifier will be inserted with the correct node type
 */
void
noisyInferTypeAtDeclaration(NoisyState * N, NoisyScope * scope, NoisyIrNode * node)
{
    NoisyIrNode * statement = lookupNodeInParents(node, kNoisyIrNodeType_Pstatement);
    if (statement == NULL)
    {
        noisyFatal(N, "There is no statement in the parent!!\n"); 
    }
    NoisyIrNode * identifierNode = statement->irLeftChild;
    if (identifierNode == NULL)
    {
        noisyFatal(N, "Left child of statement isn't identifier!!\n");
    }
    NoisySymbol * symbol = noisySymbolTableAddOrLookupSymbolForToken(N, scope, identifierNode->token);
    symbol->typeTree->type;
}

/*
 * this function accounts for the fact that sometimes
 * different types are indeed the same thing
 * e.g.) kNoisyIrNodeType_Tint and kNoisyIrNodeType_TintConst
 */
bool
areSameTypes(NoisyIrNodeType type1, NoisyIrNodeType type2)
{
    if ((type1 == kNoisyIrNodeType_Tint && type2 == kNoisyIrNodeType_TintConst) || 
        (type2 == kNoisyIrNodeType_Tint && type1 == kNoisyIrNodeType_TintConst))
        return true;
    return type1 == type2;
}

bool
isTokenToIgnoreBinOp(NoisyIrNodeType targetType) 
{
    return (targetType == kNoisyIrNodeType_Tplus ||
            targetType == kNoisyIrNodeType_Tminus ||
            targetType == kNoisyIrNodeType_Tgets); // TODO: fill in more of these
}


NoisyIrNode *
lookupNodeInParents(NoisyIrNode * node, NoisyIrNodeType targetType)
{
    NoisyIrNode * current = node;
    while (current != NULL && current->type != targetType)
    {
        current = current->irParent;
    }
    return current;
}

/* in a subtree, returns the first node of targetType it finds.
 */
NoisyIrNode *
lookupNodeInSubtree(NoisyIrNode * node, NoisyIrNodeType targetType)
{
    if (node == NULL)
        return NULL;
    if (node->type == targetType) 
        return node;

    NoisyIrNode * leftResult = lookupNodeInSubtree(node->irLeftChild, targetType);
    if (leftResult != NULL)
        return leftResult;
    NoisyIrNode * rightResult = lookupNodeInSubtree(node->irRightChild, targetType);
    if (rightResult != NULL)
        return rightResult;
    return NULL; // if children are nil
}



// TODO after plus, generalize to some binary op
void
checkBinOps(NoisyState * N, NoisyIrNode * node)
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
    for (int i = 0; i < kNoisyIrNodeTypeMax; i++)
    {
        if ((gReservedTokenDescriptions[i] != NULL) && !strcmp(gReservedTokenDescriptions[i], string))
        {
            return false;
        }
    } 
    return true;
}

bool 
isValidIdentifier(NoisyState * N, NoisyIrNode * node) 
{
    NoisyTimeStampTraceMacro(kNoisyTimeStampKeyTypeValidateIrSubtree);

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


NoisyIrNode *
noisyTypeValidateIrSubtree(NoisyState *  N, NoisyIrNode *  subtree)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyTypeValidateIrSubtree);

	return NULL;
}


bool
noisyTypeEqualsSubtreeTypes(NoisyState *  N, NoisyIrNode *  subtreeA, NoisyIrNode *  subtreeB)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyTypeEqualsSubtreeTypes);

	return false;
}


char * noisyTypeMakeTypeSignature(NoisyState *  N, NoisyIrNode *  subtree)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyTypeMakeTypeSignature);

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

	char s = gNoisyTypeNodeSignatures[subtree->type];
	if (s == 0)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s, node type is %d (%s)\n", EcannotFindTypeSignatureForNodeType, subtree->type, gNoisyAstNodeStrings[subtree->type]);
		noisyFatal(N, Esanity);
	}

	leftSignature	= noisyTypeMakeTypeSignature(N, subtree->irLeftChild);
	rightSignature	= noisyTypeMakeTypeSignature(N, subtree->irRightChild);

	signature = calloc(strlen(leftSignature) + strlen(rightSignature) + 2, sizeof(char));
	if (signature == NULL)
	{
		noisyFatal(N, Emalloc);
	}

	strcpy(signature, leftSignature);
	strcpy(&signature[strlen(leftSignature)], rightSignature);
	signature[strlen(leftSignature) + strlen(rightSignature)] = s;
	signature[strlen(leftSignature) + strlen(rightSignature) + 1] = '\0';

	free(leftSignature);
	free(rightSignature);


	return signature;
}
