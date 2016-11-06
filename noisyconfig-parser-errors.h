
void		noisyConfigParserSemanticError(NoisyConfigState *  N, const char * format, ...);
void		noisyConfigParserSyntaxError(NoisyConfigState *  N, NoisyConfigIrNodeType currentlyParsingProduction, NoisyConfigIrNodeType expectedProductionOrToken);
void		noisyConfigParserErrorRecovery(NoisyConfigState *  N, NoisyConfigIrNodeType expectedProductionOrToken) __attribute__((noreturn));
