1.	Code indented with tabs, not spaces.

2.	Variable names in `camelCase`.

3.	Type names begin with a capital. Type names specific to Noisy (similarly, for Newton) begin with `Noisy`, e.g., `NoisySymbolType`.

4.	Constant names and enum entries begin with `kNoisy` or `kNewton`, e.g., `kNoisyIrNodeType_PintegerType`. 

5.	C-style comments, in the form:
````c
	/*
	 *	Comment (offset with a single tab)
	 */
````

6.	Comments are not just "notes to self". They should provide useful explanatory information.

7.	No `#include` within header .h files if possible.

8.	No function definitions in header .h files.

9.	Files named named module `<noisy/newton/common>-camelCasedModuleName`, e.g., `noisy-xxxcamelCasedName.c`.

10.	Constants in `enum`s, not in `#define`s where possible.

11.	Avoid `#define` if possible.

12.	All `if` statement followed by curly braces, even if body is a single statement.

13.	The pattern `\t\n` (tab followed by newline) should never occur in a source file.

14.	Except for temporary debugging statements, all print statements should use `flexprint` from the `libflex` library (https://github.com/phillipstanleymarbell/libflex). This allows us to buffer print statements and makes the web interface/demos and other deployments possible. Errors go into the buffer `Fperr` and informational output (almost everything that is not an error) goes into `Fpinfo`. We sometimes have additional dedicated buffers to isolate certain outputs.
