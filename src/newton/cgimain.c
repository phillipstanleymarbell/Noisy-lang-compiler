/*
	Authored 2015--2017. Phillip Stanley-Marbell.

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
#include <unistd.h>
#include <sys/resource.h>
#include <getopt.h>
#include <setjmp.h>
#include <signal.h>

#ifdef __linux__
#else
#	include <sys/syslimits.h>
#endif

#include <fcntl.h>
#include <sys/param.h>
#include <stdint.h>
#include <inttypes.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "common-errors.h"
#include "noisy-timeStamps.h"
#include "common-timeStamps.h"
#include "common-data-structures.h"
#include "common-lexers-helpers.h"
#include "common-irPass-helpers.h"
#include "version.h"

#include "newton-parser.h"
#include "newton-lexer.h"
#include "common-symbolTable.h"
#include "newton.h"
#include "newton-irPass-dotBackend.h"
#include "newton-irPass-smtBackend.h"
#include "newton-irPass-dimensionalMatrixAnnotation.h"
#include "newton-irPass-dimensionalMatrixPiGroups.h"
#include "newton-irPass-dimensionalMatrixPrinter.h"
#include "newton-irPass-dimensionalMatrixKernelPrinter.h"
#include "newton-dimension-pass.h"

extern char *			gNewtonAstNodeStrings[kNoisyIrNodeTypeMax];

static const char		kNewtonCgiInputLogStub[]	= "XXXXXXXXXX";
static const char		kNewtonCgiInputLogExtension[]	= ".nt";

static char **			getCgiVars(void);
static void			htmlPrint(char *  s);
static void			doTail(int fmtWidth, int cgiSparameter, int cgiOparameter, int cgiTparameter);

static struct rusage		start, end;
static uint64_t			startRss, endRss;
static char *			newtonCodeBuffer = NULL;


State *				newtonCgiState;
State *				newtonCgiDimensionsState;

enum
{
	kNoisyCgiFormatWidth		= 580,
	kNoisyCgiChunkCgiParse		= 1024,
};



void
timeoutSignalHandler(int signum)
{
	/*
	 *	Cannot use printf in signal handler, but write is allowed.
	 */
	write(STDOUT_FILENO, WcgiAsync, strlen(WcgiAsync)+1);
	write(STDOUT_FILENO, "</body>\n", strlen("</body>\n")+1);
	write(STDOUT_FILENO, "</html>\n", strlen("</html>\n")+1);
	write(STDOUT_FILENO, "\n", strlen("\n")+1);

	/*
	 *	We could pass in case-specific information here, but we
	 *	just pass 0.
	 */
	if ((newtonCgiState != NULL) && (newtonCgiState->jmpbufIsValid))
	{
		longjmp(newtonCgiState->jmpbuf, 0);
	}

	exit(EXIT_FAILURE);
}



void
timerSignalHandler(int signum)
{
	write(STDOUT_FILENO, " ", strlen(" ")+1);
	
	return;
}



/*
 *	The next three functions based on the eponymous functions
 *	in hello.c by James Marshall (CGI examples on the web).
 */

char
hex2char(char *hex)
{
	return	16 *	(hex[0] >= 'A' ? ((hex[0] & 0xdf) - 'A' + 10) : (hex[0] - '0'))
		+ 	(hex[1] >= 'A' ? ((hex[1] & 0xdf) - 'A' + 10) : (hex[1] - '0'));
}

void
unescape(char *url)
{
	int	i = 0, j = 0;

	for (; url[j]; ++i, ++j)
	{
		if ((url[i] = url[j]) == '%')
		{
			url[i] = hex2char(&url[j+1]) ;
			j += 2 ;
		}
	}

	url[i] = '\0' ;
}

char **
getCgiVars(void)
{
	int	i, paircount;
	char	*reqmethod, *input = NULL, **vars, **pairlist, *nvpair, *eqpos ;


	reqmethod= getenv("REQUEST_METHOD") ;
	if (reqmethod == NULL)
	{
		error(newtonCgiState, EbadCgiQuery);
		return NULL;
	}

	if (!strcmp(reqmethod, "GET") || !strcmp(reqmethod, "HEAD"))
	{
		char	*qs;

		/*
		 *	Some servers don't provide QUERY_STRING if
		 *	it's empty, so avoid strdup()'ing a NULL
		 *	pointer here.
		 */
		qs = getenv("QUERY_STRING");
		input = strdup(qs ? qs : "");
	}
	else
	{
		error(newtonCgiState, EbadCgiQuery);
		return NULL;
	}

	/*
	 *	Convert +'s back to spaces
	 */
	for (i=0; input[i]; i++)
	{
		if (input[i] == '+')
		{
			input[i] = ' ';
		}
	}

	/*
	 *	Split on "&" and ";" to extract the name-value
	 *	pairs into pairlist.
	 */
	pairlist = (char **) malloc(kNoisyCgiChunkCgiParse*sizeof(char *));
	if (pairlist == NULL)
	{
		fatal(newtonCgiState, Emalloc);
	}

	paircount = 0;
	nvpair = strtok(input, "&;");
	while (nvpair)
	{
		pairlist[paircount++] = strdup(nvpair);

		if (!(paircount % kNoisyCgiChunkCgiParse))
		{
			pairlist = (char **) realloc(pairlist, (paircount + kNoisyCgiChunkCgiParse)*sizeof(char *));
		}

		nvpair = strtok(NULL, "&;");
	}

	/*
	 *	NULL-terminate the list.
	 */
	pairlist[paircount] = 0;

	/*
	 *	Extract names and values.
	 */
	vars = (char **) malloc((paircount*2+1)*sizeof(char *));
	if (vars == NULL)
	{
		fatal(newtonCgiState, Emalloc);
	}

	for (i= 0; i<paircount; i++)
	{
		if ((eqpos = strchr(pairlist[i], '=')))
		{
			*eqpos = '\0';
			unescape(vars[i*2+1] = strdup(eqpos+1)) ;
		}
		else
		{
			unescape(vars[i*2+1] = strdup(""));
		}

		unescape(vars[i*2] = strdup(pairlist[i]));
	}

	/*
	 *	NULL-terminate the list.
	 */
	vars[paircount*2] = 0;

	free(input);
	for (i=0; pairlist[i]; i++)
	{
		free(pairlist[i]);
	}
	free(pairlist);

	return vars;
}



int
main(void)
{
	char **			cgiVars;
	char			logFileStub[kNoisyMaxFilenameLength+1];
	int			jumpParameter, logFd, i;
	int			fmtWidth = kNoisyCgiFormatWidth, cgiSparameter = 0, cgiOparameter = 0, cgiTparameter = 0;
	char			tmp;
	char *			ep = &tmp;
	struct rlimit		rlp;
	struct timeval		t;
	struct sigaction	sa;
	struct itimerval	itv;


	gettimeofday(&t, NULL);
	srandom(t.tv_usec);


	/*
	 *	Limit CGI version resource usage. On some systems,
	 *	SIGXCPU/SIGXFSV is generated at soft limit, and SIGKILL
	 *	at hard limit, so we set them to different but meaningful
	 *	values.
	 */
	rlp.rlim_cur = kNoisyRlimitRssBytes;
	rlp.rlim_max = kNoisyRlimitRssBytes + 1;
	setrlimit(RLIMIT_RSS, &rlp);

	rlp.rlim_cur = kNoisyRlimitCpuSeconds;
	rlp.rlim_max = kNoisyRlimitCpuSeconds + 1;
	setrlimit(RLIMIT_CPU, &rlp);

	memset(&sa, 0, sizeof(sa));

	/*
	 *	Block all other signals when handling timeout
	 */
	sigfillset(&sa.sa_mask);

	sa.sa_handler = timeoutSignalHandler;
	sigaction(SIGABRT, &sa, NULL);
	sigaction(SIGALRM, &sa, NULL);
	sigaction(SIGBUS, &sa, NULL);
#ifndef __linux__
	sigaction(SIGEMT, &sa, NULL);
#endif
	sigaction(SIGFPE, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGILL, &sa, NULL);
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGKILL, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
	sigaction(SIGPROF, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);
	sigaction(SIGSEGV, &sa, NULL);
	sigaction(SIGSYS, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGTRAP, &sa, NULL);
	sigaction(SIGTSTP, &sa, NULL);
	sigaction(SIGTTIN, &sa, NULL);
	sigaction(SIGTTOU, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
	sigaction(SIGXCPU, &sa, NULL);
	sigaction(SIGXFSZ, &sa, NULL);


	/*
	 *	Install a separate signal handler for the timer. Besides giving
	 *	visual feedback as to progress, Apache will anyway  terminate cgi
	 *	programs that don't produce any output over some given timeout,
	 *	period (yielding a "Timeout waiting for output from CGI script").
	 */
	itv.it_interval.tv_sec = kNoisyProgressTimerSeconds;
	itv.it_interval.tv_usec = 0;
	itv.it_value.tv_sec = kNoisyProgressTimerSeconds;
	itv.it_value.tv_usec = 0;
	setitimer(ITIMER_VIRTUAL, &itv, NULL);

	/*
	 *	SIGVTALRM is delivered when the above timer expires.
	 */
	sa.sa_handler = timerSignalHandler;
	sigaction(SIGVTALRM, &sa, NULL);


	newtonCgiState = init(kNoisyModeDefault|kNoisyModeCallStatistics/* | kNoisyModeCallTracing */|kNoisyModeCGI);
	newtonCgiDimensionsState = init(kNoisyModeDefault|kNoisyModeCallStatistics/* | kNoisyModeCallTracing */|kNoisyModeCGI);
	timestampsInit(newtonCgiState);


	/*
	 *	Extra \n, to send the blank line:
	 */
	printf("Content-type: text/html\n\n");

	/*
	 *	KaTeX requires the use of the HTML5 doctype. Without it, KaTeX may not render properly
	 */
	printf("<!DOCTYPE html>\n");
	printf("<html>\n");
	printf("<head>\n");
	printf("<title>Newton version %s</title>\n", kNewtonVersion);
	printf("<link rel=\"stylesheet\" type=\"text/css\" href=\"http://fonts.googleapis.com/css?family=Source+Sans+Pro:400,300\">\n");
	printf("<link rel=\"stylesheet\" type=\"text/css\" href=\"http://fonts.googleapis.com/css?family=Source+Code+Pro:400,300\">\n");

	/*
	 *	MathJax
	 */
	//printf("        <script type=\"text/javascript\" async\n");
	//printf("          src=\"https://cdnjs.cloudflare.com/ajax/libs/mathjax/2.7.5/latest.js?config=TeX-MML-AM_CHTML\">\n");
	//printf("        </script>\n");

	/*
	 *	KaTeX with autorender (see https://katex.org/docs/autorender.html).
	 *
	 *	We purposefully _remove_ "pre" from the list of ignoreTags so we can
	 *	have $$...$$ inside <pre></pre> (we currently don't use this feature
	 *	since we group all the LaTeX output into a separate buffer for now).
	 */
	printf("<link rel=\"stylesheet\" href=\"https://cdn.jsdelivr.net/npm/katex@0.10.0-rc.1/dist/katex.min.css\" integrity=\"sha384-D+9gmBxUQogRLqvARvNLmA9hS2x//eK1FhVb9PiU86gmcrBrJAQT8okdJ4LMp2uv\" crossorigin=\"anonymous\">\n");
	printf("<script defer src=\"https://cdn.jsdelivr.net/npm/katex@0.10.0-rc.1/dist/katex.min.js\" integrity=\"sha384-483A6DwYfKeDa0Q52fJmxFXkcPCFfnXMoXblOkJ4JcA8zATN6Tm78UNL72AKk+0O\" crossorigin=\"anonymous\"></script>\n");
	printf("<script defer src=\"https://cdn.jsdelivr.net/npm/katex@0.10.0-rc.1/dist/contrib/auto-render.min.js\" integrity=\"sha384-yACMu8JWxKzSp/C1YV86pzGiQ/l1YUfE8oPuahJQxzehAjEt2GiQuy/BIvl9KyeF\" crossorigin=\"anonymous\"></script>\n");
	printf("<script>\n");
	printf("    document.addEventListener(\"DOMContentLoaded\", function() {\n");
	printf("        renderMathInElement(document.body, {\n");
	printf("            ignoredTags: [\"script\", \"noscript\", \"style\", \"textarea\", \"code\"]\n");
	printf("        });\n");
	printf("    });\n");
	printf("</script>\n");
	
	
	/*
	 *	Javascript for ACE editor hookup. Needs both ACE code editor plugin and jquery-git to work.
	 *
	 *	See	https://groups.google.com/forum/#!topic/ace-discuss/dDMVH_RbsAk
	 */
	printf("        <script src=\"../tmp/jquery-git.js\"></script>\n");
	printf("        <style type=\"text/css\" media=\"screen\">\n");
	printf("          body {\n");
	/*
	 *	NOTE:	Use this to prevent page ever having a scroll bar:
	 *
	 *	printf("              overflow: hidden;\n");
	 */
	printf("              overflow: scroll;\n");

	/*
	 *	More <body> defaults:
	 */
	printf("              font-family: 'Source Sans Pro', sans-serif;\n");
	printf("              font-weight: 300;\n");
	printf("              font-size: 12.5px;\n");
	printf("              color: #777777;\n");
	printf("          }\n");

	printf("          b {\n");
	printf("              font-family: 'Source Sans Pro', sans-serif;\n");
	printf("              font-weight: 400;\n");
	printf("              font-size:12.5px;\n");
	printf("          }\n");

	printf("          #editor {\n");
	printf("              margin: 0;\n");
	printf("              position: absolute;\n");
	printf("              top: 0;\n");
	printf("              bottom: 0;\n");
	printf("              left: 0;\n");
	printf("              right: 0;\n");
	printf("          }\n");

	printf("          td {\n");
	printf("              font-family: 'Source Sans Pro', sans-serif;\n");
	printf("              font-size:12.5px;\n");
	printf("              font-weight: 300;\n");
	printf("              color: #444444;\n");
	printf("          }\n");

	printf("          pre {\n");
	printf("              font-family: 'Source Code Pro', sans-serif;\n");
	printf("              font-weight: 300;\n");
	printf("              font-size:12px;\n");
	printf("              color: #444444;\n");
	printf("          }\n");

	printf("          pre b {\n");
	printf("              font-family: 'Source Code Pro', sans-serif;\n");
	printf("              font-weight: 400;\n");
	printf("              font-size:12px;\n");
	printf("              color: #444444;\n");
	printf("          }\n");

	printf("          input {\n");
	printf("              font-family: 'Source Code Pro', sans-serif;\n");
	printf("              font-weight: 400;\n");
	printf("              font-size:12px;\n");
	printf("              color: #777777;\n");
	printf("          }\n");

	printf("	a {\n");
	printf("	  outline: none;\n");
	printf("	  text-decoration: none;\n");
	printf("	}\n");

	printf("	a:link {\n");
	printf("	  color: #222222;\n");
	printf("	  font-weight: 500;\n");
	printf("	}\n");

	printf("	a:visited {\n");
	printf("	  color: #448822;\n");
	printf("	}\n");

	printf("	a:focus {\n");
 	printf("	  border-bottom: 5px solid;\n");
	printf("	  background: #FF3333;\n");
	printf("	}\n");

	printf("	a:hover {\n");
	printf("	  border-bottom: 5px solid;\n");
	printf("	  background: #CCFFAA;\n");
	printf("	}\n");

	printf("	a:active {\n");
	printf("	  background: #333333;\n");
	printf("	  color: #FF3333;\n");
	printf("	}\n");
	
	printf("	a img {\n");
	printf("	  display: block;\n");
	printf("	}\n");

	printf("        </style>\n");


	/*
	 *	A little bit of inline JavaScript show/hide "Errors".
	 *	(from http://www.dustindiaz.com/seven-togglers/)
	 */
	printf("<script type=\"text/javascript\">\n");
	printf("	function toggle(obj)\n");
	printf("	{\n");
	printf("		var x = document.getElementById(obj);\n");
	printf("		if (x.style.display != 'none')");
	printf("		{\n");
	printf("			x.style.display = 'none';\n");
	printf("		}\n");
	printf("		else");
	printf("		{\n");
	printf("			x.style.display = '';\n");
	printf("		}\n");
	printf("	}\n");
	printf("</script>\n");


	/*
	 *	See issue #292
	 */
	printf("<link rel=\"mask-icon\" href=\"../tmp/newton-pinned-tab-logo.svg\" color=\"orange\">");

	printf("</head>\n");

	printf("<body text=\"#555555\" bgcolor=\"#FFFFFC\">\n");

	cgiVars = getCgiVars();
	for (i = 0; cgiVars[i]; i+= 2)
	{
		if (!strcmp(cgiVars[i], "c"))
		{
			newtonCodeBuffer = cgiVars[i+1];
		}
		
		if (!strcmp(cgiVars[i], "w"))
		{
			unsigned long tmpUlong = strtoul(cgiVars[i+1], &ep, 0);
			if (*ep == '\0')
			{
				fmtWidth = tmpUlong;
			}
		}
		
		if (!strcmp(cgiVars[i], "s"))
		{
			unsigned long tmpUlong = strtoul(cgiVars[i+1], &ep, 0);
			if (*ep == '\0')
			{
				cgiSparameter = tmpUlong;
				newtonCgiState->irBackends = cgiSparameter;
			}
			else
			{
				//newtonCgiState->irBackends |= kNoisyIrBackendXXX;
			}
		}
		
		if (!strcmp(cgiVars[i], "t"))
		{
			unsigned long tmpUlong = strtoul(cgiVars[i+1], &ep, 0);
			if (*ep == '\0')
			{
				cgiTparameter = tmpUlong;
				newtonCgiState->dotDetailLevel = cgiTparameter;
			}
			else
			{
				newtonCgiState->dotDetailLevel = 0;
			}
		}
		
		if (!strcmp(cgiVars[i], "o"))
		{
			unsigned long tmpUlong = strtoul(cgiVars[i+1], &ep, 0);
			if (*ep == '\0')
			{
				cgiOparameter = tmpUlong;
				newtonCgiState->irPasses = cgiOparameter;
				newtonCgiState->verbosityLevel = 2;
			}
			else
			{
				//newtonCgiState->irPasses |= kNoisyIrPassXXX;
			}
		}
	}

	flexprint(newtonCgiState->Fe, newtonCgiState->Fm, newtonCgiState->Fpinfo,
		"Newton Compiler Parameters:\n\n\tBackends = [%llu],\n\tIR passes = [%llu]\n\n\n",
		newtonCgiState->irBackends, newtonCgiState->irPasses);
	fflush(stdout);


	/*
	 *	Log the input to a file; mkstemps() require the stub to be
	 *	writeable.
	 */
	snprintf(logFileStub, kNoisyMaxFilenameLength, "%sinput-%s-%s.nt", kNewtonBasePath, getenv("REMOTE_ADDR"), kNewtonCgiInputLogStub);
	logFd = mkstemps(logFileStub, strlen(kNewtonCgiInputLogExtension));
	if (logFd == -1)
	{
		fprintf(stdout, "mkstemps() failed for stub [%s]\n", logFileStub);
		fatal(newtonCgiState, Emkstemps);
	}
	else
	{
		write(logFd, newtonCodeBuffer, strlen(newtonCodeBuffer));
	}

	/*
	 *	Force all streams to be written to output, in case we have
	 *	to do error recovery.
	 */
	fflush(stdout);

	jumpParameter = setjmp(newtonCgiState->jmpbuf);
	memcpy(newtonCgiDimensionsState->jmpbuf, newtonCgiState->jmpbuf, sizeof(newtonCgiDimensionsState->jmpbuf));

	if (!jumpParameter)
	{
		newtonCgiState->jmpbufIsValid = true;
		newtonCgiDimensionsState->jmpbufIsValid = true;

		/*
		 *	Return from call to setjmp
		 *
		 *	rusage.ru_maxrss is always zero on MacOS 10.5, and on Linux
		 *	so we determine the memory usage manually.
		 */
		startRss = checkRss(newtonCgiState);
		getrusage(RUSAGE_SELF, &start);	

		/*
		 *	Get the path corresponding to the mkstemp()-created file.
		 */
		char inputFilePath[MAXPATHLEN];

#ifdef __linux__
		strncpy(inputFilePath, logFileStub, MAXPATHLEN);
#else
		if (fcntl(logFd, F_GETPATH, &inputFilePath) == -1)
		{
			fatal(newtonCgiState, Efd2path);
		}
#endif

		/*
		 *	Tokenize input, then parse it and build AST + symbol table.
		 */
		newtonLexInit(newtonCgiState, inputFilePath);

		/*
		 *	Create a top-level scope, then parse.
		 */
		newtonCgiState->newtonIrTopScope = commonSymbolTableAllocScope(newtonCgiState);
		

		newtonLexInit(newtonCgiDimensionsState, inputFilePath);
		newtonCgiDimensionsState->newtonIrTopScope = commonSymbolTableAllocScope(newtonCgiDimensionsState);
		newtonDimensionPassParse(newtonCgiDimensionsState, newtonCgiDimensionsState->newtonIrTopScope);

		newtonCgiState->newtonIrTopScope->firstDimension = newtonCgiDimensionsState->newtonIrTopScope->firstDimension;

		if(newtonCgiState->newtonIrTopScope->firstDimension != NULL)
		{
			newtonCgiState->newtonIrRoot = newtonParse(newtonCgiState, newtonCgiState->newtonIrTopScope);
		}

		/*
		 *	We don't put the following into runPasses() because they
		 *	are not general-purpose.
		 */

		/*
		 *	SMT backend.
		 */
		if (newtonCgiState->irBackends & kNewtonIrBackendSmt)
		{
			irPassSmtBackend(newtonCgiState);
		}

		/*
	 	 *	Pi groups pass (implies Dimensional matrix pass)
	 	 */
		if (newtonCgiState->irPasses & kNoisyIrPiGroupsPass)
		{
			irPassDimensionalMatrixAnnotation(newtonCgiState);
			irPassDimensionalMatrixPiGroups(newtonCgiState);
			irPassDimensionalMatrixPrinter(newtonCgiState);
			irPassDimensionalMatrixKernelPrinter(newtonCgiState);
		}

		/*
		 *	Dot backend.
		 */
		if (newtonCgiState->irBackends & kNoisyIrBackendDot)
		{
			printToFile(newtonCgiState, irPassDotBackend(newtonCgiState, newtonCgiState->newtonIrTopScope, newtonCgiState->newtonIrRoot, gNewtonAstNodeStrings), "tmpdot", kNoisyPostFileWriteActionRenderDot);
		}

		if (newtonCgiState->mode & kNoisyModeCallTracing)
		{
			timeStampDumpTimeline(newtonCgiState);
		}

		if (newtonCgiState->mode & kNoisyModeCallStatistics)
		{
			uint64_t	irNodeCount = 0, symbolTableNodeCount;


			timeStampDumpResidencies(newtonCgiState);

			irNodeCount = irPassHelperIrSize(newtonCgiState, newtonCgiState->newtonIrRoot);
			symbolTableNodeCount = irPassHelperSymbolTableSize(newtonCgiState, newtonCgiState->newtonIrTopScope);

			flexprint(newtonCgiState->Fe, newtonCgiState->Fm, newtonCgiState->Fpinfo, "Intermediate Representation Information:\n\n");
			flexprint(newtonCgiState->Fe, newtonCgiState->Fm, newtonCgiState->Fpinfo, "    IR node count                        : %llu\n", irNodeCount);
			flexprint(newtonCgiState->Fe, newtonCgiState->Fm, newtonCgiState->Fpinfo, "    Symbol Table node count              : %llu\n", symbolTableNodeCount);

			/*
			 *	Libflex malloc statistics:
			 */
			if (newtonCgiState->Fm->debug)
			{
				flexmblocksdisplay(newtonCgiState->Fe, newtonCgiState->Fm, newtonCgiState->Fpinfo);
			}
		}
	}
	else
	{
		/*
		 *	See issue #291
		 */

		/*	Return again after longjmp	*/
		newtonCgiState->jmpbufIsValid = false;
	}

	getrusage(RUSAGE_SELF, &end);
	endRss = checkRss(newtonCgiState);


	/*
	 *	doTail() is also called directly if we run into trouble while
	 *	interping in yyparse().
	 */
	doTail(fmtWidth, cgiSparameter, cgiOparameter, cgiTparameter);


	for (i = 0; cgiVars[i]; i++)
	{
		free(cgiVars[i]);
	}
	free(cgiVars);


	exit(0);
}

void
doTail(int fmtWidth, int cgiSparameter, int cgiOparameter, int cgiTparameter)
{
	int	i, lines;


	/*
	 *	We want all of these to come out directly, not in flex output buffer
	 */


	/*
	 *	Count number of lines in newtonCodeBuffer
	 */
	lines = 0;
	for (i = 0; i < strlen(newtonCodeBuffer); i++)
	{
		if (newtonCodeBuffer[i] == '\n') lines++;
	}

	printf("Newton version %s.\n", kNewtonVersion);
	printf("<br>");
	printf("Authored 2017-2018 by Jonathan Lim and Phillip Stanley-Marbell. Additional contributors, 2018 onwards. See CONTRIBUTORS.md.\n");
	printf("<br>");
	
#if defined (_OPENMP)
	printf("Multithreading enabled; detected %d hardware threads\n",
		omp_get_max_threads());
#endif

	printf("<div>\n");

	printf("<form action=\"%s-%s\">\n", kNewtonCgiExecutableUrl, kNewtonL10N);
	printf("<textarea NAME=\"c\" name=\"data-editor\" data-editor=\"newton\" COLS=1 ROWS=1>\n");
	printf("%s", newtonCodeBuffer);
	printf("</textarea>\n");

	printf("<div style=\"background-color:#EEEEEE; color:444444; padding:3px;\">\n");
	printf("&nbsp;&nbsp;(Newton/%" PRIu64 
					":&nbsp;&nbsp;Operation completed in %.6f&thinsp;seconds S+U time; &nbsp; Mem = %"
					PRIu64 "&thinsp;KB, &nbsp; &#916; Mem = %" PRIu64 "&thinsp;KB).\n",
					newtonCgiState->callAggregateTotal, 
					(	(end.ru_stime.tv_sec - start.ru_stime.tv_sec) +
						(end.ru_utime.tv_sec - start.ru_utime.tv_sec))
					+
					(	(end.ru_stime.tv_usec - start.ru_stime.tv_usec) +
						(end.ru_utime.tv_usec - start.ru_utime.tv_usec))/1E6,
					endRss, endRss-startRss);
	printf("</div>\n");

	printf("<input type=\"hidden\" name=\"w\" value=\"%d\">\n", fmtWidth);

	/*
	 *	Use div instead of span to get bgcolor to be page-wide
	 */
	printf("<div style=\"background-color:#009999; color:white; padding:3px;\" onclick=\"JavaScript:toggle('newtoninfo')\">");
	printf("&nbsp;&nbsp;Informational Report&nbsp;&nbsp;&nbsp;<b>(Click here to show/hide.)</b>&nbsp;&nbsp;</div>");
	printf("<table width=\"%d\" border=\"0\">\n", fmtWidth);
	printf("<tr><td>\n");
	printf("<pre>");
	printf("<span style=\"background-color:#D6FFFF; display:none;\" id='newtoninfo'>");
	htmlPrint(newtonCgiState->Fpinfo->circbuf);
	printf("</span>");
	printf("</pre>\n");
	printf("</td></tr>\n");
	printf("</table>\n");

	if (strlen(newtonCgiState->Fperr->circbuf) != 0)
	{
		printf("<div width=\"%d\" style=\"background-color:FFDB58; padding:3px;\" onclick=\"JavaScript:toggle('newtonerrs')\">", fmtWidth);
		printf("&nbsp;&nbsp;Error Report&nbsp;&nbsp;&nbsp;<b>(Click here to show/hide.)</b>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
		printf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div><table width=\"%d\" border=\"0\"><tr><td><pre>", fmtWidth);
		printf("<span style=\"background-color:whitesmoke; display:none;\" id='newtonerrs'>%s</span></pre></td></tr></table>", newtonCgiState->Fperr->circbuf);
	}

	if (strlen(newtonCgiDimensionsState->Fperr->circbuf) != 0)
	{
		printf("<div width=\"%d\" style=\"background-color:FFDB58; padding:3px;\" onclick=\"JavaScript:toggle('newtonerrs')\">", fmtWidth);
		printf("&nbsp;&nbsp;Error Report&nbsp;&nbsp;&nbsp;<b>(Click here to show/hide.)</b>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
		printf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div><table width=\"%d\" border=\"0\"><tr><td><pre>", fmtWidth);
		printf("<span style=\"background-color:whitesmoke; display:none;\" id='newtonerrs'>%s</span></pre></td></tr></table>", newtonCgiDimensionsState->Fperr->circbuf);
	}

	if (strlen(newtonCgiState->Fpmathjax->circbuf) != 0)
	{
		printf("<div width=\"%d\" style=\"background-color:EEEE11; padding:3px;\" onclick=\"JavaScript:toggle('newtonmathjax')\">", fmtWidth);
		printf("&nbsp;&nbsp;Equation Report&nbsp;&nbsp;&nbsp;<b>(Click here to show/hide.)</b>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
		printf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div><table width=\"%d\" border=\"0\"><tr><td>", fmtWidth);
		printf("<span style=\"background-color:whitesmoke; display:none;\" id='newtonmathjax'>%s</span></td></tr></table>", newtonCgiState->Fpmathjax->circbuf);
	}

	if (strlen(newtonCgiState->Fpsmt2->circbuf) != 0)
	{
		printf("<div width=\"%d\" style=\"background-color:00EEEE; padding:3px;\" onclick=\"JavaScript:toggle('newtonsmt2')\">", fmtWidth);
		printf("&nbsp;&nbsp;SMT2 Backend Report&nbsp;&nbsp;&nbsp;<b>(Click here to show/hide.)</b>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
		printf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</div><table width=\"%d\" border=\"0\"><tr><td><pre>", fmtWidth);
		printf("<span style=\"background-color:whitesmoke; display:none;\" id='newtonsmt2'>%s</span></pre></td></tr></table>", newtonCgiState->Fpsmt2->circbuf);
	}

	if (newtonCgiState->lastDotRender != NULL)
	{
		printf("<table width=\"%d\" border=\"0\">\n", fmtWidth);
		printf("<tr><td>\n");
		printf("<pre>");
		printf("<a href=\"%s%s.png\" target=\"_blank\"><img src=\"%spng-icon.png\" height=25 border=0/></a>&nbsp;",
			kNewtonCgiFileUrlBase, newtonCgiState->lastDotRender, kNewtonCgiFileUrlBase);
		printf("<a href=\"%s%s.pdf\" target=\"_blank\"><img src=\"%spdf-icon.png\" height=25 border=0/></a>&nbsp;",
			kNewtonCgiFileUrlBase, newtonCgiState->lastDotRender, kNewtonCgiFileUrlBase);
		printf("<a href=\"%s%s.svg\" target=\"_blank\"><img src=\"%ssvg-icon.png\" height=25 border=0/></a>&nbsp;",
			kNewtonCgiFileUrlBase, newtonCgiState->lastDotRender, kNewtonCgiFileUrlBase);
		printf("</pre>\n");
		printf("</td></tr>\n");
		printf("</table>\n");
		
		/*
		 *	Embed the PNG variant inline.
		 */
		printf("<img src=\"%s%s.png\" width=\"%d\">\n",
			kNewtonCgiFileUrlBase, newtonCgiState->lastDotRender, fmtWidth);
	}


	printf("<table cellpadding=0; border=0; style=\"width:100%%;\">\n");
	printf("<tr>\n");

	printf("<td valign=top>\n");
	printf("<table>\n");
	printf("<tr>\n");
	printf("<td>\n");
	printf("<table border=0 style=\"width:250px; padding:6px; background-color: #EfEfEf\">\n");
	printf("<tr><td><span style=\"color: black; background-color:#FF9966; padding:5px;\">");
	printf("<b>Compiler Parameters</b>&nbsp;&nbsp;</span></td></tr>\n");
	printf("<tr><td height=\"10\">&nbsp;</td></tr>\n");
	printf("<tr><td>Backends Bitmap	</td><td><input type=\"number\" name=\"s\" style=\"width: 30px\" value=\"%d\"></td></tr>\n", cgiSparameter);
	printf("<tr><td>Passes Bitmap	</td><td><input type=\"number\" name=\"o\" style=\"width: 60px\" value=\"%d\"></td></tr>\n", cgiOparameter);
	printf("<tr><td>Dot detail level</td><td><input type=\"number\" name=\"t\" style=\"width: 60px\" value=\"%d\"></td></tr>\n", cgiTparameter);
	printf("<tr><td height=\"40\">&nbsp;</td></tr>\n");
	printf("<tr><td><input style=\"font-family:'Source Sans Pro'; color: black; font-size:14px; font-weight:400; border: 1px; background-color: #FF9900;\" type=\"submit\" name=\"b\" value=\"Compile\"></td></tr>\n");
	printf("</table>\n");
	printf("</td>\n");
	printf("</tr>\n");
	//printf("<tr><td><img src=\"%s\" width=90 align=\"left\"></td></tr>\n", kNewtonLogoPath);
	printf("</table>\n");
	printf("</td>\n");

	printf("<td align=left; valign=bottom>\n");
//	printf("<b>Cite this as:</b>\n");
//	printf("<div style=\"background-color:whitesmoke;\">\n");
//	printf("P. Stanley-Marbell, P. A. Francese, and M. Rinard. \"Encoder logic for reducing serial I/O power in sensors and sensor hubs.\"\n<i>28th IEEE Hot Chips Symposium (HotChips 28)</i>, 2016.\n");
//	printf("</div>\n");
//	printf("<pre>\n");
//	printf("@inproceedings{hotchips16encoder,\n");
//	printf("  title={Encoder logic for reducing serial I/O power in sensors and sensor hubs},\n");
//	printf("  author={P. Stanley-Marbell and P.~A. Francese and M. Rinard},\n");
//	printf("  booktitle={28th IEEE Hot Chips Symposium (HotChips 28)},\n");
//	printf("  pages={1--2},\n");
//	printf("  year={2016},\n");
//	printf("  organization={IEEE}\n");
//	printf("}\n");
//	printf("</pre>\n");

	printf("<b>Additional examples:</b>&nbsp;&nbsp;\n");
	printf("<a href=\"%s-%s?c=%%23%%0D%%0A%%23%%09Authored+2018%%2C+James+Rhodes.%%0D%%0A%%23%%0D%%0A%%23%%09All+rights+reserved.%%0D%%0A%%23%%0D%%0A%%23%%09Redistribution+and+use+in+source+and+binary+forms%%2C+with+or+without%%0D%%0A%%23%%09modification%%2C+are+permitted+provided+that+the+following+conditions%%0D%%0A%%23%%09are+met%%3A%%0D%%0A%%23%%0D%%0A%%23%%09*%%09Redistributions+of+source+code+must+retain+the+above%%0D%%0A%%23%%09%%09copyright+notice%%2C+this+list+of+conditions+and+the+following%%0D%%0A%%23%%09%%09disclaimer.%%0D%%0A%%23%%0D%%0A%%23%%09*%%09Redistributions+in+binary+form+must+reproduce+the+above%%0D%%0A%%23%%09%%09copyright+notice%%2C+this+list+of+conditions+and+the+following%%0D%%0A%%23%%09%%09disclaimer+in+the+documentation+and%%2For+other+materials%%0D%%0A%%23%%09%%09provided+with+the+distribution.%%0D%%0A%%23%%0D%%0A%%23%%09*%%09Neither+the+name+of+the+author+nor+the+names+of+its%%0D%%0A%%23%%09%%09contributors+may+be+used+to+endorse+or+promote+products%%0D%%0A%%23%%09%%09derived+from+this+software+without+specific+prior+written%%0D%%0A%%23%%09%%09permission.%%0D%%0A%%23%%0D%%0A%%23%%09THIS+SOFTWARE+IS+PROVIDED+BY+THE+COPYRIGHT+HOLDERS+AND+CONTRIBUTORS%%0D%%0A%%23%%09%%22AS+IS%%22+AND+ANY+EXPRESS+OR+IMPLIED+WARRANTIES%%2C+INCLUDING%%2C+BUT+NOT%%0D%%0A%%23%%09LIMITED+TO%%2C+THE+IMPLIED+WARRANTIES+OF+MERCHANTABILITY+AND+FITNESS%%0D%%0A%%23%%09FOR+A+PARTICULAR+PURPOSE+ARE+DISCLAIMED.+IN+NO+EVENT+SHALL+THE%%0D%%0A%%23%%09COPYRIGHT+OWNER+OR+CONTRIBUTORS+BE+LIABLE+FOR+ANY+DIRECT%%2C+INDIRECT%%2C%%0D%%0A%%23%%09INCIDENTAL%%2C+SPECIAL%%2C+EXEMPLARY%%2C+OR+CONSEQUENTIAL+DAMAGES+%%28INCLUDING%%2C%%0D%%0A%%23%%09BUT+NOT+LIMITED+TO%%2C+PROCUREMENT+OF+SUBSTITUTE+GOODS+OR+SERVICES%%3B%%0D%%0A%%23%%09LOSS+OF+USE%%2C+DATA%%2C+OR+PROFITS%%3B+OR+BUSINESS+INTERRUPTION%%29+HOWEVER%%0D%%0A%%23%%09CAUSED+AND+ON+ANY+THEORY+OF+LIABILITY%%2C+WHETHER+IN+CONTRACT%%2C+STRICT%%0D%%0A%%23%%09LIABILITY%%2C+OR+TORT+%%28INCLUDING+NEGLIGENCE+OR+OTHERWISE%%29+ARISING+IN%%0D%%0A%%23%%09ANY+WAY+OUT+OF+THE+USE+OF+THIS+SOFTWARE%%2C+EVEN+IF+ADVISED+OF+THE%%0D%%0A%%23%%09POSSIBILITY+OF+SUCH+DAMAGE.%%0D%%0A%%23%%0D%%0A%%23%%0D%%0A%%23%%09Description%%3A+Physical+invariant+for+a+violin%%0D%%0A%%23%%0D%%0A%%23%%09Necessary+variables%%0D%%0A%%23%%0D%%0A%%23%%09%%09T+%%3A+tension+of+the+string%%0D%%0A%%23%%0D%%0A%%23%%09%%09L+%%3A+length+of+string+between+finger+and+bridge%%0D%%0A%%23%%0D%%0A%%23%%09%%09%%26%%23956%%3B+%%3A+mass+per+unit+length+of+the+string%%0D%%0A%%23%%0D%%0A%%23%%09%%09f+%%3A+fundamental+frequency+of+the+%%28shortened%%29+string%%0D%%0A%%23%%0D%%0A%%0D%%0Ainclude+%%22NewtonBaseSignals.nt%%22%%0D%%0A%%0D%%0Amu+%%3A+constant+%%3D+0.000078+*+kg+%%2F+m%%3B%%0D%%0A%%0D%%0AOscillatingString%%3A+invariant%%28t%%3A+force%%2C+L%%3A+distance%%2C+f%%3A+frequency%%29+%%3D%%0D%%0A%%7B%%0D%%0A%%09f+**+2+%%7E+%%28t+%%2F+mu%%29+%%2F+%%284+*+L+**+2%%29%%0D%%0A%%7D%%0D%%0A&w=980&s=0&o=0&t=0&b=Compile\">[Violin Example]</a>\n", kNewtonCgiExecutableUrl, kNewtonL10N);
	printf("&nbsp;&nbsp;\n");
	printf("<a href=\"%s-%s?c=%%23%%0D%%0A%%23%%09Authored+2017%%2C+Jonathan+Lim.%%0D%%0A%%23%%0D%%0A%%23%%09All+rights+reserved.%%0D%%0A%%23%%0D%%0A%%23%%09Redistribution+and+use+in+source+and+binary+forms%%2C+with+or+without%%0D%%0A%%23%%09modification%%2C+are+permitted+provided+that+the+following+conditions%%0D%%0A%%23%%09are+met%%3A%%0D%%0A%%23%%0D%%0A%%23%%09*%%09Redistributions+of+source+code+must+retain+the+above%%0D%%0A%%23%%09%%09copyright+notice%%2C+this+list+of+conditions+and+the+following%%0D%%0A%%23%%09%%09disclaimer.%%0D%%0A%%23%%0D%%0A%%23%%09*%%09Redistributions+in+binary+form+must+reproduce+the+above%%0D%%0A%%23%%09%%09copyright+notice%%2C+this+list+of+conditions+and+the+following%%0D%%0A%%23%%09%%09disclaimer+in+the+documentation+and%%2For+other+materials%%0D%%0A%%23%%09%%09provided+with+the+distribution.%%0D%%0A%%23%%0D%%0A%%23%%09*%%09Neither+the+name+of+the+author+nor+the+names+of+its%%0D%%0A%%23%%09%%09contributors+may+be+used+to+endorse+or+promote+products%%0D%%0A%%23%%09%%09derived+from+this+software+without+specific+prior+written%%0D%%0A%%23%%09%%09permission.%%0D%%0A%%23%%0D%%0A%%23%%09THIS+SOFTWARE+IS+PROVIDED+BY+THE+COPYRIGHT+HOLDERS+AND+CONTRIBUTORS%%0D%%0A%%23%%09%%22AS+IS%%22+AND+ANY+EXPRESS+OR+IMPLIED+WARRANTIES%%2C+INCLUDING%%2C+BUT+NOT%%0D%%0A%%23%%09LIMITED+TO%%2C+THE+IMPLIED+WARRANTIES+OF+MERCHANTABILITY+AND+FITNESS%%0D%%0A%%23%%09FOR+A+PARTICULAR+PURPOSE+ARE+DISCLAIMED.+IN+NO+EVENT+SHALL+THE%%0D%%0A%%23%%09COPYRIGHT+OWNER+OR+CONTRIBUTORS+BE+LIABLE+FOR+ANY+DIRECT%%2C+INDIRECT%%2C%%0D%%0A%%23%%09INCIDENTAL%%2C+SPECIAL%%2C+EXEMPLARY%%2C+OR+CONSEQUENTIAL+DAMAGES+%%28INCLUDING%%2C%%0D%%0A%%23%%09BUT+NOT+LIMITED+TO%%2C+PROCUREMENT+OF+SUBSTITUTE+GOODS+OR+SERVICES%%3B%%0D%%0A%%23%%09LOSS+OF+USE%%2C+DATA%%2C+OR+PROFITS%%3B+OR+BUSINESS+INTERRUPTION%%29+HOWEVER%%0D%%0A%%23%%09CAUSED+AND+ON+ANY+THEORY+OF+LIABILITY%%2C+WHETHER+IN+CONTRACT%%2C+STRICT%%0D%%0A%%23%%09LIABILITY%%2C+OR+TORT+%%28INCLUDING+NEGLIGENCE+OR+OTHERWISE%%29+ARISING+IN%%0D%%0A%%23%%09ANY+WAY+OUT+OF+THE+USE+OF+THIS+SOFTWARE%%2C+EVEN+IF+ADVISED+OF+THE%%0D%%0A%%23%%09POSSIBILITY+OF+SUCH+DAMAGE.%%0D%%0A%%23%%0D%%0A%%23%%0D%%0A%%23%%09Description%%3A+Invariants+for+a+simple+pendulum.%%0D%%0A%%23%%0D%%0A%%0D%%0Ainclude+%%22NewtonBaseSignals.nt%%22%%0D%%0A%%0D%%0ASimplePendulum+%%3A+invariant%%28L%%3A+distance%%400%%2C+period%%3A+time%%29+%%3D%%0D%%0A%%7B%%0D%%0A%%09L+%%3E%%3D+2.2+*+meter%%2C%%0D%%0A%%09L+%%3C%%3D++10+*+meter%%2C%%0D%%0A%%09L+%%40%%3C+5+*+meter%%2C%%0D%%0A%%09period+%%7E+%%284+*+kNewtonUnitfree_pi+*+kNewtonUnitfree_pi+*+L+%%2F+kNewtonUnithave_AccelerationDueToGravity%%29+**+3%%2C%%0D%%0A%%09period+%%3E%%3D+3+*+second%%2C%%0D%%0A%%09period+%%3C%%3D+9+*+second%%0D%%0A%%7D%%0D%%0A&w=980&s=0&o=0&t=0&b=Compile\">[Simple Pendulum Example]</a>\n", kNewtonCgiExecutableUrl, kNewtonL10N);
	printf("&nbsp;&nbsp;\n");
	printf("<a href=\"%s-%s?c=%%23%%0D%%0A%%23%%09Authored+2017%%2C+Jonathan+Lim.%%0D%%0A%%23%%0D%%0A%%23%%09All+rights+reserved.%%0D%%0A%%23%%0D%%0A%%23%%09Redistribution+and+use+in+source+and+binary+forms%%2C+with+or+without%%0D%%0A%%23%%09modification%%2C+are+permitted+provided+that+the+following+conditions%%0D%%0A%%23%%09are+met%%3A%%0D%%0A%%23%%0D%%0A%%23%%09*%%09Redistributions+of+source+code+must+retain+the+above%%0D%%0A%%23%%09%%09copyright+notice%%2C+this+list+of+conditions+and+the+following%%0D%%0A%%23%%09%%09disclaimer.%%0D%%0A%%23%%0D%%0A%%23%%09*%%09Redistributions+in+binary+form+must+reproduce+the+above%%0D%%0A%%23%%09%%09copyright+notice%%2C+this+list+of+conditions+and+the+following%%0D%%0A%%23%%09%%09disclaimer+in+the+documentation+and%%2For+other+materials%%0D%%0A%%23%%09%%09provided+with+the+distribution.%%0D%%0A%%23%%0D%%0A%%23%%09*%%09Neither+the+name+of+the+author+nor+the+names+of+its%%0D%%0A%%23%%09%%09contributors+may+be+used+to+endorse+or+promote+products%%0D%%0A%%23%%09%%09derived+from+this+software+without+specific+prior+written%%0D%%0A%%23%%09%%09permission.%%0D%%0A%%23%%0D%%0A%%23%%09THIS+SOFTWARE+IS+PROVIDED+BY+THE+COPYRIGHT+HOLDERS+AND+CONTRIBUTORS%%0D%%0A%%23%%09%%22AS+IS%%22+AND+ANY+EXPRESS+OR+IMPLIED+WARRANTIES%%2C+INCLUDING%%2C+BUT+NOT%%0D%%0A%%23%%09LIMITED+TO%%2C+THE+IMPLIED+WARRANTIES+OF+MERCHANTABILITY+AND+FITNESS%%0D%%0A%%23%%09FOR+A+PARTICULAR+PURPOSE+ARE+DISCLAIMED.+IN+NO+EVENT+SHALL+THE%%0D%%0A%%23%%09COPYRIGHT+OWNER+OR+CONTRIBUTORS+BE+LIABLE+FOR+ANY+DIRECT%%2C+INDIRECT%%2C%%0D%%0A%%23%%09INCIDENTAL%%2C+SPECIAL%%2C+EXEMPLARY%%2C+OR+CONSEQUENTIAL+DAMAGES+%%28INCLUDING%%2C%%0D%%0A%%23%%09BUT+NOT+LIMITED+TO%%2C+PROCUREMENT+OF+SUBSTITUTE+GOODS+OR+SERVICES%%3B%%0D%%0A%%23%%09LOSS+OF+USE%%2C+DATA%%2C+OR+PROFITS%%3B+OR+BUSINESS+INTERRUPTION%%29+HOWEVER%%0D%%0A%%23%%09CAUSED+AND+ON+ANY+THEORY+OF+LIABILITY%%2C+WHETHER+IN+CONTRACT%%2C+STRICT%%0D%%0A%%23%%09LIABILITY%%2C+OR+TORT+%%28INCLUDING+NEGLIGENCE+OR+OTHERWISE%%29+ARISING+IN%%0D%%0A%%23%%09ANY+WAY+OUT+OF+THE+USE+OF+THIS+SOFTWARE%%2C+EVEN+IF+ADVISED+OF+THE%%0D%%0A%%23%%09POSSIBILITY+OF+SUCH+DAMAGE.%%0D%%0A%%23%%0D%%0A%%23%%09Description%%3A+Invariants+for+an+instrumented+jet+engine.%%0D%%0A%%23%%0D%%0A%%0D%%0Ainclude+%%22NewtonBaseSignals.nt%%22%%0D%%0A%%0D%%0AmassFlowRate%%3A+signal+%%3D%%0D%%0A%%7B%%0D%%0A%%09derivation%%09%%3D+mass+%%2F+time%%3B%%0D%%0A%%7D%%0D%%0A%%0D%%0ASamplingTime%%3A+constant+%%3D+%%285E-3%%29+*+s%%3B%%0D%%0A%%0D%%0AMooreGreitzerJetEngineModel%%3A+invariant%%28%%0D%%0A%%09x%%3A+massFlowRate%%2C%%0D%%0A%%09y%%3A+pressure%%0D%%0A%%29+%%3D+%%0D%%0A%%7B%%0D%%0A%%09x+*+kg+**+2+*+s+**+4%%09%%3E%%09-y+*+kg+**+3+*+s+**+3+%%2F+Pa+-+1.5+*+%%28x+*+SamplingTime%%29+**+2+*+kg+*+s+**+3++-+0.5+*+%%28x+*+SamplingTime%%29+**+3+*+s+**+3+-+0.5+*+kg+**+3+*+s+**+3%%2C%%0D%%0A%%09y+%%2F+SamplingTime+*+kg%%09%%7E%%093+*+x+*+Pa+-+y+*+kg+%%2F+s+%%0D%%0A%%7D%%0D%%0A&w=980&s=0&o=0&t=0&b=Compile\">[Jet Engine]</a>\n", kNewtonCgiExecutableUrl, kNewtonL10N);
	printf("&nbsp;&nbsp;\n");
	printf("<a href=\"%s-%s?c=%%23%%0D%%0A%%23%%09Authored+2018%%2C+Phillip+Stanley-Marbell.%%0D%%0A%%23%%0D%%0A%%23%%09All+rights+reserved.%%0D%%0A%%23%%0D%%0A%%23%%09Redistribution+and+use+in+source+and+binary+forms%%2C+with+or+without%%0D%%0A%%23%%09modification%%2C+are+permitted+provided+that+the+following+conditions%%0D%%0A%%23%%09are+met%%3A%%0D%%0A%%23%%0D%%0A%%23%%09*%%09Redistributions+of+source+code+must+retain+the+above%%0D%%0A%%23%%09%%09copyright+notice%%2C+this+list+of+conditions+and+the+following%%0D%%0A%%23%%09%%09disclaimer.%%0D%%0A%%23%%0D%%0A%%23%%09*%%09Redistributions+in+binary+form+must+reproduce+the+above%%0D%%0A%%23%%09%%09copyright+notice%%2C+this+list+of+conditions+and+the+following%%0D%%0A%%23%%09%%09disclaimer+in+the+documentation+and%%2For+other+materials%%0D%%0A%%23%%09%%09provided+with+the+distribution.%%0D%%0A%%23%%0D%%0A%%23%%09*%%09Neither+the+name+of+the+author+nor+the+names+of+its%%0D%%0A%%23%%09%%09contributors+may+be+used+to+endorse+or+promote+products%%0D%%0A%%23%%09%%09derived+from+this+software+without+specific+prior+written%%0D%%0A%%23%%09%%09permission.%%0D%%0A%%23%%0D%%0A%%23%%09THIS+SOFTWARE+IS+PROVIDED+BY+THE+COPYRIGHT+HOLDERS+AND+CONTRIBUTORS%%0D%%0A%%23%%09%%22AS+IS%%22+AND+ANY+EXPRESS+OR+IMPLIED+WARRANTIES%%2C+INCLUDING%%2C+BUT+NOT%%0D%%0A%%23%%09LIMITED+TO%%2C+THE+IMPLIED+WARRANTIES+OF+MERCHANTABILITY+AND+FITNESS%%0D%%0A%%23%%09FOR+A+PARTICULAR+PURPOSE+ARE+DISCLAIMED.+IN+NO+EVENT+SHALL+THE%%0D%%0A%%23%%09COPYRIGHT+OWNER+OR+CONTRIBUTORS+BE+LIABLE+FOR+ANY+DIRECT%%2C+INDIRECT%%2C%%0D%%0A%%23%%09INCIDENTAL%%2C+SPECIAL%%2C+EXEMPLARY%%2C+OR+CONSEQUENTIAL+DAMAGES+%%28INCLUDING%%2C%%0D%%0A%%23%%09BUT+NOT+LIMITED+TO%%2C+PROCUREMENT+OF+SUBSTITUTE+GOODS+OR+SERVICES%%3B%%0D%%0A%%23%%09LOSS+OF+USE%%2C+DATA%%2C+OR+PROFITS%%3B+OR+BUSINESS+INTERRUPTION%%29+HOWEVER%%0D%%0A%%23%%09CAUSED+AND+ON+ANY+THEORY+OF+LIABILITY%%2C+WHETHER+IN+CONTRACT%%2C+STRICT%%0D%%0A%%23%%09LIABILITY%%2C+OR+TORT+%%28INCLUDING+NEGLIGENCE+OR+OTHERWISE%%29+ARISING+IN%%0D%%0A%%23%%09ANY+WAY+OUT+OF+THE+USE+OF+THIS+SOFTWARE%%2C+EVEN+IF+ADVISED+OF+THE%%0D%%0A%%23%%09POSSIBILITY+OF+SUCH+DAMAGE.%%0D%%0A%%23%%0D%%0A%%23%%0D%%0A%%23%%09Description%%3A%%09Empty+invariant+for+auto-generating+synthetic+invariants%%0D%%0A%%23%%09%%09%%09for+a+beam.%%0D%%0A%%23%%0D%%0A%%23%%09Parameters%%0D%%0A%%23%%0D%%0A%%23%%09%%09P+%%3A%%0D%%0A%%23%%09%%09L+%%3A%%0D%%0A%%23%%09%%09I+%%3A%%0D%%0A%%23%%09%%09E+%%3A%%0D%%0A%%23%%0D%%0A%%0D%%0Ainclude+%%22NewtonBaseSignals.nt%%22%%0D%%0A%%0D%%0AiDimension+%%3A+signal+%%3D%%0D%%0A%%7B%%0D%%0A%%09derivation+%%3D+distance**4%%3B%%0D%%0A%%7D%%0D%%0A%%0D%%0AeDimension+%%3A+signal+%%3D%%0D%%0A%%7B%%0D%%0A%%09derivation+%%3D+mass*%%28distance**-1%%29*%%28time**-2%%29%%3B%%0D%%0A%%7D%%0D%%0A%%0D%%0ABeamForPiGroups%%3A+invariant%%28%%09P_param%%3A+force%%2C%%0D%%0A%%09%%09%%09%%09L_param%%3A+distance%%2C%%0D%%0A%%09%%09%%09%%09I_param%%3A+iDimension%%2C%%0D%%0A%%09%%09%%09%%09E_param%%3A+eDimension%%29+%%3D%%0D%%0A%%7B%%0D%%0A%%7D&w=980&s=0&o=0&t=0&b=Compile\">[Beam with auto-generated invariants]</a>\n", kNewtonCgiExecutableUrl, kNewtonL10N);
	printf("&nbsp;&nbsp;\n");
	printf("</td>\n");

	printf("<td valign=top; align=right>\n");
	printf("<table border=0>\n");
	printf("<tr><td><a href=\"http://physcomp.eng.cam.ac.uk\"><img src=\"%s\" width=120 align=\"right\"></a></td></tr>\n", kPhyscomplabLogoPath);
	printf("</table>\n");
	printf("</td>\n");
	
	printf("</tr>\n");
	printf("</table>\n");


	printf("</form>\n");
	printf("</div>\n");

	/*
	 *	Javascript for ACE editor hookup. Adapted from 
	 *
	 *	See	https://groups.google.com/forum/#!topic/ace-discuss/dDMVH_RbsAk
	 */
	printf("<script src=\"../tmp/src-noconflict/ace.js\" type=\"text/javascript\" charset=\"utf-8\"></script>\n");
	printf("<script>\n");
	/*
	 *	Tie ACE editor to textareas with data-editor attribute
	 */
	printf("    $(function () {\n");
	printf("        $('textarea[data-editor]').each(function () {\n");
	printf("            var textarea = $(this);\n");
	printf("            var editDiv = $('<div>', {\n");
	printf("                position: 'absolute',\n");
	printf("                'class': textarea.attr('class')\n");
	printf("            }).insertBefore(textarea);\n");
	printf("            textarea.css('visibility', 'hidden');\n");
	printf("            var editor = ace.edit(editDiv[0]);\n");
	printf("            editor.focus();\n");
	printf("            editor.renderer.setShowGutter(true);\n");
	printf("            editor.getSession().setValue(textarea.val());\n");
	printf("            editor.setKeyboardHandler(\"ace/keyboard/vim\");\n");
	printf("            editor.setTheme(\"ace/theme/solarized_light\");\n");
	printf("            editor.session.setMode(\"ace/mode/c_cpp\");\n");
	printf("            editor.setShowPrintMargin(false);\n");

	/*
	 *	Was disabled (see #132). We now try both the dimensions pass's state as well as the main parser to find the line number.
	 */
	if (strlen(newtonCgiState->Fperr->circbuf) != 0)
	{
		printf("            editor.gotoLine(%"PRIu64", %"PRIu64", true);\n", lexPeek(newtonCgiState, 1)->sourceInfo->lineNumber, lexPeek(newtonCgiState, 1)->sourceInfo->columnNumber-1);
	}
	
	if (strlen(newtonCgiDimensionsState->Fperr->circbuf) != 0)
	{
		printf("            editor.gotoLine(%"PRIu64", %"PRIu64", true);\n", lexPeek(newtonCgiDimensionsState, 1)->sourceInfo->lineNumber, lexPeek(newtonCgiDimensionsState, 1)->sourceInfo->columnNumber-1);
	}

	/*
	 *	Have ACE autosize the height, with an upper limit at maxLines
	 */
	printf("	editor.setOptions({maxLines: 60});\n");
	printf("	editor.setOptions({minLines: 10});\n");
	/*
	 *	Copy the ACE editor contents back to textarea for form submission.
	 */
	printf("            textarea.closest('form').submit(function () {\n");
	printf("                textarea.val(editor.getSession().getValue());\n");
	printf("            })\n");
	printf("        });\n");
	printf("    });\n");
	printf("</script>\n");

	/*
	 *	So all uses of any instance of the CGI version show up in
	 *	Analytics logs.
	 */
	printf("<script type=\"text/javascript\">\n");
	printf("var gaJsHost = ((\"https:\" == document.location.protocol) ? \"https://ssl.\" : \"http://www.\");\n");
	printf("document.write(unescape(\"%%3Cscript src='\" + gaJsHost + \"google-analytics.com/ga.js' type='text/javascript'%%3E%%3C/script%%3E\"));\n");
	printf("</script>\n");
	printf("<script type=\"text/javascript\">\n");
	printf("try {\n");
	printf("var pageTracker = _gat._getTracker(\"UA-71071-11\");\n");
	printf("pageTracker._trackPageview();\n");
	printf("} catch(err) {}</script>\n");

	printf("</body>\n");
	printf("</html>\n");
}

void
htmlPrint(char *  s)
{
	int i;

	for (i = 0; i < strlen(s); i++)
	{
		switch (s[i])
		{
			case '<': printf("&lt;"); break;
			case '>': printf("&gt;"); break;
			case '&': printf("&amp;"); break;
			case '%': printf("&#37;"); break;
			case '#': printf("&#35;"); break;

			default : printf("%c", s[i]);
		}
	}

	return;
}
