/*
	Authored 2018. Zhengyang Gu.

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
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "newton-symbolTable.h"
#include "common-irPass-helpers.h"
#include "common-astTransform.h"
#include "newton-irPass-smtBackend.h"
#include "newton-irPass-dotBackend.h"
#include "newton-types.h"
#include "newton.h"


/*
 *	This function processes the physics list in the Newton State, which
 *	containts the definitions of all the signal "types" and constants.
 */
void
irPassSmtProcessPhysicsList(State *  N)
{
	Physics *	current = N->newtonIrTopScope->firstPhysics;
	char *		lastPhysicsName = NULL;

	for (; current != NULL; current = current->next)
	{
		if (current->isConstant)
		{
			flexprint(N->Fe, N->Fm, N->Fpsmt2, "(declare-fun %s () Real)\n(assert  ( = %s %.17g ) )\n", 
					current->identifier, current->identifier, current->value);
		}

		/*
		 *	Since we ignore the dimensions in satisfiability
		 *	checking, we can simply declare dimension alias 
		 *	abbrev. as 1. However, for vector-like signals, we
		 *	do not want to repeatedly declare and assert the 
		 *	value of the abbrev. at different components, since
		 *	they are all the same, i.e. time@0 and time@1 would
		 *	be seperate (and consequtive) entries in the physics
		 *	list, yet they have the same dimension alias abbrev.
		 *	('s;), so we should only declare 's' once. This is
		 *	checked via remembering the name of the last physics
		 *	and compare it against the current name.
		 */
		else if (current->dimensionAliasAbbreviation != NULL &&
			(lastPhysicsName == NULL || strcmp(current->identifier, lastPhysicsName) != 0))
		{
			flexprint(N->Fe, N->Fm, N->Fpsmt2, "(declare-fun %s () Real)\n(assert  ( = %s 1 ) )\n", 
						current->dimensionAliasAbbreviation, current->dimensionAliasAbbreviation);

			lastPhysicsName = current->identifier;
		}
	}

	return;
}

/*
 *	This function translates a parameter name into the following format:
 *	<invaraint name>@<signal type>@<signal index>@<parameter name>
 *
 *	'@' is used as a seperator as it is not allowed in identifier names
 *	in Newton, but is valid in variable names in SMT2, so by using it as
 *	the seperator, we guanruntee that 2 parameters will only have the same
 *	name if they have the same name and are in the same invariant (with
 *	the same type), in which case the frontend should have already reported
 *	as an error.
 */
char *
irPassSmtGenParameterName(State *  N, Invariant *  parentInvariant, IrNode *  parameter)
{
	/*
	 *	Calling snprintf with second argument being 0 returns the actual
	 *	length of the to-be-printed string, +1 as it is \0 terminated.
	 */
	int	needed = snprintf(NULL, 0, "%s@%s@%u@%s", parentInvariant->identifier,
				parameter->physics->identifier, parameter->physics->subindex,
				parameter->irLeftChild->tokenString) + 1;
	char *	output = malloc(needed);
	snprintf(output, needed, "%s@%s@%u@%s", parentInvariant->identifier,
				parameter->physics->identifier, parameter->physics->subindex,
				parameter->irLeftChild->tokenString);

	return output;
}

char *
irPassSmtGenNodeName(State *  N, Invariant *  parentInvariant, IrNode *  node)
{
	IrNode *	currentParameter = parentInvariant->parameterList;
	char *		currentParameterName = NULL;
	char *		output = NULL;

	for(; currentParameter != NULL; currentParameter = currentParameter->irRightChild)
	{
		currentParameterName = currentParameter->irLeftChild->irLeftChild->tokenString;

		if (strcmp(node->tokenString, currentParameterName) == 0)
		{
			output = irPassSmtGenParameterName(N, parentInvariant, currentParameter->irLeftChild);
			break;
		}
	}

	/*
	 *	If we cannot find the identifier among the parameter list,
	 *	we trust the front-end to have verified that it is a
	 *	constant/unit, which can be used as is. We nevertheless
	 *	create a copy of it, so that all possible outputs from this
	 *	function may/should be freed after use.
	 */
	if (currentParameter == NULL)
	{
		output = malloc(strlen(node->tokenString) + 1);
		strcpy(output, node->tokenString);
	}

	return output;
}

/*
 *	This function maps a node to a token string in the SMT2 syntax. Returned
 *	string from this function can/should be 'free'ed if not useful anymore.
 */
char *
irPassSmtNodeToStr(State *  N, Invariant *  parentInvariant, IrNode *  node)
{
	char *	output = NULL;
	switch(node->type)
	{
		case kNewtonIrNodeType_Tnumber:
		{
			int needed = snprintf(NULL, 0, "%f", node->value) + 1;
			output = malloc(needed);
			snprintf(output, needed, "%f", node->value);
			break;
		}

		case kNewtonIrNodeType_Tidentifier:
		{
			output = irPassSmtGenNodeName(N, parentInvariant, node);
			break;
		}

		case kNewtonIrNodeType_Tequivalent:
		case kNewtonIrNodeType_Tproportionality:
		{
			output = malloc(2);
			strcpy(output, "=");
			break;
		}

		case kNewtonIrNodeType_Tgt:
		{
			output = malloc(2);
			strcpy(output, ">");
			break;
		}

		case kNewtonIrNodeType_Tge:
		{
			output = malloc(3);
			strcpy(output, ">=");
			break;
		}

		case kNewtonIrNodeType_Tlt:
		{
			output = malloc(2);
			strcpy(output, "<");
			break;
		}

		case kNewtonIrNodeType_Tle:
		{
			output = malloc(3);
			strcpy(output, "<=");
			break;
		}

		case kNewtonIrNodeType_Tplus:
		{
			output = malloc(2);
			strcpy(output, "+");
			break;
		}

		case kNewtonIrNodeType_Tminus:
		{
			output = malloc(2);
			strcpy(output, "-");
			break;
		}

		case kNewtonIrNodeType_Tdiv:
		{
			output = malloc(2);
			strcpy(output, "/");
			break;
		}

		case kNewtonIrNodeType_Tmul:
		{
			output = malloc(2);
			strcpy(output, "*");
			break;
		}

		case kNewtonIrNodeType_Texponent:
		{
			output = malloc(2);
			strcpy(output, "^");
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
irPassSmtTreeWalk(State *  N, Invariant *  parentInvariant, IrNode *  root)
{
	char *	nodeString = irPassSmtNodeToStr(N, parentInvariant, root);

	/*
	 *	This branch should not be reached (by the SMT2 backend),
	 *	unless a NULL tree is fed to the first call of the function,
	 *	in which case I am slightly tempted to raise a fatal...
	 */
	if (root == NULL)
	{
		return;
	}

	/*
	 *	To add brackets correctly, we need to identifier whether
	 *	a node is a leaf before processing its children. We have
	 *	to check both children, as unary minus currently puts its
	 *	operand in rightchild.
	 */
	if (root->irLeftChild == NULL && root->irRightChild == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fpsmt2, " %s", nodeString);
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fpsmt2, " ( %s", nodeString);

		/*
		 *	This takes care of the unary minus operator case,
		 *	where the left child is stored as NULL, and the
		 *	right child is the operand. For a value of -a,
		 *	we translates it into an expression of 0 - a.
		 *	This seems to be the only way of doing it in SMT2,
		 *	as the solver does not recognize -a, where a is
		 *	a defined variable.
		 *
		 *	An alternative is to write it as -1 * a, which 
		 *	also preserves dimensions, but for that we will
		 *	need to change the operator.
		 */
		if (root->type == kNewtonIrNodeType_Tminus && root->irLeftChild == NULL)
		{
			flexprint(N->Fe, N->Fm, N->Fpsmt2, " 0");
		}
		else
		{
			irPassSmtTreeWalk(N, parentInvariant, root->irLeftChild);
		}

		irPassSmtTreeWalk(N, parentInvariant, root->irRightChild);
		flexprint(N->Fe, N->Fm, N->Fpsmt2, " )");
	}

	free(nodeString);

	return;
}

/*
 *	This function walks a transformed tree and creates a new assertion
 *	of a != 0 for any expression in the form of a/b
 */
void
irPassSmtDivisorWalk(State *  N, Invariant *  parentInvariant, IrNode *  root)
{
	if (root == NULL)
	{
		return;
	}

	if (root->type == kNewtonIrNodeType_Tdiv)
	{
		flexprint(N->Fe, N->Fm, N->Fpsmt2, "(assert ( not ( = 0 ");
		
		irPassSmtTreeWalk(N, parentInvariant, root->irRightChild);
		flexprint(N->Fe, N->Fm, N->Fpsmt2, " ) ) )\n");
	}

	irPassSmtDivisorWalk(N, parentInvariant, root->irLeftChild);
	irPassSmtDivisorWalk(N, parentInvariant, root->irRightChild);

	return;
}

void
irPassSmtProcessConstraint(State *  N, Invariant *  parentInvariant, IrNode *  root)
{
	IrNode *	transformed = commonTreeTransform(N, root);

	flexprint(N->Fe, N->Fm, N->Fpsmt2, "(assert ");
	irPassSmtTreeWalk(N, parentInvariant, transformed);
	flexprint(N->Fe, N->Fm, N->Fpsmt2,  " )\n");

	irPassSmtDivisorWalk(N, parentInvariant, transformed);

	return;
}

void
irPassDeclareParameters(State *  N, Invariant *  input)
{
	IrNode *	currentParameter = input->parameterList;

	for (; currentParameter != NULL; currentParameter = currentParameter->irRightChild)
	{
		char *  parameterName = irPassSmtGenParameterName(N, input, currentParameter->irLeftChild);
		flexprint(N->Fe, N->Fm, N->Fpsmt2, "(declare-fun %s () Real)\n", parameterName);
		
		free(parameterName);
	}

	return;
}

void
irPassSmtProcessInvariant(State *  N, Invariant *  input)
{
	IrNode *	current = input->constraints;

	irPassDeclareParameters(N, input);

	for (; current != NULL; current = current->irRightChild)
	{
		assert(current->irLeftChild->type == kNewtonIrNodeType_Pconstraint);
		irPassSmtProcessConstraint(N, input, current->irLeftChild);
	}

	return;
}

void
irPassSmtProcessInvariantList(State *  N)
{
	Invariant *	current = N->invariantList;

	for (; current != NULL; current = current->next)
	{
		irPassSmtProcessInvariant(N, current);
	}

	return;
}

void
irPassSmtBackend(State *  N)
{
	FILE *	smtFile;


	/*
	 *	Heuristic
	 */

	flexprint(N->Fe, N->Fm, N->Fpsmt2, "(set-logic QF_NRA)\n");

	irPassSmtProcessPhysicsList(N);
	irPassSmtProcessInvariantList(N);

	flexprint(N->Fe, N->Fm, N->Fpsmt2, "(check-sat)\n(exit)\n");

	if (N->outputSmtFilePath)
	{
		smtFile = fopen(N->outputSmtFilePath, "w");
		if (smtFile == NULL)
		{
			flexprint(N->Fe, N->Fm, N->Fperr, "\n%s: %s.\n", Eopen, N->outputSmtFilePath);
			consolePrintBuffers(N);
		}

		fprintf(smtFile, "%s", N->Fpsmt2->circbuf);
		fclose(smtFile);
	}


	return;
}
