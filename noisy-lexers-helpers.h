
void	checkTokenLength(NoisyState *  N, int  count);
char	cur(NoisyState *  N);
void		gobble(NoisyState *  N, int count);
void		done(NoisyState *  N, NoisyToken *  newToken);
bool		eqf(NoisyState *  N);
void		makeNumericConst(NoisyState *  N);
bool		isDecimal(NoisyState *  N, char *  string);
char *		stringAtLeft(NoisyState *  N, char *  string, char  character);
char *		stringAtRight(NoisyState *  N, char *  string, char  character);
bool		isDecimalSeparatedWithChar(NoisyState *  N, char *  string, char  character);
bool		isDecimalOrRealSeparatedWithChar(NoisyState *  N, char *  string, char  character);
bool		isRadixConst(NoisyState *  N, char *  string);
bool		isRealConst(NoisyState *  N, char *  string);
bool		isEngineeringRealConst(NoisyState *  N, char *  string);
uint64_t		stringToRadixConst(NoisyState *  N, char *  string);
double		stringToRealConst(NoisyState *  N, char *  string);
double		stringToEngineeringRealConst(NoisyState *  N, char *  string);
bool		isOperatorOrSeparator(NoisyState *  N, char c);
