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
#include "noisy-errors.h"
#include "version.h"
#include "noisy-timeStamps.h"
#include "noisy.h"
#include "noisy-parser.h"
#include "noisy-lexer.h"
#include "noisy-symbolTable.h"
#include "noisy-irPass-helpers.h"
#include "noisy-irPass-dotBackend.h"

#include "newton-parser.h"
#include "newton-lexer.h"
#include "newton-symbolTable.h"

//extern const char	gNoisyEol[];
//extern const char	gNoisyWhitespace[];
//extern const char	gNoisyStickies[];

static void		usage(NoisyState *  N);
static void		version(NoisyState *  N);


// static void		usage(NoisyState *  N);
static void		processNewtonFile(NoisyState *  N, char *  filename);
// static void		version(NoisyState *  N);

static void     recurseDimensions(NoisyState * N, NoisyScope * topScope);
static void     recursePhysics(NoisyState * N, NoisyScope * topScope);

static void
recurseDimensions(NoisyState * N, NoisyScope * topScope)
{
    Dimension * curDimension = topScope->firstDimension;
    if (curDimension == NULL)
		flexprint(N->Fe, N->Fm, N->Fpinfo, "topscope dimension doesn't exist\n");

    while (curDimension != NULL) {
		flexprint(N->Fe, N->Fm, N->Fpinfo, "dimension %s %s %x\n", curDimension->identifier, curDimension->abbreviation, curDimension);
        curDimension = curDimension->next;
    }
}

static void
recursePhysics(NoisyState * N, NoisyScope * topScope)
{
    Physics * curPhysics = topScope->firstPhysics;
    if (curPhysics == NULL)
		flexprint(N->Fe, N->Fm, N->Fpinfo, "topscope physics doesn't exist\n");
    
    while (curPhysics != NULL) 
    {
		flexprint(N->Fe, N->Fm, N->Fpinfo, "physics %s\n", curPhysics->identifier);
		flexprint(N->Fe, N->Fm, N->Fpinfo, "alias %s\n", curPhysics->dimensionAlias);
		flexprint(N->Fe, N->Fm, N->Fpinfo, "alias abbreviation %s\n", curPhysics->dimensionAliasAbbreviation);
		flexprint(N->Fe, N->Fm, N->Fpinfo, "isvector %d\n", curPhysics->isVector);
        if (curPhysics->vectorCounterpart)
		    flexprint(N->Fe, N->Fm, N->Fpinfo, "vectorCounterpart %s\n", curPhysics->vectorCounterpart->identifier);
        if (curPhysics->scalarCounterpart)
		    flexprint(N->Fe, N->Fm, N->Fpinfo, "scalarCounterpart %s\n", curPhysics->scalarCounterpart->identifier);
        
        Dimension * curDimension = curPhysics->numeratorDimensions;
        while (curDimension != NULL) {
	    	flexprint(N->Fe, N->Fm, N->Fpinfo, "numerator dimension %s %d %x\n", curDimension->identifier, curDimension->primeNumber, curDimension);
            curDimension = curDimension->next;
        }
        
        curDimension = curPhysics->denominatorDimensions;
        while (curDimension != NULL) {
	    	flexprint(N->Fe, N->Fm, N->Fpinfo, "denominator dimension %s %d %x\n", curDimension->identifier, curDimension->primeNumber, curDimension);
            curDimension = curDimension->next;
        }
		
        flexprint(N->Fe, N->Fm, N->Fpinfo, "==============================================================================================\n");
        
        curPhysics = curPhysics->next;
    }

    IntegralList* curVectorIntegralList = N->vectorIntegralLists;
    while (curVectorIntegralList != NULL)
    {
        Physics * curIntegralPhysics = curVectorIntegralList->head;
        while (curIntegralPhysics != NULL)
        {
	    	flexprint(N->Fe, N->Fm, N->Fpinfo, "vector integral element %s\n", curIntegralPhysics->identifier);
            curIntegralPhysics = curIntegralPhysics->next;
        }
    
        curVectorIntegralList = curVectorIntegralList->next; 
    }
    
    IntegralList* curScalarIntegralList = N->scalarIntegralLists;
    while (curScalarIntegralList != NULL)
    {
        Physics * curIntegralPhysics = curScalarIntegralList->head;
        while (curIntegralPhysics != NULL)
        {
	    	flexprint(N->Fe, N->Fm, N->Fpinfo, "scalar integral element %s\n", curIntegralPhysics->identifier);
            curIntegralPhysics = curIntegralPhysics->next;
        }
    
        curScalarIntegralList = curScalarIntegralList->next; 
    }
    
    return;
}

int
main(int argc, char *argv[])
{
	int			jumpParameter;
	NoisyState *		N;

	N = noisyInit(kNoisyModeDefault);
	
    if (N == NULL)
	{
		noisyFatal(NULL, Emalloc);

		/*	Not reached	*/
		noisyConsolePrintBuffers(N);
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
				noisyConsolePrintBuffers(N);
				exit(EXIT_SUCCESS);

				/*	Not reached 	*/
				break;
			}

			case 'V':
			{
				version(N);
				noisyConsolePrintBuffers(N);
				exit(EXIT_SUCCESS);

				/*	Not reached 	*/
				break;
			}

			case 'd':
			{
				N->irBackends |= kNoisyIrBackendDot;
				
				//TODO: rather than accepting the raw enum value as integer, accept string and compare to table of options
				uint64_t tmpInt = strtoul(optarg, &ep, 0);
				if (*ep == '\0')
				{
					N->dotDetailLevel = tmpInt;
				}
				else
				{
					usage(N);
					noisyConsolePrintBuffers(N);
					exit(EXIT_FAILURE);
				}

				break;
			}

			case 't':
			{
				N->mode |= kNoisyModeCallTracing;
				N->mode |= kNoisyModeCallStatistics;
				noisyTimestampsInit(N);

				break;
			}

			case 's':
			{
				N->mode |= kNoisyModeCallStatistics;
				noisyTimestampsInit(N);

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
					noisyConsolePrintBuffers(N);
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
					noisyConsolePrintBuffers(N);
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
				noisyConsolePrintBuffers(N);
				exit(EXIT_FAILURE);

				break;
			}

			default:
			{
				usage(N);
				noisyConsolePrintBuffers(N);
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
                processNewtonFile(N, argv[optind++]);
			}
			else
			{
				//TODO: we could intelligently use the incoming jumpParameter

				/*	Return again after longjmp	*/
			}
		}
	}
	else
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "\n%s\n", Enoinput);
		usage(N);
		noisyConsolePrintBuffers(N);
		exit(EXIT_FAILURE);
	}

	if (!(N->mode & kNoisyModeCGI))
	{
		noisyConsolePrintBuffers(N);
	}

	return 0;
}

/*
 * TODO: change this to be more flexible and take an arg from command line
 * https://github.com/phillipstanleymarbell/Noisy-lang-compiler/issues/28
 */
static void		
processNewtonFile(NoisyState *  N, char *  filename)
{

	/*
	 *	Tokenize input, then parse it and build AST + symbol table.
	 */
	newtonLexInit(N, filename);

	/*
	 *	Create a top-level scope, then parse.
	 */
	N->newtonIrTopScope = newtonSymbolTableAllocScope(N);
	N->newtonIrRoot = newtonParse(N, N->newtonIrTopScope);

    recurseDimensions(N, N->newtonIrTopScope);
    recursePhysics(N, N->newtonIrTopScope);

	/*
	 *	Dot backend.
	 */
	if (N->irBackends & kNoisyIrBackendDot)
	{
		// fprintf(stdout, "%s\n", noisyIrPassDotBackend(N, N->noisyIrTopScope, N->noisyIrRoot));
	}
    


	// if (N->mode & kNoisyConfigModeCallTracing)
	// {
	// 	noisyConfigTimeStampDumpTimeline(N);
	// }
    
    noisyConsolePrintBuffers(N);
}


static void
version(NoisyState *  N)
{
	flexprint(N->Fe, N->Fm, N->Fperr, "\nNewton version %s.\n\n", kNewtonVersion);
}


static void
usage(NoisyState *  N)
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