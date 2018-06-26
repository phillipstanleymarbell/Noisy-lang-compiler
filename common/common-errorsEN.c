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
const char	Esanity[]				= "Sanity check failed";
const char	Eopen[]					= "Could not open \"%s\": %s\n";
const char	EbadXseqNode[]				= "Xseq node with no children seen";
const char	EillegallyPlacedXseqNode[]		= "Illegally placed Xseq node in Node.left";
const char	EexpectedElementOrStar[]		= "Expected element or \"*\"";
const char	EterminalTcons[]			= "Tcons case should have prevented us from getting here";
const char	Enoinput[]				= "No input files specified!";
const char	Epipe[]					= "popen() failed";
const char	Efatal[]				= "Noisy Fatal error: ";
const char	Eerror[]				= "Noisy Error: ";
const char	Efork[]					= "fork() failed";
const char	EnullFileNameInPrintToFile[]		= "Null file name supplied to noisyPrintToFile()";
const char	EdotRenderFailed[]			= "GraphViz/Dot rendering failed";
const char	EbadCgiQuery[]				= "bad CGI query (no request, or request not a HEAD or GET)";
const char	Emkstemps[]				= "mkstemps() failed";
const char	EelementOrStar[]			= "Sanity check failed: Expected element or \"*\"";
const char	EsyntaxA[]				= "Syntax error";
const char	EsyntaxB[]				= "while parsing";
const char	EsyntaxC[]				= "Expected";
const char	EsyntaxD[]				= "at";
const char	EsemanticsA[]				= "Semantics error";
const char	EsemanticsB[]				= "while parsing";
const char	EsemanticsD[]				= "followed by";
const char	EassignTypesSanity[]			= "Sanity check failed in assignTypes(): non-identifiers can't be assigned a type";
const char	EtokenTooLong[]				= "Saw a token longer than kNoisyMaxBufferLength during lexing";
const char	EstringTooLongOrWithNewline[]		= "Saw a string longer than kNoisyMaxBufferLength or one containing an unescaped newline, during lexing";
const char	EruntTokenInNumericConst[]		= "Run token passed to makeNumericConst()";
const char	EcannotFindTypeSignatureForNodeType[] 	= "Cannot find type signature character in gNoisyTypeNodeSignatures for this node type";
const char	EassignTypeSanity[]			= "Sanity check failed in type assignment";
const char	Efd2path[]				= "fcntl() failed to get path for file descriptor";


/*
 *	These are mostly passed in eSemantics()
 */
const char	Eundeclared[]				= "Variable use before declaration";



/*
 *	HTML-format errors for CGI version
 */
const char	WcgiAsync[] 				= "\n<span width=\"580\" style=\"background-color:FFCC00; color:#FF0000\"><br><b>&#9888;</b>&nbsp;&nbsp;&nbsp; NoisyCompiler halted the computation. Memory or CPU usage limit exceeded.<br><br>See the \"SETCPULIMIT\" and \"SETMEMLIMIT\" commands; \"help\" for more information.</span>\n";
