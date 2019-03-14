/*
	Authored 2019. Vasileios Tsoutsouras.

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

#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <stdlib.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "version.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "noisy-parser.h"
#include "newton-parser.h"
#include "noisy-lexer.h"
#include "newton-lexer.h"
#include "common-irPass-helpers.h"
#include "common-lexers-helpers.h"
#include "common-irHelpers.h"
#include "newton-types.h"
#include "newton-symbolTable.h"

#define Q_PARAMETER 15
#define N_PARAMETER 32

static char multiplierVersion[32] = "qmultSerial";

static char qmultSerial[4096] = "\
module qmultSerial#(\n\
	//Parameterized values\n\
	parameter Q = 15,\n\
	parameter N = 32\n\
	)\n\
	(\n\
	input 	[N-1:0]  i_multiplicand,\n\
	input 	[N-1:0]	i_multiplier,\n\
	input 	i_start,\n\
	input 	i_clk,\n\
	output 	[N-1:0] o_result_out,\n\
	output 	o_complete,\n\
	output	o_overflow\n\
	);\n\
\n\
	reg [2*N-2:0]	reg_working_result;		//	a place to accumulate our result\n\
	reg [2*N-2:0]	reg_multiplier_temp;		//	a working copy of the multiplier\n\
	reg [N-1:0]	reg_multiplicand_temp;		//	a working copy of the umultiplicand\n\
	reg [N-1:0]	reg_result_temp;\n\
	wire [N-1:0] 	i_multiplicand_unsigned;\n\
	wire [N-1:0] 	i_multiplier_unsigned;\n\
	reg [N-1:0] 	reg_count; 			//	This is obviously a lot bigger than it needs to be, as we only need \n\
							//		count to N, but computing that number of bits requires a \n\
							//		logarithm (base 2), and I don't know how to do that in a \n\
							//		way that will work for every possibility						 \n\
	reg	reg_done;				//	Computation completed flag\n\
	reg	reg_sign;				//	The result's sign bit\n\
	reg	reg_overflow;				//	Overflow flag\n\
\n\
	initial reg_done = 1'b1;		//	Initial state is to not be doing anything\n\
	initial reg_overflow = 1'b0;		//		And there should be no woverflow present\n\
	initial reg_sign = 1'b0;		//		And the sign should be positive\n\
\n\
	assign o_result_out[N-1:0] = reg_result_temp[N-1:0];\n\
	assign i_multiplicand_unsigned = ~i_multiplicand[N-1:0] + 1;\n\
	assign i_multiplier_unsigned = ~i_multiplier[N-1:0] + 1;\n\
	assign o_complete = reg_done;					//	Done flag\n\
	assign o_overflow = reg_overflow;				//	Overflow flag\n\
\n\
	always @( posedge i_clk ) begin\n\
		if( reg_done && i_start ) begin				//	This is our startup condition\n\
			reg_done <= 1'b0;				//	We're not done			\n\
			reg_count <= 0;					//	Reset the count\n\
			reg_working_result <= 0;			//	Clear out the result register\n\
			reg_overflow <= 1'b0;				//	Clear the overflow register\n\
\n\
			reg_sign <= i_multiplicand[N-1] ^ i_multiplier[N-1];		//	Set the sign bit\n\
\n\
			if (i_multiplicand[N-1] == 1) \n\
				reg_multiplicand_temp <= i_multiplicand_unsigned[N-1:0]; //~i_multiplicand[N-1:0] + 1;	//	Left-align the dividend in its working register\n\
			else\n\
				reg_multiplicand_temp <= i_multiplicand[N-1:0];		//	Left-align the dividend in its working register\n\
\n\
			if (i_multiplier[N-1] == 1) \n\
				reg_multiplier_temp <= i_multiplier_unsigned[N-1:0]; //~i_multiplier[N-1:0] + 1;	//	Left-align the divisor into its working register\n\
			else\n\
				reg_multiplier_temp <= i_multiplier[N-1:0];		//	Left-align the divisor into its working register \n\
\n\
		end \n\
		else if (!reg_done) begin\n\
			if (reg_multiplicand_temp[reg_count] == 1'b1)				//	if the appropriate multiplicand bit is 1\n\
				reg_working_result <= reg_working_result + reg_multiplier_temp;	//	then add the temp multiplier\n\
\n\
			reg_multiplier_temp <= reg_multiplier_temp << 1;			//	Do a left-shift on the multiplier\n\
			reg_count <= reg_count + 1;						//	Increment the count\n\
\n\
			//stop condition\n\
			if(reg_count == N) begin\n\
				reg_done <= 1'b1;						//	If we're done, it's time to tell the calling process\n\
\n\
				if (reg_sign == 1)\n\
					reg_result_temp[N-1:0] <= ~reg_working_result[N-2+Q:Q] + 1;\n\
				else\n\
					reg_result_temp[N-1:0] <= reg_working_result[N-2+Q:Q];\n\
\n\
				if (reg_working_result[2*N-2:N-1+Q] > 0)			// Check for an overflow\n\
					reg_overflow <= 1'b1;\n\
												//	Increment the count\n\
			end\n\
		end\n\
	end\n\
endmodule\n";

typedef struct multChainTag multChain;

struct multChainTag {
	char *operandName;
	multChain *next;
}; 

bool
irPassRTLIsExpectedTypePresentInRightChild(IrNode *  parentNode, IrNodeType expectedType)
{
	bool	isExpectedTypePresent = false;

	if (parentNode->irRightChild == NULL)
	{
		return isExpectedTypePresent;
	}

	if (parentNode->irRightChild->type == expectedType)
	{
		isExpectedTypePresent = true;
	}

	return isExpectedTypePresent;
}

/*
 *	This function maps a node to a token string for C.
 *	Adapted from smtBackEnd, author Zhengyang Gu.
 */
char *
irPassRTLNodeToStr(State *  N, IrNode *  node)
{
	char *	output = NULL;
	switch(node->type)
	{
		case kNewtonIrNodeType_TnumericConst:
		{
			int needed = snprintf(NULL, 0, "%f", node->value) + 1;
			output = malloc(needed);
			snprintf(output, needed, "%f", node->value);
			break;
		}

		case kNewtonIrNodeType_Tidentifier:
		{
			int needed = snprintf(NULL, 0, "%s", node->tokenString) + 1;
			output = malloc(needed);
			snprintf(output, needed, "%s", node->tokenString);
			break;
		}

		case kNewtonIrNodeType_Tplus:
		{
			output = malloc(4);
			strcpy(output, " + ");
			break;
		}

		case kNewtonIrNodeType_Tminus:
		{
			output = malloc(4);
			strcpy(output, " - ");
			break;
		}

		case kNewtonIrNodeType_Tdiv:
		{
			output = malloc(4);
			strcpy(output, " / ");
			break;
		}

		case kNewtonIrNodeType_Tmul:
		{
			output = malloc(4);
			strcpy(output, " * ");
			break;
		}
		/*
		 *	There is no operator for power exponents
		 *	operation in C. We therefore must use
		 *	double pow( double para, double exponent)
		 *	This switch case prints out the ','
		 */
		case kNewtonIrNodeType_Texponent:
		{
			output = malloc(4);
			strcpy(output, ",");
			break;
		}

		default:
		{
			fatal(N, EtokenInSMT);
		}
	}

	return output;
}

void
irPassRTLSearchAndPrintNodeType(State *  N, IrNode *  root, IrNodeType expectedType, bool isLastParameter, int depthWalk)
{
	if (root == NULL)
	{
		return;
	}

	static int	depth = 0;

	if (root->irRightChild == NULL && root->irLeftChild == NULL
		&& root->type == expectedType)
	{
		flexprint(N->Fe, N->Fm, N->Fprtl, "double");
		flexprint(N->Fe, N->Fm, N->Fprtl, " %s", root->tokenString);
		if(isLastParameter == true)
		{
			depth += 1;
		}
		if(isLastParameter == false || (isLastParameter == true && depth != depthWalk))
		{
			flexprint(N->Fe, N->Fm, N->Fprtl, ", ");
		}
		else if (isLastParameter == true && depth == depthWalk)
		{
			depth = 0;
		}
		return;
	}

	irPassRTLSearchAndPrintNodeType(N, root->irLeftChild, expectedType, isLastParameter, depthWalk);
	irPassRTLSearchAndPrintNodeType(N, root->irRightChild, expectedType, isLastParameter, depthWalk);

	return;
}

void
irPassRTLSearchAndCreateArgList(State *  N, IrNode *  root, IrNodeType expectedType, char **argList, int argumentIndex)
{
	if (root == NULL)
	{
		return;
	}

	if (root->irRightChild == NULL && root->irLeftChild == NULL
		&& root->type == expectedType)
	{
		argList[argumentIndex] = (char *) malloc((strlen(root->tokenString) + 1) * sizeof(char));
		strcpy(argList[argumentIndex], root->tokenString);
		//flexprint(N->Fe, N->Fm, N->Fprtl, "%s", root->tokenString);
		
		return;
	}

	irPassRTLSearchAndCreateArgList(N, root->irLeftChild, expectedType, argList, argumentIndex);
	
	return;
}

int
irPassRTLCountRemainingParameters(State *  N, IrNode *  root, int depth)
{
	if (root == NULL)
	{
		return depth;
	}

	if (root->irRightChild == NULL && root->irLeftChild == NULL
		&& root->type == kNewtonIrNodeType_Tidentifier)
	{
		depth += 1;
		return depth;
	}

	depth = irPassRTLCountRemainingParameters(N, root->irLeftChild, depth);
	depth = irPassRTLCountRemainingParameters(N, root->irRightChild, depth);

	return depth;
}

void
irPassRTLConstraintTreeWalk(State *  N, IrNode *  root)
{
	/*
	 *	This branch should be reached for return.
	 *	It completes a preorder tree traversal.
	 */
	if (root == NULL)
	{
		return;
	}

	static bool	isleftBracketPrinted = false;

	/*
	 *	There is no operator for power exponents
	 *	operation in C. We therefore must use
	 *	double pow( double para, double exponent)
	 */
	if (irPassRTLIsExpectedTypePresentInRightChild(root, kNewtonIrNodeType_PhighPrecedenceBinaryOp) == true)
	{
		flexprint(N->Fe, N->Fm, N->Fprtl, "pow(");
		isleftBracketPrinted = true;
	}

	if (root->irRightChild == NULL && root->irLeftChild == NULL)
	{
		char *		nodeTokenString = NULL;
		nodeTokenString = irPassRTLNodeToStr(N, root);

		flexprint(N->Fe, N->Fm, N->Fprtl, " %s", nodeTokenString);
		/*
		 *	Print out the right bracket of pow() function.
		 */
		if (isleftBracketPrinted == true && root->type == kNewtonIrNodeType_TnumericConst)
		{
			
			flexprint(N->Fe, N->Fm, N->Fprtl, ")");
			isleftBracketPrinted = false;
		}

		return;
	}

	irPassRTLConstraintTreeWalk(N, root->irLeftChild);
	irPassRTLConstraintTreeWalk(N, root->irRightChild);
}

/*
 *	Generate format:
 *	{
 *		double calculatedValue = 0.0;
 		calculatedValue = pow(parameter1 , exponent1) * parameter2 ... ;
 *		return calculatedValue;
 *	}
 */
void
irPassRTLGenFunctionBody(State *  N, IrNode *  constraint, bool isLeft)
{
	/*
	 *	Declare
	 */	
	flexprint(N->Fe, N->Fm, N->Fprtl, "{\n\tdouble calculatedValue = 0.0;\n");

	/*
	 *	Calculation
	 */	
	flexprint(N->Fe, N->Fm, N->Fprtl, "\tcalculatedValue =");

	if (isLeft == false)
	{
		irPassRTLConstraintTreeWalk(N, constraint->irRightChild->irRightChild->irLeftChild->irLeftChild);
	}
	else
	{
		irPassRTLConstraintTreeWalk(N, constraint->irLeftChild->irLeftChild);
	}

	flexprint(N->Fe, N->Fm, N->Fprtl, ";\n");

	/*
	 *	Return
	 */
	flexprint(N->Fe, N->Fm, N->Fprtl, "\n\treturn calculatedValue;\n}\n\n");
}

/*
 *	Generate format: (double parameter1, double parameter 2 ...)
 */
void
irPassRTLGenFunctionArgument(State *  N, IrNode *  constraint, bool isLeft)
{
	flexprint(N->Fe, N->Fm, N->Fprtl, "(");

	IrNode *	constraintsXSeq;

	if (isLeft == false)
	{
		constraintsXSeq = constraint->irRightChild->irRightChild->irLeftChild->irLeftChild;
	}
	else
	{
		constraintsXSeq = constraint->irLeftChild->irLeftChild;
	}

	while (constraintsXSeq->irRightChild != NULL)
	{
		irPassRTLSearchAndPrintNodeType(N, constraintsXSeq->irLeftChild, kNewtonIrNodeType_Tidentifier, false, -1);
		constraintsXSeq = constraintsXSeq->irRightChild;
	}

	irPassRTLSearchAndPrintNodeType(N, constraintsXSeq->irLeftChild, kNewtonIrNodeType_Tidentifier, true,
					irPassRTLCountRemainingParameters(N, constraintsXSeq->irLeftChild, 0));
	flexprint(N->Fe, N->Fm, N->Fprtl, ")\n");
}

/*
 *	Generate format: double functionName
 */
void
irPassRTLGenFunctionName(State *  N, IrNode *  constraints, int countFunction)
{
	flexprint(N->Fe, N->Fm, N->Fprtl, "double\n%sRHS%d",
				constraints->irLeftChild->irLeftChild->irLeftChild->irLeftChild->tokenString, countFunction);
}

/* FIXME check if mallocs and copies fail */
void irPassRTLAddMultChain (multChain **multChainHead, char *parameterName, int power) {
	multChain *tmpChainNode=NULL;
	int i;

	if (*multChainHead == NULL) {
		*multChainHead = (multChain *) malloc(sizeof(multChain));
		(*multChainHead)->operandName = NULL;
		(*multChainHead)->next = NULL;
	}

	tmpChainNode = *multChainHead;

	/* Go to the end of the list */
	while (tmpChainNode->next != NULL) {
		tmpChainNode = tmpChainNode->next;
	}

	for (i=0; i<power; i++) {
		if (tmpChainNode->operandName != NULL) { /* If it is empty then it should be filled */
			tmpChainNode->next = (multChain *) malloc(sizeof(multChain));
			tmpChainNode = tmpChainNode->next;
		}

		tmpChainNode->operandName = (char *) malloc((strlen(parameterName)+1) * sizeof(char)); /* +1 for \0 at the end */
		tmpChainNode->operandName[0] = '\0';

		strcpy(tmpChainNode->operandName, parameterName);
		tmpChainNode->next = NULL;
	}
}

void
irPassRTLProcessInvariantList(State *  N)
{
	/*
	 *	Check if the file called is simply an include.nt
	 */
	if (N->invariantList == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fprtl, "/*\n *\tPlease specify a valid file\n */\n");
		return;
	}

	Invariant *	invariant = N->invariantList;

	if (invariant->constraints == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fprtl, "/*\n *\tNo constraints created\n */\n");
		return;
	}

    /* Constraints list removed */
	IrNode *	parameterListXSeq = invariant->parameterList->irParent->irLeftChild;

	char ** argumentsList;
		
	int index = 0;

	int countFunction = 0;

    /* FIXME add checks - to CBackend as well if current assumptions are met */

	flexprint(N->Fe, N->Fm, N->Fprtl, "/*\n *\tGenerated .v file from Newton\n */\n\n");
	//flexprint(N->Fe, N->Fm, N->Fprtl, "\n#include <math.h>\n\n");

	flexprint(N->Fe, N->Fm, N->Fprtl, "%s\n", qmultSerial);

	while(invariant)
	{
		argumentsList = (char **) malloc(invariant->dimensionalMatrixColumnCount * sizeof(char *));

		/*
		*	Print calculation module -- FIXME more clear support of various architectures
		*/
		countFunction = 0;	
		flexprint(N->Fe, N->Fm, N->Fprtl, "module %s%dSerial",
				invariant->identifier, countFunction);

		flexprint(N->Fe, N->Fm, N->Fprtl, " #(\n\t//Parameterized values\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\tparameter Q = %d,\n", Q_PARAMETER);
        flexprint(N->Fe, N->Fm, N->Fprtl, "\tparameter N = %d", N_PARAMETER);
        flexprint(N->Fe, N->Fm, N->Fprtl, "\n\t)\n\t(\n", N_PARAMETER);

        flexprint(N->Fe, N->Fm, N->Fprtl, "\tinput\ti_clk,\n");

		while (parameterListXSeq != NULL) /* Create a list of function parameters */
		{
			irPassRTLSearchAndCreateArgList(N, parameterListXSeq->irLeftChild, kNewtonIrNodeType_Tidentifier, argumentsList, index);
			parameterListXSeq = parameterListXSeq->irRightChild;
			index++;
		}
		
		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fprtl, "\tinput\t[N-1:0] %s_sig,\n", argumentsList[index]);
			/*
            if (index < invariant->dimensionalMatrixColumnCount - 1)
			{
				flexprint(N->Fe, N->Fm, N->Fprtl, ", ");
			}
            */
		}

		/*
		*	Declare
		*/
        flexprint(N->Fe, N->Fm, N->Fprtl, "\toutput\t[N-1:0] ratio_sig\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\t);\n\n");

		flexprint(N->Fe, N->Fm, N->Fprtl, "\twire [N-1:0] division_res;\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\twire [N-1:0] dividend;\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\twire [N-1:0] divisor;\n\n");
	
		flexprint(N->Fe, N->Fm, N->Fprtl, "\twire oc_dividend;\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\treg oc_divisor;\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\twire oc_Pi;\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\twire of;\n\n");
	
		flexprint(N->Fe, N->Fm, N->Fprtl, "\treg i_st;\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\twire i_st_ratio;\n");
    	flexprint(N->Fe, N->Fm, N->Fprtl, "\treg [N-1:0] ratio_sig_reg;\n\n");

		/*
		 *	We construct a temporary array to re-locate the positions of the permuted parameters
		 */
		int *		tmpPosition = (int *)calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));
		int divisorMultiplications=0, dividendMultiplications=0;
		multChain *dividendMultChainHead=NULL, *divisorMultChainHead=NULL; /* FIXME free variables */

		/*
		 *	Assuming that invariant->numberOfUniqueKernels is always 1 -- FIXME add check?
		 */
		index = 0;
		for (int countKernel = 0; countKernel < invariant->numberOfUniqueKernels; countKernel++)
		{
			for (int j = 0; j < invariant->dimensionalMatrixColumnCount; j++)
			{
				tmpPosition[invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + j]] = j;
			}

			/*
			 *	One pass to determine the number of required multiplications in divisor and dividend
			 */			
			for (int col = 0; col < invariant->kernelColumnCount; col++)
			{
				for (int row = 0; row < invariant->dimensionalMatrixColumnCount; row++)
				{
					if (invariant->nullSpace[countKernel][tmpPosition[row]][col] != 0) 
					{
						//flexprint(N->Fe, N->Fm, N->Fprtl, "%s", argumentsList[index]);
						//flexprint(N->Fe, N->Fm, N->Fprtl, ", %f) * ", invariant->nullSpace[countKernel][tmpPosition[row]][col]);

						if (invariant->nullSpace[countKernel][tmpPosition[row]][col] >= 0) { /* Can there be a zero? */
							dividendMultiplications += (int) invariant->nullSpace[countKernel][tmpPosition[row]][col]; /* FIXME what happens in case of fraction? */
							irPassRTLAddMultChain (&dividendMultChainHead, argumentsList[index], (int) invariant->nullSpace[countKernel][tmpPosition[row]][col]);
						} else {
							divisorMultiplications += (int) -invariant->nullSpace[countKernel][tmpPosition[row]][col]; /* FIXME what happens in case of fraction? */
							irPassRTLAddMultChain (&divisorMultChainHead, argumentsList[index], (int) -invariant->nullSpace[countKernel][tmpPosition[row]][col]);
						}
					}
					index++;
				}
			}
			
			/* One less multiplication than the sum of powers */
			dividendMultiplications--;
			divisorMultiplications--;

			flexprint(N->Fe, N->Fm, N->Fprtl, "\tinitial i_st = 1'b0;\n");
			
			/*
			 *	Print declarations of intermediate dividend multiplication wires
			 */
			if (dividendMultiplications == 0) {
				flexprint(N->Fe, N->Fm, N->Fprtl, "\tinitial oc_dividend = 1'b1;\n");
			} else if (dividendMultiplications == 1) {
				
			} else if (dividendMultiplications > 1) {	
				for (index=1; index < dividendMultiplications; index++) {
					flexprint(N->Fe, N->Fm, N->Fprtl, "\twire [N-1:0] mult_res_dividend_inter%d;\n",index);
					flexprint(N->Fe, N->Fm, N->Fprtl, "\twire oc_dividend%d;\n",index);
				}
			} else {
				/* FIXME if negative */	
			}

			/*
			 *	Print declarations of intermediate dividend multiplication wires
			 */
			flexprint(N->Fe, N->Fm, N->Fprtl, "\n");
			if (dividendMultiplications == 0) {
				flexprint(N->Fe, N->Fm, N->Fprtl, "\tinitial oc_divisor = 1'b1;\n");
			} else if (dividendMultiplications == 1) {

			} else if (divisorMultiplications > 1) {
				for (index=1; index < divisorMultiplications; index++) {
					flexprint(N->Fe, N->Fm, N->Fprtl, "\twire [N-1:0] mult_res_divisor_inter%d;\n",index);	
					flexprint(N->Fe, N->Fm, N->Fprtl, "\twire oc_divisor%d;\n",index);;
				}
			} else {
				/* FIXME if negative */	
			}
			
			flexprint(N->Fe, N->Fm, N->Fprtl, "\t/* ----- Dividend ----- */\n");
			if (dividendMultiplications == 0 ) {
				
			} else if (dividendMultiplications == 1 ) {
				flexprint(N->Fe, N->Fm, N->Fprtl, "\t%s mul_inst_dividend (.i_multiplicand(%s_sig), ",multiplierVersion, dividendMultChainHead->operandName);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_multiplier(%s_sig), ", dividendMultChainHead->next->operandName);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_start(i_st), .i_clk(i_clk), .o_result_out(dividend), .o_complete(oc_dividend), .o_overflow(of));\n\n");
			} else {
				index = 1;

				/* First multiplication with two straightforward operands */
				flexprint(N->Fe, N->Fm, N->Fprtl, "\t%s mul_inst_dividend%d (.i_multiplicand(%s_sig), ",multiplierVersion, index, dividendMultChainHead->operandName);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_multiplier(%s_sig), ", dividendMultChainHead->next->operandName);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_start(i_st), ");
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_clk(i_clk), .o_result_out(mult_res_dividend_inter%d), .o_complete(oc_dividend%d), .o_overflow(of));\n\n",index,index);

				dividendMultChainHead = dividendMultChainHead->next->next;
				index++;

				while (dividendMultChainHead->next != NULL) {
				
					flexprint(N->Fe, N->Fm, N->Fprtl, "\t%s mul_inst_dividend%d (.i_multiplicand(mult_res_dividend_inter%d), ",multiplierVersion, index, index-1);
					flexprint(N->Fe, N->Fm, N->Fprtl, ".i_multiplier(%s_sig), ", dividendMultChainHead->operandName);
					flexprint(N->Fe, N->Fm, N->Fprtl, ".i_start(oc_dividend%d), ",index-1);
					flexprint(N->Fe, N->Fm, N->Fprtl, ".i_clk(i_clk), .o_result_out(mult_res_dividend_inter%d), .o_complete(oc_dividend%d), .o_overflow(of));\n\n",index, index);
				
					dividendMultChainHead = dividendMultChainHead->next;
					index++;
				}

				flexprint(N->Fe, N->Fm, N->Fprtl, "\t%s mul_inst_dividend%d (.i_multiplicand(mult_res_dividend_inter%d), ",multiplierVersion, index, index-1);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_multiplier(%s_sig), ", dividendMultChainHead->operandName);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_start(oc_dividend%d), ",index-1);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_clk(i_clk), .o_result_out(dividend), .o_complete(oc_dividend), .o_overflow(of));\n\n",index,index);
			}

			flexprint(N->Fe, N->Fm, N->Fprtl, "\t/* ----- Divisor ----- */\n");
			if (divisorMultiplications == 0 ) {
				flexprint(N->Fe, N->Fm, N->Fprtl, "\tassign divisor = %s_sig;\n\n",divisorMultChainHead->operandName);	
			} else if (divisorMultiplications == 1 ) {
				flexprint(N->Fe, N->Fm, N->Fprtl, "\t%s mul_inst_divisor (.i_multiplicand(%s_sig), ",multiplierVersion, divisorMultChainHead->operandName);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_multiplier(%s_sig), ", divisorMultChainHead->next->operandName);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_start(i_st), .i_clk(i_clk), .o_result_out(divisor), .o_complete(oc_divisor), .o_overflow(of));\n\n");
			} else {
				index = 1;

				/* First multiplication with two straightforward operands */
				flexprint(N->Fe, N->Fm, N->Fprtl, "\t%s mul_inst_divisor%d (.i_multiplicand(%s_sig), ",multiplierVersion, index, divisorMultChainHead->operandName);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_multiplier(%s_sig), ", divisorMultChainHead->next->operandName);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_start(i_st), ");
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_clk(i_clk), .o_result_out(mult_res_divisor_inter%d), .o_complete(oc_divisor%d), .o_overflow(of));\n\n",index,index);

				divisorMultChainHead = divisorMultChainHead->next->next;
				index++;

				while (divisorMultChainHead->next != NULL) {
				
					flexprint(N->Fe, N->Fm, N->Fprtl, "\t%s mul_inst_divisor%d (.i_multiplicand(mult_res_divisor_inter%d), ",multiplierVersion, index, index-1);
					flexprint(N->Fe, N->Fm, N->Fprtl, ".i_multiplier(%s_sig), ", divisorMultChainHead->operandName);
					flexprint(N->Fe, N->Fm, N->Fprtl, ".i_start(oc_divisor%d), ",index-1);
					flexprint(N->Fe, N->Fm, N->Fprtl, ".i_clk(i_clk), .o_result_out(mult_res_divisor_inter%d), .o_complete(oc_divisor%d), .o_overflow(of));\n\n",index, index);
				
					divisorMultChainHead = divisorMultChainHead->next;
					index++;
				}

				flexprint(N->Fe, N->Fm, N->Fprtl, "\t%s mul_inst_divisor%d (.i_multiplicand(mult_res_inter%d), ",multiplierVersion, index, index-1);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_multiplier(%s_sig), ", divisorMultChainHead->operandName);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_start(oc%d), ",index-1);
				flexprint(N->Fe, N->Fm, N->Fprtl, ".i_clk(i_clk), .o_result_out(divisor), .o_complete(oc_divisor), .o_overflow(of));\n\n",index,index);
			}
			
		}

		flexprint(N->Fe, N->Fm, N->Fprtl, "\t/* ----- Division enabling ----- */\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\tassign i_st_ratio = oc_dividend & oc_divisor;\n\n");

		flexprint(N->Fe, N->Fm, N->Fprtl, "\t/* ----- Division ----- */\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\tqdivSec qdiv_inst (.i_dividend(dividend), .i_divisor(divisor), .i_start(i_st_ratio), .i_clk(i_clk), .o_quotient_out(division_res), .o_complete(oc_Pi), .o_overflow(of));\n\n");
	
		flexprint(N->Fe, N->Fm, N->Fprtl, "\tassign ratio_sig = ratio_sig_reg;\n\n");
	
		flexprint(N->Fe, N->Fm, N->Fprtl, "\talways @( posedge i_clk )\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\tbegin\n\n");
	   
       	flexprint(N->Fe, N->Fm, N->Fprtl, "\t\tif (~i_st) begin\n");
        flexprint(N->Fe, N->Fm, N->Fprtl, "\t\t\ti_st <= 1'b1;\n");
       	flexprint(N->Fe, N->Fm, N->Fprtl, "\t\tend\n");
       	flexprint(N->Fe, N->Fm, N->Fprtl, "\t\telse begin\n");
        flexprint(N->Fe, N->Fm, N->Fprtl, "\t\t\tif (oc_Pi) begin\n");
    	flexprint(N->Fe, N->Fm, N->Fprtl, "\t\t\t\tratio_sig_reg = division_res;\n");
    	flexprint(N->Fe, N->Fm, N->Fprtl, "\t\t\t\ti_st = 1'b0;\n");
        flexprint(N->Fe, N->Fm, N->Fprtl, "\t\t\tend\n");
       	flexprint(N->Fe, N->Fm, N->Fprtl, "\t\tend\n");	   
		flexprint(N->Fe, N->Fm, N->Fprtl, "\tend\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\tendmodule\n");

		free(tmpPosition);

		/*
		
		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fprtl, "\tdouble %s", argumentsList[index]);
			if (index < invariant->dimensionalMatrixColumnCount - 1)
			{
				flexprint(N->Fe, N->Fm, N->Fprtl, ";\n");
			}
		}
		flexprint(N->Fe, N->Fm, N->Fprtl, ";");

		flexprint(N->Fe, N->Fm, N->Fprtl, "\n\tdouble calculatedProportion = 0.0;\n\n");

		flexprint(N->Fe, N->Fm, N->Fprtl, "\tif (argc < %d) {\n",invariant->dimensionalMatrixColumnCount+1);
		flexprint(N->Fe, N->Fm, N->Fprtl, "\t\tprintf(\"Usage is exec_name ");

		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fprtl,  "%s ", argumentsList[index]);
		}

		flexprint(N->Fe, N->Fm, N->Fprtl, "\\n\");\n");
		flexprint(N->Fe, N->Fm, N->Fprtl, "\t\treturn -1;\n\t}\n\n");

		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fprtl, "\t%s = atof(argv[%d]);\n", argumentsList[index], index+1);
		}

		flexprint(N->Fe, N->Fm, N->Fprtl, "\n\tcalculatedProportion = ");

		countFunction = 0;	
		flexprint(N->Fe, N->Fm, N->Fprtl, "%s%d", invariant->identifier, countFunction);

		flexprint(N->Fe, N->Fm, N->Fprtl, "(");

		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			flexprint(N->Fe, N->Fm, N->Fprtl, "%s", argumentsList[index]);
			if (index < invariant->dimensionalMatrixColumnCount - 1)
			{
				flexprint(N->Fe, N->Fm, N->Fprtl, ", ");
			}
		}

		*/

		/*
		for (index = 0; index < invariant->dimensionalMatrixColumnCount; index++) 
		{
			free(argumentsList[index]);
		}
		free(argumentsList);
		*/

		invariant = invariant->next;
	}
	
	flexprint(N->Fe, N->Fm, N->Fprtl, "\n/*\n *\tEnd of the generated .v file\n */\n");
}

void
irPassRTLBackend(State *  N)
{
	FILE *	rtlFile;

	irPassRTLProcessInvariantList(N);

	if (N->outputRTLFilePath) 
	{
		rtlFile = fopen(N->outputRTLFilePath, "w"); 

		if (rtlFile == NULL)
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\n%s: %s.\n", Eopen, N->outputRTLFilePath);
			consolePrintBuffers(N);
		}

		fprintf(rtlFile, "%s", N->Fprtl->circbuf);
		fclose(rtlFile);
	}
}
