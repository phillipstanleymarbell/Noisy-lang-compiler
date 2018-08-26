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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// setup for pressure_sensors.nt ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IrNode *
makeTestParameterTuplePressureCaseBoyles()
{
    State * newton = newtonApiInit("../../Examples/pressure_sensors.nt");
	IrNode *	root = genIrNode(newton,	kNewtonIrNodeType_PparameterTuple,
								 NULL /* left child */,
								 NULL /* right child */,
								 NULL /* source info */);
	IrNode * pressure = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"pressure",
		1
		);
	pressure->physics = newtonApiGetPhysicsTypeByNameAndSubindex(newton, pressure->token->identifier, 0);
	assert(pressure->physics->subindex == 0);
	newtonApiAddLeaf(newton, root, pressure);

	IrNode * volume = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"volume",
		3.5
		);
	volume->physics = newtonApiGetPhysicsTypeByNameAndSubindex(newton, volume->token->identifier, 0);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, volume);

	newtonApiNumberParametersZeroToN(newton, root);
	return root;
}

IrNode *
makeTestParameterTuplePressureCaseGayLussac()
{
    State * newton = newtonApiInit("../../Examples/pressure_sensors.nt");
	IrNode *	root = genIrNode(newton,	kNewtonIrNodeType_PparameterTuple,
								 NULL /* left child */,
								 NULL /* right child */,
								 NULL /* source info */);
	IrNode * pressure = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"pressure",
		2.5
		);
	pressure->physics = newtonApiGetPhysicsTypeByNameAndSubindex(newton, pressure->token->identifier, 0);
	assert(pressure->physics->subindex == 0);
	newtonApiAddLeaf(newton, root, pressure);

	IrNode * temperature = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"temperature",
		2
		);
	temperature->physics = newtonApiGetPhysicsTypeByNameAndSubindex(newton, temperature->token->identifier, 0);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, temperature);

	newtonApiNumberParametersZeroToN(newton, root);
	return root;
}

IrNode *
makeTestParameterTuplePressureCaseAvogadro()
{
    State * newton = newtonApiInit("../../Examples/pressure_sensors.nt");
	IrNode *	root = genIrNode(newton,	kNewtonIrNodeType_PparameterTuple,
								 NULL /* left child */,
								 NULL /* right child */,
								 NULL /* source info */);
	IrNode * volume = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"volume",
		2
		);
	volume->physics = newtonApiGetPhysicsTypeByNameAndSubindex(newton, volume->token->identifier, 0);
	newtonApiAddLeaf(newton, root, volume);

	IrNode * amount = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"amount",
		5
		);
	amount->physics = newtonApiGetPhysicsTypeByNameAndSubindex(newton, amount->token->identifier, 0);
	assert(amount->physics->subindex == 0);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, amount);

	newtonApiNumberParametersZeroToN(newton, root);
	return root;
}
/*
 * Constructs an example tree for the statement
 * foo / bar + fizz / bazz = foo / bar + fizz * bazz, where
 * acceleration foo, bar = 8;
 * time fizz, bazz = 2;
 *
 */
IrNode *
makeSampleIncorrectTestStatementPressureCase()
{
    State * newton = newtonApiInit("../../Examples/pressure_sensors.nt");
	IrNode * root = genIrNode(newton,
							  kNewtonIrNodeType_PquantityStatement,
							  NULL,
							  NULL,
							  NULL);

	newtonApiAddLeaf(newton,
					 root,
					 makeSampleCorrectTestExpressionPressureCase()
		);

	newtonApiAddLeafWithChainingSeqNoLexer(newton,
					 root,
					 genIrNode(newton,
							   kNewtonIrNodeType_Tequals,
							   NULL,
							   NULL,
							   NULL)
		);

	newtonApiAddLeaf(newton,
					 root,
					 makeSampleIncorrectTestExpressionPressureCase()
		);
	return root;
}

/*
 * Constructs an example tree for the expression
 * foo / bar + fizz / bazz, where
 * pressure foo, fizz = 8;
 * volume bar, bazz = 2;
 */
// TODO make this method take in two identifiers (and maybe an operator) and do it that way
IrNode *
makeSampleIncorrectTestExpressionPressureCase()
{
    State * newton = newtonApiInit("../../Examples/pressure_sensors.nt");
	IrNode * root = genIrNode(newton,
							  kNewtonIrNodeType_PquantityExpression,
							  NULL,
							  NULL,
							  NULL);

	IrNode * leftTerm = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_PquantityTerm,
		NULL,
		0
		);
	IrNode * foo = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"pressure",
		8
		);
	IrNode * div = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tdiv,
		NULL,
		0
		);
	IrNode * bar = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"volume",
		2
		);
    foo->physics = newtonApiGetPhysicsTypeByName(newton, foo->token->identifier);
	newtonApiAddLeaf(newton, leftTerm, foo);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, leftTerm, div);
    bar->physics = newtonApiGetPhysicsTypeByName(newton, bar->token->identifier);
	newtonApiAddLeaf(newton, leftTerm, bar);

	IrNode * add = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tplus,
		NULL,
		0
		);
	IrNode * rightTerm = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_PquantityTerm,
		NULL,
		0
		);
	IrNode * fizz = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"pressure",
		8
		);
	IrNode * mul = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tmul,
		NULL,
		0
		);
	IrNode * bazz = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"volume",
		2
		);
    fizz->physics = newtonApiGetPhysicsTypeByName(newton, fizz->token->identifier);
	newtonApiAddLeaf(newton, rightTerm, fizz);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, rightTerm, mul);
    bazz->physics = newtonApiGetPhysicsTypeByName(newton, bazz->token->identifier);
	newtonApiAddLeaf(newton, rightTerm, bazz);

	newtonApiAddLeaf(newton, root, leftTerm);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, add);
	newtonApiAddLeaf(newton, root, rightTerm);

	return root;
}

/*
 * Constructs an example tree for the statement
 * foo / bar + fizz / bazz = foo / bar + fizz / bazz, where
 * acceleration foo, bar = 8;
 * time fizz, bazz = 2;
 *
 */
IrNode *
makeSampleCorrectTestStatementPressureCase()
{
    State * newton = newtonApiInit("../../Examples/pressure_sensors.nt");
	IrNode * root = genIrNode(newton,
							  kNewtonIrNodeType_PquantityStatement,
							  NULL,
							  NULL,
							  NULL);

	newtonApiAddLeaf(newton,
					 root,
					 makeSampleCorrectTestExpressionPendulumCase()
		);

	newtonApiAddLeafWithChainingSeqNoLexer(newton,
					 root,
					 genIrNode(newton,
							   kNewtonIrNodeType_Tequals,
							   NULL,
							   NULL,
							   NULL)
		);

	newtonApiAddLeaf(newton,
					 root,
					 makeSampleCorrectTestExpressionPendulumCase()
		);
	return root;
}

/*
 * Constructs an example tree for the expression
 * foo / bar + fizz / bazz, where
 * acceleration foo, bar = 8;
 * time fizz, bazz = 2;
 */
IrNode *
makeSampleCorrectTestExpressionPressureCase()
{
    State * newton = newtonApiInit("../../Examples/pressure_sensors.nt");
	IrNode * root = genIrNode(newton,
							  kNewtonIrNodeType_PquantityExpression,
							  NULL,
							  NULL,
							  NULL);

	IrNode * leftTerm = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_PquantityTerm,
		NULL,
		0
		);
	IrNode * foo = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"pressure",
		8
		);
	IrNode * div = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tdiv,
		NULL,
		0
		);
	IrNode * bar = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"volume",
		2
		);
    foo->physics = newtonApiGetPhysicsTypeByName(newton, foo->token->identifier);
	newtonApiAddLeaf(newton, leftTerm, foo);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, leftTerm, div);
    bar->physics = newtonApiGetPhysicsTypeByName(newton, bar->token->identifier);
	newtonApiAddLeaf(newton, leftTerm, bar);

	IrNode * add = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tplus,
		NULL,
		0
		);
	IrNode * rightTerm = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_PquantityTerm,
		NULL,
		0
		);
	IrNode * fizz = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"pressure",
		8
		);
	IrNode * div2 = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tdiv,
		NULL,
		0
		);
	IrNode * bazz = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"volume",
		2
		);
    fizz->physics = newtonApiGetPhysicsTypeByName(newton, fizz->token->identifier);
	newtonApiAddLeaf(newton, rightTerm, fizz);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, rightTerm, div2);
    bazz->physics = newtonApiGetPhysicsTypeByName(newton, bazz->token->identifier);
	newtonApiAddLeaf(newton, rightTerm, bazz);

	newtonApiAddLeaf(newton, root, leftTerm);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, add);
	newtonApiAddLeaf(newton, root, rightTerm);

	return root;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////// setup for pendulum_acceleration.nt ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
IrNode *
makeTestParameterTuplePendulumCase()
{
    State * newton = newtonApiInit("../../Examples/pendulum_acceleration.nt");
	IrNode *	root = genIrNode(newton,	kNewtonIrNodeType_PparameterTuple,
								 NULL /* left child */,
								 NULL /* right child */,
								 NULL /* source info */);
	IrNode * accelerationX = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"acceleration",
		1
		);
	accelerationX->physics = newtonApiGetPhysicsTypeByNameAndSubindex(newton, accelerationX->token->identifier, 0);
	accelerationX->physics->subindex = 0;
	newtonApiAddLeaf(newton, root, accelerationX);

	IrNode * accelerationY = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"acceleration",
		5
		);
	accelerationY->physics = newtonApiGetPhysicsTypeByNameAndSubindex(newton, accelerationY->token->identifier, 1);
	accelerationY->physics->subindex = 1;
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, accelerationY);

	IrNode * accelerationZ = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"acceleration",
		0
		);
	accelerationZ->physics = newtonApiGetPhysicsTypeByNameAndSubindex(newton, accelerationZ->token->identifier, 2);
	accelerationZ->physics->subindex = 2;
	newtonApiAddLeaf(newton, root, accelerationZ);
	newtonApiNumberParametersZeroToN(newton, root);
	return root;
}

/*
 * Constructs an example tree for the statement
 * foo / bar + fizz / bazz = foo / bar + fizz * bazz, where
 * acceleration foo, bar = 8;
 * time fizz, bazz = 2;
 *
 */
IrNode *
makeSampleIncorrectTestStatementPendulumCase()
{
    State * newton = newtonApiInit("../../Examples/pendulum_acceleration.nt");
	IrNode * root = genIrNode(newton,
							  kNewtonIrNodeType_PquantityStatement,
							  NULL,
							  NULL,
							  NULL);

	newtonApiAddLeaf(newton,
					 root,
					 makeSampleCorrectTestExpressionPendulumCase()
		);

	newtonApiAddLeafWithChainingSeqNoLexer(newton,
					 root,
					 genIrNode(newton,
							   kNewtonIrNodeType_Tequals,
							   NULL,
							   NULL,
							   NULL)
		);

	newtonApiAddLeaf(newton,
					 root,
					 makeSampleIncorrectTestExpressionPendulumCase()
		);
	return root;
}

/*
 * Constructs an example tree for the expression
 * foo / bar + fizz / bazz, where
 * acceleration foo, bar = 8;
 * time fizz, bazz = 2;
 */
IrNode *
makeSampleIncorrectTestExpressionPendulumCase()
{
    State * newton = newtonApiInit("../../Examples/pendulum_acceleration.nt");
	IrNode * root = genIrNode(newton,
							  kNewtonIrNodeType_PquantityExpression,
							  NULL,
							  NULL,
							  NULL);

	IrNode * leftTerm = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_PquantityTerm,
		NULL,
		0
		);
	IrNode * foo = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"acceleration",
		8
		);
	IrNode * div = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tdiv,
		NULL,
		0
		);
	IrNode * bar = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"time",
		2
		);
    foo->physics = newtonApiGetPhysicsTypeByName(newton, foo->token->identifier);
	foo->physics->subindex = 0;
	newtonApiAddLeaf(newton, leftTerm, foo);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, leftTerm, div);
    bar->physics = newtonApiGetPhysicsTypeByName(newton, bar->token->identifier);
	bar->physics->subindex = 1;
	newtonApiAddLeaf(newton, leftTerm, bar);

	IrNode * add = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tplus,
		NULL,
		0
		);
	IrNode * rightTerm = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_PquantityTerm,
		NULL,
		0
		);
	IrNode * fizz = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"acceleration",
		8
		);
	IrNode * mul = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tmul,
		NULL,
		0
		);
	IrNode * bazz = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"time",
		2
		);
    fizz->physics = newtonApiGetPhysicsTypeByName(newton, fizz->token->identifier);
	fizz->physics->subindex = 0;
	newtonApiAddLeaf(newton, rightTerm, fizz);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, rightTerm, mul);
    bazz->physics = newtonApiGetPhysicsTypeByName(newton, bazz->token->identifier);
	bazz->physics->subindex = 1;
	newtonApiAddLeaf(newton, rightTerm, bazz);

	newtonApiAddLeaf(newton, root, leftTerm);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, add);
	newtonApiAddLeaf(newton, root, rightTerm);

	return root;
}

/*
 * Constructs an example tree for the statement
 * foo / bar + fizz / bazz = foo / bar + fizz / bazz, where
 * acceleration foo, bar = 8;
 * time fizz, bazz = 2;
 *
 */
IrNode *
makeSampleCorrectTestStatementPendulumCase()
{
    State * newton = newtonApiInit("../../Examples/pendulum_acceleration.nt");
	IrNode * root = genIrNode(newton,
							  kNewtonIrNodeType_PquantityStatement,
							  NULL,
							  NULL,
							  NULL);

	newtonApiAddLeaf(newton,
					 root,
					 makeSampleCorrectTestExpressionPendulumCase()
		);

	newtonApiAddLeafWithChainingSeqNoLexer(newton,
					 root,
					 genIrNode(newton,
							   kNewtonIrNodeType_Tequals,
							   NULL,
							   NULL,
							   NULL)
		);

	newtonApiAddLeaf(newton,
					 root,
					 makeSampleCorrectTestExpressionPendulumCase()
		);
	return root;
}

/*
 * Constructs an example tree for the expression
 * foo / bar + fizz / bazz, where
 * acceleration foo, bar = 8;
 * time fizz, bazz = 2;
 */
IrNode *
makeSampleCorrectTestExpressionPendulumCase()
{
    State * newton = newtonApiInit("../../Examples/pendulum_acceleration.nt");
	IrNode * root = genIrNode(newton,
							  kNewtonIrNodeType_PquantityExpression,
							  NULL,
							  NULL,
							  NULL);

	IrNode * leftTerm = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_PquantityTerm,
		NULL,
		0
		);
	IrNode * foo = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"acceleration",
		8
		);
	IrNode * div = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tdiv,
		NULL,
		0
		);
	IrNode * bar = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"time",
		2
		);
    foo->physics = newtonApiGetPhysicsTypeByName(newton, foo->token->identifier);
	foo->physics->subindex = 0;
	newtonApiAddLeaf(newton, leftTerm, foo);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, leftTerm, div);
    bar->physics = newtonApiGetPhysicsTypeByName(newton, bar->token->identifier);
	bar->physics->subindex = 1;
	newtonApiAddLeaf(newton, leftTerm, bar);

	IrNode * add = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tplus,
		NULL,
		0
		);
	IrNode * rightTerm = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_PquantityTerm,
		NULL,
		0
		);
	IrNode * fizz = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"acceleration",
		8
		);
	IrNode * div2 = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tdiv,
		NULL,
		0
		);
	IrNode * bazz = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"time",
		2
		);
    fizz->physics = newtonApiGetPhysicsTypeByName(newton, fizz->token->identifier);
	fizz->physics->subindex = 0;
	newtonApiAddLeaf(newton, rightTerm, fizz);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, rightTerm, div2);
    bazz->physics = newtonApiGetPhysicsTypeByName(newton, bazz->token->identifier);
	bazz->physics->subindex = 1;
	newtonApiAddLeaf(newton, rightTerm, bazz);

	newtonApiAddLeaf(newton, root, leftTerm);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, add);
	newtonApiAddLeaf(newton, root, rightTerm);

	return root;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////// setup for invariants.nt ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * Constructs an example tree for the statement
 * foo / bar + fizz / bazz = foo / bar + fizz * bazz, where
 * distance foo, bar = 8;
 * time fizz, bazz = 2;
 *
 */
IrNode *
makeSampleIncorrectTestStatement()
{
    State * newton = newtonApiInit("../../Examples/invariants.nt");
	IrNode * root = genIrNode(newton,
							  kNewtonIrNodeType_PquantityStatement,
							  NULL,
							  NULL,
							  NULL);

	newtonApiAddLeaf(newton,
					 root,
					 makeSampleCorrectTestExpression()
		);

	newtonApiAddLeafWithChainingSeqNoLexer(newton,
					 root,
					 genIrNode(newton,
							   kNewtonIrNodeType_Tequals,
							   NULL,
							   NULL,
							   NULL)
		);

	newtonApiAddLeaf(newton,
					 root,
					 makeSampleIncorrectTestExpression()
		);
	return root;
}

/*
 * Constructs an example tree for the expression
 * foo / bar + fizz * bazz, where
 * distance foo, bar = 8;
 * time fizz, bazz = 2;
 */
IrNode *
makeSampleIncorrectTestExpression()
{
    State * newton = newtonApiInit("../../Examples/invariants.nt");
	IrNode * root = genIrNode(newton,
							  kNewtonIrNodeType_PquantityExpression,
							  NULL,
							  NULL,
							  NULL);

	IrNode * leftTerm = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_PquantityTerm,
		NULL,
		0
		);
	IrNode * foo = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"distance",
		8
		);
	IrNode * div = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tdiv,
		NULL,
		0
		);
	IrNode * bar = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"time",
		2
		);
    foo->physics = newtonApiGetPhysicsTypeByName(newton, foo->token->identifier);
	newtonApiAddLeaf(newton, leftTerm, foo);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, leftTerm, div);
    bar->physics = newtonApiGetPhysicsTypeByName(newton, bar->token->identifier);
	newtonApiAddLeaf(newton, leftTerm, bar);

	IrNode * add = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tplus,
		NULL,
		0
		);
	IrNode * rightTerm = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_PquantityTerm,
		NULL,
		0
		);
	IrNode * fizz = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"distance",
		8
		);
	IrNode * mul = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tmul,
		NULL,
		0
		);
	IrNode * bazz = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"time",
		2
		);
    fizz->physics = newtonApiGetPhysicsTypeByName(newton, fizz->token->identifier);
	newtonApiAddLeaf(newton, rightTerm, fizz);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, rightTerm, mul);
    bazz->physics = newtonApiGetPhysicsTypeByName(newton, bazz->token->identifier);
	newtonApiAddLeaf(newton, rightTerm, bazz);

	newtonApiAddLeaf(newton, root, leftTerm);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, add);
	newtonApiAddLeaf(newton, root, rightTerm);

	return root;
}

/*
 * Constructs an example tree for the statement
 * foo / bar + fizz / bazz = foo / bar + fizz / bazz, where
 * distance foo, bar = 8;
 * time fizz, bazz = 2;
 *
 */
IrNode *
makeSampleCorrectTestStatement()
{
    State * newton = newtonApiInit("../../Examples/invariants.nt");
	IrNode * root = genIrNode(newton,
							  kNewtonIrNodeType_PquantityStatement,
							  NULL,
							  NULL,
							  NULL);

	newtonApiAddLeaf(newton,
					 root,
					 makeSampleCorrectTestExpression()
		);

	newtonApiAddLeafWithChainingSeqNoLexer(newton,
					 root,
					 genIrNode(newton,
							   kNewtonIrNodeType_Tequals,
							   NULL,
							   NULL,
							   NULL)
		);

	newtonApiAddLeaf(newton,
					 root,
					 makeSampleCorrectTestExpression()
		);
	return root;
}

/*
 * Constructs an example tree for the expression
 * foo / bar + fizz / bazz, where
 * distance foo, bar = 8;
 * time fizz, bazz = 2;
 */
IrNode *
makeSampleCorrectTestExpression()
{
    State * newton = newtonApiInit("../../Examples/invariants.nt");
	IrNode * root = genIrNode(newton,
							  kNewtonIrNodeType_PquantityExpression,
							  NULL,
							  NULL,
							  NULL);

	IrNode * leftTerm = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_PquantityTerm,
		NULL,
		0
		);
	IrNode * foo = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"distance",
		8
		);
	IrNode * div = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tdiv,
		NULL,
		0
		);
	IrNode * bar = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"time",
		2
		);
    foo->physics = newtonApiGetPhysicsTypeByName(newton, foo->token->identifier);
	newtonApiAddLeaf(newton, leftTerm, foo);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, leftTerm, div);
    bar->physics = newtonApiGetPhysicsTypeByName(newton, bar->token->identifier);
	newtonApiAddLeaf(newton, leftTerm, bar);

	IrNode * add = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tplus,
		NULL,
		0
		);
	IrNode * rightTerm = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_PquantityTerm,
		NULL,
		0
		);
	IrNode * fizz = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"distance",
		8
		);
	IrNode * div2 = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tdiv,
		NULL,
		0
		);
	IrNode * bazz = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Tidentifier,
		"time",
		2
		);
    fizz->physics = newtonApiGetPhysicsTypeByName(newton, fizz->token->identifier);
	newtonApiAddLeaf(newton, rightTerm, fizz);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, rightTerm, div2);
    bazz->physics = newtonApiGetPhysicsTypeByName(newton, bazz->token->identifier);
	newtonApiAddLeaf(newton, rightTerm, bazz);

	newtonApiAddLeaf(newton, root, leftTerm);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, add);
	newtonApiAddLeaf(newton, root, rightTerm);

	return root;
}


IrNode *
makeTestParameterTuple(State * newton)
{
	IrNode *	root = genIrNode(newton,	kNewtonIrNodeType_PparameterTuple,
								 NULL /* left child */,
								 NULL /* right child */,
								 NULL /* source info */);
	IrNode * distanceParameter = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"distance",
		5
		);
	distanceParameter->physics = newtonApiGetPhysicsTypeByName(newton, distanceParameter->token->identifier);
	newtonApiAddLeaf(newton, root, distanceParameter);

	IrNode * timeParameter = makeIrNodeSetValue(
		newton,
		kNewtonIrNodeType_Pparameter,
		"time",
		6.6
		);
	timeParameter->physics = newtonApiGetPhysicsTypeByName(newton, timeParameter->token->identifier);
	newtonApiAddLeafWithChainingSeqNoLexer(newton, root, timeParameter);

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
