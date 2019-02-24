/*
	Authored 2015, Phillip Stanley-Marbell.

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

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTorS
	"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
	LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
	FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
	COPYRIGHT OWNER OR CONTRIBUTorS BE LIABLE FOR ANY DIRECT, INDIRECT,
	INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
	BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
	LIABILITY, OR TorT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
	ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
	POSSIBILITY OF SUCH DAMAGE.
*/

void		checkTokenLength(State *  N, int  count);
char		cur(State *  N);
void		gobble(State *  N, int count);
void		done(State *  N, Token *  newToken);
bool		eqf(State *  N);
bool		isDecimal(State *  N, char *  string);
char *		stringAtLeft(State *  N, char *  string, char  character);
char *		stringAtRight(State *  N, char *  string, char  character);
bool		isDecimalSeparatedWithChar(State *  N, char *  string, char  character);
bool		isDecimalOrRealSeparatedWithChar(State *  N, char *  string, char  character);
bool		isRadixConst(State *  N, char *  string);
bool		isRealConst(State *  N, char *  string);
bool		isEngineeringRealConst(State *  N, char *  string);
bool		isHexConstWithoutLeading0x(State *  N, char *  string);
uint64_t	stringToRadixConst(State *  N, char *  string);
double		stringToRealConst(State *  N, char *  string);
double		stringToEngineeringRealConst(State *  N, char *  string);


SourceInfo *	lexAllocateSourceInfo(	State *  N, char **  genealogy, 
							char *  fileName, uint64_t lineNumber,
							uint64_t columnNumber, uint64_t length);
Token *		lexAllocateToken(	State *  N, IrNodeType type, 
							char *  identifier, uint64_t integerConst,
							double realConst, char * stringConst,
							SourceInfo *  sourceInfo);
void		lexPut(State *  N, Token *  newToken);
Token *		lexGet(State *  N, const char *  tokenDescriptionArray[kNoisyIrNodeTypeMax]);
Token *		lexPeek(State *  N, int lookAhead);

void		lexPrintToken(State *  N, Token *  t, const char *  tokenDescriptionArray[kNoisyIrNodeTypeMax]);
void		lexDebugPrintToken(State *  N, Token *  t, const char *  tokenDescriptionArray[kNoisyIrNodeTypeMax]);
void		lexPeekPrint(State *  N, int maxTokens, int formatCharacters, const char *tokenDescriptionArray[kNoisyIrNodeTypeMax]);
