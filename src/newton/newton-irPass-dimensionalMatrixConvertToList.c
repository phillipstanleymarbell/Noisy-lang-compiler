/*
	Authored 2018. Phillip Stanley-Marbell, Youchao Wang.

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

IrNode *
searchNull(IrNode *  root) //checked, correct
{
	static int	debugCount = 0; //used for debug purpose
	if (root->irRightChild == NULL) //if root, then its the nil node
	{
		return root;
	}
		debugCount++;
		return searchNull(root->irRightChild); //recursive search until Null
}

bool
isPiGroupConstant(Invariant *  invariant, IrNode *  parameter, Physics *  physic)
{
	bool		areRightHandPiGroupsConstant = false;
	/*
	 *	check all the parameters to see if they are definded as constant 
	 *	this is currently incorrect, since it should be that all pi groups on the right hand side are constant
	 */
	for ( ; parameter ; parameter = parameter->irRightChild)
	{
		if (physic->isConstant == false)// once there is a parameter not defined as constant, we break the loop.
		{
			break;
		}
		areRightHandPiGroupsConstant = true;//otherwise set as true
	}

	return areRightHandPiGroupsConstant;
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

IrNode *
genIrNodeLeftExpression(State *  N, IrNode *  node, Invariant *  invariant, SourceInfo *  genSrcInfo)
{
	IrNode *	newNode;
	newNode = genIrNode(N, kNoisyIrNodeType_Xseq,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, node, newNode); // lldb node = 0x000000010c217a70; newNode = 0x000000010c217e20; node->irRightChild = 0x000000010c217e20

	IrNode *	newNodeConstraint;
	newNodeConstraint = genIrNode(N, kNewtonIrNodeType_Pconstraint,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNode, newNodeConstraint);

	IrNode *	newNodeQExpression;
	newNodeQExpression = genIrNode(N, kNewtonIrNodeType_PquantityExpression,
					NULL,
					NULL,
					genSrcInfo);
	//newNodeQExpression->physics = 
	addLeaf(N, newNodeConstraint, newNodeQExpression);

	IrNode *	newNodeQTerm;
	newNodeQTerm = genIrNode(N, kNewtonIrNodeType_PquantityTerm,
				NULL,
				NULL,
				genSrcInfo);
	//newNodeQTerm->physics =
	//newNodeQTerm->value = 
	addLeaf(N, newNodeQExpression, newNodeQTerm);
	//newNodeQExpression->value = newNodeQTerm->value;
	//newNodeQExpression->physics = newNodeQTerm->physics;

	IrNode *	newNodeQFactor;
	newNodeQFactor = genIrNode(N, kNewtonIrNodeType_PquantityFactor,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQTerm, newNodeQFactor);

	IrNode *	newNodeIdentifier;
	newNodeIdentifier = genIrNode(N, kNewtonIrNodeType_Tidentifier,
				NULL,
				NULL,
				genSrcInfo);
////////// this is to save the tokenString, need a recursive walk/////////////////
	char *			tokenString;
	tokenString = (char *)calloc(8, sizeof(char));
	newNodeIdentifier->tokenString = tokenString;
	addLeaf(N, newNodeQFactor, newNodeIdentifier);
//////////////////////////////////////////////////////////////////////////////////
	IrNode *	newNodeHighPreOp;
	newNodeHighPreOp = genIrNode(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQFactor, newNodeHighPreOp);

	IrNode *	newNodeExponent;
	newNodeExponent = genIrNode(N, kNewtonIrNodeType_Texponent,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeHighPreOp, newNodeExponent);
//////////////////////////////////////////////////////////////////////////////////
	IrNode *	newNodeQExpressionExponent;
	newNodeQExpressionExponent = genIrNode(N, kNewtonIrNodeType_PquantityExpression,
				NULL,
				NULL,
				genSrcInfo);
	addLeafWithChainingSeqNoLex(N, newNodeHighPreOp, newNodeQExpressionExponent, genSrcInfo);

	IrNode *	newNodeQTermExponent;
	newNodeQTermExponent = genIrNode(N, kNewtonIrNodeType_PquantityTerm,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQExpressionExponent, newNodeQTermExponent);

	IrNode *	newNodeExponentConst;
	newNodeExponentConst = genIrNode(N, kNewtonIrNodeType_TnumericConst,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQTermExponent, newNodeExponentConst);

	return newNodeConstraint;
}

IrNode *
genIrNodeLoopInRightExpression(State *  N, IrNode *  newNodeQTerm, SourceInfo *  genSrcInfo)
{
	IrNode *	newNodeMul;
	newNodeMul = genIrNode(N, kNoisyIrNodeType_Xseq,
				NULL,
				NULL,
				genSrcInfo);
	addLeafWithChainingSeqNoLex(N, newNodeQTerm, newNodeMul, genSrcInfo);

	IrNode *	newNodeMulExpression;
	newNodeMulExpression = genIrNode(N, kNewtonIrNodeType_Tmul,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeMul, newNodeMulExpression);
//////////////////////////////////////////////////////////////////////////////////
	IrNode *	newNodeMulQFactor;
	newNodeMulQFactor = genIrNode(N, kNoisyIrNodeType_Xseq,
					NULL,
					NULL,
					genSrcInfo);
	addLeafWithChainingSeqNoLex(N, newNodeQTerm, newNodeMulQFactor, genSrcInfo);

	IrNode *	newNodeQFactor;
	newNodeQFactor = genIrNode(N, kNewtonIrNodeType_PquantityFactor,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQTerm, newNodeQFactor);

	IrNode *	newNodeIdentifier;
	newNodeIdentifier = genIrNode(N, kNewtonIrNodeType_Tidentifier,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQFactor, newNodeIdentifier);
//////////////////////////////////////////////////////////////////////////////////
	IrNode *	newNodeHighPreOp;
	newNodeHighPreOp = genIrNode(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQFactor, newNodeHighPreOp);

	IrNode *	newNodeExponent;
	newNodeExponent = genIrNode(N, kNewtonIrNodeType_Texponent,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeHighPreOp, newNodeExponent);
//////////////////////////////////////////////////////////////////////////////////
	IrNode *	newNodeQExpressionExponent;
	newNodeQExpressionExponent = genIrNode(N, kNewtonIrNodeType_PquantityExpression,
				NULL,
				NULL,
				genSrcInfo);
	addLeafWithChainingSeqNoLex(N, newNodeHighPreOp, newNodeQExpressionExponent, genSrcInfo);

	IrNode *	newNodeQTermExponent;
	newNodeQTermExponent = genIrNode(N, kNewtonIrNodeType_PquantityTerm,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQExpressionExponent, newNodeQTermExponent);

	IrNode *	newNodeExponentConst;
	newNodeExponentConst = genIrNode(N, kNewtonIrNodeType_TnumericConst,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQTermExponent, newNodeExponentConst);

	return newNodeMul;
}


IrNode *
genIrNodeRightExpression(State *  N, IrNode *  node, Invariant *  invariant, SourceInfo *  genSrcInfo, int countLoop)
{
	IrNode *	newNode;
	newNode = genIrNode(N, kNoisyIrNodeType_Xseq,
				NULL,
				NULL,
				genSrcInfo);
	addLeafWithChainingSeqNoLex(N, node, newNode, genSrcInfo);

	IrNode *	newNodeQExpression;
	newNodeQExpression = genIrNode(N, kNewtonIrNodeType_PquantityExpression,
				NULL,
				NULL,
				genSrcInfo);
	//newNodeQExpression->physics = 
	addLeaf(N, newNode, newNodeQExpression);

	//IrNode *	newNodeRight;
	// clear the mind before sorting this logic out
	IrNode *	newNodeQTerm;
	newNodeQTerm = genIrNode(N, kNewtonIrNodeType_PquantityTerm,
				NULL,
				NULL,
				genSrcInfo);
	//newNodeQTerm->physics =
	//newNodeQTerm->value = 
	addLeaf(N, newNodeQExpression, newNodeQTerm);
//////////////////////////////////////////////////////////////////////////////////
	IrNode *	newNodeQFactor;
	newNodeQFactor = genIrNode(N, kNewtonIrNodeType_PquantityFactor,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQTerm, newNodeQFactor);

	IrNode *	newNodeIdentifier;
	newNodeIdentifier = genIrNode(N, kNewtonIrNodeType_Tidentifier,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQFactor, newNodeIdentifier);
//////////////////////////////////////////////////////////////////////////////////
	IrNode *	newNodeHighPreOp;
	newNodeHighPreOp = genIrNode(N, kNewtonIrNodeType_PhighPrecedenceBinaryOp,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQFactor, newNodeHighPreOp);

	IrNode *	newNodeExponent;
	newNodeExponent = genIrNode(N, kNewtonIrNodeType_Texponent,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeHighPreOp, newNodeExponent);
//////////////////////////////////////////////////////////////////////////////////
	IrNode *	newNodeQExpressionExponent;
	newNodeQExpressionExponent = genIrNode(N, kNewtonIrNodeType_PquantityExpression,
				NULL,
				NULL,
				genSrcInfo);
	addLeafWithChainingSeqNoLex(N, newNodeHighPreOp, newNodeQExpressionExponent, genSrcInfo);

	IrNode *	newNodeQTermExponent;
	newNodeQTermExponent = genIrNode(N, kNewtonIrNodeType_PquantityTerm,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQExpressionExponent, newNodeQTermExponent);

	IrNode *	newNodeExponentConst;
	newNodeExponentConst = genIrNode(N, kNewtonIrNodeType_TnumericConst,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNodeQTermExponent, newNodeExponentConst);
//////////////////////////////////////////////////////////////////////////////////

	for(;countLoop;countLoop--)
	{
		//need to move this into a new function.
		addLeaf(N, newNodeQTerm, genIrNodeLoopInRightExpression(N, newNodeQTerm, genSrcInfo));
	}

	return newNodeQExpression;
}

void
genIrNodeInvariantExpression(State *  N, IrNode *  node, Invariant *  invariant, SourceInfo *  genSrcInfo, int countLoop)
{
	IrNode *	nodeConstraint;
	nodeConstraint = genIrNodeLeftExpression(N, node, invariant, genSrcInfo);

	IrNode *	nodeProportional;
	nodeProportional = genIrNode(N, kNewtonIrNodeType_Tproportional,
					NULL,
					NULL,
					genSrcInfo);
	addLeafWithChainingSeqNoLex(N, nodeConstraint, nodeProportional, genSrcInfo);

	IrNode *	nodeConstraintRight;
	nodeConstraintRight = genIrNodeRightExpression(N, nodeConstraint, invariant, genSrcInfo, countLoop);
}

void
irPassDimensionalMatrixConvertToList(State *  N)
{
	Invariant *	invariant = N->invariantList;
	//IrNode *	parameter = invariant->parameterList;
	//Physics *	physic = parameter->irLeftChild->physics;

	SourceInfo *	genSrcInfo = (SourceInfo *)calloc(1, sizeof(SourceInfo));
	genSrcInfo->fileName = (char *)calloc(sizeof("GeneratedByDA"),sizeof(char));
	genSrcInfo->lineNumber = 1;
	genSrcInfo->columnNumber = 1;
	genSrcInfo->length = 1;

	while (invariant)
	{
		/*
		 *	In Newton, proportionality is expressed as "o<" (currently as "@<")
		 *	Currently we are still using temporary arrays to store the re-ordered null space
		 *	i.e. re-ordered kernels
		 *	When newton-irPass-dimensionalMatrixPiGroupCanonicalization is complete, issue #371
		 *	we should be using the canonicalized kernels instead.
		 *	We currently do not do this.
		 *	Additionally the current method applied only checks pi groups within each kernel,
		 *	as we form the relationship in the traditional way. Each kernel corresponds to one
		 *	function which describes the relationship between different pi's
		 *	Alternatively, we form the unoion of pi groups based on Jonsson's (2014) basis
		 *	and circuit basis.
		 */
		
		/*
		 *	Assign a set of fake sourceInfo, without any interaction with the lexer.
		 *	line, column and length are temporary values, ideally should be more meaningful.
		 *	Currently using an allocated SourceInfo struct to purposefully store these values.
		 */

		int ***		tmpPosition = (int ***)calloc(invariant->numberOfUniqueKernels, sizeof(int **));
		invariant->reorderNullSpace = (double ***)calloc(invariant->numberOfUniqueKernels, sizeof(double **));

		for (int countKernel = 0; countKernel < invariant->numberOfUniqueKernels; countKernel++)
		{	
			tmpPosition[countKernel] = (int **)calloc(invariant->kernelColumnCount, sizeof(int *));
			invariant->reorderNullSpace[countKernel] = (double **)calloc(invariant->kernelColumnCount, sizeof(double *));

			/*
			 *	Construct the new re-ordered null space
			 *	using the permutedIndexArrayPointer
			 */
			for (int countColumn = 0; countColumn < invariant->kernelColumnCount; countColumn++)
			{
				tmpPosition[countKernel][countColumn] = (int *)calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));
				invariant->reorderNullSpace[countKernel][countColumn] = (double *)calloc(invariant->dimensionalMatrixColumnCount, sizeof(double));
				
				for (int countRow = 0; countRow < invariant->dimensionalMatrixColumnCount; countRow++)
				{
					tmpPosition[countKernel][countColumn]
						[invariant->permutedIndexArrayPointer[countKernel * invariant->dimensionalMatrixColumnCount + countRow]] = countRow;
				}

				for (int countRow = 0; countRow < invariant->dimensionalMatrixColumnCount; countRow++)
				{
					invariant->reorderNullSpace[countKernel][countColumn][countRow] = invariant->nullSpace[countKernel]
													[tmpPosition[countKernel][countColumn][countRow]][countColumn];
				}
			}
		}//checked, working
		/*
		 *	End of the reordered kernel construction
		 *	Begin to set up the constraints
		 */

	//	IrNode *	constraint = invariant->constraints;
		int **		dependentVariableColumn;
		int **		dependentVariableRow;

		dependentVariableColumn = (int **) calloc(invariant->numberOfUniqueKernels, sizeof(int *));
		dependentVariableRow = (int **) calloc(invariant->numberOfUniqueKernels, sizeof(int *));

		for (int countKernel = 0; countKernel < invariant->numberOfUniqueKernels; countKernel++)
		{
			/*
			 *	Note that a new null space reordered lexicographically and which rules out
			 *	all the duplicate pi groups should be available once the irPass in issue #372
			 *	is complete. We currently do not have it ready.
			 *	Within each kernel, we find the invariant(s) which occur only once, 
			 *	and set this as the dependent variable(s). By pulral form, we want to express 
			 *	these variables on the left hand side of separate propotionality equations.
			 *	It is important that we identify which of those invariants described by Newton
			 *	are constants at the first instance, by checking the 'constant' keyword.
			 */
			dependentVariableColumn[countKernel] = (int *) calloc(invariant->kernelColumnCount, sizeof(int));
			dependentVariableRow[countKernel] = (int *) calloc(invariant->kernelColumnCount, sizeof(int));

			int	zerosCount = 0;
			int	onesCount = 0;
			int	whichColumn = 0;
			int	countDependentInvariant = 0;
			int	countAddRightExpression = 0;

			if (invariant->kernelColumnCount == 1)
			{
				for (int countRow = 0; countRow < invariant->dimensionalMatrixColumnCount; countRow++)
				{
					if (invariant->reorderNullSpace[countKernel][0][countRow] != 0)
					{
						//onesCount += 1;
						dependentVariableRow[countKernel][countDependentInvariant] = countRow;
						dependentVariableColumn[countKernel][countDependentInvariant++] = 0;
						countAddRightExpression = countDependentInvariant;
					}
				}
				IrNode *	node; //back to X_Seq //////////////IMPORTANT, this currently only creates one subtree in one kernel. Within one kernel there may be
				//countDependentInvariant number of proportional equations to be expressed. 
				//Also there will be numberOfUniqueKernels number of kernels to be looped through.
				/*
				 *	We walk through the tree to find the NULL in the right child, recursively
				 */
				node = searchNull(invariant->constraints->irParent);//checked, this is correct

				genIrNodeInvariantExpression(N, node, invariant, genSrcInfo, countAddRightExpression);
			}
			else if (invariant->kernelColumnCount != 0)
			{
				for (int countRow = 0; countRow < invariant->dimensionalMatrixColumnCount; countRow++)
				{
					for (int countColumn = 0; countColumn < invariant->kernelColumnCount; countColumn++)
					{
						if (invariant->reorderNullSpace[countKernel][countColumn][countRow] == 0)
						{
							zerosCount += 1;
						}
						else
						{
							onesCount += 1;
							whichColumn = countColumn;//only useful when onesCount == 1
						}
					}

					if (onesCount == 1 && zerosCount != 0)//meeting the criteria that it occurs only once
					{
						dependentVariableRow[countKernel][countDependentInvariant] = countRow; //referring to the parameter in invariantList
						dependentVariableColumn[countKernel][countDependentInvariant++] = whichColumn;//countDependentInvariant will determine the number of new constraints added
					}
				}
				/*
				 *	The below checks the number of invariants with exponents not equal to 1 within each column
				 *	Also, for each column in the kernel, there should be one proportional constraint added to the list
				 */
				for (int countColumn = 0; countColumn < invariant->kernelColumnCount; countColumn++)
				{
					for (int countRow = 0; countRow < invariant->dimensionalMatrixColumnCount; countRow++)
					{
						if (invariant->reorderNullSpace[countKernel][countColumn][countRow] != 0)
						{
							countAddRightExpression += 1;
						}
					}
					countAddRightExpression -= 1;//since this is right expression, we take away the one which is the left expression
					for (; countDependentInvariant; countDependentInvariant--)
					{
						IrNode *	node; //back to X_Seq //////////////IMPORTANT, this currently only creates one subtree in one kernel. Within one kernel there may be
							//countDependentInvariant number of proportional equations to be expressed. 
							//Also there will be numberOfUniqueKernels number of kernels to be looped through.
						/*
						 *	We walk through the tree to find the NULL in the right child, recursively
						 */
						node = searchNull(invariant->constraints->irParent);//checked, this is correct

						genIrNodeInvariantExpression(N, node, invariant, genSrcInfo, countAddRightExpression);
					}
					countAddRightExpression = 0;//reset value
				}
				countDependentInvariant = 0;//reset value
			}
			// this, should, be correct?

		} // checked working

		/*
		 *	check each of the pi's
		 */

		/*
		 *	After identifying the dependent variable, we assign the corresponding pi consisting the parameter in question
		 *	to the proportionality equation
		 *	On the left hand side there is the dependent variable Q_1
		 *	On the right hand side there is the remaining invariants within this pi group
		 */

		/*
		 *	There will be two cases for the operation
		 *	Case one: where all the pi groups which should be on the right hand side are constants (fixed values)
		 *	Case two: we do not know if they are constant at compile, therefore need to setup boolean flags
		 */

		/*
		 *	Finally we are done
		 */

		invariant = invariant->next;

		free(tmpPosition);
		free(dependentVariableColumn);
		free(dependentVariableRow);
	}
}
