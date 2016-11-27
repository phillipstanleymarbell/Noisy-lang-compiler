#ifdef NoisyOsMacOSX
#	include <mach/mach.h>
#	include <mach/mach_time.h>
#	include <unistd.h>
#endif

#ifdef NoisyOsMacOSX
#	include <dispatch/dispatch.h>
#	include <OpenCL/opencl.h>
#endif

#ifdef NoisyConfigOsLinux
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
#include "noisy-errors.h"
#include "noisyconfig.h"
#include "noisyconfig-irPass-helpers.h"
#include "noisyconfig-irPass-dotBackend.h"

/*
 *	NOTE / TODO / BUG: (delete this once th eimplementation settles)
 *
 *	(1) 	The way we handle lexing in M and NoisyConfig compilers does not use the
 *		'stickies' as we do in our Yacc-based parsers.
 *
 *	(2)	We currently split up the input by '\n'-separated newline. This is
 *		OK, since we also recognize '\r' as being a discardable whitespace.
 */
//const char	gNoisyConfigEol[]			= "\n\r";
//const char	gNoisyConfigWhitespace[]		= " \t\n\r";
//const char	gNoisyConfigStickies[]		= "~!%&*()+=[]{}\\|:;'\",<.>/?";

/*
 *	NOTE: -Tpng:gd gets rid of the ugly edge borders which are there in
 *	-Tpng and -Tpng:cairo. However, putting the ":gd" in the arg makes
 *	it not work via rfork, though it does via command line??!!
 */
static char *	kNoisyConfigDotArgsPNG[]		= {"dot", "-Tpng", "-O"};
static char	kNoisyConfigRenderExtensionPNG[]	= ".png";
static char *	kNoisyConfigDotArgsPDF[]		= {"dot", "-Tpdf", "-O"};
static char	kNoisyConfigRenderExtensionPDF[]	= ".pdf";
static char *	kNoisyConfigDotArgsSVG[]		= {"dot", "-Tsvg", "-O"};
static char	kNoisyConfigRenderExtensionSVG[]	= ".svg";



NoisyConfigState *
noisyConfigInit(NoisyConfigMode mode)
{
	NoisyConfigState *	N;

	N = (NoisyConfigState *)calloc(1, sizeof(NoisyConfigState));
	if (N == NULL)
	{
		noisyConfigFatal(NULL, Emalloc);
	}
	N->mode = mode;



	N->Fe = (FlexErrState *)calloc(1, sizeof(FlexErrState));
	if (N->Fe == NULL)
	{
		noisyConfigFatal(NULL, Emalloc);
	}

	N->Fm = (FlexMstate *)calloc(1, sizeof(FlexMstate));
	if (N->Fm == NULL)
	{
		noisyConfigFatal(NULL, Emalloc);
	}
	N->Fm->debug = 0;



	/*
	 *	Used to hold error messages
	 */
	N->Fperr = (FlexPrintBuf *)calloc(1, sizeof(FlexPrintBuf));
	if (N->Fperr == NULL)
	{
		noisyConfigFatal(NULL, Emalloc);
	}

	//TODO: need to figure out right buffer size dynamically. 
	N->Fperr->circbuf = (char *)calloc(1, FLEX_CIRCBUFSZ);
	if (N->Fperr->circbuf == NULL)
	{
		noisyConfigFatal(NULL, Emalloc);
	}

	/*
	 *	Used to hold informational messages
	 */
	N->Fpinfo = (FlexPrintBuf *)calloc(1, sizeof(FlexPrintBuf));
	if (N->Fpinfo == NULL)
	{
		noisyConfigFatal(NULL, Emalloc);
	}

	//TODO: need to figure out right buffer size dynamically. 
	N->Fpinfo->circbuf = (char *)calloc(1, FLEX_CIRCBUFSZ);
	if (N->Fpinfo->circbuf == NULL)
	{
		noisyConfigFatal(NULL, Emalloc);
	}

	/*
	 *	Used during lexing
	 */
	N->currentToken = calloc(kNoisyConfigMaxBufferLength, sizeof(char));
	if (N->currentToken == NULL)
	{
		noisyConfigFatal(NULL, Emalloc);
	}

#ifdef NoisyOsMacOSX
	dispatch_queue_t queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_GPU, NULL);
	if (queue == NULL)
	{
		queue = gcl_create_dispatch_queue(CL_DEVICE_TYPE_CPU, NULL);
	}

	char		name[128];
	cl_device_id	gpu = gcl_get_device_id_with_dispatch_queue(queue);
	clGetDeviceInfo(gpu, CL_DEVICE_NAME, 128, name, NULL);
//	flexprint(N->Fe, N->Fm, N->Fpinfo, "OpenCL enabled on device %s\n", name);
#endif

	return N;
}



void
noisyConfigDealloc(NoisyConfigState *  N)
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


	//TODO: recursively free all the nodes
}


void
noisyConfigRunPasses(NoisyConfigState *  N)
{
	/*
	 *	Convert the literal strings in tree for numeric and real-valued constants into uint64_t / double.
	 */
	//if (N->irPasses & kNoisyConfigIrPassXXX)
	{
		//noisyConfigIrPassXXX(C, N->noisyConfigIrRoot, true);
	}
}



uint64_t
noisyConfigCheckRss(NoisyConfigState *  N)
{
	char		tmp, *ep = &tmp, buf[kNoisyConfigMaxBufferLength];
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
		noisyConfigError(N, Epipe);
		return 0;
	}
	
	fgets(buf, kNoisyConfigMaxBufferLength, pipe);
	ret = strtoul(buf, &ep, 0);

	pclose(pipe);
#endif


	return ret;	
}



void
noisyConfigConsolePrintBuffers(NoisyConfigState *  N)
{
	//TODO: need a better thought out way to handle printing out the internal buffers when we are running from the command line	
	if (N && N->Fpinfo && strlen(N->Fpinfo->circbuf))
	{
		fprintf(stdout, "\nInformational Report:\n---------------------\n%s", N->Fpinfo->circbuf);
		if (N->mode & kNoisyConfigModeCGI)
		{
			fflush(stdout);
		}
	}

	if (N && N->Fperr && strlen(N->Fperr->circbuf))
	{
		if (N->mode & kNoisyConfigModeCGI)
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
noisyConfigPrintToFile(NoisyConfigState *  N, const char *  msg, const char *  fileName, NoisyConfigPostFileWriteAction action)
{
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
	if (N->mode & kNoisyConfigModeCGI)
	{
		int	stubAndRandomDigitsNameLength = strlen(fileName)+kNoisyConfigCgiRandomDigits+1;

		randomizedFileName = (char *) calloc(stubAndRandomDigitsNameLength, sizeof(char));
		if (randomizedFileName == NULL)
		{
			noisyConfigFatal(N, Emalloc);
		}

		snprintf(randomizedFileName, stubAndRandomDigitsNameLength, "%s%0*d", fileName, kNoisyConfigCgiRandomDigits, (int)random());
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
		noisyConfigFatal(N, Emalloc);
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
	if (action & kNoisyConfigPostFileWriteActionRenderDot)
	{
		noisyConfigRenderDotInFile(N, pathName, randomizedFileName);
	}
	
	free(pathName);
	free(randomizedFileName);


	return;
}



void
noisyConfigRenderDotInFile(NoisyConfigState *  N, char *  pathName, char *  randomizedFileName)
{
	/*	The N->lastrender purposefully does not contain VM_BASEPATH	*/
	N->lastDotRender = realloc(N->lastDotRender, (strlen(randomizedFileName)+1) * sizeof(char));
	if (N->lastDotRender == NULL)
	{
		noisyConfigFatal(N, Emalloc);
	}

	strcpy(N->lastDotRender, randomizedFileName);

	/*	Need to fork once for each child renderer:	*/
	switch (fork())
	{
		case 0:
		{
			execl(kNoisyDotCommand, kNoisyConfigDotArgsPNG[0], kNoisyConfigDotArgsPNG[1], kNoisyConfigDotArgsPNG[2], pathName, NULL);
			exit(0);
		}

		case -1:
		{
			noisyConfigFatal(N, Efork);
			break;
		}

		default:
		{
			if (N->mode & kNoisyConfigModeCGI)
			{
				noisyConfigCheckCgiCompletion(N, pathName, kNoisyConfigRenderExtensionPNG);
			}
		}
	}

	switch (fork())
	{
		case 0:
		{
			execl(kNoisyDotCommand, kNoisyConfigDotArgsPDF[0], kNoisyConfigDotArgsPDF[1], kNoisyConfigDotArgsPDF[2], pathName, NULL);
			exit(0);
		}

		case -1:
		{
			noisyConfigFatal(N, Efork);
			break;
		}

		default:
		{
			if (N->mode & kNoisyConfigModeCGI)
			{
				noisyConfigCheckCgiCompletion(N, pathName, kNoisyConfigRenderExtensionPDF);
			}
		}
	}
	switch (fork())
	{
		case 0:
		{
			execl(kNoisyDotCommand, kNoisyConfigDotArgsSVG[0], kNoisyConfigDotArgsSVG[1], kNoisyConfigDotArgsSVG[2], pathName, NULL);
			exit(0);
		}

		case -1:
		{
			noisyConfigFatal(N, Efork);
			break;
		}

		default:
		{
			if (N->mode & kNoisyConfigModeCGI)
			{
				noisyConfigCheckCgiCompletion(N, pathName, kNoisyConfigRenderExtensionSVG);
			}
		}
	}
}



//TODO/NOTE: this is not a bulletproof check for render success. it simply checks if the desired file is there and can be opened...
void
noisyConfigCheckCgiCompletion(NoisyConfigState *  N, const char *  pathName, const char *  renderExtension)
{
	char *	renderPathName;

	wait(NULL);


	renderPathName = (char *) calloc(strlen(pathName)+strlen(renderExtension)+1, sizeof(char));
	if (renderPathName == NULL)
	{
		noisyConfigFatal(N, Emalloc);
	}

	strcat(renderPathName, pathName);
	strcat(renderPathName, renderExtension);

	//TODO: replace with flexopen when that is cleaned up
	if (open(renderPathName, O_RDONLY) < 0)
	{
		flexprint(N->Fe, N->Fm, N->Fperr, "%s %ds\n", EdotRenderFailed, kNoisyConfigRlimitCpuSeconds);
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
noisyConfigFatal(NoisyConfigState *  N, const char *  msg)
{
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
		flexprint(N->Fe, N->Fm, N->Fperr, "\n%s%s\n\n", Efatal, msg);
	}

	if ((N != NULL) && (N->jmpbufIsValid))
	{
		noisyConfigConsolePrintBuffers(N);

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
	 *	a badly-fromed IR, noisyConfigIrDotBackend() will have to be
	 *	very cautious.
	 */
	if (N != NULL)
	{
		//flexprint(N->Fe, N->Fm, N->Fperr, "\nDump of NoisyConfig IR at point of failure:\n%s\n", noisyConfigIrDotBackend(N));
	}

	noisyConfigConsolePrintBuffers(N);

	/*
	 *	CGI depends on clean failure of cgi program
	 */
	if ((N != NULL) && (N->mode & kNoisyConfigModeCGI))
	{
		exit(EXIT_SUCCESS);
	}


	exit(EXIT_FAILURE);
}


void
noisyConfigError(NoisyConfigState *  N, const char *  msg)
{
	if (N == NULL)
	{
		noisyConfigFatal(N, Esanity);
	}

	if (!(N->verbosityLevel & kNoisyConfigVerbosityVerbose))
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
