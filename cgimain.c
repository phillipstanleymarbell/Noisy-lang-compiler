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
#include <signal.h>
#include "flextypes.h"
#include "flexerror.h"
#include "flex.h"
#include "noisy-errors.h"
#include "version.h"
#include "noisy-ff.h"
#include "noisy-timeStamps.h"
#include "noisy.h"
#include "noisy-parser.h"
#include "noisy-irPass-helpers.h"
#include "noisy-irPass-dotBackend.h"

static const char		kNoisyCgiInputLogStub[]		= "XXXXXXXXXX";
static const char		kNoisyCgiInputLogExtension[]	= ".noisy";
static const char		kNoisyCgiHtmlInputStyle[]	= "font-family: 'lucida console', monospace; font-size:10px;color:#333333;background-color:#DFDFDF";

static char **			getCgiVars(void);
static void			htmlPrint(char *  s);
static void			doTail(int fmtWidth, int cgiSparameter, int cgiOparameter, int cgiTparameter);

static struct rusage		start, end;
static uint64_t			startRss, endRss;
static char *			noisyCodeBuffer = NULL;

//extern const char		gNoisyStickies[];
//extern const char		gNoisyWhitespace[];


NoisyState *			noisyCgiState;


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
	if ((noisyCgiState != NULL) && (noisyCgiState->jmpbufIsValid))
	{
		longjmp(noisyCgiState->jmpbuf, 0);
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
		noisyError(noisyCgiState, EbadCgiQuery);
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
		noisyError(noisyCgiState, EbadCgiQuery);
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
		noisyFatal(noisyCgiState, Emalloc);
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
		noisyFatal(noisyCgiState, Emalloc);
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
	//int			curLine = 1, curCol = 1;
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


	noisyCgiState = noisyInit(kNoisyModeDefault|kNoisyModeCallStatistics|kNoisyModeCGI|kNoisyModeCallStatistics);


	/*
	 *	Extra \n, to send the blank line:
	 */
	printf("Content-type: text/html\n\n");

	printf("<html>\n");
	printf("<head>\n");
	printf("<title>Noisy version %s</title>\n", kNoisyVersion);


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
	 *	A little bit of inline JavaScript to force cursor to end of
	 *	textarea 'x' below.
	 */
	printf("<script type=\"text/javascript\">\n");
	printf("	window.onload=function()\n");
	printf("	{\n");
	printf("		ta = document.getElementById('inputarea');\n");
	printf("		ta.focus();\n");
	printf("		ta.setSelectionRange(ta.scrollHeight*ta.scrollWidth, ta.scrollHeight*ta.scrollWidth);\n");
	printf("		ta.scrollTop = ta.scrollHeight;\n");
	printf("		toggle('noisyerrs');\n");
	printf("		toggle('noisyinfo');\n");
	printf("	}\n");
	printf("</script>\n");


	/*
	 *		CSS hack to force wrapping of <pre> tag body.
	 *	(www.longren.org/2006/09/27/wrapping-text-inside-pre-tags/)
	 */
	printf("<style type=\"text/css\">\n");
	printf("pre\n");
	printf("{\n");
	printf("	overflow-x: auto;\n");
	printf("	white-space: pre-wrap;\n");
	printf("	white-space: -moz-pre-wrap;\n");
	printf("	white-space: -pre-wrap;\n");
	printf("	white-space: -o-pre-wrap;\n");
	printf("	word-wrap: break-word;\n");
	printf("	font-size:12px;\n");
	printf("}\n");
	printf("</style>\n");


	//TODO: move logo URL into a constant definition
	printf("<link rel=\"mask-icon\" href=\"../tmp/noisy-pinned-tab-logo.svg\" color=\"orange\">");

	printf("</head>\n");


	printf("<body text=\"#555555\" bgcolor=\"#FFFFFF\">\n");
	printf("<font face=\"Arial, Helvetica, sans-serif\" style=\"font-family: arial, 'lucida console', sans-serif; font-size:12px;color:#555555;\">\n");


	/*
	 *	We want this to come out here, not in flex output buffer
	 */
	printf("Noisy version %s\n", kNoisyVersion);
	printf("<br>");
	printf("Authored 2015&#8211;2015, Phillip Stanley-Marbell\n");
	printf("<br>");
#if defined (_OPENMP)
	printf("Multithreading enabled; detected %d hardware threads\n",
		omp_get_max_threads());
#endif

	printf("<img src=\"%s\" width=150 align=right>\n", kNoisyLogoPath);
	printf("<br><br>\n");

	cgiVars = getCgiVars();
	for (i = 0; cgiVars[i]; i+= 2)
	{
		if (!strcmp(cgiVars[i], "c"))
		{
			noisyCodeBuffer = cgiVars[i+1];
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
				noisyCgiState->irBackends = cgiSparameter;
			}
			else
			{
				//noisyCgiState->irBackends |= kNoisyIrBackendXXX;
			}
		}
		
		if (!strcmp(cgiVars[i], "t"))
		{
			unsigned long tmpUlong = strtoul(cgiVars[i+1], &ep, 0);
			if (*ep == '\0')
			{
				cgiTparameter = tmpUlong;
				noisyCgiState->dotDetailLevel = cgiTparameter;
			}
			else
			{
				noisyCgiState->dotDetailLevel = 0;
			}
		}
		
		if (!strcmp(cgiVars[i], "o"))
		{
			unsigned long tmpUlong = strtoul(cgiVars[i+1], &ep, 0);
			if (*ep == '\0')
			{
				cgiOparameter = tmpUlong;
				noisyCgiState->irPasses = cgiOparameter;
			}
			else
			{
				//noisyCgiState->irPasses |= kNoisyIrPassXXX;
			}
		}
	}

	flexprint(noisyCgiState->Fe, noisyCgiState->Fm, noisyCgiState->Fpinfo,
		"Noisy Compiler Parameters:\n\n\tBackends = [%llu],\n\tIR passes = [%llu]\n\n\n",
		noisyCgiState->irBackends, noisyCgiState->irPasses);
	fflush(stdout);


	/*
	 *	Log the input to a file; mkstemps() require the stub to be
	 *	writeable.
	 */


	/*
	 *	Linux does not have mkstemps()
	 */
#ifdef __linux__
	snprintf(logFileStub, kNoisyMaxFilenameLength, "%sinput-%s-%s", kNoisyBasePath, getenv("REMOTE_ADDR"), kNoisyCgiInputLogStub);
	logFd = mkstemp(logFileStub);
#else
	snprintf(logFileStub, kNoisyMaxFilenameLength, "%sinput-%s-%s.noisy", kNoisyBasePath, getenv("REMOTE_ADDR"), kNoisyCgiInputLogStub);
	logFd = mkstemps(logFileStub, strlen(kNoisyCgiInputLogExtension));
#endif

	if (logFd == -1)
	{
		fprintf(stderr, "%s\n", Emkstemps);
	}
	else
	{
		write(logFd, noisyCodeBuffer, strlen(noisyCodeBuffer));
	}

	flexstreamclear(noisyCgiState->Fe, noisyCgiState->Fm, noisyCgiState->Fperr, noisyCgiState->Fi);
	//flexstreammunch(noisyCgiState->Fe, noisyCgiState->Fm, noisyCgiState->Fperr, noisyCgiState->Fi, gNoisyWhitespace, gNoisyStickies, noisyCodeBuffer, &curLine, &curCol);
	//flexstreamchk(noisyCgiState->Fe, noisyCgiState->Fm, noisyCgiState->Fperr, noisyCgiState->Fi, -1, 32);
	flexstreamscan(noisyCgiState->Fe, noisyCgiState->Fm, noisyCgiState->Fperr, noisyCgiState->Fi);
	//flexstreamchk(noisyCgiState->Fe, noisyCgiState->Fm, noisyCgiState->Fperr, noisyCgiState->Fi, -1, 32);

	/*
	 *	Force all streams to be written to output, in case we have
	 *	to do error recovery.
	 */
	fflush(stdout);

	jumpParameter = setjmp(noisyCgiState->jmpbuf);

	if (!jumpParameter)
	{
		noisyCgiState->jmpbufIsValid = true;

		/*
		 *	Return from call to setjmp
		 *
		 *	rusage.ru_maxrss is always zero on MacOS 10.5, and on Linux
		 *	so we determine the memory usage manually.
		 */
		startRss = noisyCheckRss(noisyCgiState);
		getrusage(RUSAGE_SELF, &start);	
		//noisyCgiState->noisyIrRoot = parseNoisyProgram(noisyCgiState);
		noisyRunPasses(noisyCgiState);


		/*
		 *	We don't put the following into noisyRunPasses() because they
		 *	are not general-purpose.
		 */


		/*
		 *	Dot backend.
		 */
		if (noisyCgiState->irBackends & kNoisyIrBackendDot)
		{
			//noisyPrintToFile(noisyCgiState, noisyIrDotBackend(noisyCgiState), "tmpdot", kNoisyPostFileWriteActionRenderDot);
		}


		/*
		 *	XXX
		 */
		//if (noisyCgiState->irBackends & kNoisyIrBackendXXX)
		{
			//noisyIrXXXBackend(noisyCgiState, noisyCgiState->colorOptimizationLambda, 
			//				noisyCgiState->shapeOptimizationPercent, noisyCgiState->colorOptimizationBackgroundBrightness);
		}


		if (noisyCgiState->mode & kNoisyModeCallTracing)
		{
			noisyTimeStampDumpTimeline(noisyCgiState);
		}

		if (noisyCgiState->mode & kNoisyModeCallStatistics)
		{
			uint64_t	irNodeCount = 0;


			noisyTimeStampDumpResidencies(noisyCgiState);

			//irNodeCount = noisyIrHelperTreeSize(noisyCgiState, noisyCgiState->noisyIrRoot);

			flexprint(noisyCgiState->Fe, noisyCgiState->Fm, noisyCgiState->Fpinfo, "Intermediate Representation Information:\n\n");
			flexprint(noisyCgiState->Fe, noisyCgiState->Fm, noisyCgiState->Fpinfo, "    IR node count                        : %llu\n", irNodeCount);

			/*
			 *	Libflex malloc statistics:
			 */
			if (noisyCgiState->Fm->debug)
			{
				flexmblocksdisplay(noisyCgiState->Fe, noisyCgiState->Fm, noisyCgiState->Fpinfo);
			}
		}
	}
	else
	{
		//TODO: we could intelligently set (and use) the jumpParameter...
		
		/*	Return again after longjmp	*/
		noisyCgiState->jmpbufIsValid = false;
	}
	getrusage(RUSAGE_SELF, &end);
	endRss = noisyCheckRss(noisyCgiState);


	if (noisyCgiState->lastDotRender != NULL)
	{
		printf("<table width=\"%d\" border=\"0\">\n", fmtWidth);
		printf("<tr><td>\n");
		printf("<pre>");
		printf("<a href=\"%s%s.png\" target=\"_blank\"><img src=\"%spng-icon.png\" height=25 border=0/></a>&nbsp;",
			kNoisyCgiFileUrlBase, noisyCgiState->lastDotRender, kNoisyCgiFileUrlBase);
		printf("<a href=\"%s%s.pdf\" target=\"_blank\"><img src=\"%spdf-icon.png\" height=25 border=0/></a>&nbsp;",
			kNoisyCgiFileUrlBase, noisyCgiState->lastDotRender, kNoisyCgiFileUrlBase);
		printf("<a href=\"%s%s.svg\" target=\"_blank\"><img src=\"%ssvg-icon.png\" height=25 border=0/></a>&nbsp;",
			kNoisyCgiFileUrlBase, noisyCgiState->lastDotRender, kNoisyCgiFileUrlBase);
		printf("</pre>\n");
		printf("</td></tr>\n");
		printf("</table>\n");
		
		/*
		 *	Embed the PNG variant inline.
		 */
		printf("<img src=\"%s%s.png\" width=\"%d\">\n",
			kNoisyCgiFileUrlBase, noisyCgiState->lastDotRender, fmtWidth);
	}


	/*
	 *	Use div instead of span to get bgcolor to be page-wide
	 */
	printf("<br>\n");
	printf("<br>\n");
	printf("<span style=\"background-color:#99CCFF; color:#000000;\" onclick=\"JavaScript:toggle('noisyinfo')\">");
	printf("&nbsp;&nbsp;Informational Report&nbsp;&nbsp;&nbsp;<b>(Click here to show/hide.)</b>&nbsp;&nbsp;</span><br>");
	printf("<table width=\"%d\" border=\"0\">\n", fmtWidth);
	printf("<tr><td>\n");
	printf("<pre>");
	printf("<span style=\"background-color:#99CCFF; color:#000000;\" id='noisyinfo'>");
	htmlPrint(noisyCgiState->Fpinfo->circbuf);
	printf("</span>");
	printf("</pre>\n");
	printf("</td></tr>\n");
	printf("</table>\n");

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
	
	
	if (strlen(noisyCgiState->Fperr->circbuf) != 0)
	{
		printf("<span width=\"%d\" style=\"background-color:FFCC00; color:#FF0000\" onclick=\"JavaScript:toggle('noisyerrs')\">", fmtWidth);
		printf("&nbsp;&nbsp;Error Report&nbsp;&nbsp;&nbsp;<b>(Click here to show/hide.)</b>&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;");
		printf("&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span><br><table width=\"%d\" border=\"0\"><tr><td><pre>", fmtWidth);
		printf("<span style=\"background-color:whitesmoke\" id='noisyerrs'>%s</span></pre></td></tr></table>", noisyCgiState->Fperr->circbuf);
	}

	printf("<span style=\"background-color:#CCFF33\">\n");
	printf("&nbsp;&nbsp;&nbsp;<b>&#9879;</b>&nbsp;&nbsp;&nbsp;(<b>Noisy/" FLEX_UVLONGFMT 
					":</b>&nbsp;&nbsp;Operation completed in %.6f&thinsp;seconds S+U time; &nbsp; Mem = "
					FLEX_UVLONGFMT "&thinsp;KB, &nbsp; &#916; Mem = " FLEX_UVLONGFMT "&thinsp;KB).\n",
					noisyCgiState->callAggregateTotal, 
					(	(end.ru_stime.tv_sec - start.ru_stime.tv_sec) +
						(end.ru_utime.tv_sec - start.ru_utime.tv_sec))
					+
					(	(end.ru_stime.tv_usec - start.ru_stime.tv_usec) +
						(end.ru_utime.tv_usec - start.ru_utime.tv_usec))/1E6,
					endRss, endRss-startRss);
	printf("</span>\n");
	printf("<br>\n");


	/*
	 *	Count number of lines in noisyCodeBuffer
	 */
	lines = 0;
	for (i = 0; i < strlen(noisyCodeBuffer); i++)
	{
		if (noisyCodeBuffer[i] == '\n') lines++;
	}

	printf("<div>\n");
	printf("<form action=\"%s-%s\">\n", kNoisyCgiExecutableUrl, kNoisyL10N);
	
	/*
	 *	"spellcheck=false" is in the HTML5 spec. We do this to have
	 *	the nicety of browser (e.g., safari) not underlining code
	 *	as typos.
	 */
	printf("        <textarea spellcheck=\"false\" style=\"%s\" type=\"textarea\" cols=\"%d\" rows=\"%d\" id=\"inputarea\" name=\"c\">\n",
			kNoisyCgiHtmlInputStyle, (fmtWidth*100)/618, lines+2);
	printf("%s", noisyCodeBuffer);
	printf("</textarea>\n");
	printf("<br><input type=\"hidden\" name=\"w\" value=\"%d\">\n", fmtWidth);
	
	printf("<br><b>Compiler Parameters</b><br>\n");
	printf("Backends Bitmap&nbsp;<input type=\"number\" name=\"s\" style=\"width: 30px\" value=\"%d\"><br>\n", cgiSparameter);
	printf("Passes Bitmap&nbsp;<input type=\"number\" name=\"o\" style=\"width: 60px\" value=\"%d\"><br>\n", cgiOparameter);
	printf("Dot detail level (14=SCDG)&nbsp;<input type=\"number\" name=\"t\" style=\"width: 60px\" value=\"%d\"><br>\n", cgiTparameter);
	printf("<input type=\"submit\" name=\"b\" value=\"compile\">\n");
	printf("</form>\n");
	printf("</div>\n");

	printf("</font>\n");

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
//			case 'ó': printf("&#243;"); break;
//			case 'é': printf("&#233;"); break;
//			case 'ú': printf("&#250;"); break;

			default : printf("%c", s[i]);
		}
	}

	return;
}
