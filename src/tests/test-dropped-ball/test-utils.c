/*
	Authored 2017. Jonathan Lim.

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

#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "newton-data-structures.h"
#include "common-irHelpers.h"
#include "common-lexers-helpers.h"
#include "newton-parser.h"
#include "newton-parser-expression.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"
#include "newton-api.h"

#include "minunit.h"
#include "test-utils.h"


IrNode *
makeTestParameterTuple(State * newton)
{
	IrNode *	root = genIrNode(newton,	kNewtonIrNodeType_PparameterTuple,
								 NULL /* left child */,
								 NULL /* right child */,
								 NULL /* source info */);
	IrNode * height = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"distance",
		4
		);
	height->physics = newtonApiGetPhysicsTypeByNameAndSubindex(newton, height->token->identifier, 2);
	newtonApiAddLeaf(newton, root, height);

	IrNode * x = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"acceleration",
		1
		);
	x->physics = newtonApiGetPhysicsTypeByNameAndSubindex(newton, x->token->identifier, 0);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, x);

	IrNode * y = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"acceleration",
		1
		);
	y->physics = newtonApiGetPhysicsTypeByNameAndSubindex(newton, y->token->identifier, 1);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, y);

	IrNode * z = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"acceleration",
		1
		);
	z->physics = newtonApiGetPhysicsTypeByNameAndSubindex(newton, z->token->identifier, 2);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, z);

	newtonApiNumberParametersZeroToN(newton, root);
	return root;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////Util functions////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////


IrNode *
makeIrNodeSetValue(
    State * N,
    IrNodeType nodeType,
    char * identifier,
    double realConst
) {
	IrNode * node = genIrNode(
        N,
        nodeType,
	    NULL /* left child */,
	    NULL /* right child */,
	    NULL /* source info */
    );

	node->token = lexAllocateToken(
		N,
		nodeType /* type */,
		identifier /* identifier */,
		0	/* integerConst	*/,
		realConst	/* realConst	*/,
		NULL  /* stringConst	*/,
		NULL	/* sourceInfo	*/
		);

	node->value = node->token->integerConst + node->token->realConst; /* this works because either one is always zero */

    return node;
}

int
numberOfConstraintsPassed(NewtonAPIReport* newtonReport)
{
	int count = 0;
	ConstraintReport* current = newtonReport->firstConstraintReport;

	while (current != NULL)
    {
		printf("satisfiesValueConstraint %d\n", current->satisfiesValueConstraint);
		printf("satisfiesDimensionConstraint %d\n\n", current->satisfiesDimensionConstraint);
		printf("valueErrorMessage %s\n", current->valueErrorMessage);
		printf("dimensionErrorMessage %s\n\n", current->dimensionErrorMessage);
		if (current->satisfiesValueConstraint && current->satisfiesDimensionConstraint)
			count++;
		current = current->next;
    }

	return count;
}

IrNode *
setupNthIrNodeType(State* noisy)
{
	IrNode * numberNode = makeIrNodeSetValue(
		noisy,
		kNewtonIrNodeType_TnumericConst,
		NULL,
		5.0
		);
	IrNode * distanceNode = makeIrNodeSetValue(
		noisy,
		kNewtonIrNodeType_Tidentifier,
		"distance",
		0.0
		);
	IrNode * plus = makeIrNodeSetValue(
		noisy,
		kNewtonIrNodeType_Tplus,
		NULL,
		0.0
		);
	IrNode * minus = makeIrNodeSetValue(
		noisy,
		kNewtonIrNodeType_Tminus,
		NULL,
		0.0
		);
	IrNode * exponent = makeIrNodeSetValue(
		noisy,
		kNewtonIrNodeType_Texponent,
		NULL,
		0.0
		);

	newtonApiAddLeaf(noisy, numberNode, distanceNode);
	newtonApiAddLeafWithChainingSeqNoLexer(noisy, numberNode, plus);
	newtonApiAddLeafWithChainingSeqNoLexer(noisy, numberNode, minus);
	newtonApiAddLeafWithChainingSeqNoLexer(noisy, numberNode, exponent);

	return numberNode;
}
