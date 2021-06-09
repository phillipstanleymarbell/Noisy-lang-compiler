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
#include "common-errors.h"
#include "version.h"
#include "noisy-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "noisy-parser.h"
#include "noisy-lexer.h"
#include "common-symbolTable.h"
#include "common-irPass-helpers.h"
#include "noisy-irPass-dotBackend.h"
#include "noisy-irPass-protobufBackend.h"
#include "noisy-codeGeneration.h"
#include "noisy-typeCheck.h"

static void		usage(State *  N);
static void		processFile(State *  N, char *  filename);
static void		version(State *  N);



int
main(int argc, char *argv[])
{
	int			jumpParameter;
	State *		N;

	N = init(kCommonModeDefault);
	
	if (N == NULL)
	{
		fatal(NULL, Emalloc);

		/*	Not reached	*/
		consolePrintBuffers(N);
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
			{"statistics",		no_argument,		0,	's'},
			{"optimize",		required_argument,	0,	'O'},
			{0,			0,			0,	0}
		};

		c = getopt_long(argc, argv, "v:hVd:b:stO:", options, &optionIndex);

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
				consolePrintBuffers(N);
				exit(EXIT_SUCCESS);

				/*	Not reached 	*/
				break;
			}

			case 'V':
			{
				version(N);
				consolePrintBuffers(N);
				exit(EXIT_SUCCESS);

				/*	Not reached 	*/
				break;
			}

			case 'd':
			{
				N->irBackends |= kNoisyIrBackendDot;
				
				/*
				 *	See issue #293.
				 */

				uint64_t tmpInt = strtoul(optarg, &ep, 0);
				if (*ep == '\0')
				{
					N->dotDetailLevel = tmpInt;
				}
				else
				{
					usage(N);
					consolePrintBuffers(N);
					exit(EXIT_FAILURE);
				}

				break;
			}

			case 'b':
			{
				N->irBackends |= kNoisyIrBackendProtobuf;
				N->outputFilePath = optarg;

				break;
			}

			case 't':
			{
				N->mode |= kCommonModeCallTracing;
				N->mode |= kCommonModeCallStatistics;
				timestampsInit(N);

				break;
			}

			case 's':
			{
				N->mode |= kCommonModeCallStatistics;
				timestampsInit(N);

				break;
			}

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
					consolePrintBuffers(N);
					exit(EXIT_FAILURE);
				}

				break;
			}

			case 'O':
			{
				/*
				 *	Implies one or more passes (see issue #294)
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
					consolePrintBuffers(N);
					exit(EXIT_FAILURE);
				}

				break;
			}

			case '?':
			{
				/*
				 *	getopt_long() should have already printed an error message.
				 */
				usage(N);
				consolePrintBuffers(N);
				exit(EXIT_FAILURE);

				break;
			}

			default:
			{
				usage(N);
				consolePrintBuffers(N);
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
			else
			{
				/*
				 *	See issue #291
				 */

				/*	Return again after longjmp	*/
			}
		}
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n%s\n", Enoinput);
		usage(N);
		consolePrintBuffers(N);
		exit(EXIT_FAILURE);
	}

	if (!(N->mode & kCommonModeCGI))
	{
		consolePrintBuffers(N);
	}

	return 0;
}


static void
processFile(State *  N, char *  fileName)
{
	/*
	 *	Tokenize input, then parse it and build AST + symbol table.
	 */
	noisyLexInit(N, fileName);

	/*
	 *	Create a top-level scope, then parse.
	 */
	N->noisyIrTopScope = commonSymbolTableAllocScope(N);
	N->noisyIrRoot = noisyParse(N, N->noisyIrTopScope);


	/*
	 *	Run passes requested in the command line flags.
	 */
	//runPasses(N);


	/*
	 *	We don't put the following into runPasses() because they are not general-purpose
	 */

	/*
	*	Actual code generation takes place.
	*/

	noisyTypeCheck(N);

	noisyCodeGen(N);

	/*
	 *	Bytecode backend. Emit IR in protobuf.
	 */
	if (N->irBackends & kNoisyIrBackendProtobuf)
	{
		noisyIrPassProtobufBackend(N);
	}


	/*
	 *	Dot backend.
	 */
	if (N->irBackends & kNoisyIrBackendDot)
	{
		fprintf(stdout, "%s\n", noisyIrPassDotBackend(N, N->noisyIrTopScope, N->noisyIrRoot));
	}


	if (N->mode & kCommonModeCallTracing)
	{
		timeStampDumpTimeline(N);
	}

	if (N->mode & kCommonModeCallStatistics)
	{
		uint64_t	irNodeCount = 0, symbolTableNodeCount = 0;


		timeStampDumpResidencies(N);

		irNodeCount = irPassHelperIrSize(N, N->noisyIrRoot);
		symbolTableNodeCount = irPassHelperSymbolTableSize(N, N->noisyIrTopScope);


		flexprint(N->Fe, N->Fm, N->Fpinfo, "Intermediate Representation Information:\n\n");
		flexprint(N->Fe, N->Fm, N->Fpinfo, "    IR node count                        : %llu\n", irNodeCount);
		flexprint(N->Fe, N->Fm, N->Fpinfo, "    Symbol Table node count              : %llu\n", symbolTableNodeCount);

		/*
		 *	Libflex malloc statistics:
		 */
		if (N->Fm->debug)
		{
			flexmblocksdisplay(N->Fe, N->Fm, N->Fperr);
		}
	}
}


static void
version(State *  N)
{
	flexprint(N->Fe, N->Fm, N->Fperr, "\nNoisy version %s.\n\n", kNoisyVersion);
}


static void
usage(State *  N)
{
	version(N);
	flexprint(N->Fe, N->Fm, N->Fperr,	"Usage:    noisy [ (--help, -h)                                       \n"
						"                | (--version, --V)                                   \n"
						"                | (--verbose <level>, -v <level>)                    \n"
						"                | (--dot <level>, -d <level>)                        \n"
						"                | (--bytecode <output file name>, -b <output file name>)\n"
						"                | (--optimize <level>, -O <level>)                   \n"
						"                | (--trace, -t)                                      \n"
						"                | (--statistics, -s) ]                               \n"
						"                                                                     \n"
						"              <filenames>\n\n");
}
