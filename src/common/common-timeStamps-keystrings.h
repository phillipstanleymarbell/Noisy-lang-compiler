/*
	Authored 2015-2019. Phillip Stanley-Marbell.

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
	 *	For inclusion in TimeStampKeyStrings[kCommonTimeStampKeyMax]. Generated from body of kNewtonTimeStampKey enum by piping through:
	 *
	 *		grep 'kCommon' | grep -v kCommonTimeStampKeyMax| awk -F',' '{print "\t["$1"]\t\t\""$1"\","}'
	 */
	[	kCommonTimeStampKeyMin]					"kCommonTimeStampKeyMin",

	[	kCommonTimeStampKeyCheckCgiCompletion]			"kCommonTimeStampKeyCheckCgiCompletion",
	[	kCommonTimeStampKeyCheckRss]				"kCommonTimeStampKeyCheckRss",
	[	kCommonTimeStampKeyCommonInit]				"kCommonTimeStampKeyCommonInit",
	[	kCommonTimeStampKeyConsolePrintBuffers]			"kCommonTimeStampKeyConsolePrintBuffers",
	[	kCommonTimeStampKeyLexAllocateSourceInfo]		"kCommonTimeStampKeyLexAllocateSourceInfo",
	[	kCommonTimeStampKeyLexAllocateToken]			"kCommonTimeStampKeyLexAllocateToken",
	[	kCommonTimeStampKeyLexDebugPrintToken]			"kCommonTimeStampKeyLexDebugPrintToken",
	[	kCommonTimeStampKeyLexGet]				"kCommonTimeStampKeyLexGet",
	[	kCommonTimeStampKeyLexPeekPrint]			"kCommonTimeStampKeyLexPeekPrint",
	[	kCommonTimeStampKeyLexPeek]				"kCommonTimeStampKeyLexPeek",
	[	kCommonTimeStampKeyLexPrintToken]			"kCommonTimeStampKeyLexPrintToken",
	[	kCommonTimeStampKeyLexPut]				"kCommonTimeStampKeyLexPut",
	[	kCommonTimeStampKeyLexerCheckTokenLength]		"kCommonTimeStampKeyLexerCheckTokenLength",
	[	kCommonTimeStampKeyLexerCur]				"kCommonTimeStampKeyLexerCur",
	[	kCommonTimeStampKeyLexerDone]				"kCommonTimeStampKeyLexerDone",
	[	kCommonTimeStampKeyLexerEqf]				"kCommonTimeStampKeyLexerEqf",
	[	kCommonTimeStampKeyLexerGobble]				"kCommonTimeStampKeyLexerGobble",
	[	kCommonTimeStampKeyLexerIsDecimalSeparatedWithChar]	"kCommonTimeStampKeyLexerIsDecimalSeparatedWithChar",
	[	kCommonTimeStampKeyLexerIsDecimal]			"kCommonTimeStampKeyLexerIsDecimal",
	[	kCommonTimeStampKeyLexerIsEngineeringRealConst]		"kCommonTimeStampKeyLexerIsEngineeringRealConst",
	[	kCommonTimeStampKeyLexerIsHexConstWithoutLeading0x]	"kCommonTimeStampKeyLexerIsHexConstWithoutLeading0x",
	[	kCommonTimeStampKeyLexerIsRadixConst]			"kCommonTimeStampKeyLexerIsRadixConst",
	[	kCommonTimeStampKeyLexerIsRealConst]			"kCommonTimeStampKeyLexerIsRealConst",
	[	kCommonTimeStampKeyLexerStringAtLeft]			"kCommonTimeStampKeyLexerStringAtLeft",
	[	kCommonTimeStampKeyLexerStringAtRight]			"kCommonTimeStampKeyLexerStringAtRight",
	[	kCommonTimeStampKeyLexerStringToEngineeringRealConst]	"kCommonTimeStampKeyLexerStringToEngineeringRealConst",
	[	kCommonTimeStampKeyLexerStringToRealConst]		"kCommonTimeStampKeyLexerStringToRealConst",
	[	kCommonTimeStampKeyPrintToFile]				"kCommonTimeStampKeyPrintToFile",
	[	kCommonTimeStampKeyRenderDotInFile]			"kCommonTimeStampKeyRenderDotInFile",
	[	kNCommonTimeStampKeyLexerStringToRadixConst]		"kNCommonTimeStampKeyLexerStringToRadixConst",
	[	kCommonTimeStampKeyTimeStampInit]			"kCommonTimeStampKeyTimeStampInit",

	[	kCommonTimeStampKeyError]				"kCommonTimeStampKeyError",
	[	kCommonTimeStampKeyFatal]				"kCommonTimeStampKeyFatal",
