/*
	Authored 2018. Youchao Wang.

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
#include <math.h>
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

char *
irPassSearchParameterListTokenString(IrNode *  root, char *  parameterLabel)
{
	/*
	 *	Walk through the parameterTuple sub-tree.
	 *	When the label string matches the one stored in rightChild of a Pparameter
	 *	sub-tree we return the leftChild of this Pparameter, otherwise continue
	 *	searching. By convention the left child is the name, and the right child
	 *	is the name of the Physics.
	 */
	if (strcmp(parameterLabel, root->irLeftChild->irRightChild->tokenString) == 0)
	{
		return root->irLeftChild->irLeftChild->tokenString;
	}

	return irPassSearchParameterListTokenString(root->irRightChild, parameterLabel);
}

void
irPassGenLoopsInRightExpression(State *  N, IrNode *  newNodeQTerm, SourceInfo *  genSrcInfo,
				int **  row, int **  col, int ***  rowIndependent, int ***  colIndependent,
				int kernel, int whichParameter, int whichIndependentParameter)
{
	/*
	 *	TODO: This function should ideally be compressed for a better 'hygiene'
	 *	since most of the time it is doing some genIrNode() loops
	 */
	IrNode *	newNodeMulExpression;
	newNodeMulExpression = genIrNode(N, kNewtonIrNodeType_Tmul,
				NULL,
				NULL,
				genSrcInfo);

	Token *		tokenExpression = (Token *) calloc(1, sizeof(Token));
	tokenExpression->type = kNewtonIrNodeType_Tmul;
	newNodeMulExpression->token = tokenExpression;
	addLeafWithChainingSeqNoLex(N, newNodeQTerm, newNodeMulExpression, genSrcInfo);

	IrNode *	newNodeQFactor;
	newNodeQFactor = genIrNode(N, kNewtonIrNodeType_PquantityFactor,
				NULL,
				NULL,
				genSrcInfo);
	addLeafWithChainingSeqNoLex(N, newNodeQTerm, newNodeQFactor, genSrcInfo);

	IrNode *	newNodeIdentifier;
	newNodeIdentifier = genIrNode(N, kNewtonIrNodeType_Tidentifier,
				NULL,
				NULL,
				genSrcInfo);
	newNodeIdentifier->tokenString = irPassSearchParameterListTokenString(N->invariantList->parameterList,
						N->invariantList->dimensionalMatrixColumnLabels[rowIndependent[kernel][whichParameter][whichIndependentParameter]]);
	addLeaf(N, newNodeQFactor, newNodeIdentifier);

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

	Token *		tokenExponent = (Token *) calloc(1, sizeof(Token));
	tokenExponent->type = kNewtonIrNodeType_Texponent;
	newNodeExponent->token = tokenExponent;
	
	addLeaf(N, newNodeHighPreOp, newNodeExponent);

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
	newNodeExponentConst->value = -1; //- (N->invariantList->nullSpaceCanonicallyReordered[kernel]
					//				[colIndependent[kernel][whichParameter][whichIndependentParameter]]
					//				[rowIndependent[kernel][whichParameter][whichIndependentParameter]]);
	printf("\n ExponentConstValue %f\n", newNodeExponentConst->value);
	Token *		tokenExponentConst = (Token *) calloc(1, sizeof(Token));
	tokenExponentConst->type = kNewtonIrNodeType_TnumericConst;
	newNodeExponentConst->token = tokenExponentConst;

	addLeaf(N, newNodeQTermExponent, newNodeExponentConst);

}


IrNode *
irPassGenExpression(State *  N, IrNode *  node, SourceInfo *  genSrcInfo, int countLoop, bool isLeft,
			int **  row, int **  col, int ***  rowIndependent, int ***  colIndependent, int kernel, int whichParameter)
{
	/*
	 *	TODO: This function should ideally be compressed for a better 'hygiene'
	 *	since most of the time it is doing some genIrNode() loops
	 */
	IrNode *	newNodeConstraint;
	newNodeConstraint = genIrNode(N, kNewtonIrNodeType_Pconstraint,
				NULL,
				NULL,
				genSrcInfo);
	
	IrNode *	newNode;
	newNode = genIrNode(N, kNewtonIrNodeType_PquantityExpression,
				NULL,
				NULL,
				genSrcInfo);
	if(isLeft == true)
	{
		addLeafWithChainingSeqNoLex(N, node, newNodeConstraint, genSrcInfo);
		addLeafWithChainingSeqNoLex(N, newNodeConstraint, newNode, genSrcInfo);
	}
	else
	{
		addLeafWithChainingSeqNoLex(N, node, newNode, genSrcInfo);
	}

	IrNode *	newNodeQTerm;
	newNodeQTerm = genIrNode(N, kNewtonIrNodeType_PquantityTerm,
				NULL,
				NULL,
				genSrcInfo);
	addLeaf(N, newNode, newNodeQTerm);

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
	if(isLeft == true)
	{
		newNodeIdentifier->tokenString = irPassSearchParameterListTokenString(N->invariantList->parameterList, 
							N->invariantList->dimensionalMatrixColumnLabels[row[kernel][whichParameter]]);
	}
	else
	{
		newNodeIdentifier->tokenString = irPassSearchParameterListTokenString(N->invariantList->parameterList, 
							N->invariantList->dimensionalMatrixColumnLabels[rowIndependent[kernel][whichParameter][0]]);
	}

	addLeaf(N, newNodeQFactor, newNodeIdentifier);

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
	Token *		tokenExponent = (Token *) calloc(1, sizeof(Token));
	tokenExponent->type = kNewtonIrNodeType_Texponent;
	newNodeExponent->token = tokenExponent;

	addLeaf(N, newNodeHighPreOp, newNodeExponent);

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
	if(isLeft == true)
	{
		newNodeExponentConst->value = N->invariantList->nullSpaceCanonicallyReordered[kernel]
										[col[kernel][whichParameter]]
										[row[kernel][whichParameter]];
	}
	else
	{
		newNodeExponentConst->value = 0 - N->invariantList->nullSpaceCanonicallyReordered[kernel]
										[colIndependent[kernel][whichParameter][0]]
										[rowIndependent[kernel][whichParameter][0]];
	}

	Token *		tokenExponentConst = (Token *) calloc(1, sizeof(Token));
	tokenExponentConst->type = kNewtonIrNodeType_TnumericConst;
	newNodeExponentConst->token = tokenExponentConst;

	addLeaf(N, newNodeQTermExponent, newNodeExponentConst);

	countLoop -= 1;

	int	whichIndependentParameter = 1;
	for (;countLoop && isLeft == false; countLoop--)
	{
		irPassGenLoopsInRightExpression(N, depthFirstWalkNoLeftChild(N, newNodeQTerm), genSrcInfo,
						row, col,
						rowIndependent, colIndependent,
						kernel,
						whichParameter, whichIndependentParameter);
		whichIndependentParameter += 1;
	}

	return newNodeConstraint;
}

void
irPassGenInvariantPiGroupsExpression(State *  N, IrNode *  node, SourceInfo *  genSrcInfo, int countLoop,
				int **  row, int **  col, int *** rowIndependent, int *** colIndependent, int kernel, int whichParameter)
{
	/*
	 *	After identifying the dependent variable, we assign the corresponding
	 *	pi consisting the parameter in question to the proportionality equation.
	 *	On the left hand side there is the dependent variable Q_1.
	 */
	IrNode *	nodeConstraint;
	nodeConstraint = irPassGenExpression(N, node,
					genSrcInfo, 0,
					true,
					row, col,
					rowIndependent, colIndependent,
					kernel,
					whichParameter);

	/*
	 *	The expression symbol 'o<'
	 */
	IrNode *	nodeProportional;
	nodeProportional = genIrNode(N, kNewtonIrNodeType_Tproportional,
					NULL,
					NULL,
					genSrcInfo);
	addLeafWithChainingSeqNoLex(N, nodeConstraint, nodeProportional, genSrcInfo);

	/*
	 *	On the right hand side the remaining parameters within
	 *	this pi group, Q_2 ... Q_n
	 */
	irPassGenExpression(N, nodeConstraint,
				genSrcInfo, countLoop,
				false,
				row, col,
				rowIndependent, colIndependent,
				kernel,
				whichParameter);
}

void
irPassDimensionalMatrixConvertToList(State *  N)
{
	Invariant *	invariant = N->invariantList;
	IrNode *	node;

	/*
	 *	We currently use the sourceInfo to indicate the generated constraints 
	 *	without any interaction with the lexer. line, column and length are
	 *	temporary values, ideally should be more meaningful.
	 */
	SourceInfo *	genSrcInfo = (SourceInfo *)calloc(1, sizeof(SourceInfo));
	genSrcInfo->fileName = (char *)calloc(sizeof("GeneratedByDA") + 1,sizeof(char));
	genSrcInfo->fileName = "GeneratedByDA";
	genSrcInfo->lineNumber = -1;
	genSrcInfo->columnNumber = -1;
	genSrcInfo->length = 0;

	while (invariant)
	{
		/*
		 *	Proportionality is expressed as "o<" (currently as "@<", see #374)
		 *	History: Before #372, #383 and #384 were completed, we used to reorder
		 *	the null space using permutedIndexArrayPointer.
		 *	We are now using the nullSpaceCanonicallyReordered instead.
		 *
		 *	The current method applied only checks pi groups within each kernel,
		 *	as we form the relationship in the traditional way. Each kernel corresponds
		 *	to one function which describes the relationship between different pi's.
		 *	Alternatively, we form the unoion of pi groups based on Jonsson's (2014)
		 *	basis and circuit basis.
		 *
		 *	Now begin to set up the constraints.
		 */
		int **		locateDependentInvariantColumn;
		int **		locateDependentInvariantRow;
		int ***		locateIndependentInvariantColumn;
		int ***		locateIndependentInvariantRow;

		locateDependentInvariantRow = (int **) calloc(invariant->numberOfUniqueKernels, sizeof(int *));
		locateDependentInvariantColumn = (int **) calloc(invariant->numberOfUniqueKernels, sizeof(int *));
		locateIndependentInvariantRow = (int ***) calloc(invariant->numberOfUniqueKernels, sizeof(int **));
		locateIndependentInvariantColumn = (int ***) calloc(invariant->numberOfUniqueKernels, sizeof(int **));

		for (int countKernel = 0; countKernel < invariant->numberOfUniqueKernels; countKernel++)
		{
			/*
			 *	Note that a new null space reordered lexicographically and which rules
			 *	out all the duplicate pi groups is available as nullSpaceCanonicallyReordered.
			 *	See issues #372, #383 and #384 for more.
			 *
			 *	Within each kernel, we find the invariant(s) which occurs only once,
			 *	and set this as the dependent variable(s). When we find more than one
			 *	dependent variables, we express these variables in separate propotionality
			 *	equations (left hand side).
			 *	
			 *	It is also important that we identify which of those invariants described
			 *	by Newton are constants at the first instance, by checking the 'constant'
			 *	keyword, we currently do not do this.
			 */
			locateDependentInvariantRow[countKernel] = (int *) calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));
			locateDependentInvariantColumn[countKernel] = (int *) calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));

			int	zerosCount = 0;
			int	onesCount = 0;
			int	whichColumn = 0;
			int	countDependentInvariant = 0;
			int	countAddRightExpression = 0;

			/*
			 *	There will be two cases for the operation, the operations for kernels 
			 *	with one column and kernels with multiple columns will be slightly
			 *	different.
			 *
			 *	(1) Kernels with one column. All the invariants with exponent not equal
			 *	to zero could be regarded as dependent invariants. 
			 *	
			 *	(2) Kernels with multiple columns. We first do a thorough search and
			 *	determine whichi nvariant(s) occurs only once among all the columns.
			 */
			if (invariant->kernelColumnCount == 1)
			{
				for (int countRow = 0; countRow < invariant->dimensionalMatrixColumnCount; countRow++)
				{
					if (fabs(invariant->nullSpaceCanonicallyReordered[countKernel][0][countRow]) != 0)
					{
						locateDependentInvariantRow[countKernel][countDependentInvariant] = countRow;
						locateDependentInvariantColumn[countKernel][countDependentInvariant] = 0;
						countAddRightExpression = countDependentInvariant++;
					}
				}
				locateIndependentInvariantRow[countKernel] = (int **) calloc(countDependentInvariant, sizeof(int *));
				locateIndependentInvariantColumn[countKernel] = (int **) calloc(countDependentInvariant, sizeof(int *));

				for (int i = 0; i < countDependentInvariant; i++)
				{
					locateIndependentInvariantRow[countKernel][i] = (int *) calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));
					locateIndependentInvariantColumn[countKernel][i] = (int *) calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));

					for (int countRow = 0, countIndependentInvariant = 0; countRow < invariant->dimensionalMatrixColumnCount; countRow++)
					{
						if (fabs(invariant->nullSpaceCanonicallyReordered[countKernel][0][countRow]) != 0
							&& locateDependentInvariantRow[countKernel][i] != countRow)
						{
							locateIndependentInvariantRow[countKernel][i][countIndependentInvariant] = countRow;
							locateIndependentInvariantColumn[countKernel][i][countIndependentInvariant++] = 0;
						}
						countIndependentInvariant = 0; //should this be here or not?
					}
				}

				/*
				 *	We walk through the tree to find the NULL in the right child, recursively
				 *	invariant->parameterList->irParent points to the X_Seq whose right child
				 *	would be constraints such avoids the need to locate invariant->constraints
				 *	(which points to the first constraint) since there may not be any human
				 *	written constraints available at compile time.
				 */
				for (int j = 0; j < countDependentInvariant; j++)
				{
					node = depthFirstWalkNoLeftChild(N, invariant->parameterList->irParent);

					irPassGenInvariantPiGroupsExpression(N,	node /* node with null irRightChild */,
									genSrcInfo /* 'fake' source information */,
									countAddRightExpression /* number of invariants on right hand side */,
									locateDependentInvariantRow,
									locateDependentInvariantColumn,
									locateIndependentInvariantRow,
									locateIndependentInvariantColumn,
									countKernel /* The current kernel */,
									j /* corresponding to the related dependent invariant */);
				}

			}
			else if (invariant->kernelColumnCount != 0)
			{
				for (int countRow = 0; countRow < invariant->dimensionalMatrixColumnCount; countRow++)
				{
					for (int countColumn = 0; countColumn < invariant->kernelColumnCount; countColumn++)
					{
						if (fabs(invariant->nullSpaceCanonicallyReordered[countKernel][countColumn][countRow]) == 0)
						{
							zerosCount += 1;
						}
						else
						{
							onesCount += 1;
							whichColumn = countColumn;
						}
					}
					/*
					 *	Meeting the criteria that it occurs only once
					 */
					if (onesCount == 1 && zerosCount != 0)
					{
						/*
						 *	Referring to the parameter in invariantList
						 */
						locateDependentInvariantRow[countKernel][countDependentInvariant] = countRow;
						/*
						 *	CountDependentInvariant will determine the number of new constraints added
						 */
						locateDependentInvariantColumn[countKernel][countDependentInvariant++] = whichColumn;
					}
				}
				locateIndependentInvariantRow[countKernel] = (int **) calloc(countDependentInvariant, sizeof(int *));
				locateIndependentInvariantColumn[countKernel] = (int **) calloc(countDependentInvariant, sizeof(int *));

				for (int countColumn = 0; countColumn < invariant->kernelColumnCount; countColumn++)
				{
					for (int i = 0, countIndependentInvariant = 0; i < countDependentInvariant; i++)
					{
						locateIndependentInvariantRow[countKernel][i] = (int *) calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));
						locateIndependentInvariantColumn[countKernel][i] = (int *) calloc(invariant->dimensionalMatrixColumnCount, sizeof(int));

						for (int countRow = 0; countRow < invariant->dimensionalMatrixColumnCount; countRow++)
						{
							if (fabs(invariant->nullSpaceCanonicallyReordered[countKernel][countColumn][countRow]) != 0
								&& locateDependentInvariantRow[countKernel][i] != countRow)
							{
								locateIndependentInvariantRow[countKernel][i][countIndependentInvariant] = countRow;
								printf("\n CountRow %d\n", countRow);
								locateIndependentInvariantColumn[countKernel][i][countIndependentInvariant++] = countColumn;
								printf("\n CountColumn %d\n", countColumn);
							}
						}
						countIndependentInvariant = 0; //should this be here or not?
					}

				}
				/*
				 *	Check the number of invariants with exponents not equal to 0 within each column
				 */
				for (int countColumn = 0; countColumn < invariant->kernelColumnCount; countColumn++)
				{
					for (int countRow = 0; countRow < invariant->dimensionalMatrixColumnCount; countRow++)
					{
						if (fabs(invariant->nullSpaceCanonicallyReordered[countKernel][countColumn][countRow]) != 0)
						{
							countAddRightExpression += 1;
						}
					}
					/*
					 *	Since this is right expression, we take away the one which is the left expression
					 */
					countAddRightExpression -= 1;

					for (int j = 0; j < countDependentInvariant; j++)
					{
						/*
						 *	We walk through the tree to find the NULL in the right child, recursively
						 */
						node = depthFirstWalkNoLeftChild(N, invariant->parameterList->irParent);

						irPassGenInvariantPiGroupsExpression(N,	node /* node with null irRightChild */,
										genSrcInfo /* 'fake' source information */,
										countAddRightExpression /* number of invariants on right hand side */,
										locateDependentInvariantRow,
										locateDependentInvariantColumn,
										locateIndependentInvariantRow,
										locateIndependentInvariantColumn,
										countKernel /* The current kernel */,
										j /* corresponding to the related dependent invariant */);
					}
					countAddRightExpression = 0;
				}
			}
		}
		/*
		 *	Relocate invariant->constraints to point to the first constraint in the
		 *	invariant list in AST.
		 */
		invariant->constraints = invariant->parameterList->irParent->irRightChild->irLeftChild;
		
		invariant = invariant->next;

		free(locateDependentInvariantColumn);
		free(locateDependentInvariantRow);
		free(locateIndependentInvariantColumn);
		free(locateIndependentInvariantRow);

	}
	free(genSrcInfo);
}
