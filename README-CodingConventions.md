1.	Code indented with tabs, not spaces. If you use `vim`, set your `.vimrc` as follows so you can see where you have stray spaces:
	````
	set list
	set listchars=tab:>-
	````
	For visual alignment, assume the code will be viewed in an editor where tabs are rendered to be as wide as eight spaces.

2.	Variable names in `camelCase`.

3.	Type names begin with a capital. Type names specific to Noisy (similarly, for Newton) begin with `Noisy`, e.g., `NoisySymbolType`.

4.	Constant names and enum entries begin with `kNoisy` or `kNewton`, e.g., `kNoisyIrNodeType_PintegerType`.

5.	Function names in `camelCase`. Context-specific function names begin with context, e.g. for the estimator synthesis IR pass: `irPassEstimatorSynthesisProcessInvariantList`.

6.	C-style comments, in the form:
	````c
		/*
		 *	Comment (offset with a single tab)
		 */
	````

7.	Comments are not just "notes to self". They should provide useful explanatory information.

8.	No `#include` within header .h files if possible.

9.	No function definitions in header .h files.

10.	Files named named module `<noisy/newton/common>-camelCasedModuleName`, e.g., `noisy-xxxcamelCasedName.c`.

11.	Constants in `enum`s, not in `#define`s where possible.

12.	Avoid `#define` if possible.

13.	All `if` statement followed by curly braces, even if body is a single statement.

14.	The pattern `\t\n` (tab followed by newline) should never occur in a source file.

15.	Except for temporary debugging statements, all print statements should use `flexprint` from the `libflex` library (https://github.com/phillipstanleymarbell/libflex). This allows us to buffer print statements and makes the web interface/demos and other deployments possible. Errors go into the buffer `Fperr` and informational output (almost everything that is not an error) goes into `Fpinfo`. We sometimes have additional dedicated buffers to isolate certain outputs.


### .clang-format

The .clang-format file is a clang-format specification that you can use as a starting point for formatting your code. Please note that simply applying  this formatting style is not enough for covering all of the above conventions. For example, conventions (8) to (12) are something you should ensure yourself.
