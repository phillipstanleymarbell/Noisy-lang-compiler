#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <getopt.h>
#include <setjmp.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisyconfig-errors.h"
#include "version.h"
#include "noisyconfig-timeStamps.h"
#include "noisyconfig.h"
#include "noisyconfig-parser.h"
#include "noisyconfig-lexer.h"
#include "noisyconfig-symbolTable.h"
#include "noisyconfig-irPass-helpers.h"
#include "noisyconfig-irPass-dotBackend.h"
// #include "noisyconfig-irPass-protobufBackend.h"
#include "noisyconfig-types.h"

static void		usage(NoisyConfigState *  N);
static void		processFile(NoisyConfigState *  N, char *  filename);
static void		version(NoisyConfigState *  N);



int
main(int argc, char *argv[])
{
    
	int			jumpParameter;
	NoisyConfigState *		N;


	N = noisyConfigInit(kNoisyConfigModeDefault);
	if (N == NULL)
	{
		noisyConfigFatal(NULL, Emalloc);

		/*	Not reached	*/
		noisyConfigConsolePrintBuffers(N);
		exit(EXIT_FAILURE);
	}

	while (1)
	{
		char			tmp;
		char *			ep = &tmp;
		int			optionIndex	= 0, c;
		static struct option	options[]	=
		{
			{"verbose",		required_argument,	0,	'v'},
			{"help",		no_argument,		0,	'h'},
			{"version",		no_argument,		0,	'V'},
			{"dot",			required_argument,	0,	'd'},
			{"bytecode",		required_argument,	0,	'b'},
			{"trace",		no_argument,		0,	't'},
			{"optimize",		required_argument,	0,	'O'},
			{0,			0,			0,	0}
		};

		c = getopt_long(argc, argv, "v:hVd:b:tO:", options, &optionIndex);

		if (c == -1)
		{
			break;
		}

		switch (c)
		{
			case 0:
			{
				/*
				 *	Not sure what the expected behavior for getopt_long is here...
				 */
				break;
			}

			case 'h':
			{
				usage(N);
				noisyConfigConsolePrintBuffers(N);
				exit(EXIT_SUCCESS);

				/*	Not reached 	*/
				break;
			}

			case 'V':
			{
				version(N);
				noisyConfigConsolePrintBuffers(N);
				exit(EXIT_SUCCESS);

				/*	Not reached 	*/
				break;
			}

			case 'd':
			{
				N->irBackends |= kNoisyConfigIrBackendDot;
				
				//TODO: rather than accepting the raw enum value as integer, accept string and compare to table of options
				uint64_t tmpInt = strtoul(optarg, &ep, 0);
				if (*ep == '\0')
				{
					N->dotDetailLevel = tmpInt;
				}
				else
				{
					usage(N);
					noisyConfigConsolePrintBuffers(N);
					exit(EXIT_FAILURE);
				}

				break;
			}

			// case 'b':
			// {
			// 	N->irBackends |= kNoisyConfigIrBackendProtobuf;
			// 	N->outputFilePath = optarg;

			// 	break;
			// }

			// case 't':
			// {
			// 	N->mode |= kNoisyConfigModeCallTracing;
			// 	N->mode |= kNoisyConfigModeCallStatistics;
			// 	noisyConfigTimestampsInit(N);

			// 	break;
			// }

			case 'v':
			{
				uint64_t tmpInt = strtoul(optarg, &ep, 0);
				if (*ep == '\0')
				{
					/*
					 *	The verbosity bitmaps are:
					 *
					 *		kNoisyVerbosityAST
					 *		kNoisyVerbosityFF
					 *		kNoisyVerbosityLex
					 *		kNoisyVerbosityParse
					 */

					N->verbosityLevel = tmpInt;
				}
				else
				{
					usage(N);
					noisyConfigConsolePrintBuffers(N);
					exit(EXIT_FAILURE);
				}

				break;
			}

			case 'O':
			{
				//TODO: define a structured way for which passes depend on which

				/*
				 *	Implies the following (basic) passes:
				 */
				//N->irPasses |= xxx;
				//N->irPasses |= yyy;

				uint64_t tmpInt = strtoul(optarg, &ep, 0);
				if (*ep == '\0')
				{
					N->optimizationLevel = tmpInt;
				}
				else
				{
					usage(N);
					noisyConfigConsolePrintBuffers(N);
					exit(EXIT_FAILURE);
				}

				break;
			}

			case '?':
			{
				/*
				 *    getopt_long() should have already printed an error message.
				 */
				usage(N);
				noisyConfigConsolePrintBuffers(N);
				exit(EXIT_FAILURE);

				break;
			}

			default:
			{
				usage(N);
				noisyConfigConsolePrintBuffers(N);
				exit(EXIT_FAILURE);
			}
		}
	}

	if (optind < argc)
	{
		while (optind < argc)
		{
			jumpParameter = setjmp(N->jmpbuf);
			if (!jumpParameter)
			{
				processFile(N, argv[optind++]);
			}
		}
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n%s\n", Enoinput);
		usage(N);
		noisyConfigConsolePrintBuffers(N);
		exit(EXIT_FAILURE);
	}

    noisyConfigConsolePrintBuffers(N);

	return 0;
}

static void
processFile(NoisyConfigState *  N, char *  fileName)
{
	/*
	 *	Tokenize input, then parse it and build AST + symbol table.
	 */
	noisyConfigLexInit(N, fileName);

	/*
	 *	Create a top-level scope, then parse.
	 */
	N->noisyConfigIrTopScope = noisyConfigSymbolTableAllocScope(N);
	N->noisyConfigIrRoot = noisyConfigParse(N, N->noisyConfigIrTopScope);


	/*
	 *	Run passes requested in the command line flags.
	 */
	noisyConfigRunPasses(N);


	/*
	 *	Bytecode backend. Emit IR in protobuf.
	 */
	// if (N->irBackends & kNoisyConfigIrBackendProtobuf)
	// {
	// 	noisyConfigIrPassProtobufBackend(N);
	// }


	/*
	 *	Dot backend.
	 */
	if (N->irBackends & kNoisyConfigIrBackendDot)
	{
		fprintf(stdout, "%s\n", noisyConfigIrPassDotBackend(N));
	}
    


	// if (N->mode & kNoisyConfigModeCallTracing)
	// {
	// 	noisyConfigTimeStampDumpTimeline(N);
	// }
    
    noisyConfigConsolePrintBuffers(N);
}


static void
version(NoisyConfigState *  N)
{
	flexprint(N->Fe, N->Fm, N->Fperr, "\nNoisy Config version %s.\n\n", kNoisyConfigVersion);
}


static void
usage(NoisyConfigState *  N)
{
	version(N);
	flexprint(N->Fe, N->Fm, N->Fperr,	"Usage:    noisyconfig [ (--help, -h)                                       \n"
						"                | (--version, --V)                                   \n"
						"                | (--verbose <level>, -v <level>)                    \n"
						"                | (--dot <level>, -d <level>)                        \n"
						"                | (--bytecode <output file name>, -b <output file name>)\n"
						"                | (--optimize <level>, -O <level>)                   \n"
						"                | (--trace, -t)                                      \n"
						"                                                                     \n"
						"              <filenames>\n\n");
}
