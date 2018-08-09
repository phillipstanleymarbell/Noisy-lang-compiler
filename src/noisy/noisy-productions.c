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
 *	Strings representing the various productions, for debugging and error reporting. TODO: this needs to be cleaned up next in finishing up the language refresh.
 */
const char *	gNoisyProductionDescriptions[kCommonIrNodeTypeMax] =
{
									[kNoisyIrNodeType_PaccuracyTolerance]		= "",
									[kNoisyIrNodeType_PadtTypeDecl]			= "ADT type declaration",
									[kNoisyIrNodeType_PanonAggrCastExpr]		= "array/list/tuple or set cast expression",
									[kNoisyIrNodeType_PanonAggregateType]		= "anonymous aggregate data type (array, list, tuple, or set)",
									[kNoisyIrNodeType_Parith2BoolOp]		= "",
									[kNoisyIrNodeType_ParithConst]			= "",
									[kNoisyIrNodeType_ParrayCastExpr]		= "array cast expression",
									[kNoisyIrNodeType_ParrayType]			= "array data type",
									[kNoisyIrNodeType_PassignOp]			= "assignment operator",
									[kNoisyIrNodeType_PassignmentStatement]		= "",
									[kNoisyIrNodeType_PbaseConst]			= "",
									[kNoisyIrNodeType_PbasicSignalDimension]	= "",
									[kNoisyIrNodeType_PbasicSignalUnits]		= "",
									[kNoisyIrNodeType_PbasicSignal]			= "",
									[kNoisyIrNodeType_PbasicType]			= "basic type, \"bit\", \"nibble\", \"byte\", \"int\", \"real\", \"fixed\", \"string\"",
									[kNoisyIrNodeType_PchanEventExpr]		= "channel event expression",
									[kNoisyIrNodeType_PcmpOp]			= "comparator operator",
									[kNoisyIrNodeType_PcomplexCastExpr]		= "",
									[kNoisyIrNodeType_PcomplexType]			= "",
									[kNoisyIrNodeType_PconstSetExpr]		= "",
									[kNoisyIrNodeType_PconstantDecl]		= "constant declaration",
									[kNoisyIrNodeType_PdimensionArithExpr]		= "",
									[kNoisyIrNodeType_PdimensionArithFactor]	= "",
									[kNoisyIrNodeType_PdimensionArithTerm]		= "",
									[kNoisyIrNodeType_PdimensionsDesignation]	= "",
									[kNoisyIrNodeType_Pelement]			= "initialization list element",
									[kNoisyIrNodeType_Pexpression]			= "expression",
									[kNoisyIrNodeType_Pfactor]			= "factor",
									[kNoisyIrNodeType_PfieldSelect]			= "field selection expression",
									[kNoisyIrNodeType_PfixedType]			= "fixed-point type, \"fixed\" intconst \".\" intconst",
									[kNoisyIrNodeType_PfunctionDecl]		= "function declaration",
									[kNoisyIrNodeType_PfunctionDefn]		= "function definition",
									[kNoisyIrNodeType_PguardedExpressionList]	= "",
									[kNoisyIrNodeType_PguardedStatementList]	= "",
									[kNoisyIrNodeType_PhighPrecedenceArith2ArithOp]	= "",
									[kNoisyIrNodeType_PhighPrecedenceBinaryBoolOp]	= "",
									[kNoisyIrNodeType_PhighPrecedenceBinaryOp]	= "high precedence operator: \"*\", \"/\", \"%\", \"^\" or \"::\"",
									[kNoisyIrNodeType_PhighPrecedenceBoolSetOp]	= "",
									[kNoisyIrNodeType_PidentifierList]		= "identifier or list of identifiers",
									[kNoisyIrNodeType_PidentifierOrNilList]		= "",
									[kNoisyIrNodeType_PidentifierOrNil]		= "identifier or nil",
									[kNoisyIrNodeType_PidxInitList]			= "indexed initialization list",
									[kNoisyIrNodeType_PinitList]			= "initialization list",
									[kNoisyIrNodeType_PintParamOrConst]		= "",
									[kNoisyIrNodeType_PintegerType]			= "",
									[kNoisyIrNodeType_PiterateStatement]		= "iteration statement",
									[kNoisyIrNodeType_PlatencyTolerance]		= "latency tolerance specification",
									[kNoisyIrNodeType_PlistCastExpr]		= "list cast expression",
									[kNoisyIrNodeType_PlistType]			= "list data type",
									[kNoisyIrNodeType_PloadExpr]			= "",
									[kNoisyIrNodeType_PlossTolerance]		= "loss/erasure tolerance specification",
									[kNoisyIrNodeType_PlowPrecedenceArith2ArithOp]	= "",
									[kNoisyIrNodeType_PlowPrecedenceBinaryBoolOp]	= "",
									[kNoisyIrNodeType_PlowPrecedenceBinaryOp]	= "low precedence operator: \"+\", \"-\", \">>\", \"<<\", \"|\", \"==\", \"!=\", \">\", \"<\", \"<=\", \">=\" \"&&\" or \"||\"",
									[kNoisyIrNodeType_PlowPrecedenceBoolSetOp]	= "",
									[kNoisyIrNodeType_PmatchStatement]		= "match or matchseq statement",
									[kNoisyIrNodeType_PmaxOverExpr]			= "",
									[kNoisyIrNodeType_PminOverExpr]			= "",
									[kNoisyIrNodeType_PmoduleDeclBody]		= "",
									[kNoisyIrNodeType_PmoduleDecl]			= "",
									[kNoisyIrNodeType_PmoduleTypeNameDecl]		= "",
									[kNoisyIrNodeType_PnamegenInvokeShorthand]	= "",
									[kNoisyIrNodeType_PnumericConst]		= "",
									[kNoisyIrNodeType_PnumericType]			= "",
									[kNoisyIrNodeType_PoperatorToleranceDecl]	= "",
									[kNoisyIrNodeType_PorderingHead]		= "",
									[kNoisyIrNodeType_PparallelStatement]		= "",
									[kNoisyIrNodeType_PpredArithExpr]		= "",
									[kNoisyIrNodeType_PpredArithFactor]		= "",
									[kNoisyIrNodeType_PpredArithTerm]		= "",
									[kNoisyIrNodeType_PpredExpr]			= "",
									[kNoisyIrNodeType_PpredFactor]			= "",
									[kNoisyIrNodeType_PpredStmtList]		= "",
									[kNoisyIrNodeType_PpredStmt]			= "",
									[kNoisyIrNodeType_PpredTerm]			= "",
									[kNoisyIrNodeType_PpredicateFnDecl]		= "",
									[kNoisyIrNodeType_PpredicateFnDefn]		= "",
									[kNoisyIrNodeType_PprobdefDecl]			= "",
									[kNoisyIrNodeType_PproblemDefn]			= "",
									[kNoisyIrNodeType_PproductOverExpr]		= "",
									[kNoisyIrNodeType_Pprogram]			= "Noisy language program",
									[kNoisyIrNodeType_PqualifiedIdentifier]		= "",
									[kNoisyIrNodeType_PquantifiedBoolTerm]		= "",
									[kNoisyIrNodeType_PquantifierOp]		= "",
									[kNoisyIrNodeType_PquantizeExpression]		= "",
									[kNoisyIrNodeType_PrationalCastExpr]		= "",
									[kNoisyIrNodeType_PrationalType]		= "",
									[kNoisyIrNodeType_PreadTypeSignature]		= "",
									[kNoisyIrNodeType_PrealParamOrConst]		= "",
									[kNoisyIrNodeType_PrealType]			= "real-valued type, \"real\" or \"fixed\" intconst \".\" intconst",
									[kNoisyIrNodeType_PreturnSignature]		= "",
									[kNoisyIrNodeType_PreturnStatement]		= "",
									[kNoisyIrNodeType_PsampleExpression]		= "",
									[kNoisyIrNodeType_PscopedPredStmtList]		= "",
									[kNoisyIrNodeType_PscopedStatementList]		= "scoped statement list",
									[kNoisyIrNodeType_PsequenceStatement]		= "",
									[kNoisyIrNodeType_PsetCastExpr]			= "set cast expression",
									[kNoisyIrNodeType_PsetCmpOp]			= "",
									[kNoisyIrNodeType_PsetCmpTerm]			= "",
									[kNoisyIrNodeType_PsetExpr]			= "",
									[kNoisyIrNodeType_PsetFactor]			= "",
									[kNoisyIrNodeType_PsetHead]			= "",
									[kNoisyIrNodeType_PsetTerm]			= "",
									[kNoisyIrNodeType_PsetType]			= "set data type",
									[kNoisyIrNodeType_PsigfigDesignation]		= "",
									[kNoisyIrNodeType_PsignalDesignation]		= "",
									[kNoisyIrNodeType_Psignature]			= "signature",
									[kNoisyIrNodeType_PstarInitList]		= "initialization list",
									[kNoisyIrNodeType_PstatementList]		= "statement list",
									[kNoisyIrNodeType_Pstatement]			= "program statement",
									[kNoisyIrNodeType_PstringParamOrConst]		= "",
									[kNoisyIrNodeType_PsumOverExpr]			= "",
									[kNoisyIrNodeType_PsumProdMinMaxBody]		= "",
									[kNoisyIrNodeType_Pterm]			= "term",
									[kNoisyIrNodeType_PtimeseriesDesignation]	= "",
									[kNoisyIrNodeType_Ptolerance]			= "tolerance specification",
									[kNoisyIrNodeType_PtupleType]			= "tuple data type",
									[kNoisyIrNodeType_PtupleValue]			= "tuple value",
									[kNoisyIrNodeType_Ptuple]			= "",
									[kNoisyIrNodeType_PtypeAnnoteDecl]		= "",
									[kNoisyIrNodeType_PtypeAnnoteItem]		= "",
									[kNoisyIrNodeType_PtypeAnnoteList]		= "",
									[kNoisyIrNodeType_PtypeDecl]			= "type declaration",
									[kNoisyIrNodeType_PtypeExpr]			= "type expression",
									[kNoisyIrNodeType_PtypeMaxExpr]			= "",
									[kNoisyIrNodeType_PtypeMinExpr]			= "",
									[kNoisyIrNodeType_PtypeName]			= "type name",
									[kNoisyIrNodeType_PtypeParameterList]		= "type parameter list",
									[kNoisyIrNodeType_PunaryBoolOp]			= "",
									[kNoisyIrNodeType_PunaryOp]			= "unary operator",
									[kNoisyIrNodeType_PunarySetOp]			= "",
									[kNoisyIrNodeType_PunitsArithExpr]		= "",
									[kNoisyIrNodeType_PunitsArithFactor]		= "",
									[kNoisyIrNodeType_PunitsArithTerm]		= "",
									[kNoisyIrNodeType_PunitsDesignation]		= "",
									[kNoisyIrNodeType_PvalfnSignature]		= "",
									[kNoisyIrNodeType_PvarIntroList]		= "",
									[kNoisyIrNodeType_PvarIntro]			= "",
									[kNoisyIrNodeType_PvarTuple]			= "",
									[kNoisyIrNodeType_PvectorTypeDecl]		= "",
									[kNoisyIrNodeType_PwriteTypeSignature]		= "",

									[kNoisyIrNodeTypeMax]				= "",
									[kNoisyIrNodeType_PMax]				= "",
									[kNoisyIrNodeType_PMin]				= "",
									[kNoisyIrNodeType_Xseq]				= "",

};
