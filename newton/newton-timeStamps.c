/*
	Authored 2018. Phillip Stanley-Marbell.

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
#include <string.h>
#include <setjmp.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "newton-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"



const char *	TimeStampKeyStrings[kCommonTimeStampKeyMax] =
{
	/*
	 *	Generated from body of kNewtonTimeStampKey enum by piping through:
	 *
	 *		grep 'kNewton' | grep -v kNewtonTimeStampKeyMax| awk -F',' '{print "\t["$1"]\t\t\""$1"\","}'
	 */

	[	kNewtonTimeStampKeyParse					]	"kNewtonTimeStampKeyParse",
	[	kNewtonTimeStampKeyParseFile					]	"kNewtonTimeStampKeyParseFile",
	[	kNewtonTimeStampKeyParseRuleList				]	"kNewtonTimeStampKeyParseRuleList",
	[	kNewtonTimeStampKeyParseRule					]	"kNewtonTimeStampKeyParseRule",
	[	kNewtonTimeStampKeyParseInvariant				]	"kNewtonTimeStampKeyParseInvariant",
	[	kNewtonTimeStampKeyParseSubindex				]	"kNewtonTimeStampKeyParseSubindex",
	[	kNewtonTimeStampKeyParseSubindexTuple				]	"kNewtonTimeStampKeyParseSubindexTuple",
	[	kNewtonTimeStampKeyParseParameterTuple				]	"kNewtonTimeStampKeyParseParameterTuple",
	[	kNewtonTimeStampKeyParseParameter				]	"kNewtonTimeStampKeyParseParameter",
	[	kNewtonTimeStampKeyParseConstant				]	"kNewtonTimeStampKeyParseConstant",
	[	kNewtonTimeStampKeyParseBaseSignal				]	"kNewtonTimeStampKeyParseBaseSignal",
	[	kNewtonTimeStampKeyParseName					]	"kNewtonTimeStampKeyParseName",
	[	kNewtonTimeStampKeyParseSymbol					]	"kNewtonTimeStampKeyParseSymbol",
	[	kNewtonTimeStampKeyParseDerivation				]	"kNewtonTimeStampKeyParseDerivation",
	[	kNewtonTimeStampKeyParseTerminal				]	"kNewtonTimeStampKeyParseTerminal",
	[	kNewtonTimeStampKeyParseIdentifier				]	"kNewtonTimeStampKeyParseIdentifier",
	[	kNewtonTimeStampKeyParseFindNodeByPhysicsId			]	"kNewtonTimeStampKeyParseFindNodeByPhysicsId",
	[	kNewtonTimeStampKeyParseFindNodeByParameterNumberAndSubindex	]	"kNewtonTimeStampKeyParseFindNodeByParameterNumberAndSubindex",
	[	kNewtonTimeStampKeyParseFindParameterByTokenString		]	"kNewtonTimeStampKeyParseFindParameterByTokenString",
	[	kNewtonTimeStampKeyParseGetPhysicsByBoundIdentifier		]	"kNewtonTimeStampKeyParseGetPhysicsByBoundIdentifier",
	[	kNewtonTimeStampKeyParseIdentifierUsageTerminal			]	"kNewtonTimeStampKeyParseIdentifierUsageTerminal",
	[	kNewtonTimeStampKeyParseConstraint				]	"kNewtonTimeStampKeyParseConstraint",
	[	kNewtonTimeStampKeyParseIdentifierDefinitionTerminal		]	"kNewtonTimeStampKeyParseIdentifierDefinitionTerminal",
	[	kNewtonTimeStampKeyParserSyntaxError				]	"kNewtonTimeStampKeyParserSyntaxError",
	[	kNewtonTimeStampKeyParserSemanticError				]	"kNewtonTimeStampKeyParserSemanticError",
	[	kNewtonTimeStampKeyParserErrorRecovery				]	"kNewtonTimeStampKeyParserErrorRecovery",

	[	kNewtonTimeStampKeyUnknown					]	"kNewtonTimeStampKeyUnknown",
};
