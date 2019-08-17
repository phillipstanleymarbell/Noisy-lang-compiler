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



/*
 *	NOTE:   Add entries here for new locations we want to time stamp; all
 *	functions in the implementation should ideally have entries here.
 */
typedef enum
{
	/*
	 *	Catchall default
	 */
	kNewtonTimeStampKey,

	/*
	 *	Dimension pre-scan
	 */
	kNewtonTimeStampKeyDimensionPassParse,
	kNewtonTimeStampKeyDimensionPassParseFile,
	kNewtonTimeStampKeyDimensionPassParseStatementList,
	kNewtonTimeStampKeyDimensionPassParseStatement,
	kNewtonTimeStampKeyDimensionPassParseSubindex,
	kNewtonTimeStampKeyDimensionPassParseSubindexTuple,
	kNewtonTimeStampKeyDimensionPassParseBaseSignal,
	kNewtonTimeStampKeyDimensionPassParseName,

	/*
	 *	C backend
	 */
	kNewtonTimeStampKeyIrPassCIsExpectedTypePresentInRightChild,
	kNewtonTimeStampKeyIrPassCIsConstraintHumanWritten,
	kNewtonTimeStampKeyIrPassCNodeToStr,
	kNewtonTimeStampKeyIrPassCSearchAndPrintNodeType,
	kNewtonTimeStampKeyIrPassCCountRemainingParameters,
	kNewtonTimeStampKeyIrPassCConstraintTreeWalk,
	kNewtonTimeStampKeyIrPassCGenFunctionBody,
	kNewtonTimeStampKeyIrPassCGenFunctionArgument,
	kNewtonTimeStampKeyIrPassCGenFunctionName,
	kNewtonTimeStampKeyIrPassCProcessInvariantList,
	kNewtonTimeStampKeyIrPassCBackend,


	/*
	 *	Dimensional Matrix Kernel Printer
	 */
	kNewtonTimeStampKeyIrPassDimensionalMatrixKernelPrinter,


	/*
	 *	Parser global routines in newton-parser.c.
	 */
	kNewtonTimeStampKeyParse,
	kNewtonTimeStampKeyParseFile,
	kNewtonTimeStampKeyParseRuleList,
	kNewtonTimeStampKeyParseRule,
	kNewtonTimeStampKeyParseInvariant,
	kNewtonTimeStampKeyParseSubindex,
	kNewtonTimeStampKeyParseSubindexTuple,
	kNewtonTimeStampKeyParseParameterTuple,
	kNewtonTimeStampKeyParseParameter,
	kNewtonTimeStampKeyParseConstant,
	kNewtonTimeStampKeyParseBaseSignal,
	kNewtonTimeStampKeyParseName,
	kNewtonTimeStampKeyParseSymbol,
	kNewtonTimeStampKeyParseDerivation,
	kNewtonTimeStampKeyParseTerminal,
	kNewtonTimeStampKeyParseIdentifier,
	kNewtonTimeStampKeyParseFindNodeByPhysicsId,
	kNewtonTimeStampKeyParseFindNodeByParameterNumberAndSubindex,
	kNewtonTimeStampKeyParseFindParameterByTokenString,
	kNewtonTimeStampKeyParseGetPhysicsByBoundIdentifier,
	kNewtonTimeStampKeyParseIdentifierUsageTerminal,
	kNewtonTimeStampKeyParseConstraint,
	kNewtonTimeStampKeyParseIdentifierDefinitionTerminal,
	kNewtonTimeStampKeyParseUnitExpression,
	kNewtonTimeStampKeyParseUnitTerm,
	kNewtonTimeStampKeySetPhysicsOfBaseNode,
	kNewtonTimeStampKeyParseUnitFactor,
	kNewtonTimeStampKeyParseUnit,
	kNewtonTimeStampKeyParseQuantityExpression,

	/*
	 *	Parser local routines in newton-parser.c.
	 */
	kNewtonTimeStampKeyParserSyntaxError,
	kNewtonTimeStampKeyParserSemanticError,
	kNewtonTimeStampKeyParserErrorRecovery,


	/*
	 *	Symbol table routines in newton-symbolTable.c.
	 */


	/*
	 *	Ir helper routines in common-irHelpers.c.
	 */


	/*
	 *	Lexer public routines in newton-lexer.c.
	 */

	/*
	 *	Lexer local routines in newton-lexer.c.
	 */


	/*
	 *	First/Follow set routines in newton-firstAndFollow.c.
	 */


	/*
	 *	Type-related routines in newton-types.c
	 */


	/*
	 *	Miscellaneous platform and glue routines in main.c and cgimain.c.
	 */
	kNewtonTimeStampKeyNewtonInit,
	kNewtonTimeStampKeyRunPasses,
	kNewtonTimeStampKeyCheckRss,
	kNewtonTimeStampKeyConsolePrintBuffers,
	kNewtonTimeStampKeyPrintToFile,
	kNewtonTimeStampKeyRenderDotInFile,
	kNewtonTimeStampKeyCheckCgiCompletion,


	/*
	 *	Public routines in Dotbackend
	 */
	kNewtonTimeStampKeyIrPassDotAstDotFmt,
	kNewtonTimeStampKeyIrPassDotSymbotTableDotFmt,
	kNewtonTimeStampKeyIrPassAstDotPrintWalk,
	kNewtonTimeStampKeyIrPassSymbolTableDotPrintWalk,
	kNewtonTimeStampKeyIrPassDotBackend,

	/*
	 *	Private routines in Dotbackend
	 */
	kNewtonTimeStampKeyIrPassDotIsType,
	kNewtonTimeStampKeyIrPassDotScope2Id,
	kNewtonTimeStampKeyIrPassDotScope2Id2,
	kNewtonTimeStampKeyIrPassDotSymbol2Id,


	/*
	 *	IR pass helpers. TODO: should merge this into IR helpers...
	 */
	kNewtonTimeStampKeyIrPassHelperColorIr,
	kNewtonTimeStampKeyIrPassHelperColorSymbolTable,
	kNewtonTimeStampKeyIrPassHelperIrSize,
	kNewtonTimeStampKeyIrPassHelperSymbolTableSize,

	/*
	 *	Used to tag un-tracked time.
	 */
	kNewtonTimeStampKeyUnknown,


#	include "common-timeStamps-keys.h"
} TimeStampKey;
