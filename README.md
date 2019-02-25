# Noisy and Newton
Noisy is a programming language for talking to sensors. Newton is a specification language for describing physics. Noisy is descended from the [M programming language](http://www.gemusehaken.org/people/pip/papers/pmup06.pdf) [Stanley-Marbell and Marculescu, 2006] which is in turn descended from the [Limbo programming language](https://en.wikipedia.org/wiki/Limbo_(programming_language)) [Dorward, Pike, Trickey, 1994], and the [Alef programming language](https://en.wikipedia.org/wiki/Alef_(programming_language)) [Winterbottom, 1992].

Newton is a language for specifying assertions (invariants) about physical systems. Newton was originally intended to be a configuration language for the Noisy compiler to encapsulate information about temporally-invariant physical properties of the hardware on which a Noisy program executes. The first implementation of Newton based on the Noisy code base, and the API for interfacing to the Newton intermediate representation, was the focus of the [M.Eng. thesis of Jonatham Lim](https://dspace.mit.edu/bitstream/handle/1721.1/119591/1066741153-MIT.pdf?sequence=1). A  [short ArXiv paper](https://arxiv.org/abs/1811.04626) summarizes the concepts in Newton. Newton has evolved into a self-contained foundation for research investigating automated dimensional analysis of physical system descriptions, automated analysis for differential privacy in sensors, and automated generation of physics-constrained function approximation, among other things. In contrast to Newton, which is designed for specifying physical assertions/invariants, alternative such as [Modelica](https://modelica.org), allow you to imperatively model the dynamics of physical systems. 

Because the Newton compiler started out as a modification of the Noisy compiler implementation to test out ideas, the implementation of Newton borrows/shares many components from the Noisy compiler and is therefore distributed with it.
- - - -

The correct way to clone this repository to get the submodules is:

	git clone --recursive git@github.com:phillipstanleymarbell/Noisy-lang-compiler.git

To update all submodules:

	git pull --recurse-submodules
	git submodule update --remote --recursive

If you forgot to clone with `--recursive` and end up with empty submodule directories, you can remedy this with

	git submodule update --init

Building the Noisy compiler and debug tools depends on the [`libflex`](https://github.com/phillipstanleymarbell/libflex), [`Wirth-tools`](https://github.com/phillipstanleymarbell/Wirth-tools), and [`DTrace-scripts`](https://github.com/phillipstanleymarbell/DTrace-scripts) repositories. These repositories are already included as submodules:

	Libflex:		git@github.com:phillipstanleymarbell/libflex.git
	Wirth tools:		git@github.com:phillipstanleymarbell/Wirth-tools.git
	DTrace-scripts:		git@github.com:phillipstanleymarbell/DTrace-scripts.git
	
For linear algebra, we use the Eigen library. This is also already linked to the repository as a submodule:

	Eigen:			git@github.com:eigenteam/eigen-git-mirror.git	
	
The build also depends on the C protobuf compiler, `sloccount`, and on Graphviz. On Mac OS X, the easiest way to
install these is to use macports (macports.org) to install the packages `protobuf-c` and `protobuf-cpp` (on Debian, you want the package `libprotobuf-c-dev` and on Ubuntu you also want `protobuf-c-compiler`), `sloccount`, and `graphviz-devel`.

Once you have the above repositories, 

1.	Create a file `config.local` in the root of the Noisy tree and edit it to contain 

	LIBFLEXPATH     = full-path-to-libflex-repository-clone 
	CONFIGPATH      = full-path-to-libflex-repository-clone
	OSTYPE		= <one of 'linux' or 'darwin'>
	MACHTYPE	= x86_64

For example,

	LIBFLEXPATH=/home/me/Noisy-lang-compiler/submodules/libflex
	CONFIGPATH=/home/me/Noisy-lang-compiler/submodules/libflex
	OSTYPE		= linux
	MACHTYPE	= x86_64

2.	Edit `precommitStatisticsHook-<your os type>.sh` to set

	dtraceDirectory=full-path-to-DTrace-repository-clone
	libflexDirectory=full-path-to-libflex-repository-clone

For the following steps, valid options for OSTYPE are currently `linux` 
for Gnu/Linux and `darwin` for macOS.

3.	Build Libflex by going to the directory you cloned for Libflex and 
running `make`. The Makefile assumes the environment variables `OSTYPE`
and `MACHTYPE` are set. If that is not the case, you will need to 
explicitly do:

	make OSTYPE=darwin MACHTYPE=x86_64

4.	Build the noisy compiler by running `make`. The makefile assumes the 
environment variables `OSTYPE` and `MACHTYPE` are set. If that is not the 
case, you will need to explicitly do:

	make OSTYPE=darwin MACHTYPE=x86_64


The Noisy compiler
------------------
The Noisy compiler takes Noisy programs and compiles them to either
Noisy Bytecode (the Noisy IR serialized via Google's Protocol
Buffers), or renders the IR and symbol table using GraphViz/Dot for
debugging.

You can invoke the compiler on your platform, e.g., `./noisy-darwin-EN`,
with the flags `-h` or `--help` to see the usage:

	Noisy version 0.1-alpha-2655d9edbe4e+ (build 11-22-2015-18:41-pip@owl-Darwin-15.0.0-x86_64), Phillip Stanley-Marbell.
	
	Usage:    noisy [ (--help, -h)                                       
	                | (--version, --V)                                   
	                | (--verbose <level>, -v <level>)                    
	                | (--dot <level>, -d <level>)                        
	                | (--bytecode <output file name>, -b <output file name>)
	                | (--optimize <level>, -O <level>)                   
	                | (--trace, -t)                                      
	                | (--statistics, -s) ]                               
	                                                                     
	              <filenames>

To compile a Noisy program and display statistics on internal
routine calls:

	% ./src/noisy/noisy-darwin-EN --optimize 0 --statistics applications/noisy/helloWorld.n

To compile a Noisy program and emit its IR into `dot`, and render
the generated `dot` code through `dot`:

	% ./src/noisy/noisy-darwin-EN --optimize 0 --dot 0 applications/noisy/helloWorld.n | dot -Tpdf -O ; open noname.gv.pdf

The `dot` detail levels are bit masks: `1<<0` (i.e., 1): no text,
`1<<1` (i.e., 2): no nil nodes

Rendering of the IR can be simplified by using one of the helper
scripts described below.



The Noisy helper scripts: `noisyIr2dot.sh`
------------------------------------------
The scripts `noisyIr2dot.sh` generates renderings of the Noisy AST
and symbol table. It takes two arguments: a Noisy source file,
a rendering format (e.g., "pdf" or "png"), and a `dot` detail level
(see the section above in README.md) for the Noisy dot backend (e.g., 
'0').  It is a simple wrapper to the noisy compiler, which it invokes
with a useful default set of flags.

For example, from the `noisy` build directory:

	% ./noisyIr2dot.sh ../../applications/noisy/helloWorld.n pdf 0


Implementation and the Wirth tools
----------------------------------
The Noisy compiler implementation uses the Wirth tools 
(https://github.com/phillipstanleymarbell/Wirth-tools) to generate
various helper header files. The Wirth tools are not yet well polished,
so the process is a bit messy:

First, run `ffi2code` on noisy.ffi to generate all the header definitions in a single file.
Ignore any debugging statements that appear on `stderr` and focus only on the output
directed via `stdout` to the output file as in the following example. From the `noisy` build directory:

	 ../../submodules/Wirth-tools/ffi2code-darwin-EN noisy.ffi > noisy-ff-debug.txt

Next, manually copy the part of the result to the appropriate header files:

1.	The array `ASTnodeType` goes into `noisy.h` as `NoisyIrNodeType`

2.	The rest of the generated code goes into `noisy-ffi2code-autoGeneratedSets.c`. See the comments therein for more.

For an explanation of the `T_XXX` tokens in older files related to
`ffi2code`, see `https://github.com/phillipstanleymarbell/Wirth-tools/blob/master/EXAMPLES/bug.0.ffi`

Development
-----------
There are pre- and post-commit hooks that will build the compiler,
run it against a reference input, and record statistics on number
of calls made and time spent in most of the compiler's implementation
routines.

The hooks can be enabled for mercurial by adding the following to
`.hg/hgrc`:

	[hooks]
	pretxncommit    = ./precommitStatisticsHook-<your-os-type>.sh
	commit          = ./postcommitStatisticsHook.sh

The generated statistics are stored in the `analysis/statistics/` subdirectory,
and can be analyzed using the Mathematica notebook that resides at
`analysis/mathematica/AnalyzeStatistics.nb`.


CGI on Mac OS X
---------------
We use a CGI interface along with any web browser to provide a
poor-person's GUI interface. Installing the CGI version of the
compiler lets us use a web browser and some minimal Javascript to
create a cross-platform GUI and IDE.

(On Mac OS X, `$kNoisyBasePath` is `/Library/WebServer/Documents/tmp`.
See `config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT)` for other
platforms.)

	% mkdir $kNoisyBasePath
	% cp icons/* $kNoisyBasePath/
	% cp noisycgi-darwin-EN /Library/WebServer/CGI-Executables/
	% chmod 777 $kNoisyBasePath
	% sudo chmod 755 $kNoisyBasePath/*.png

On older versions of MacOS (~10.8 and earlier), enable the web
server via the MacOS System Preferences --> Sharing. On Mac OS 10.10
and later, edit `/etc/apache2/httpd.conf` and (1) uncomment the line
for LoadModule cgi_module (2) restart apache (`sudo apachectl restart`),
then (3) and visit:

	% open  http://localhost/cgi-bin/noisycgi-darwin-EN?c=HelloWorld+%3A+progtype%0D%0A%7B%0D%0A++++++++init++++%3A+namegen+%28list+of+string%29%3A%28list+of+string%29%3B%0D%0A%7D%0D%0A%0D%0Ainit+%3D%0D%0A%7B%0D%0A++++++++print+%3A%3D+name2chan+string+%22system.print%22+0.0%3B%0D%0A++++++++print+%3C-%3D+%22Hello+World%21%22%3B%0D%0A%7D%0D%0A&w=980&s=0&o=0&t=0&b=compile

(the above URL encodes the parameters for the backends, passes, as
well as the code, and html render width)

The text editor with syntax coloring we now use is ACE (ace.c9.io),
in conjunction with the jquery-git plugin to make it work for us
(see comments in `cgimain.c`).

Retrieve the JQuery-git from `http://code.jquery.com/jquery-git.js`
and copy it to `$kNoisyBasePath`:

	% wget http://code.jquery.com/jquery-git.js /tmp/
	% sudo cp /tmp/jquery-git.js $kNoisyBasePath/

Git clone `https://github.com/ajaxorg/ace-builds.git` and copy the `src-noconflict` subdirectory to `$kNoisyBasePath`:

	% git clone https://github.com/ajaxorg/ace-builds.git /tmp/ace-builds
	% sudo cp -r /tmp/ace-builds/src-noconflict $kNoisyBasePath



Details on command line parameters:
-----------------------------------
Compiler backend bitmaps:

	typedef enum
	{
		kNoisyIrBackendDot				= (1 << 1),
	} NoisyIrBackend;


Dot rendering detail bitmaps:

	typedef enum
	{
		kNoisyDotDetailLevelNoText			= (1 << 0),
		kNoisyDotDetailLevelNoNilNodes			= (1 << 1),
	} NoisyDotDetailLevel;
