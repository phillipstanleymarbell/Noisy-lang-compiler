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

6.	No `#include` within header .h files if possible.

7.	No function definitions in header .h files.

8.	Files named named module `<noisy/newton/common>-camelCasedModuleName`, e.g., `noisy-xxxcamelCasedName.c`.

9.	Constants in `enum`s, not in `#define`s where possible.

10.	Avoid `#define` if possible.

11.	All `if` statement followed by curly braces, even if body is a single statement.

12.	The pattern `\t\n` (tab followed by newline) should never occur in a source file.
