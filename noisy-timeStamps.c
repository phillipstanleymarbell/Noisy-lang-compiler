/*
	Authored 2015. Phillip Stanley-Marbell.

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
#ifdef NoisyOsMacOSX
#	include <mach/mach.h>
#	include <mach/mach_time.h>
#	include <unistd.h>
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisy-errors.h"
#include "version.h"
#include "noisy-timeStamps.h"
#include "noisy.h"



const char *	NoisyTimeStampKeyStrings[kNoisyTimeStampKeyMax] =
{
	/*
	 *	Generated from body of kNoisyTimeStampKey enum by piping through
	 *
	 *		grep 'kNoisy' | grep -v kNoisyTimeStampKeyMax| awk -F',' '{print "\t["$1"]\t\t\""$1"\","}'
	 */
	[kNoisyTimeStampKeyParse]					"kNoisyTimeStampKeyParse",
	[kNoisyTimeStampKeyParseProgram]				"kNoisyTimeStampKeyParseProgram",
	[kNoisyTimeStampKeyParseProgtypeDeclaration]			"kNoisyTimeStampKeyParseProgtypeDeclaration",
	[kNoisyTimeStampKeyParseProgtypeBody]				"kNoisyTimeStampKeyParseProgtypeBody",
	[kNoisyTimeStampKeyParseProgtypeTypenameDeclaration]		"kNoisyTimeStampKeyParseProgtypeTypenameDeclaration",
	[kNoisyTimeStampKeyParseConstantDeclaration]			"kNoisyTimeStampKeyParseConstantDeclaration",
	[kNoisyTimeStampKeyParseTypeDeclaration]			"kNoisyTimeStampKeyParseTypeDeclaration",
	[kNoisyTimeStampKeyParseAdtTypeDeclaration]			"kNoisyTimeStampKeyParseAdtTypeDeclaration",
	[kNoisyTimeStampKeyParseNamegenDeclaration]			"kNoisyTimeStampKeyParseNamegenDeclaration",
	[kNoisyTimeStampKeyParseIdentifierOrNil]			"kNoisyTimeStampKeyParseIdentifierOrNil",
	[kNoisyTimeStampKeyParseIdentifierOrNilList]			"kNoisyTimeStampKeyParseIdentifierOrNilList",
	[kNoisyTimeStampKeyParseIdentifierList]				"kNoisyTimeStampKeyParseIdentifierList",
	[kNoisyTimeStampKeyParseTypeExpression]				"kNoisyTimeStampKeyParseTypeExpression",
	[kNoisyTimeStampKeyParseTypeName]				"kNoisyTimeStampKeyParseTypeName",
	[kNoisyTimeStampKeyParseTolerance]				"kNoisyTimeStampKeyParseTolerance",
	[kNoisyTimeStampKeyParseErrorMagnitudeTolerance]		"kNoisyTimeStampKeyParseErrorMagnitudeTolerance",
	[kNoisyTimeStampKeyParseLossTolerance]				"kNoisyTimeStampKeyParseLossTolerance",
	[kNoisyTimeStampKeyParseLatencyTolerance]			"kNoisyTimeStampKeyParseLatencyTolerance",
	[kNoisyTimeStampKeyParseBasicType]				"kNoisyTimeStampKeyParseBasicType",
	[kNoisyTimeStampKeyParseRealType]				"kNoisyTimeStampKeyParseRealType",
	[kNoisyTimeStampKeyParseFixedType]				"kNoisyTimeStampKeyParseFixedType",
	[kNoisyTimeStampKeyParseAnonAggregateType]			"kNoisyTimeStampKeyParseAnonAggregateType",
	[kNoisyTimeStampKeyParseArrayType]				"kNoisyTimeStampKeyParseArrayType",
	[kNoisyTimeStampKeyParseListType]				"kNoisyTimeStampKeyParseListType",
	[kNoisyTimeStampKeyParseTupleType]				"kNoisyTimeStampKeyParseTupleType",
	[kNoisyTimeStampKeyParseSetType]				"kNoisyTimeStampKeyParseSetType",
	[kNoisyTimeStampKeyParseInitList]				"kNoisyTimeStampKeyParseInitList",
	[kNoisyTimeStampKeyParseIdxInitList]				"kNoisyTimeStampKeyParseIdxInitList",
	[kNoisyTimeStampKeyParseStarInitList]				"kNoisyTimeStampKeyParseStarInitList",
	[kNoisyTimeStampKeyParseElement]				"kNoisyTimeStampKeyParseElement",
	[kNoisyTimeStampKeyParseNamegenDefinition]			"kNoisyTimeStampKeyParseNamegenDefinition",
	[kNoisyTimeStampKeyParseScopedStatementList]			"kNoisyTimeStampKeyParseScopedStatementList",
	[kNoisyTimeStampKeyParseStatementList]				"kNoisyTimeStampKeyParseStatementList",
	[kNoisyTimeStampKeyParseStatement]				"kNoisyTimeStampKeyParseStatement",
	[kNoisyTimeStampKeyParseAssignOp]				"kNoisyTimeStampKeyParseAssignOp",
	[kNoisyTimeStampKeyParseMatchStatement]				"kNoisyTimeStampKeyParseMatchStatement",
	[kNoisyTimeStampKeyParseIterStatement]				"kNoisyTimeStampKeyParseIterStatement",
	[kNoisyTimeStampKeyParseGuardBody]				"kNoisyTimeStampKeyParseGuardBody",
	[kNoisyTimeStampKeyParseExpression]				"kNoisyTimeStampKeyParseExpression",
	[kNoisyTimeStampKeyParseListCastExpression]			"kNoisyTimeStampKeyParseListCastExpression",
	[kNoisyTimeStampKeyParseSetCastExpression]			"kNoisyTimeStampKeyParseSetCastExpression",
	[kNoisyTimeStampKeyParseArrayCastExpression]			"kNoisyTimeStampKeyParseArrayCastExpression",
	[kNoisyTimeStampKeyParseAnonAggregateCastExpression]		"kNoisyTimeStampKeyParseAnonAggregateCastExpression",
	[kNoisyTimeStampKeyParseChanEventExpression]			"kNoisyTimeStampKeyParseChanEventExpression",
	[kNoisyTimeStampKeyParseChan2nameExpression]			"kNoisyTimeStampKeyParseChan2nameExpression",
	[kNoisyTimeStampKeyParseVar2nameExpression]			"kNoisyTimeStampKeyParseVar2nameExpression",
	[kNoisyTimeStampKeyParseName2chanExpression]			"kNoisyTimeStampKeyParseName2chanExpression",
	[kNoisyTimeStampKeyParseTerm]					"kNoisyTimeStampKeyParseTerm",
	[kNoisyTimeStampKeyParseFactor]					"kNoisyTimeStampKeyParseFactor",
	[kNoisyTimeStampKeyParseFieldSelect]				"kNoisyTimeStampKeyParseFieldSelect",
	[kNoisyTimeStampKeyParseHighPrecedenceBinaryO]			"kNoisyTimeStampKeyParseHighPrecedenceBinaryO",
	[kNoisyTimeStampKeyParseLowPrecedenceBinaryOp]			"kNoisyTimeStampKeyParseLowPrecedenceBinaryOp",
	[kNoisyTimeStampKeyParseCmpOp]					"kNoisyTimeStampKeyParseCmpOp",
	[kNoisyTimeStampKeyParseBooleanOp]				"kNoisyTimeStampKeyParseBooleanOp",
	[kNoisyTimeStampKeyParseUnaryOp]				"kNoisyTimeStampKeyParseUnaryOp",
	[kNoisyTimeStampKeyParseTerminal]				"kNoisyTimeStampKeyParseTerminal",
	[kNoisyTimeStampKeyParseIdentifierUsageTerminal]		"kNoisyTimeStampKeyParseIdentifierUsageTerminal",
	[kNoisyTimeStampKeyParseIdentifierDefinitionTerminal]		"kNoisyTimeStampKeyParseIdentifierDefinitionTerminal",
	[kNoisyTimeStampKeyParserSyntaxError]				"kNoisyTimeStampKeyParserSyntaxError",
	[kNoisyTimeStampKeyParserSemanticError]				"kNoisyTimeStampKeyParserSemanticError",
	[kNoisyTimeStampKeyParserErrorRecovery]				"kNoisyTimeStampKeyParserErrorRecovery",
	[kNoisyTimeStampKeyParserProgtypeName2scope]			"kNoisyTimeStampKeyParserProgtypeName2scope",
	[kNoisyTimeStampKeyParserErrorUseBeforeDefinition]		"kNoisyTimeStampKeyParserErrorUseBeforeDefinition",
	[kNoisyTimeStampKeyParserErrorMultiDefinition]			"kNoisyTimeStampKeyParserErrorMultiDefinition",
	[kNoisyTimeStampKeyParserTermSyntaxError]			"kNoisyTimeStampKeyParserTermSyntaxError",
	[kNoisyTimeStampKeyParserTermErrorRecovery]			"kNoisyTimeStampKeyParserTermErrorRecovery",
	[kNoisyTimeStampKeyParserPeekCheck]				"kNoisyTimeStampKeyParserPeekCheck",
	[kNoisyTimeStampKeyParserDepthFirstWalk]			"kNoisyTimeStampKeyParserDepthFirstWalk",
	[kNoisyTimeStampKeyParserAddLeaf]				"kNoisyTimeStampKeyParserAddLeaf",
	[kNoisyTimeStampKeyParserAddLeafWithChainingSeq]		"kNoisyTimeStampKeyParserAddLeafWithChainingSeq",
	[kNoisyTimeStampKeyParserAddToProgtypeScopes]			"kNoisyTimeStampKeyParserAddToProgtypeScopes",
	[kNoisyTimeStampKeyParserAssignTypes]				"kNoisyTimeStampKeyParserAssignTypes",
	[kNoisyTimeStampKeySymbolTableAllocScope]			"kNoisyTimeStampKeySymbolTableAllocScope",
	[kNoisyTimeStampKeySymbolTableAddOrLookupSymbolForToken]	"kNoisyTimeStampKeySymbolTableAddOrLookupSymbolForToken",
	[kNoisyTimeStampKeySymbolTableSymbolForIdentifier]		"kNoisyTimeStampKeySymbolTableSymbolForIdentifier",
	[kNoisyTimeStampKeySymbolTableOpenScope]			"kNoisyTimeStampKeySymbolTableOpenScope",
	[kNoisyTimeStampKeySymbolTableCloseScope]			"kNoisyTimeStampKeySymbolTableCloseScope",
	[kNoisyTimeStampKeyGenNoisyIrNode]				"kNoisyTimeStampKeyGenNoisyIrNode",
	[kNoisyTimeStampKeyLexAllocateSourceInfo]			"kNoisyTimeStampKeyLexAllocateSourceInfo",
	[kNoisyTimeStampKeyLexAllocateToken]				"kNoisyTimeStampKeyLexAllocateToken",
	[kNoisyTimeStampKeyLexPut]					"kNoisyTimeStampKeyLexPut",
	[kNoisyTimeStampKeyLexGet]					"kNoisyTimeStampKeyLexGet",
	[kNoisyTimeStampKeyLexPeek]					"kNoisyTimeStampKeyLexPeek",
	[kNoisyTimeStampKeyLexInit]					"kNoisyTimeStampKeyLexInit",
	[kNoisyTimeStampKeyLexPrintToken]				"kNoisyTimeStampKeyLexPrintToken",
	[kNoisyTimeStampKeyLexerCheckTokenLength]			"kNoisyTimeStampKeyLexerCheckTokenLength",
	[kNoisyTimeStampKeyLexerCur]					"kNoisyTimeStampKeyLexerCur",
	[kNoisyTimeStampKeyLexerGobble]					"kNoisyTimeStampKeyLexerGobble",
	[kNoisyTimeStampKeyLexerDone]					"kNoisyTimeStampKeyLexerDone",
	[kNoisyTimeStampKeyLexerEqf]					"kNoisyTimeStampKeyLexerEqf",
	[kNoisyTimeStampKeyLexerCheckComment]				"kNoisyTimeStampKeyLexerCheckComment",
	[kNoisyTimeStampKeyLexerCheckWeq]				"kNoisyTimeStampKeyLexerCheckWeq",
	[kNoisyTimeStampKeyLexerCheckWeq3]				"kNoisyTimeStampKeyLexerCheckWeq3",
	[kNoisyTimeStampKeyLexerCheckSingle]				"kNoisyTimeStampKeyLexerCheckSingle",
	[kNoisyTimeStampKeyLexerCheckDot]				"kNoisyTimeStampKeyLexerCheckDot",
	[kNoisyTimeStampKeyLexerCheckGt]				"kNoisyTimeStampKeyLexerCheckGt",
	[kNoisyTimeStampKeyLexerCheckLt]				"kNoisyTimeStampKeyLexerCheckLt",
	[kNoisyTimeStampKeyLexerCheckSingleQuote]			"kNoisyTimeStampKeyLexerCheckSingleQuote",
	[kNoisyTimeStampKeyLexerCheckDoubleQuote]			"kNoisyTimeStampKeyLexerCheckDoubleQuote",
	[kNoisyTimeStampKeyLexerCheckMinus]				"kNoisyTimeStampKeyLexerCheckMinus",
	[kNoisyTimeStampKeyLexerFinishToken]				"kNoisyTimeStampKeyLexerFinishToken",
	[kNoisyTimeStampKeyLexerMakeNumericConst]			"kNoisyTimeStampKeyLexerMakeNumericConst",
	[kNoisyTimeStampKeyLexerIsDecimal]				"kNoisyTimeStampKeyLexerIsDecimal",
	[kNoisyTimeStampKeyLexerStringAtLeft]				"kNoisyTimeStampKeyLexerStringAtLeft",
	[kNoisyTimeStampKeyLexerStringAtRight]				"kNoisyTimeStampKeyLexerStringAtRight",
	[kNoisyTimeStampKeyLexerIsDecimalSeparatedWithChar]		"kNoisyTimeStampKeyLexerIsDecimalSeparatedWithChar",
	[kNoisyTimeStampKeyLexerIsRadixConst]				"kNoisyTimeStampKeyLexerIsRadixConst",
	[kNoisyTimeStampKeyLexerIsRealConst]				"kNoisyTimeStampKeyLexerIsRealConst",
	[kNoisyTimeStampKeyLexerIsEngineeringRealConst]			"kNoisyTimeStampKeyLexerIsEngineeringRealConst",
	[kNoisyTimeStampKeyLexerStringToRadixConst]			"kNoisyTimeStampKeyLexerStringToRadixConst",
	[kNoisyTimeStampKeyLexerStringToRealConst]			"kNoisyTimeStampKeyLexerStringToRealConst",
	[kNoisyTimeStampKeyLexerStringToEngineeringRealConst]		"kNoisyTimeStampKeyLexerStringToEngineeringRealConst",
	[kNoisyTimeStampKeyLexerIsOperatorOrSeparator]			"kNoisyTimeStampKeyLexerIsOperatorOrSeparator",
	[kNoisyTimeStampKeyInFirst]					"kNoisyTimeStampKeyInFirst",
	[kNoisyTimeStampKeyInFollow]					"kNoisyTimeStampKeyInFollow",
	[kNoisyTimeStampKeyTypeValidateIrSubtree]			"kNoisyTimeStampKeyTypeValidateIrSubtree",
	[kNoisyTimeStampKeyTypeEqualsSubtreeTypes]			"kNoisyTimeStampKeyTypeEqualsSubtreeTypes",
	[kNoisyTimeStampKeyTypeMakeTypeSignature]			"kNoisyTimeStampKeyTypeMakeTypeSignature",
	[kNoisyTimeStampKeyNoisyInit]					"kNoisyTimeStampKeyNoisyInit",
	[kNoisyTimeStampKeyRunPasses]					"kNoisyTimeStampKeyRunPasses",
	[kNoisyTimeStampKeyCheckRss]					"kNoisyTimeStampKeyCheckRss",
	[kNoisyTimeStampKeyConsolePrintBuffers]				"kNoisyTimeStampKeyConsolePrintBuffers",
	[kNoisyTimeStampKeyPrintToFile]					"kNoisyTimeStampKeyPrintToFile",
	[kNoisyTimeStampKeyRenderDotInFile]				"kNoisyTimeStampKeyRenderDotInFile",
	[kNoisyTimeStampKeyCheckCgiCompletion]				"kNoisyTimeStampKeyCheckCgiCompletion",
	[kNoisyTimeStampKeyFatal]					"kNoisyTimeStampKeyFatal",
	[kNoisyTimeStampKeyError]					"kNoisyTimeStampKeyError",

	[kNoisyTimeStampKeyUnknown]					"kNoisyTimeStampKeyUnknown",
};



//TODO: move this to libflex...
static uint64_t
machtimeToNanoseconds(uint64_t machTime)
{
#ifdef NoisyOsMacOSX
	static mach_timebase_info_data_t	sTimebaseInfo;


	if (sTimebaseInfo.denom == 0)
	{
		mach_timebase_info(&sTimebaseInfo);
	}

	//TODO: there might be multitplication overflow...
	return (machTime * sTimebaseInfo.numer / sTimebaseInfo.denom);
#else
	return 0;
#endif
}

void
noisyTimeStampDumpTimeline(NoisyState *  N)
{
	flexprint(N->Fe, N->Fm, N->Fpinfo, "\nNoisy routine invocation trace (%d calls):\n\n",
						N->timestampCount);
	for (int i = 0; i < N->timestampCount; i++)
	{
		flexprint(N->Fe, N->Fm, N->Fpinfo, "    %-6d\t(init + %-06.1f us) in routine %s\n",
							i, (double)machtimeToNanoseconds(N->timestamps[i].nanoseconds - N->initializationTimestamp)/1000.0,
							NoisyTimeStampKeyStrings[N->timestamps[i].key]);
	}
	flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");
}

void
noisyTimeStampDumpResidencies(NoisyState *  N)
{
	flexprint(N->Fe, N->Fm, N->Fpinfo, "\nNon-zero Noisy routine residency time upper bounds and counts (%d calls, total of %-02.4f us):\n\n",
						N->callAggregateTotal, (double)machtimeToNanoseconds(N->timeAggregateTotal)/1000.0);
	for (int i = 0; i < kNoisyTimeStampKeyMax; i++)
	{
		if (N->timeAggregates[i] == 0)
		{
			continue;
		}

		flexprint(N->Fe, N->Fm, N->Fpinfo, "    Routine %45s, %9d calls (%-02.4f %%), total residency = %f us (%-02.4f %%)\n",
							&NoisyTimeStampKeyStrings[i][strlen("kNoisyTimeStampKey")], N->callAggregates[i],
							100*(double)N->callAggregates[i]/(double)N->callAggregateTotal,
							(double)machtimeToNanoseconds(N->timeAggregates[i])/1000.0,
							100.0*((double)machtimeToNanoseconds(N->timeAggregates[i])/(double)machtimeToNanoseconds(N->timeAggregateTotal)));
	}
	flexprint(N->Fe, N->Fm, N->Fpinfo, "\n");

}
