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
#ifdef CommonOsMacOSX
#	include <mach/mach.h>
#	include <mach/mach_time.h>
#	include <unistd.h>
#	include <dispatch/dispatch.h>
#	include <OpenCL/opencl.h>
#endif

#ifdef CommonOsLinux
#	include <sys/types.h>
#	include <sys/wait.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <setjmp.h>
#include <fcntl.h>
#include <stdint.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#ifdef VariantNoisy
#	include "noisy-timeStamps.h"
#endif
#ifdef VariantNewton
#	include "newton-timeStamps.h"
#endif
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "common-errors.h"

/*
 *	NOTE: -Tpng:gd gets rid of the ugly edge borders which are there in
 *	-Tpng and -Tpng:cairo. However, putting the ":gd" in the arg makes
 *	it not work via rfork, though it does via command line??!!
 */
static char *	kCommonDotArgsPNG[]		= {"dot", "-Tpng", "-O"};
static char	kCommonRenderExtensionPNG[]	= ".png";
static char *	kCommonDotArgsPDF[]		= {"dot", "-Tpdf", "-O"};
static char	kCommonRenderExtensionPDF[]	= ".pdf";
static char *	kCommonDotArgsSVG[]		= {"dot", "-Tsvg", "-O"};
static char	kCommonRenderExtensionSVG[]	= ".svg";



void
timestampsInit(State *  N)
{
	N->timestamps = (TimeStamp *) calloc(kCommonTimestampTimelineLength, sizeof(TimeStamp));
	if (N->timestamps == NULL)
	{
		fatal(NULL, Emalloc);
	}
	N->timestampSlots = kCommonTimestampTimelineLength;

	//TODO: replace this with a libflex call...
#ifdef CommonOsMacOSX
	N->initializationTimestamp = mach_absolute_time();
#endif


	N->timeAggregates = (uint64_t *) calloc(kCommonTimeStampKeyMax, sizeof(uint64_t));
	if (N->timeAggregates == NULL)
	{
		fatal(NULL, Emalloc);
	}

	//TODO: revisit naming of timeAgregates/callAggregates, as well as the NoisyTimeStampKey prefix
	N->callAggregates = (uint64_t *) calloc(kCommonTimeStampKeyMax, sizeof(uint64_t));
	if (N->callAggregates == NULL)
	{
		fatal(NULL, Emalloc);
	}

	N->timestampCount = 0;
	N->callAggregateTotal = 0;
	N->timeAggregatesLastTimestamp = TimeMacro;
	N->timeAggregatesLastKey = kCommonTimeStampKeyTimeStampInit;

	return;
}

/*
 *	We used to use the `mode` to determine whether to initialize the
 *	timestamp statistics gathering and hence to call timestampsInit(N),
 *	but we now only call the latter when either the tracing or stats mode
 *	is on. We do need the mode set, since this routine is how CGI etc.
 *	initialize the State *N instance, and they pass in an appropriate mode.
 */
State *
init(CommonMode mode)
{
	State *	N;


	N = (State *)calloc(1, sizeof(State));
	if (N == NULL)
	{
		fatal(NULL, Emalloc);
	}
	N->mode = mode;


	N->Fe = (FlexErrState *)calloc(1, sizeof(FlexErrState));
	if (N->Fe == NULL)
	{
		fatal(NULL, Emalloc);
	}

	N->Fm = (FlexMstate *)calloc(1, sizeof(FlexMstate));
	if (N->Fm == NULL)
	{
		fatal(NULL, Emalloc);
	}
	N->Fm->debug = 0;



	/*
	 *	Used to hold error messages
	 */
	N->Fperr = (FlexPrintBuf *)calloc(1, sizeof(FlexPrintBuf));
	if (N->Fperr == NULL)
	{
		fatal(NULL, Emalloc);
	}

	//TODO: need to figure out right buffer size dynamically. 
	N->Fperr->circbuf = (char *)calloc(1, FLEX_CIRCBUFSZ);
	if (N->Fperr->circbuf == NULL)
	{
		fatal(NULL, Emalloc);
	}



	/*
	 *	Used to hold informational messages
	 */
	N->Fpinfo = (FlexPrintBuf *)calloc(1, sizeof(FlexPrintBuf));
	if (N->Fpinfo == NULL)
	{
		fatal(NULL, Emalloc);
	}

	//TODO: need to figure out right buffer size dynamically. 
	N->Fpinfo->circbuf = (char *)calloc(1, FLEX_CIRCBUFSZ);
	if (N->Fpinfo->circbuf == NULL)
	{
		fatal(NULL, Emalloc);
	}



	/*
	 *	Used to hold MathJax-formatted output
	 */
	N->Fpmathjax = (FlexPrintBuf *)calloc(1, sizeof(FlexPrintBuf));
	if (N->Fpmathjax == NULL)
	{
		fatal(NULL, Emalloc);
	}
	
	//TODO: need to figure out right buffer size dynamically. 
	N->Fpmathjax->circbuf = (char *)calloc(1, FLEX_CIRCBUFSZ);
	if (N->Fpmathjax->circbuf == NULL)
	{
		fatal(NULL, Emalloc);
	}



	/*
	 *	Used to hold SMT2 backend output
	 */
	N->Fpsmt2 = (FlexPrintBuf *)calloc(1, sizeof(FlexPrintBuf));
	if (N->Fpsmt2 == NULL)
	{
		fatal(NULL, Emalloc);
	}

	//TODO: need to figure out right buffer size dynamically. 
	N->Fpsmt2->circbuf = (char *)calloc(1, FLEX_CIRCBUFSZ);
	if (N->Fpsmt2->circbuf == NULL)
	{
		fatal(NULL, Emalloc);
	}



	/*
	 *	Used to hold C backend output
	 */
	N->Fpc = (FlexPrintBuf *)calloc(1, sizeof(FlexPrintBuf));
	if (N->Fpc == NULL)
	{
		fatal(NULL, Emalloc);
	}

	/*
	 *	FIXME: need to figure out right buffer size dynamically.
	 */
	N->Fpc->circbuf = (char *)calloc(1, FLEX_CIRCBUFSZ);
	if (N->Fpc->circbuf == NULL)
	{
		fatal(NULL, Emalloc);
	}

	/*
	 *	Used to hold signal typedef header file generation backend output
	 */
	N->Fph = (FlexPrintBuf *)calloc(1, sizeof(FlexPrintBuf));
	if (N->Fph == NULL)
	{
		fatal(NULL, Emalloc);
	}

	/*
	 *	Used to hold Noisy Code Generation output.
	 */
	N->Fpg = (FlexPrintBuf *)calloc(1, sizeof(FlexPrintBuf));
	if (N->Fpg == NULL)
	{
		fatal(NULL, Emalloc);
	}

	/*
	 *	FIXME: need to figure out right buffer size dynamically.
	 */
	N->Fph->circbuf = (char *)calloc(1, FLEX_CIRCBUFSZ);
	if (N->Fph->circbuf == NULL)
	{
		fatal(NULL, Emalloc);
	}

	N->Fpg->circbuf = (char *)calloc(1, FLEX_CIRCBUFSZ);
	if (N->Fpg->circbuf == NULL)
	{
		fatal(NULL, Emalloc);
	}

	/*
	 *	Used to hold RTL backend output
	 */
	N->Fprtl = (FlexPrintBuf *)calloc(1, sizeof(FlexPrintBuf));
	if (N->Fprtl == NULL)
	{
		fatal(NULL, Emalloc);
	}

	N->Fprtl->circbuf = (char *)calloc(1, FLEX_CIRCBUFSZ);
	if (N->Fprtl->circbuf == NULL)
	{
		fatal(NULL, Emalloc);
	}

	/*
	 *	Used to hold Ipsa backend output
	 */
	N->Fpipsa = (FlexPrintBuf *)calloc(1, sizeof(FlexPrintBuf));
	if (N->Fpipsa == NULL)
	{
		fatal(NULL, Emalloc);
	}

	N->Fpipsa->circbuf = (char *)calloc(1, FLEX_CIRCBUFSZ);
	if (N->Fpipsa->circbuf == NULL)
	{
		fatal(NULL, Emalloc);
	}

	/*
	 *	Used during lexing
	 */
	N->currentToken = calloc(kCommonMaxBufferLength, sizeof(char));
	if (N->currentToken == NULL)
	{
		fatal(NULL, Emalloc);
	}



	/*
	 *	Both 'gcl_create_dispatch_queue' and 'gcl_get_device_id_with_dispatch_queue'
	 *	are deprecated: first deprecated in macOS 10.14.
	 *	The following block is commented out after discussion with Phillip.
	 */
#ifdef CommonOsMacOSX
/*
	dispatch_queue_t queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_GPU, NULL);
	if (queue == NULL)
	{
		queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_CPU, NULL);
	}

	char		name[128];
	cl_device_id	gpu = gcl_get_device_id_with_dispatch_queue(queue);
	clGetDeviceInfo(gpu, CL_DEVICE_NAME, 128, name, NULL);
	flexprint(N->Fe, N->Fm, N->Fpinfo, "OpenCL enabled on device %s\n", name);
*/
#endif

	/*
	 *	Initialization for the targetParam backend. 
	 *	Used to signify whether an invariant for target param has been identified.
	 *	If not then the default kernel is the first regardless of how many there are.
	 */
	N->targetParamLocatedKernel = 0;

	/*
	 *	This is data type that a signal will be typedef'ed to
	 *	in the signal typedef generation backend. The default
	 *	data type is "double"
	 */
	N->signalTypedefDatatype = calloc(kCommonMaxBufferLength, sizeof(char));
	if (N->signalTypedefDatatype == NULL)
	{
		fatal(NULL, Emalloc);
	}
	else
	{
		strcpy(N->signalTypedefDatatype, "double");
	}

	return N;
}



void
dealloc(State *  N)
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


	if (N->mode & kCommonModeCallStatistics)
	{
		free(N->timestamps);
		free(N->timeAggregates);
		free(N->callAggregates);
	}

	//TODO: recursively free all the nodes
}



uint64_t
checkRss(State *  N)
{
	TimeStampTraceMacro(kCommonTimeStampKeyCheckRss);

	char		tmp, *ep = &tmp, buf[kCommonMaxBufferLength];
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
		error(N, Epipe);
		return 0;
	}
	
	fgets(buf, kCommonMaxBufferLength, pipe);
	ret = strtoul(buf, &ep, 0);

	pclose(pipe);
#endif


	return ret;
}



void
consolePrintBuffers(State *  N)
{
	TimeStampTraceMacro(kCommonTimeStampKeyConsolePrintBuffers);

	/*
	 *	TODO: need a better thought out way to handle printing out the internal buffers when we are running from the command line
	 */
	if (N && N->Fpmathjax && strlen(N->Fpmathjax->circbuf))
	{
		fprintf(stdout, "\nLaTeX Backend Output:\n---------------------\n%s", N->Fpmathjax->circbuf);
		if (N->mode & kCommonModeCGI)
		{
			fflush(stdout);
		}
	}

	if (N && N->Fpinfo && strlen(N->Fpinfo->circbuf))
	{
		fprintf(stdout, "\nInformational Report:\n---------------------\n%s", N->Fpinfo->circbuf);
		if (N->mode & kCommonModeCGI)
		{
			fflush(stdout);
		}
	}

	if (N && N->Fpsmt2 && strlen(N->Fpsmt2->circbuf))
	{
		fprintf(stdout, "\nSMT2 Backend output:\n---------------------\n%s", N->Fpsmt2->circbuf);
		if (N->mode & kCommonModeCGI)
		{
			fflush(stdout);
		}
	}

	if (N && N->Fpc && strlen(N->Fpc->circbuf))
	{
		fprintf(stdout, "\nC Backend output:\n---------------------\n%s", N->Fpc->circbuf);
		if (N->mode & kCommonModeCGI)
		{
			fflush(stdout);
		}
	}

	if (N && N->Fph && strlen(N->Fph->circbuf))
	{
		fprintf(stdout, "\nSignal typedef header generation Backend output:\n---------------------\n%s", N->Fph->circbuf);
		if (N->mode & kCommonModeCGI)
		{
			fflush(stdout);
		}
	}

	if (N && N->Fpg && strlen(N->Fpg->circbuf))
	{
		fprintf(stdout, "\nNoisy Code Generation:\n---------------------\n%s", N->Fpg->circbuf);
		if (N->mode & kCommonModeCGI)
		{
			fflush(stdout);
		}
	}

	if (N && N->Fprtl && strlen(N->Fprtl->circbuf))
	{
		fprintf(stdout, "\nRTL Backend output:\n---------------------\n%s", N->Fprtl->circbuf);
		
		if (N->mode & kCommonModeCGI)
		{
			fflush(stdout);
		}
		
	}

	if (N && N->Fpipsa && strlen(N->Fpipsa->circbuf))
	{
		fprintf(stdout, "\nIpsa Backend output:\n---------------------\n%s", N->Fpipsa->circbuf);
		
		if (N->mode & kCommonModeCGI)
		{
			fflush(stdout);
		}
		
	}

	if (N && N->Fperr && strlen(N->Fperr->circbuf))
	{
		if (N->mode & kCommonModeCGI)
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
printToFile(State *  N, const char *  msg, const char *  fileName, PostFileWriteAction action)
{
	TimeStampTraceMacro(kCommonTimeStampKeyPrintToFile);

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
	if (N->mode & kCommonModeCGI)
	{
		int	stubAndRandomDigitsNameLength = strlen(fileName)+kCommonCgiRandomDigits+1;

		randomizedFileName = (char *) calloc(stubAndRandomDigitsNameLength, sizeof(char));
		if (randomizedFileName == NULL)
		{
			fatal(N, Emalloc);
		}

		snprintf(randomizedFileName, stubAndRandomDigitsNameLength, "%s%0*d", fileName, kCommonCgiRandomDigits, (int)random());
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

	pathName =  (char *) calloc(strlen(endName)+strlen(kCommonBasePath) + 1, sizeof(char));
	if (pathName == NULL)
	{
		fatal(N, Emalloc);
	}

	strcat(pathName, kCommonBasePath);
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
	if (action & kCommonPostFileWriteActionRenderDot)
	{
		renderDotInFile(N, pathName, randomizedFileName);
	}
	
	free(pathName);
	free(randomizedFileName);


	return;
}



void
renderDotInFile(State *  N, char *  pathName, char *  randomizedFileName)
{
	TimeStampTraceMacro(kCommonTimeStampKeyRenderDotInFile);

	/*	The N->lastrender purposefully does not contain VM_BASEPATH	*/
	N->lastDotRender = realloc(N->lastDotRender, (strlen(randomizedFileName)+1) * sizeof(char));
	if (N->lastDotRender == NULL)
	{
		fatal(N, Emalloc);
	}

	strcpy(N->lastDotRender, randomizedFileName);

	/*	Need to fork once for each child renderer:	*/
	switch (fork())
	{
		case 0:
		{
			execl(kCommonDotCommand, kCommonDotArgsPNG[0], kCommonDotArgsPNG[1], kCommonDotArgsPNG[2], pathName, NULL);
			exit(0);
		}

		case -1:
		{
			fatal(N, Efork);
			break;
		}

		default:
		{
			if (N->mode & kCommonModeCGI)
			{
				checkCgiCompletion(N, pathName, kCommonRenderExtensionPNG);
			}
		}
	}

	switch (fork())
	{
		case 0:
		{
			execl(kCommonDotCommand, kCommonDotArgsPDF[0], kCommonDotArgsPDF[1], kCommonDotArgsPDF[2], pathName, NULL);
			exit(0);
		}

		case -1:
		{
			fatal(N, Efork);
			break;
		}

		default:
		{
			if (N->mode & kCommonModeCGI)
			{
				checkCgiCompletion(N, pathName, kCommonRenderExtensionPDF);
			}
		}
	}
	switch (fork())
	{
		case 0:
		{
			execl(kCommonDotCommand, kCommonDotArgsSVG[0], kCommonDotArgsSVG[1], kCommonDotArgsSVG[2], pathName, NULL);
			exit(0);
		}

		case -1:
		{
			fatal(N, Efork);
			break;
		}

		default:
		{
			if (N->mode & kCommonModeCGI)
			{
				checkCgiCompletion(N, pathName, kCommonRenderExtensionSVG);
			}
		}
	}
}



//TODO/NOTE: this is not a bulletproof check for render success. it simply checks if the desired file is there and can be opened...
void
checkCgiCompletion(State *  N, const char *  pathName, const char *  renderExtension)
{
	TimeStampTraceMacro(kCommonTimeStampKeyCheckCgiCompletion);

	char *	renderPathName;

	wait(NULL);


	renderPathName = (char *) calloc(strlen(pathName)+strlen(renderExtension)+1, sizeof(char));
	if (renderPathName == NULL)
	{
		fatal(N, Emalloc);
	}

	strcat(renderPathName, pathName);
	strcat(renderPathName, renderExtension);

	//TODO: replace with flexopen when that is cleaned up
	if (open(renderPathName, O_RDONLY) < 0)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s %ds\n", EdotRenderFailed, kCommonRlimitCpuSeconds);
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
fatal(State *  N, const char *  msg)
{
	TimeStampTraceMacro(kCommonTimeStampKeyFatal);

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
		if (N->mode & kCommonModeCallTracing)
		{
			timeStampDumpTimeline(N);
		}

		if (N->mode & kCommonModeCallStatistics)
		{
			timeStampDumpResidencies(N);
		}

		TimeStampTraceMacro(kCommonTimeStampKeyFatal);
		flexprint(N->Fe, N->Fm, N->Fperr, "\n%s%s\n\n", Efatal, msg);
	}

	if ((N != NULL) && (N->jmpbufIsValid))
	{
		consolePrintBuffers(N);

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
	 *	a badly-formed IR, noisyIrDotBackend() will have to be
	 *	very cautious.
	 */
	if (N != NULL)
	{
		//flexprint(N->Fe, N->Fm, N->Fperr, "\nDump of IR at point of failure:\n%s\n", noisyIrDotBackend(N));
	}

	consolePrintBuffers(N);

	/*
	 *	CGI depends on clean failure of cgi program
	 */
	if ((N != NULL) && (N->mode & kCommonModeCGI))
	{
		exit(EXIT_SUCCESS);
	}


	exit(EXIT_FAILURE);
}


void
error(State *  N, const char *  msg)
{
	TimeStampTraceMacro(kCommonTimeStampKeyError);

	if (N == NULL)
	{
		fatal(N, Esanity);
	}

	TimeStampTraceMacro(kCommonTimeStampKeyError);

	if (!(N->verbosityLevel & kCommonVerbosityVerbose))
	{
		return;
	}

	flexprint(N->Fe, N->Fm, N->Fperr, "\n%s%s\n", Eerror, msg);

	/*
	 *	The commented out condition of the if-clause below causes the
	 *	following error with GCC 12.1:
	 *
	 *		common-utils.c: In function ‘error’:
	 *		common-utils.c:839:64: error: the comparison will always evaluate as ‘true’ for the address of ‘errstr’ will never be NULL [-Werror=address]
  	 *		  839 |         if (N != NULL && N->Fe != NULL && (char*)N->Fe->errstr != NULL)
	 *
	 */
	if (N != NULL && N->Fe != NULL /* && (char*)N->Fe->errstr != NULL */)
	{
		snprintf(N->Fe->errstr, N->Fe->errlen, "%s%s", Eerror, msg);
	}

	return;
}
