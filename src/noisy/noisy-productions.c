/*
	Authored 2015-2018. Phillip Stanley-Marbell.

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
#include <stdlib.h>
#include <stdbool.h>
#include <setjmp.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisy-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"



/*
 *	Strings representing the various productions, for debugging and error reporting.
 */
const char *	gNoisyProductionDescriptions[kCommonIrNodeTypeMax] =
{
									[kNoisyIrNodeType_PaccuracyTolerance]		= "accuracy tolerance",
									[kNoisyIrNodeType_PadtTypeDecl]			= "ADT type declaration",
									[kNoisyIrNodeType_PanonAggrCastExpr]		= "anonymous aggregate (array/list/tuple or set) cast expression",
									[kNoisyIrNodeType_PanonAggregateType]		= "anonymous aggregate data type (array, list, tuple, or set)",
									[kNoisyIrNodeType_Parith2BoolOp]		= "arithmetic to Boolean operator",
									[kNoisyIrNodeType_ParithParamOrConst]		= "arithmetic constant",
									[kNoisyIrNodeType_ParrayCastExpr]		= "array cast expression",
									[kNoisyIrNodeType_ParrayType]			= "array data type",
									[kNoisyIrNodeType_PassignOp]			= "assignment operator",
									[kNoisyIrNodeType_PassignmentStatement]		= "assignment statement",
									[kNoisyIrNodeType_PbaseConst]			= "base constant",
									[kNoisyIrNodeType_PbasicSignalDimension]	= "basic signal dimension",
									[kNoisyIrNodeType_PbasicSignalUnits]		= "basic signal units",
									[kNoisyIrNodeType_PbasicSignal]			= "basic signal",
									[kNoisyIrNodeType_PbasicType]			= "basic type",
									[kNoisyIrNodeType_PchanEventExpr]		= "channel event expression",
									[kNoisyIrNodeType_PcmpOp]			= "comparison operator",
									[kNoisyIrNodeType_PcomplexCastExpr]		= "complex-valued cast expression",
									[kNoisyIrNodeType_PcomplexType]			= "complex type",
									[kNoisyIrNodeType_PconstSetExpr]		= "constant set expression",
									[kNoisyIrNodeType_PconstantDecl]		= "constant declaration",
									[kNoisyIrNodeType_PdimensionArithExpr]		= "dimension arithmetic expression",
									[kNoisyIrNodeType_PdimensionArithFactor]	= "dimension arithmetic factor",
									[kNoisyIrNodeType_PdimensionArithTerm]		= "dimension arithmetic term",
									[kNoisyIrNodeType_PdimensionsDesignation]	= "dimensions designation",
									[kNoisyIrNodeType_Pelement]			= "initialization list element",
									[kNoisyIrNodeType_Pexpression]			= "expression",
									[kNoisyIrNodeType_Pfactor]			= "factor",
									[kNoisyIrNodeType_PfieldSelect]			= "field selection expression",
									[kNoisyIrNodeType_PfixedType]			= "fixed-point type",
									[kNoisyIrNodeType_PfunctionDecl]		= "function declaration",
									[kNoisyIrNodeType_PfunctionDefn]		= "function definition",
									[kNoisyIrNodeType_PguardedExpressionList]	= "guarded expression list",
									[kNoisyIrNodeType_PguardedStatementList]	= "guarded statement list",
									[kNoisyIrNodeType_PhighPrecedenceArith2ArithOp]	= "high-precedence arithmetic to arithmetic operator",
									[kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp]	= "high-precedence binary Boolean operator",
									[kNoisyIrNodeType_PhighPrecedenceBinaryOp]	= "high precedence operator: \"*\", \"/\", \"%\", \"^\", \"::\", \"lowpass\", \"highpass\", \"dotproduct\", \"crossproduct\", \"centralmoment\", or \"&&\"",
									[kNoisyIrNodeType_PhighPrecedenceBoolSetOp]	= "high-precedence Boolean set operator",
									[kNoisyIrNodeType_PidentifierList]		= "identifier or list of identifiers",
									[kNoisyIrNodeType_PidentifierOrNilList]		= "identifier or nil list",
									[kNoisyIrNodeType_PidentifierOrNil]		= "identifier or nil",
									[kNoisyIrNodeType_PidxInitList]			= "indexed initialization list",
									[kNoisyIrNodeType_PinitList]			= "initialization list",
									[kNoisyIrNodeType_PintParamOrConst]		= "integer parameter or constant",
									[kNoisyIrNodeType_PintegerType]			= "integer type",
									[kNoisyIrNodeType_PiterateStatement]		= "iteration statement",
									[kNoisyIrNodeType_PlatencyTolerance]		= "latency tolerance specification",
									[kNoisyIrNodeType_PlistCastExpr]		= "list cast expression",
									[kNoisyIrNodeType_PlistType]			= "list data type",
									[kNoisyIrNodeType_PloadExpr]			= "load expression",
									[kNoisyIrNodeType_PlossTolerance]		= "loss/erasure tolerance specification",
									[kNoisyIrNodeType_PlowPrecedenceArith2ArithOp]	= "low-precedence arithmetic to arithmetic operator",
									[kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp]	= "low-precedence binary to Boolean operator",
									[kNoisyIrNodeType_PlowPrecedenceBinaryOp]	= "low-precedence operator: \"+\", \"-\", \">>\", \"<<\", \"|\", \"==\", \"!=\", \">\", \"<\", \"<=\", \">=\"  or \"||\"",
									[kNoisyIrNodeType_PlowPrecedenceBoolSetOp]	= "low-precedence Boolean set operator",
									[kNoisyIrNodeType_PmatchStatement]		= "match or matchseq statement",
									[kNoisyIrNodeType_PmaxForExpr]			= "\"max for ...\" predicate expression",
									[kNoisyIrNodeType_PminForExpr]			= "\"min for ...\" predicate expression",
									[kNoisyIrNodeType_PmoduleDeclBody]		= "module declaration body",
									[kNoisyIrNodeType_PmoduleDecl]			= "module declaration",
									[kNoisyIrNodeType_PmoduleTypeNameDecl]		= "module typename declaration",
									[kNoisyIrNodeType_PnamegenInvokeShorthand]	= "function invocation",
									[kNoisyIrNodeType_PnumericConst]		= "numeric constant",
									[kNoisyIrNodeType_PnumericType]			= "numeric type",
									[kNoisyIrNodeType_PoperatorToleranceDecl]	= "operator tolerance declaration",
									[kNoisyIrNodeType_PorderingHead]		= "order-based head for `parallel` or `sequence` statement",
									[kNoisyIrNodeType_PparallelStatement]		= "parallel statement",
									[kNoisyIrNodeType_PpredArithExpr]		= "predicate arithmetic expression",
									[kNoisyIrNodeType_PpredArithFactor]		= "predicate arithmetic factor",
									[kNoisyIrNodeType_PpredArithTerm]		= "predicate arithmetic term",
									[kNoisyIrNodeType_PpredExpr]			= "predicate expression",
									[kNoisyIrNodeType_PpredFactor]			= "predicate factor",
									[kNoisyIrNodeType_PpredStmtList]		= "predicate statement list",
									[kNoisyIrNodeType_PpredStmt]			= "predicate statement",
									[kNoisyIrNodeType_PpredTerm]			= "predicate term",
									[kNoisyIrNodeType_PpredicateFnDecl]		= "predicate function declaration",
									[kNoisyIrNodeType_PpredicateFnDefn]		= "predicate function definition",
									[kNoisyIrNodeType_PprobdefDecl]			= "probdef declaration",
									[kNoisyIrNodeType_PproblemDefn]			= "probdef definition",
									[kNoisyIrNodeType_PproductForExpr]		= "\"product for ...\" predicate expression",
									[kNoisyIrNodeType_Pprogram]			= "Noisy language program",
									[kNoisyIrNodeType_PqualifiedIdentifier]		= "qualified identifier",
									[kNoisyIrNodeType_PquantifiedBoolTerm]		= "quantified Boolean term",
									[kNoisyIrNodeType_PquantifierOp]		= "quantifier operator",
									[kNoisyIrNodeType_PquantizeExpression]		= "quantize expression",
									[kNoisyIrNodeType_PrationalCastExpr]		= "rational cast expression",
									[kNoisyIrNodeType_PrationalType]		= "rational type",
									[kNoisyIrNodeType_PreadTypeSignature]		= "read-type signature",
									[kNoisyIrNodeType_PrealParamOrConst]		= "real-valued parameter or constant",
									[kNoisyIrNodeType_PrealType]			= "real-valued type",
									[kNoisyIrNodeType_PreturnSignature]		= "return signature",
									[kNoisyIrNodeType_PreturnStatement]		= "return statement",
									[kNoisyIrNodeType_PsampleExpression]		= "sample expression",
									[kNoisyIrNodeType_PscopedPredStmtList]		= "scoped predicate statement list",
									[kNoisyIrNodeType_PscopedStatementList]		= "scoped statement list",
									[kNoisyIrNodeType_PsequenceStatement]		= "sequence statement",
									[kNoisyIrNodeType_PsetCastExpr]			= "set cast expression",
									[kNoisyIrNodeType_PsetCmpOp]			= "set comparison operator",
									[kNoisyIrNodeType_PsetCmpTerm]			= "set comparison term",
									[kNoisyIrNodeType_PsetExpr]			= "set expression",
									[kNoisyIrNodeType_PsetFactor]			= "set factor",
									[kNoisyIrNodeType_PsetHead]			= "set-based head for `parallel` or `sequence` statement",
									[kNoisyIrNodeType_PsetTerm]			= "set term",
									[kNoisyIrNodeType_PsetType]			= "set data type",
									[kNoisyIrNodeType_PsigfigDesignation]		= "significant figures designation",
									[kNoisyIrNodeType_PsignalDesignation]		= "signal designation",
									[kNoisyIrNodeType_Psignature]			= "signature",
									[kNoisyIrNodeType_PstarInitList]		= "initialization list",
									[kNoisyIrNodeType_PstatementList]		= "statement list",
									[kNoisyIrNodeType_Pstatement]			= "program statement",
									[kNoisyIrNodeType_PstringParamOrConst]		= "string parameter or constant",
									[kNoisyIrNodeType_PsumForExpr]			= "\"sum for ...\" predicate expression",
									[kNoisyIrNodeType_PsumProdMinMaxBody]		= "sum/product/min/max predicate statement body",
									[kNoisyIrNodeType_Pterm]			= "term",
									[kNoisyIrNodeType_PtimeseriesDesignation]	= "time series designation",
									[kNoisyIrNodeType_Ptolerance]			= "tolerance specification",
									[kNoisyIrNodeType_PtupleType]			= "tuple data type",
									[kNoisyIrNodeType_PtupleValue]			= "tuple value",
									[kNoisyIrNodeType_Ptuple]			= "tuple",
									[kNoisyIrNodeType_PtypeAnnoteDecl]		= "typeannote declaration",
									[kNoisyIrNodeType_PtypeAnnoteItem]		= "typeannote item",
									[kNoisyIrNodeType_PtypeAnnoteList]		= "typeannote list",
									[kNoisyIrNodeType_PtypeDecl]			= "type declaration",
									[kNoisyIrNodeType_PtypeExpr]			= "type expression",
									[kNoisyIrNodeType_PtypeMaxExpr]			= "typemax expression",
									[kNoisyIrNodeType_PtypeMinExpr]			= "typemin expression",
									[kNoisyIrNodeType_PtypeName]			= "type name",
									[kNoisyIrNodeType_PtypeParameterList]		= "type parameter list",
									[kNoisyIrNodeType_PunaryBoolOp]			= "unary Boolean operator",
									[kNoisyIrNodeType_PunaryOp]			= "unary operator",
									[kNoisyIrNodeType_PunarySetOp]			= "unary set operator",
									[kNoisyIrNodeType_PunitsArithExpr]		= "units arithmetic expression",
									[kNoisyIrNodeType_PunitsArithFactor]		= "units arithmetic factor",
									[kNoisyIrNodeType_PunitsArithTerm]		= "units arithmetic term",
									[kNoisyIrNodeType_PunitsDesignation]		= "units designation",
									[kNoisyIrNodeType_PvalfnSignature]		= "valfn signature",
									[kNoisyIrNodeType_PvarIntroList]		= "variable introduction list",
									[kNoisyIrNodeType_PvarIntro]			= "variable introduction",
									[kNoisyIrNodeType_PvarTuple]			= "variable tuple",
									[kNoisyIrNodeType_PvectorTypeDecl]		= "vector type declaration",
									[kNoisyIrNodeType_PwriteTypeSignature]		= "write-type signature",

									[kNoisyIrNodeTypeMax]				= "internal representation special node type: kNoisyIrNodeTypeMax",
									[kNoisyIrNodeType_PMax]				= "internal representation special node type: kNoisyIrNodeType_PMax",
									[kNoisyIrNodeType_PMin]				= "internal representation special node type: kNoisyIrNodeType_PMin",
									[kNoisyIrNodeType_Xseq]				= "internal representation special node type: kNoisyIrNodeType_Xseq",

};
