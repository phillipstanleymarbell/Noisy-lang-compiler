/*
	Authored 2019. Phillip Stanley-Marbell.

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
#include <stdarg.h>
#include <math.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "version.h"
#include "common-errors.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "common-irHelpers.h"
#include "common-irHelpers.h"
#include "newton-irPass-constantFolding.h"



void
newtonIrPassConstantFolding(State *  N)
{
	return;
}



static double
applyOperator(State *  N, IrNodeType operator, double src1, double src2)
{
	switch(operator)
	{
		case kNewtonIrNodeType_Tplus:
		{
			return src1 + src2;
		}

		case kNewtonIrNodeType_Tminus:
		{
			return src1 - src2;
		}

		case kNewtonIrNodeType_Tmul:
		{
			return src1 * src2;
		}

		case kNewtonIrNodeType_Tdiv:
		{
			return src1 / src2;
		}

		case kNewtonIrNodeType_PexponentiationOperator:
		{
			return pow(src1, src2);
		}

		case kNewtonIrNodeType_Tpercent:
		{
			return fmod(src1, src2);
		}

		case kNewtonIrNodeType_TrightShift:
		{
			return ((int)src1) >> ((int)src2);
		}

		case kNewtonIrNodeType_TleftShift:
		{
			return ((int)src1) << ((int)src2);
		}

		case kNewtonIrNodeType_TbitwiseOr:
		{
			return ((int)src1) | ((int)src2);
		}

		default:
		{
			/*
			 *	Should not happen.
			 */
			fatal(N, Esanity);
		}
	}

	/*
	 *	Should not happen.
	 */
	fatal(N, Esanity);

	return 0;
}

double
newtonIrPassConstantFoldingSubtreeEvaluate(State *  N, IrNode *  node)
{
	if (node->type == kNewtonIrNodeType_PnumericConst)
	{
		/*
		 *	numericConst			::=	integerConst | realConst .
		 */
		return node->value;
	}
	else if (node->type == kNewtonIrNodeType_PnumericFactor)
	{
		/*
		 *	E.g., a single const as a factor.
		 */
		if (R(node) == NULL)
		{
			return newtonIrPassConstantFoldingSubtreeEvaluate(N, L(node));
		}

		/*
		 *	numericFactor			::=	(numericConst [exponentiationOperator numericConst]) | "(" numericExpression ")" .
		 */
		if ((L(node)->type == kNewtonIrNodeType_PnumericConst) && (RL(node)->type == kNewtonIrNodeType_PexponentiationOperator))
		{
			double	base = newtonIrPassConstantFoldingSubtreeEvaluate(N, L(node));
			double	exponent = newtonIrPassConstantFoldingSubtreeEvaluate(N, RRL(node));

			return pow(base, exponent);
		}
		else if (node->irLeftChild->type == kNewtonIrNodeType_PnumericExpression)
		{
			/*
			 *	Recurse
			 */
			return newtonIrPassConstantFoldingSubtreeEvaluate(N, L(node));
		}
		else
		{
			/*
			 *	Should not happen.
			 */
			fatal(N, Esanity);
		}
	}
	else if ((node->type == kNewtonIrNodeType_PnumericTerm) || (node->type == kNewtonIrNodeType_PnumericExpression))
	{
		/*
		 *	numericTerm			::=	numericFactor {highPrecedenceOperator numericFactor} .
		 *	numericExpression		::=	numericTerm {lowPrecedenceOperator numericTerm} .
		 */
		double	result = newtonIrPassConstantFoldingSubtreeEvaluate(N, L(node));

		/*
		 *	Loop invariant: currentSubtree is the node in the IR
		 *	whose left child is a highPrecedenceOperator and whose
		 *	RL() is the adjoining factor:
		 */
		IrNode *	currentSubtree = R(node);

		while (currentSubtree != NULL)
		{
			if (R(currentSubtree) == NULL)
			{
				/*
				 *	Should not happen.
				 */
				fatal(N, Esanity);
			}

			IrNode *	operatorNode = L(currentSubtree);
			IrNode *	nextFactorNode = RL(currentSubtree);

			/*
			 *	All operators are tucked under a production, so dig through the left subtree
			 *	to find the actual operator type:
			 */
			while (L(operatorNode) != NULL)
			{
				operatorNode = L(operatorNode);
			}
			IrNodeType	operatorType = getTypeFromOperatorSubtree(N, operatorNode);

			result = applyOperator(N, operatorType, result, newtonIrPassConstantFoldingSubtreeEvaluate(N, nextFactorNode));
			currentSubtree = RR(currentSubtree);
		}

		return result;
	}


	/*
	 *	Should not happen.
	 */
	fatal(N, Esanity);


	/*
	 *	Not reached.
	 */
	return 0;
}
