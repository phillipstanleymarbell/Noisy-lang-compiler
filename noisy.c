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
#ifdef NoisyOsMacOSX
#	include <mach/mach.h>
#	include <mach/mach_time.h>
#	include <unistd.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <setjmp.h>
#include <fcntl.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisy-timeStamps.h"
#include "noisy.h"
#include "noisy-errors.h"
#include "noisy-irPass-helpers.h"
#include "noisy-irPass-dotBackend.h"

/*
 *	NOTE / TODO / BUG: (delete this once th eimplementation settles)
 *
 *	(1) 	The way we handle lexing in M and Noisy compilers does not use the
 *		'stickies' as we do in our Yacc-based parsers.
 *
 *	(2)	We currently split up the input by '\n'-separated newline. This is
 *		OK, since we also recognize '\r' as being a discardable whitespace.
 */
//const char	gNoisyEol[]			= "\n\r";
//const char	gNoisyWhitespace[]		= " \t\n\r";
//const char	gNoisyStickies[]		= "~!%&*()+=[]{}\\|:;'\",<.>/?";

/*
 *	NOTE: -Tpng:gd gets rid of the ugly edge borders which are there in
 *	-Tpng and -Tpng:cairo. However, putting the ":gd" in the arg makes
 *	it not work via rfork, though it does via command line??!!
 */
static char *	kNoisyDotArgsPNG[]		= {"dot", "-Tpng", "-O"};
static char	kNoisyRenderExtensionPNG[]	= ".png";
static char *	kNoisyDotArgsPDF[]		= {"dot", "-Tpdf", "-O"};
static char	kNoisyRenderExtensionPDF[]	= ".pdf";
static char *	kNoisyDotArgsSVG[]		= {"dot", "-Tsvg", "-O"};
static char	kNoisyRenderExtensionSVG[]	= ".svg";



void
noisyTimestampsInit(NoisyState *  N)
{
	N->timestamps = (NoisyTimeStamp *) calloc(kNoisyTimestampTimelineLength, sizeof(NoisyTimeStamp));
	if (N->timestamps == NULL)
	{
		noisyFatal(NULL, Emalloc);
	}
	N->timestampSlots = kNoisyTimestampTimelineLength;


	//TODO: replace this with a libflex call...
#ifdef NoisyOsMacOSX
	N->initializationTimestamp = mach_absolute_time();
#endif


	N->timeAggregates = (uint64_t *) calloc(kNoisyTimeStampKeyMax, sizeof(uint64_t));
	if (N->timeAggregates == NULL)
	{
		noisyFatal(NULL, Emalloc);
	}

	//TODO: revisit naming of timeAgregates/callAggregates, as well as the NoisyTimeStampKey prefix
	N->callAggregates = (uint64_t *) calloc(kNoisyTimeStampKeyMax, sizeof(uint64_t));
	if (N->callAggregates == NULL)
	{
		noisyFatal(NULL, Emalloc);
	}

	return;
}

NoisyState *
noisyInit(NoisyMode mode)
{
	NoisyState *	N;

	N = (NoisyState *)calloc(1, sizeof(NoisyState));
	if (N == NULL)
	{
		noisyFatal(NULL, Emalloc);
	}
	N->mode = mode;


	/*
	 *	We initialize this as early as possible. In most use cases however,
	 *	noisyInit() is called with kNoisyModeDefault, and as a result we
	 *	will not pick up noisyInit in statistics trace.
	 */
	if (mode & kNoisyModeCallStatistics)
	{
		noisyTimestampsInit(N);
	}
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyNoisyInit);



	N->Fe = (FlexErrState *)calloc(1, sizeof(FlexErrState));
	if (N->Fe == NULL)
	{
		noisyFatal(NULL, Emalloc);
	}

	N->Fm = (FlexMstate *)calloc(1, sizeof(FlexMstate));
	if (N->Fm == NULL)
	{
		noisyFatal(NULL, Emalloc);
	}
	N->Fm->debug = 0;



	/*
	 *	Used to hold error messages
	 */
	N->Fperr = (FlexPrintBuf *)calloc(1, sizeof(FlexPrintBuf));
	if (N->Fperr == NULL)
	{
		noisyFatal(NULL, Emalloc);
	}

	//TODO: need to figure out right buffer size dynamically. 
	N->Fperr->circbuf = (char *)calloc(1, FLEX_CIRCBUFSZ);
	if (N->Fperr->circbuf == NULL)
	{
		noisyFatal(NULL, Emalloc);
	}

	/*
	 *	Used to hold informational messages
	 */
	N->Fpinfo = (FlexPrintBuf *)calloc(1, sizeof(FlexPrintBuf));
	if (N->Fpinfo == NULL)
	{
		noisyFatal(NULL, Emalloc);
	}

	//TODO: need to figure out right buffer size dynamically. 
	N->Fpinfo->circbuf = (char *)calloc(1, FLEX_CIRCBUFSZ);
	if (N->Fpinfo->circbuf == NULL)
	{
		noisyFatal(NULL, Emalloc);
	}

	/*
	 *	Used during lexing
	 */
	N->currentToken = calloc(kNoisyMaxBufferLength, sizeof(char));
	if (N->currentToken == NULL)
	{
		noisyFatal(NULL, Emalloc);
	}
	

	return N;
}



void
noisyDealloc(NoisyState *  N)
{
	free(N->Fe);
	free(N->Fm);


	/*
	 *	These are not always allocated.
	 */
	if (N->Fperr)
	{
		//TODO: also dealloc the circular buffers...
		free(N->Fperr);
	}
	if (N->Fpinfo)
	{
		//TODO: also dealloc the circular buffers...
		free(N->Fpinfo);
	}


	if (N->mode & kNoisyModeCallStatistics)
	{
		free(N->timestamps);
		free(N->timeAggregates);
		free(N->callAggregates);
	}

	//TODO: recursively free all the nodes
}


void
noisyRunPasses(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyRunPasses);

	/*
	 *	Convert the literal strings in tree for numeric and real-valued constants into uint64_t / double.
	 */
	//if (N->irPasses & kNoisyIrPassXXX)
	{
		//noisyIrPassXXX(C, N->noisyIrRoot, true);
	}
}



uint64_t
noisyCheckRss(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyCheckRss);

	char		tmp, *ep = &tmp, buf[kNoisyMaxBufferLength];
	FILE *		pipe;
	uint64_t	ret;
	pid_t		pid = getpid();


#ifndef __CYGWIN__
	/*								*/
	/*	Using "rss=" causes an empty header, thus only		*/
	/*	value of rss.						*/
	/*								*/
	snprintf(buf, sizeof(buf), "ps -o rss= -p %d", pid);
	fflush(stdout);
	fflush(stderr);
	fflush(stdin);
	pipe = popen(buf, "r");
	if (pipe == NULL)
	{
		noisyError(N, Epipe);
		return 0;
	}
	
	fgets(buf, kNoisyMaxBufferLength, pipe);
	ret = strtoul(buf, &ep, 0);

	pclose(pipe);
#endif


	return ret;	
}



void
noisyConsolePrintBuffers(NoisyState *  N)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyConsolePrintBuffers);

	//TODO: need a better thought out way to handle printing out the internal buffers when we are running from the command line	
	if (N && N->Fpinfo && strlen(N->Fpinfo->circbuf))
	{
		fprintf(stdout, "\nInformational Report:\n---------------------\n%s", N->Fpinfo->circbuf);
		if (N->mode & kNoisyModeCGI)
		{
			fflush(stdout);
		}
	}

	if (N && N->Fperr && strlen(N->Fperr->circbuf))
	{
		if (N->mode & kNoisyModeCGI)
		{
			fprintf(stdout, "Error Report:\n-------------\n%s", N->Fperr->circbuf);
			fflush(stdout);
		}
		else
		{
			fprintf(stderr, "\nError Report:\n-------------\n%s", N->Fperr->circbuf);
		}
	}
}



void
noisyPrintToFile(NoisyState *  N, const char *  msg, const char *  fileName, NoisyPostFileWriteAction action)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyPrintToFile);

	int	fd;
	char *	endName;
	char *	pathName;
	char *	randomizedFileName;


	if (fileName == NULL)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s; Message to be printed to file was [%s].", EnullFileNameInPrintToFile, msg);
		
		return;
	}


	/*
	 *	For CGI mode, we randomize the fileName.
	 */
	if (N->mode & kNoisyModeCGI)
	{
		int	stubAndRandomDigitsNameLength = strlen(fileName)+kNoisyCgiRandomDigits+1;

		randomizedFileName = (char *) calloc(stubAndRandomDigitsNameLength, sizeof(char));
		if (randomizedFileName == NULL)
		{
			noisyFatal(N, Emalloc);
		}

		snprintf(randomizedFileName, stubAndRandomDigitsNameLength, "%s%0*d", fileName, kNoisyCgiRandomDigits, (int)random());
	}
	else
	{
		/*
		 *	NOTE: we dealloc rfilename below.
		 */
		randomizedFileName = strdup(fileName);
	}


	/*							*/
	/*	We do not permit arbitrary path, and only use	*/
	/*	end of path.					*/
	/*							*/
	endName	 = strrchr(randomizedFileName, '/');
	if (endName == NULL)
	{
		endName = randomizedFileName;
	}

	pathName =  (char *) calloc(strlen(endName)+strlen(kNoisyBasePath) + 1, sizeof(char));
	if (pathName == NULL)
	{
		noisyFatal(N, Emalloc);
	}

	strcat(pathName, kNoisyBasePath);
	strcat(pathName, endName);

//TODO: at the moment, flexlib's handling of opens is way too screwed... use normal open()
//	fd = flexcreate(N->Fe, N->Fm, N->Fperr, pathName, FLEX_OWRITE|FLEX_OTRUNCATE);
	fd = open(pathName, O_WRONLY|O_TRUNC|O_CREAT, S_IRWXU|S_IRGRP|S_IROTH);
	if (fd < 0)
	{
		/*								*/
		/*	We purposefully don't include BASEPATH in		*/
		/*	report, and don't include the randomized		*/
		/*	suffix either (i.e., fileName, not randomizedFileName)	*/
		/*								*/
		flexprint(N->Fe, N->Fm, N->Fperr, "%s \"%s\"...\n", Eopen, fileName);
		free(pathName);
		free(randomizedFileName);

		return;
	}

	flexwrite(N->Fe, N->Fm, N->Fperr, fd, (char *)msg, strlen(msg));
	flexclose(N->Fe, N->Fm, N->Fperr, fd);


	/*
	 *	Perform any post-write actions. For now, only post-write
	 *	hook is to render it via graphviz/dot.
	 */
	if (action & kNoisyPostFileWriteActionRenderDot)
	{
		noisyRenderDotInFile(N, pathName, randomizedFileName);
	}
	
	free(pathName);
	free(randomizedFileName);


	return;
}



void
noisyRenderDotInFile(NoisyState *  N, char *  pathName, char *  randomizedFileName)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyRenderDotInFile);

	/*	The N->lastrender purposefully does not contain VM_BASEPATH	*/
	N->lastDotRender = realloc(N->lastDotRender, (strlen(randomizedFileName)+1) * sizeof(char));
	if (N->lastDotRender == NULL)
	{
		noisyFatal(N, Emalloc);
	}

	strcpy(N->lastDotRender, randomizedFileName);

	/*	Need to fork once for each child renderer:	*/
	switch (fork())
	{
		case 0:
		{
			execl(kNoisyDotCommand, kNoisyDotArgsPNG[0], kNoisyDotArgsPNG[1], kNoisyDotArgsPNG[2], pathName, NULL);
			exit(0);
		}

		case -1:
		{
			noisyFatal(N, Efork);
			break;
		}

		default:
		{
			if (N->mode & kNoisyModeCGI)
			{
				noisyCheckCgiCompletion(N, pathName, kNoisyRenderExtensionPNG);
			}
		}
	}

	switch (fork())
	{
		case 0:
		{
			execl(kNoisyDotCommand, kNoisyDotArgsPDF[0], kNoisyDotArgsPDF[1], kNoisyDotArgsPDF[2], pathName, NULL);
			exit(0);
		}

		case -1:
		{
			noisyFatal(N, Efork);
			break;
		}

		default:
		{
			if (N->mode & kNoisyModeCGI)
			{
				noisyCheckCgiCompletion(N, pathName, kNoisyRenderExtensionPDF);
			}
		}
	}
	switch (fork())
	{
		case 0:
		{
			execl(kNoisyDotCommand, kNoisyDotArgsSVG[0], kNoisyDotArgsSVG[1], kNoisyDotArgsSVG[2], pathName, NULL);
			exit(0);
		}

		case -1:
		{
			noisyFatal(N, Efork);
			break;
		}

		default:
		{
			if (N->mode & kNoisyModeCGI)
			{
				noisyCheckCgiCompletion(N, pathName, kNoisyRenderExtensionSVG);
			}
		}
	}
}



//TODO/NOTE: this is not a bulletproof check for render success. it simply checks if the desired file is there and can be opened...
void
noisyCheckCgiCompletion(NoisyState *  N, const char *  pathName, const char *  renderExtension)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyCheckCgiCompletion);

	char *	renderPathName;

	wait(NULL);


	renderPathName = (char *) calloc(strlen(pathName)+strlen(renderExtension)+1, sizeof(char));
	if (renderPathName == NULL)
	{
		noisyFatal(N, Emalloc);
	}

	strcat(renderPathName, pathName);
	strcat(renderPathName, renderExtension);

	//TODO: replace with flexopen when that is cleaned up
	if (open(renderPathName, O_RDONLY) < 0)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s %ds\n", EdotRenderFailed, kNoisyRlimitCpuSeconds);
		if (N->lastDotRender)
		{
			free(N->lastDotRender);
			N->lastDotRender = NULL;
		}
	}
	free(renderPathName);

	return;
}


void
noisyFatal(NoisyState *  N, const char *  msg)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyFatal);

	fflush(stdout);
	fflush(stderr);

	/*										*/
	/*	Fatal message should always go to stderr / console, and not to GUI	*/
	/*	NOTE: F is NULL when fatal is evoked early in initialization.		*/
	/*										*/
	if (N == NULL)
	{
		fprintf(stderr, "\n%s%s\n\n", Efatal, msg);
	}
	else
	{
		if (N->mode & kNoisyModeCallTracing)
		{
			noisyTimeStampDumpTimeline(N);
		}

		if (N->mode & kNoisyModeCallStatistics)
		{
			noisyTimeStampDumpResidencies(N);
		}

		NoisyTimeStampTraceMacro(kNoisyTimeStampKeyFatal);
		flexprint(N->Fe, N->Fm, N->Fperr, "\n%s%s\n\n", Efatal, msg);
	}

	if ((N != NULL) && (N->jmpbufIsValid))
	{
		noisyConsolePrintBuffers(N);

		/*
		 *	Could pass in case-specific info here, but just
		 *	pass 0.
		 *
		 *	TODO: We could, e.g., return info on which line
		 *	number of the input we have reached, and let, e.g.,
		 *	the CGI version highlight the point at which
		 *	processing stopped.
		 */
		longjmp(N->jmpbuf, 0);
	}

	/*
	 *	TODO: Double check that we should not be able to get here if N->jmpbuf is set...
	 *	NOTE: When used as a library, N->jmpbuf will not be set, and we will indeed get here...
	 */


	/*
	 *	Dump the IR to Fperr. Because we might be trying to dump 
	 *	a badly-fromed IR, noisyIrDotBackend() will have to be
	 *	very cautious.
	 */
	if (N != NULL)
	{
		//flexprint(N->Fe, N->Fm, N->Fperr, "\nDump of Noisy IR at point of failure:\n%s\n", noisyIrDotBackend(N));
	}

	noisyConsolePrintBuffers(N);

	/*
	 *	CGI depends on clean failure of cgi program
	 */
	if ((N != NULL) && (N->mode & kNoisyModeCGI))
	{
		exit(EXIT_SUCCESS);
	}


	exit(EXIT_FAILURE);
}


void
noisyError(NoisyState *  N, const char *  msg)
{
	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyError);

	if (N == NULL)
	{
		noisyFatal(N, Esanity);
	}

	NoisyTimeStampTraceMacro(kNoisyTimeStampKeyError);

	if (!(N->verbosityLevel & kNoisyVerbosityVerbose))
	{
		return;
	}

	flexprint(N->Fe, N->Fm, N->Fperr, "\n%s%s\n", Eerror, msg);

	if (N != NULL && N->Fe != NULL && (char*)N->Fe->errstr != NULL)
	{
		snprintf(N->Fe->errstr, N->Fe->errlen, "%s%s", Eerror, msg);
	}

	return;
}
