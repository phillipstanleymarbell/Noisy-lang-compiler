/*
    Authored 2015-2018, Phillip Stanley-Marbell. Modified 2017, Jonathan Lim.
    Updated  2019, Kiseki Hirakawa.

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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
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
#include "common-errors.h"
#include "noisy-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "common-irHelpers.h"
#include "common-lexers-helpers.h"
#include "common-symbolTable.h"


/*
 *	This implements a very shallow copy: all the constituent pointers
 *	are still the original references. The only thing new are the
 *	child pointers, which get reset.
 */
IrNode *
shallowCopyIrNode(State *  N, IrNode *  original)
{
	IrNode *	clone = calloc(1, sizeof(IrNode));
	if (clone == NULL)
	{
		fatal(N, Emalloc);
	}

	memcpy(clone, original, sizeof(IrNode));
	// clone->irLeftChild = NULL;
	// clone->irRightChild = NULL;

	return clone;
}

/*
*	This implements a deepCopy of IrNodes. Deep copy creates a copy for each node of the tree
*	and also for the nodes that are linked with a symbol from the symbol table, creates a copy
*	of the symbol. The copies of the symbols are also inserted in the corresponding scopes of the
*	symbol table but with different names so we can differentiate between the different
*	symbol instances. LoadCount variable is used to create the distinction between the symbol names
*	so they don't confilct. This deep copy technique is currently only used for templated functions.
*	For those we create copies of the original templated Ir tree and then we parse the copy of the
*	tree without alternating the original one.
*/
IrNode *
deepCopyIrNode(State * N,IrNode * original,int loadCount)
{
	IrNode *	clone = calloc(1, sizeof(IrNode));
	if (clone == NULL)
	{
		fatal(N, Emalloc);
	}
	memcpy(clone,original,sizeof(IrNode));


	if (original->symbol != NULL)
	{
		Symbol * symbolClone = calloc(1,sizeof(Symbol));

		if (symbolClone == NULL)
		{
			fatal(N, Emalloc);
		}

		memcpy(symbolClone,original->symbol,sizeof(Symbol));
		char * newSymbolName;
		asprintf(&newSymbolName,"%s_%d",symbolClone->identifier,loadCount);
		symbolClone->identifier = strdup(newSymbolName);

		clone->symbol = commonSymbolTableSymbolForIdentifier(N,symbolClone->scope,newSymbolName);

		if (clone->symbol != NULL)
		{
			free(symbolClone);
		}
		else
		{
			symbolClone->next = original->symbol->scope->firstSymbol;
			original->symbol->scope->firstSymbol = symbolClone;
			symbolClone->prev = NULL;
			clone->symbol = symbolClone;
		}
	}


	if (original->irLeftChild != NULL)
	{
		clone->irLeftChild = deepCopyIrNode(N,original->irLeftChild,loadCount);
		clone->irLeftChild->irParent = clone;
	}
	if (original->irRightChild)
	{
		clone->irRightChild = deepCopyIrNode(N,original->irRightChild,loadCount);
		clone->irRightChild->irParent = clone;
	}
	return clone;
}

IrNode *
genIrNode(State *  N, IrNodeType type, IrNode *  irLeftChild, IrNode *  irRightChild, SourceInfo *  sourceInfo)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyGenNoisyIrNode);

	IrNode *		node;

	node = (IrNode *) calloc(1, sizeof(IrNode));
	if (node == NULL)
	{
		fatal(N, Emalloc);

		/*	Not reached	*/
	}

	node->type		= type;
	node->sourceInfo	= sourceInfo;
	node->irLeftChild	= irLeftChild;
	node->irRightChild	= irRightChild;

	if (irLeftChild != NULL)
	{
		irLeftChild->irParent = node;
	}

	if (irRightChild != NULL)
	{
		irRightChild->irParent = node;
	}

	/*
	 *	Not madatory, but provides higher-fidelity attribution, by making 
	 *	sure that any time between here and next stamping is not attributed
	 *	to genIrNode().
	 */
	//NoisyTimeStampTraceMacro(kNoisyTimeStampKeyUnknown);


	return node;
}


void
errorUseBeforeDefinition(State *  N, const char *  identifier)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParserErrorUseBeforeDefinition);

	flexprint(N->Fe, N->Fm, N->Fperr, Eusedef, identifier);
	flexprint(N->Fe, N->Fm, N->Fperr, "\n");
}

void
errorMultiDefinition(State *  N, Symbol *  symbol)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParserErrorMultiDefinition);

	flexprint(N->Fe, N->Fm, N->Fperr, Emultidef, symbol->identifier, symbol->sourceInfo->fileName, symbol->sourceInfo->lineNumber, symbol->sourceInfo->columnNumber);
	flexprint(N->Fe, N->Fm, N->Fperr, " ");
	flexprint(N->Fe, N->Fm, N->Fperr, Epreviousdef, symbol->definition->sourceInfo->fileName, symbol->definition->sourceInfo->lineNumber, symbol->definition->sourceInfo->columnNumber);
	flexprint(N->Fe, N->Fm, N->Fperr, "\n");
	flexprint(N->Fe, N->Fm, N->Fperr, "\n");
}

IrNode*
findNthIrNodeOfTypes(State * N, IrNode * root, IrNodeType productionOrToken, int firsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax], int nth)
{
	int ith = nth; // copy so we do not modify the caller's count variable
	return findNthIrNodeOfTypesHelper(N, root, productionOrToken, firsts, &ith);
}

IrNode*
findNthIrNodeOfTypesHelper(State * N, IrNode * root, IrNodeType productionOrToken, int firsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax], int *nth)
{
	assert(root != NULL);
	for (int i = 0; i < kNoisyIrNodeTypeMax && firsts[productionOrToken][i] != kNoisyIrNodeTypeMax; i++)
	{
		if (firsts[productionOrToken][i] == root->type)
		{
			if(*nth == 0)
			{
				return root;
			}
			*nth = *nth - 1;

			break;
		}
	}

	IrNode * nthNode;
	if (root->irLeftChild != NULL &&\
		(nthNode = findNthIrNodeOfTypesHelper(N, root->irLeftChild, productionOrToken, firsts, nth)) != NULL)
	{
		return nthNode;
	}

	if (root->irRightChild != NULL &&\
		(nthNode = findNthIrNodeOfTypesHelper(N, root->irRightChild, productionOrToken, firsts, nth)) != NULL)
	{
		return nthNode;
	}

	return NULL;
}

IrNode*
findNthIrNodeOfType(State * N, IrNode * root, IrNodeType expectedType, int nth)
{
	int ith = nth;
	return findNthIrNodeOfTypeHelper(N, root, expectedType, &ith);
}

IrNode*
findNthIrNodeOfTypeHelper(State * N, IrNode * root, IrNodeType expectedType, int* nth)
{
	if (root->type == expectedType)
	{
		if(*nth == 0)
		{
			return root;
		}
		*nth = *nth - 1;
	}

	IrNode * nthNode;
	if (root->irLeftChild != NULL &&\
		(nthNode = findNthIrNodeOfTypeHelper(N, root->irLeftChild, expectedType, nth)) != NULL)
	{
		return nthNode;
	}

	if (root->irRightChild != NULL &&\
		(nthNode = findNthIrNodeOfTypeHelper(N, root->irRightChild, expectedType, nth)) != NULL)
	{
		return nthNode;
	}

	return NULL;
}

int 
countIrNodeOfType(State *  N, IrNode *  node, IrNodeType expectedType) 
{
    int counter = 0;
	
	// TODO: Possible redundant checks
	if (node->irLeftChild == NULL && node->irRightChild == NULL && node->type == expectedType && node->isVisited==false)
	{
		counter += 1;
		node->isVisited = true;
	}
		
		
	if (node->irLeftChild != NULL)
	{
		counter += countIrNodeOfType(N, node->irLeftChild, expectedType);
	} 	

	if (node->irRightChild != NULL)
	{
		counter += countIrNodeOfType(N, node->irRightChild, expectedType);
	}	
				
	return counter;
}

IrNode *
depthFirstWalk(State *  N, IrNode *  node)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParserDepthFirstWalk);

	if (node->irLeftChild == NULL || node->irRightChild == NULL)
	{
		return node;
	}

	return depthFirstWalk(N, node->irRightChild);
}

IrNode *
depthFirstWalkNoLeftChild(State *  N, IrNode *  node)
{
	/*
	 *	Function identical to depthFirstWalk(), except this doesn't
	 *	check irLeftChild.
	 */
	TimeStampTraceMacro(kNoisyTimeStampKeyParserDepthFirstWalk);

	if (node->irRightChild == NULL)
	{
		return node;
	}

	return depthFirstWalkNoLeftChild(N, node->irRightChild);
}

void
addLeaf(State *  N, IrNode *  parent, IrNode *  newNode)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParserAddLeaf);

	IrNode *	node = depthFirstWalk(N, parent);

	if (node == NULL)
	{
		fatal(N, Esanity);
	}

	newNode->irParent = node;

	if (node->irLeftChild == NULL)
	{
		node->irLeftChild = newNode;

		return;
	}

	node->irRightChild = newNode;
}

void
addLeafWithChainingSeq(State *  N, IrNode *  parent, IrNode *  newNode)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParserAddLeafWithChainingSeq);

	IrNode *	node = depthFirstWalk(N, parent);

	if (node == NULL)
	{
		fatal(N, Esanity);
	}

	newNode->irParent = node;

	if (node->irLeftChild == NULL)
	{
		node->irLeftChild = newNode;

		return;
	}

	node->irRightChild = genIrNode(N,	kNoisyIrNodeType_Xseq,
						newNode /* left child */,
						NULL /* right child */,
						lexPeek(N, 1)->sourceInfo /* source info */);
	node->irRightChild->irParent = node;
}

void
addLeafWithChainingSeqNoLex(State *  N, IrNode *  parent, IrNode *  newNode, SourceInfo *  srcInfo)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParserAddLeafWithChainingSeq);

	IrNode *	node = depthFirstWalk(N, parent);

	if (node == NULL)
	{
		fatal(N, Esanity);
	}

	if (node->irLeftChild == NULL)
	{
		node->irLeftChild = newNode;

		return;
	}

	node->irRightChild = genIrNode(N,	kNoisyIrNodeType_Xseq,
						newNode /* left child */,
						NULL /* right child */,
						srcInfo /* source info */);
}

bool
peekCheck(State *  N, int lookAhead, IrNodeType expectedType)
{
	TimeStampTraceMacro(kNoisyTimeStampKeyParserPeekCheck);

	if (lexPeek(N, lookAhead) == NULL)
	{
		return false;
	}

	return (lexPeek(N, lookAhead)->type == expectedType);
}

IrNodeType
getTypeFromOperatorSubtree(State *  N, IrNode *  n)
{
	/*
	 *	All operators are tucked under a production, so dig through the left subtree
	 *	to find the actual operator type:
	 */
	while (L(n) != NULL)
	{
		n = L(n);
	}

	return n->type;
}

void
printDimensionsOfNode(State *  N, IrNode *  n, FlexPrintBuf *  flexBuf)
{
	Dimension *	tmpDimensionsNode;

	for (tmpDimensionsNode = n->physics->dimensions; tmpDimensionsNode != NULL; tmpDimensionsNode = tmpDimensionsNode->next)
	{
		flexprint(N->Fe, N->Fm, flexBuf, "\tDimension \"%s\" with exponent %f\n", 
			tmpDimensionsNode->name, tmpDimensionsNode->exponent);
	}

	return;
}

/*
 *	Find unique Symbols in expression.
 *	Return: A chain of IrNodes, shallow copies of the ones detected.
 */
IrNode *
findExpressionIdentifiers(State *  N, IrNode *  root)
{
	int	i = 0;
	IrNode *	currSymbolNode = findNthIrNodeOfType(N, root, kNewtonIrNodeType_Tidentifier, i++);
	if (currSymbolNode == NULL)
	{
		return NULL;
	}

	IrNode *	head = shallowCopyIrNode(N, currSymbolNode);
	IrNode *	tail = head;

	while ((currSymbolNode = findNthIrNodeOfType(N, root, kNewtonIrNodeType_Tidentifier, i++)) != NULL)
	{
		bool	alreadyAdded = false;

		for (IrNode *  currNode = head; currNode != NULL; currNode = currNode->irRightChild)
		{
			if (currNode->symbol == currSymbolNode->symbol)
			{
				alreadyAdded = true;
				break;
			}
		}
		if (alreadyAdded == true)
		{
			continue;
		}

		IrNode *	newNode = shallowCopyIrNode(N, currSymbolNode);
		
		tail->irRightChild = newNode;
		tail->irRightChild->irParent = tail;
		tail = tail->irRightChild;
	}

	return head;
}
