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
#include "test-newton-api.h"
#include "test-common.h"
#include "test-utils.h"

extern int gNewtonFirsts[kNoisyIrNodeTypeMax][kNoisyIrNodeTypeMax];
extern int tests_run;



char * test_testNthIrNodeOfType()
{
    return 0;
}

char * test_testNthIrNodeOfTypes()
{
  State * noisy = init(kNoisyModeDefault);
	IrNode * numberNode = setupNthIrNodeType(noisy);

  int termIndex = 0;
  mu_assert(
			  "test_testNthIrNodeOfTypes: looked for a number and wasn't a number",
			  findNthIrNodeOfTypes(
                                             noisy,
                                             numberNode,
                                             kNewtonIrNodeType_PquantityTerm,
                                             gNewtonFirsts,
                                             termIndex
                                             )->type == kNewtonIrNodeType_Tnumber
			  );

   termIndex = 1;
   mu_assert(
   		"test_testNthIrNodeOfTypes: identifier should be a first of quantityTerm",
   		findNthIrNodeOfTypes(
   							 noisy,
   							 numberNode,
   							 kNewtonIrNodeType_PquantityTerm,
   							 gNewtonFirsts,
   							 termIndex
   							 )->type == kNewtonIrNodeType_Tidentifier
   		);

    int factorIndex = 1;
    mu_assert(
			  "test_testNthIrNodeOfTypes: identifier should be a first of quantityFactor",
			  findNthIrNodeOfTypes(
								   noisy,
								   numberNode,
								   kNewtonIrNodeType_PquantityFactor,
								   gNewtonFirsts,
								   factorIndex
								   )->type == kNewtonIrNodeType_Tidentifier
			  );
    int expressionIndex = 1;
    mu_assert(
			  "test_testNthIrNodeOfTypes: identifier should be a first of quantityExpression",
			  findNthIrNodeOfTypes(
								   noisy,
								   numberNode,
								   kNewtonIrNodeType_PquantityExpression,
								   gNewtonFirsts,
								   expressionIndex
								   )->type == kNewtonIrNodeType_Tidentifier
			  );
    int lowBinOpIndex = 0;
    mu_assert(
			  "test_testNthIrNodeOfTypes: the first low binop should be plus",
			  findNthIrNodeOfTypes(
								   noisy,
								   numberNode,
								   kNewtonIrNodeType_PlowPrecedenceBinaryOp,
								   gNewtonFirsts,
								   lowBinOpIndex
								   )->type == kNewtonIrNodeType_Tplus
			  );
    lowBinOpIndex = 1;
    IrNode* n = findNthIrNodeOfTypes(
                                          noisy,
                                          numberNode,
                                          kNewtonIrNodeType_PlowPrecedenceBinaryOp,
                                          gNewtonFirsts,
                                          lowBinOpIndex
                                          );
    mu_assert(
			  "test_testNthIrNodeOfTypes: the second low binop should be minus",
			  n->type == kNewtonIrNodeType_Tminus
			  );
    int highBinOpIndex = 0;
    mu_assert(
			  "test_testNthIrNodeOfTypes: the first high binop should be exponent",
			  findNthIrNodeOfTypes(
								   noisy,
								   numberNode,
								   kNewtonIrNodeType_PhighPrecedenceBinaryOp,
								   gNewtonFirsts,
								   highBinOpIndex
								   )->type == kNewtonIrNodeType_Texponent
			  );
    return 0;
}
