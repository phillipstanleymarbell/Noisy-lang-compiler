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

	[	kNewtonTimeStampKey						]	"kNewtonTimeStampKey",
	[	kNewtonTimeStampKeyCheckCgiCompletion				]	"kNewtonTimeStampKeyCheckCgiCompletion",
	[	kNewtonTimeStampKeyCheckRss					]	"kNewtonTimeStampKeyCheckRss",
	[	kNewtonTimeStampKeyConsolePrintBuffers				]	"kNewtonTimeStampKeyConsolePrintBuffers",
	[	kNewtonTimeStampKeyDimensionPassParse				]	"kNewtonTimeStampKeyDimensionPassParse",
	[	kNewtonTimeStampKeyDimensionPassParseBaseSignal			]	"kNewtonTimeStampKeyDimensionPassParseBaseSignal",
	[	kNewtonTimeStampKeyDimensionPassParseFile			]	"kNewtonTimeStampKeyDimensionPassParseFile",
	[	kNewtonTimeStampKeyDimensionPassParseName			]	"kNewtonTimeStampKeyDimensionPassParseName",
	[	kNewtonTimeStampKeyDimensionPassParseStatement			]	"kNewtonTimeStampKeyDimensionPassParseStatement",
	[	kNewtonTimeStampKeyDimensionPassParseStatementList		]	"kNewtonTimeStampKeyDimensionPassParseStatementList",
	[	kNewtonTimeStampKeyDimensionPassParseSubindex			]	"kNewtonTimeStampKeyDimensionPassParseSubindex",
	[	kNewtonTimeStampKeyDimensionPassParseSubindexTuple		]	"kNewtonTimeStampKeyDimensionPassParseSubindexTuple",
	[	kNewtonTimeStampKeyIrPassAstDotPrintWalk			]	"kNewtonTimeStampKeyIrPassAstDotPrintWalk",
	[	kNewtonTimeStampKeyIrPassCBackend				]	"kNewtonTimeStampKeyIrPassCBackend",
	[	kNewtonTimeStampKeyIrPassCConstraintTreeWalk			]	"kNewtonTimeStampKeyIrPassCConstraintTreeWalk",
	[	kNewtonTimeStampKeyIrPassCCountRemainingParameters		]	"kNewtonTimeStampKeyIrPassCCountRemainingParameters",
	[	kNewtonTimeStampKeyIrPassCGenFunctionArgument			]	"kNewtonTimeStampKeyIrPassCGenFunctionArgument",
	[	kNewtonTimeStampKeyIrPassCGenFunctionBody			]	"kNewtonTimeStampKeyIrPassCGenFunctionBody",
	[	kNewtonTimeStampKeyIrPassCGenFunctionName			]	"kNewtonTimeStampKeyIrPassCGenFunctionName",
	[	kNewtonTimeStampKeyIrPassCIsConstraintHumanWritten		]	"kNewtonTimeStampKeyIrPassCIsConstraintHumanWritten",
	[	kNewtonTimeStampKeyIrPassCIsExpectedTypePresentInRightChild	]	"kNewtonTimeStampKeyIrPassCIsExpectedTypePresentInRightChild",
	[	kNewtonTimeStampKeyIrPassCNodeToStr				]	"kNewtonTimeStampKeyIrPassCNodeToStr",
	[	kNewtonTimeStampKeyIrPassCProcessInvariantList			]	"kNewtonTimeStampKeyIrPassCProcessInvariantList",
	[	kNewtonTimeStampKeyIrPassCSearchAndPrintNodeType		]	"kNewtonTimeStampKeyIrPassCSearchAndPrintNodeType",
	[	kNewtonTimeStampKeyIrPassDimensionalMatrixKernelPrinter		]	"kNewtonTimeStampKeyIrPassDimensionalMatrixKernelPrinter",
	[	kNewtonTimeStampKeyIrPassDotAstDotFmt				]	"kNewtonTimeStampKeyIrPassDotAstDotFmt",
	[	kNewtonTimeStampKeyIrPassDotBackend				]	"kNewtonTimeStampKeyIrPassDotBackend",
	[	kNewtonTimeStampKeyIrPassDotIsType				]	"kNewtonTimeStampKeyIrPassDotIsType",
	[	kNewtonTimeStampKeyIrPassDotScope2Id				]	"kNewtonTimeStampKeyIrPassDotScope2Id",
	[	kNewtonTimeStampKeyIrPassDotScope2Id2				]	"kNewtonTimeStampKeyIrPassDotScope2Id2",
	[	kNewtonTimeStampKeyIrPassDotSymbol2Id				]	"kNewtonTimeStampKeyIrPassDotSymbol2Id",
	[	kNewtonTimeStampKeyIrPassDotSymbotTableDotFmt			]	"kNewtonTimeStampKeyIrPassDotSymbotTableDotFmt",
	[	kNewtonTimeStampKeyIrPassHelperColorIr				]	"kNewtonTimeStampKeyIrPassHelperColorIr",
	[	kNewtonTimeStampKeyIrPassHelperColorSymbolTable			]	"kNewtonTimeStampKeyIrPassHelperColorSymbolTable",
	[	kNewtonTimeStampKeyIrPassHelperIrSize				]	"kNewtonTimeStampKeyIrPassHelperIrSize",
	[	kNewtonTimeStampKeyIrPassHelperSymbolTableSize			]	"kNewtonTimeStampKeyIrPassHelperSymbolTableSize",
	[	kNewtonTimeStampKeyIrPassSymbolTableDotPrintWalk		]	"kNewtonTimeStampKeyIrPassSymbolTableDotPrintWalk",
	[	kNewtonTimeStampKeyNewtonInit					]	"kNewtonTimeStampKeyNewtonInit",
	[	kNewtonTimeStampKeyParse					]	"kNewtonTimeStampKeyParse",
	[	kNewtonTimeStampKeyParseBaseSignal				]	"kNewtonTimeStampKeyParseBaseSignal",
	[	kNewtonTimeStampKeyParseConstant				]	"kNewtonTimeStampKeyParseConstant",
	[	kNewtonTimeStampKeyParseConstraint				]	"kNewtonTimeStampKeyParseConstraint",
	[	kNewtonTimeStampKeyParseDerivation				]	"kNewtonTimeStampKeyParseDerivation",
	[	kNewtonTimeStampKeyParseFile					]	"kNewtonTimeStampKeyParseFile",
	[	kNewtonTimeStampKeyParseFindNodeByParameterNumberAndSubindex	]	"kNewtonTimeStampKeyParseFindNodeByParameterNumberAndSubindex",
	[	kNewtonTimeStampKeyParseFindNodeByPhysicsId			]	"kNewtonTimeStampKeyParseFindNodeByPhysicsId",
	[	kNewtonTimeStampKeyParseFindParameterByTokenString		]	"kNewtonTimeStampKeyParseFindParameterByTokenString",
	[	kNewtonTimeStampKeyParseGetPhysicsByBoundIdentifier		]	"kNewtonTimeStampKeyParseGetPhysicsByBoundIdentifier",
	[	kNewtonTimeStampKeyParseIdentifier				]	"kNewtonTimeStampKeyParseIdentifier",
	[	kNewtonTimeStampKeyParseIdentifierDefinitionTerminal		]	"kNewtonTimeStampKeyParseIdentifierDefinitionTerminal",
	[	kNewtonTimeStampKeyParseIdentifierUsageTerminal			]	"kNewtonTimeStampKeyParseIdentifierUsageTerminal",
	[	kNewtonTimeStampKeyParseInvariant				]	"kNewtonTimeStampKeyParseInvariant",
	[	kNewtonTimeStampKeyParseName					]	"kNewtonTimeStampKeyParseName",
	[	kNewtonTimeStampKeyParseParameter				]	"kNewtonTimeStampKeyParseParameter",
	[	kNewtonTimeStampKeyParseParameterTuple				]	"kNewtonTimeStampKeyParseParameterTuple",
	[	kNewtonTimeStampKeyParseRule					]	"kNewtonTimeStampKeyParseRule",
	[	kNewtonTimeStampKeyParseRuleList				]	"kNewtonTimeStampKeyParseRuleList",
	[	kNewtonTimeStampKeyParseSubindex				]	"kNewtonTimeStampKeyParseSubindex",
	[	kNewtonTimeStampKeyParseSubindexTuple				]	"kNewtonTimeStampKeyParseSubindexTuple",
	[	kNewtonTimeStampKeyParseSymbol					]	"kNewtonTimeStampKeyParseSymbol",
	[	kNewtonTimeStampKeyParseTerminal				]	"kNewtonTimeStampKeyParseTerminal",
	[	kNewtonTimeStampKeyParserErrorRecovery				]	"kNewtonTimeStampKeyParserErrorRecovery",
	[	kNewtonTimeStampKeyParserSemanticError				]	"kNewtonTimeStampKeyParserSemanticError",
	[	kNewtonTimeStampKeyParserSyntaxError				]	"kNewtonTimeStampKeyParserSyntaxError",
	[	kNewtonTimeStampKeyPrintToFile					]	"kNewtonTimeStampKeyPrintToFile",
	[	kNewtonTimeStampKeyRenderDotInFile				]	"kNewtonTimeStampKeyRenderDotInFile",
	[	kNewtonTimeStampKeyRunPasses					]	"kNewtonTimeStampKeyRunPasses",
	[	kNewtonTimeStampKeyUnknown					]	"kNewtonTimeStampKeyUnknown",
	[	kNewtonTimeStampKeyParseUnitExpression				]	"kNewtonTimeStampKeyParseUnitExpression",
	[	kNewtonTimeStampKeyParseUnitTerm				]	"kNewtonTimeStampKeyParseUnitTerm",
	[	kNewtonTimeStampKeySetPhysicsOfBaseNode				]	"kNewtonTimeStampKeySetPhysicsOfBaseNode",
	[	kNewtonTimeStampKeyParseUnitFactor				]	"kNewtonTimeStampKeyParseUnitFactor",
	[	kNewtonTimeStampKeyParseUnit					]	"kNewtonTimeStampKeyParseUnit",
	[	kNewtonTimeStampKeyParseQuantityExpression			]	"kNewtonTimeStampKeyParseQuantityExpression",


#	include "common-timeStamps-keystrings.h"

};
