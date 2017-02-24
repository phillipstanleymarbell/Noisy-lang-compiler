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
#include <assert.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "common-timeStamps.h"
#include "data-structures.h"
#include "common-irHelpers.h"
#include "common-lexers-helpers.h"

char* gNewtonAstNodeStrin[kNoisyIrNodeTypeMax]	= {
                                               [    kNewtonIrNodeType_PcompareOp]            = "kNewtonIrNodeType_PcompareOp",
                                               [     kNewtonIrNodeType_PvectorOp]            = "kNewtonIrNodeType_PvectorOp",
                                               [kNewtonIrNodeType_PhighPrecedenceBinaryOp]            = "kNewtonIrNodeType_PhighPrecedenceBinaryOp",
                                               [kNewtonIrNodeType_PmidPrecedenceBinaryOp]            = "kNewtonIrNodeType_PmidPrecedenceBinaryOp",
                                               [kNewtonIrNodeType_PlowPrecedenceBinaryOp]            = "kNewtonIrNodeType_PlowPrecedenceBinaryOp",
                                               [      kNewtonIrNodeType_PunaryOp]            = "kNewtonIrNodeType_PunaryOp",
                                               [       kNewtonIrNodeType_PtimeOp]            = "kNewtonIrNodeType_PtimeOp",
                                               [     kNewtonIrNodeType_Pquantity]            = "kNewtonIrNodeType_Pquantity",
                                               [kNewtonIrNodeType_PquantityFactor]            = "kNewtonIrNodeType_PquantityFactor",
                                               [ kNewtonIrNodeType_PquantityTerm]            = "kNewtonIrNodeType_PquantityTerm",
                                               [kNewtonIrNodeType_PquantityExpression]            = "kNewtonIrNodeType_PquantityExpression",
                                               [    kNewtonIrNodeType_Pparameter]            = "kNewtonIrNodeType_Pparameter",
                                               [kNewtonIrNodeType_PparameterTuple]            = "kNewtonIrNodeType_PparameterTuple",
                                               [   kNewtonIrNodeType_Pderivation]            = "kNewtonIrNodeType_Pderivation",
                                               [       kNewtonIrNodeType_Psymbol]            = "kNewtonIrNodeType_Psymbol",
                                               [         kNewtonIrNodeType_Pname]            = "kNewtonIrNodeType_Pname",
                                               [   kNewtonIrNodeType_Pconstraint]            = "kNewtonIrNodeType_Pconstraint",
                                               [kNewtonIrNodeType_PconstraintList]            = "kNewtonIrNodeType_PconstraintList",
                                               [   kNewtonIrNodeType_PbaseSignal]            = "kNewtonIrNodeType_PbaseSignal",
                                               [    kNewtonIrNodeType_Pinvariant]            = "kNewtonIrNodeType_Pinvariant",
                                               [     kNewtonIrNodeType_Pconstant]            = "kNewtonIrNodeType_Pconstant",
                                               [         kNewtonIrNodeType_Prule]            = "kNewtonIrNodeType_Prule",
                                               [     kNewtonIrNodeType_PruleList]            = "kNewtonIrNodeType_PruleList",
                                               [   kNewtonIrNodeType_PnewtonFile]            = "kNewtonIrNodeType_PnewtonFile",
                                               [          kNewtonIrNodeType_Tnil]            = "kNewtonIrNodeType_Tnil",
                                               [kNewtonIrNodeType_ZbadIdentifier]            = "kNewtonIrNodeType_ZbadIdentifier",
                                               [kNewtonIrNodeType_ZbadStringConst]            = "kNewtonIrNodeType_ZbadStringConst",
                                               [      kNewtonIrNodeType_Zepsilon]            = "kNewtonIrNodeType_Zepsilon",
                                               [          kNewtonIrNodeType_Zeof]            = "kNewtonIrNodeType_Zeof",
                                               [        kNewtonIrNodeType_Tcross]            = "kNewtonIrNodeType_Tcross",
                                               [          kNewtonIrNodeType_Tdot]            = "kNewtonIrNodeType_Tdot",
                                               [          kNewtonIrNodeType_Texponent]            = "kNewtonIrNodeType_Texponent",
                                               [     kNewtonIrNodeType_Tintegral]            = "kNewtonIrNodeType_Tintegral",
                                               [   kNewtonIrNodeType_Tderivative]            = "kNewtonIrNodeType_Tderivative",
                                               [kNoisyIrNodeType_Xseq]                       = "kNoisyIrNodeType_Xseq",
                                               [      kNewtonIrNodeType_TSpanish]            = "kNewtonIrNodeType_TSpanish",
                                               [      kNewtonIrNodeType_TEnglish]            = "kNewtonIrNodeType_TEnglish",
                                               [   kNewtonIrNodeType_Tsignal]            = "kNewtonIrNodeType_Tsignal",
                                               [    kNewtonIrNodeType_Tinvariant]            = "kNewtonIrNodeType_Tinvariant",
                                               [     kNewtonIrNodeType_Tconstant]            = "kNewtonIrNodeType_Tconstant",
                                               [   kNewtonIrNodeType_Tderivation]            = "kNewtonIrNodeType_Tderivation",
                                               [       kNewtonIrNodeType_Tsymbol]            = "kNewtonIrNodeType_Tsymbol",
                                               [       kNewtonIrNodeType_Tequals]            = "kNewtonIrNodeType_Tequals",
                                               [       kNewtonIrNodeType_Tnone]            = "kNewtonIrNodeType_Tnone",
                                               [       kNewtonIrNodeType_Tnumber]            = "kNewtonIrNodeType_Tnumber",
                                               [       kNewtonIrNodeType_Tplus]            = "kNewtonIrNodeType_Tplus",
                                               [       kNewtonIrNodeType_Tminus]            = "kNewtonIrNodeType_Tminus",
                                               [       kNewtonIrNodeType_Tmul]            = "kNewtonIrNodeType_Tmul",
                                               [       kNewtonIrNodeType_Tdiv]            = "kNewtonIrNodeType_Tdiv",
                                               [         kNewtonIrNodeType_Tname]            = "kNewtonIrNodeType_Tname",
                                               [    kNewtonIrNodeType_TrightBrace]            = "kNewtonIrNodeType_TrightBrace",
                                               [     kNewtonIrNodeType_TleftBrace]            = "kNewtonIrNodeType_TleftBrace",
                                               [   kNewtonIrNodeType_TrightParen]            = "kNewtonIrNodeType_TrightParen",
                                               [    kNewtonIrNodeType_TleftParen]            = "kNewtonIrNodeType_TleftParen",
                                               [   kNewtonIrNodeType_Tidentifier]            = "kNewtonIrNodeType_Tidentifier",
                                   };

NoisyIrNode *
genNoisyIrNode(NoisyState *  N, NoisyIrNodeType type, NoisyIrNode *  irLeftChild, NoisyIrNode *  irRightChild, NoisySourceInfo *  sourceInfo)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyGenNoisyIrNode);

	NoisyIrNode *		node;

	node = (NoisyIrNode *) calloc(1, sizeof(NoisyIrNode));
	if (node == NULL)
	{
		noisyFatal(N, Emalloc);

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
	 *	to genNoisyIrNode().
	 */
	//NoisyTimeStampTraceMacro(kNoisyTimeStampKeyUnknown);


	return node;
}


void
errorUseBeforeDefinition(NoisyState *  N, const char *  identifier)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserErrorUseBeforeDefinition);

	flexprint(N->Fe, N->Fm, N->Fperr, "Saw identifier \"%s\" in use before definition\n", identifier);
}

void
errorMultiDefinition(NoisyState *  N, NoisySymbol *  symbol)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserErrorMultiDefinition);
}

NoisyIrNode*
findNthIrNodeOfTypes(NoisyState * N, NoisyIrNode * root, NoisyIrNodeType productionOrToken, int firsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax], int nth)
{
  int ith = nth; // copy so we do not modify the caller's count variable
  return findNthIrNodeOfTypesHelper(N, root, productionOrToken, firsts, &ith);
}

NoisyIrNode*
findNthIrNodeOfTypesHelper(NoisyState * N, NoisyIrNode * root, NoisyIrNodeType productionOrToken, int firsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax], int *nth)
{
  assert(root != NULL);
	for (int i = 0; i < kNoisyIrNodeTypeMax && firsts[productionOrToken][i] != kNoisyIrNodeTypeMax; i++)
	{
		if (firsts[productionOrToken][i] == root->type)
		{
            if(*nth == 0)
                return root;
            *nth = *nth - 1;
            break;
		}
	}

  NoisyIrNode * nthNode;
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

NoisyIrNode*
findNthIrNodeOfType(NoisyState * N, NoisyIrNode * root, NoisyIrNodeType expectedType, int nth)
{
  int ith = nth;
  return findNthIrNodeOfTypeHelper(N, root, expectedType, &ith);
}


NoisyIrNode*
findNthIrNodeOfTypeHelper(NoisyState * N, NoisyIrNode * root, NoisyIrNodeType expectedType, int* nth)
{
  if (root->type == expectedType)
    {
      if(*nth == 0)
        return root;
      *nth = *nth - 1;
    }

  NoisyIrNode * nthNode;
  if (root->irLeftChild != NULL &&\
      (nthNode = findNthIrNodeOfTypeHelper(N, root->irLeftChild, expectedType, nth)) != NULL)
    return nthNode;

  if (root->irRightChild != NULL &&\
      (nthNode = findNthIrNodeOfTypeHelper(N, root->irRightChild, expectedType, nth)) != NULL)
    return nthNode;

  return NULL;
}

NoisyIrNode *
depthFirstWalk(NoisyState *  N, NoisyIrNode *  node)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserDepthFirstWalk);

	if (node->irLeftChild == NULL || node->irRightChild == NULL)
	{
		return node;
	}

	return depthFirstWalk(N, node->irRightChild);
}

void
addLeaf(NoisyState *  N, NoisyIrNode *  parent, NoisyIrNode *  newNode)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserAddLeaf);

	NoisyIrNode *	node = depthFirstWalk(N, parent);

	if (node == NULL)
	{
		noisyFatal(N, Esanity);
	}
	
	if (node->irLeftChild == NULL)
	{
		node->irLeftChild = newNode;
		
		return;
	}

	node->irRightChild = newNode;
}

void
addLeafWithChainingSeq(NoisyState *  N, NoisyIrNode *  parent, NoisyIrNode *  newNode)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserAddLeafWithChainingSeq);

	NoisyIrNode *	node = depthFirstWalk(N, parent);

	if (node->irLeftChild == NULL)
	{
		node->irLeftChild = newNode;

		return;
	}

	node->irRightChild = genNoisyIrNode(N,	kNoisyIrNodeType_Xseq,
						newNode /* left child */,
						NULL /* right child */,
						noisyLexPeek(N, 1)->sourceInfo /* source info */);
}

void
addLeafWithChainingSeqNoLexer(NoisyState *  N, NoisyIrNode *  parent, NoisyIrNode *  newNode)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserAddLeafWithChainingSeq);

	NoisyIrNode *	node = depthFirstWalk(N, parent);

	if (node->irLeftChild == NULL)
    {
      node->irLeftChild = newNode;

      return;
    }

	node->irRightChild = genNoisyIrNode(N,	kNoisyIrNodeType_Xseq,
                                      newNode /* left child */,
                                      NULL /* right child */,
                                      NULL/* source info */);
}

bool
peekCheck(NoisyState *  N, int lookAhead, NoisyIrNodeType expectedType)
{
    NoisyTimeStampTraceMacro(kNoisyTimeStampKeyParserPeekCheck);

    if (noisyLexPeek(N, lookAhead) == NULL)
    {
        return false;
    }

    return (noisyLexPeek(N, lookAhead)->type == expectedType);
}
