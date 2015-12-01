Noisy: A Language for Talking Sensors to Sleep
==============================================



The [noisy compiler]
---------------------
The Noisy compiler takes Noisy programs and compiles them to either
Noisy Bytecode (the Noisy IR serialized via Google's Protocol
Buffers), or renders the IR and symbol table using GraphViz/Dot for
debugging.

You can invoke the compiler on your platform, e.g., [./noisy-darwin-EN],
with the flags -h or --help to see the usage:

>	Noisy version 0.1-alpha-2655d9edbe4e+ (build 11-22-2015-18:41-pip@owl-Darwin-15.0.0-x86_64), Phillip Stanley-Marbell.
>	
>	Usage:    noisy [ (--help, -h)                                       
>	                | (--version, --V)                                   
>	                | (--verbose <level>, -v <level>)                    
>	                | (--dot <level>, -d <level>)                        
>	                | (--bytecode <output file name>, -b <output file name>)
>	                | (--optimize <level>, -O <level>)                   
>	                | (--trace, -t)                                      
>	                | (--statistics, -s) ]                               
>	                                                                     
>	              <filenames>

To compile a Noisy program and display statistics on internal
routine calls:

>	% ./noisy-darwin-EN --optimize 0 --statistics Examples/helloWorld.n

To compile a Noisy program and emit its IR into [dot], and render
the generated [dot] code through [dot]:

>	% ./noisy-darwin-EN --optimize 0 --dot 0 Examples/helloWorld.n | dot -Tpdf -O ; open noname.gv.pdf

The [dot] detail levels are bit masks: 1<<0: no text, 1<<1: no nil nodes

Rendering of the IR can be simplified by using one of the helper
scripts described below.



The [noisy helper scripts]: [noisyIr2dot.sh]
----------------------------------------------------------------------
The scripts [noisyIr2dot.sh] generates renderings of the Noisy AST
and symbol table. It takes two arguments: a Noisy source file and
a rendering format (e.g., "pdf" or "png").  It is a simple wrapper
to the noisy compiler, which it invokes with a useful default set 
of flags.

Example:

>	% ./noisyIr2dot.sh Examples/helloWorld.n pdf


Development
-----------
There are pre- and post-commit hooks that will build the compiler,
run it against a reference input, and record statistics on number
of calls made and time spent in most of the compiler's implementation
routines.

The hooks can be enabled for mercurial by adding the following to
.hg/hgrc:

>	[hooks]
>	pretxncommit    = ./precommitStatisticsHook.sh
>	commit          = ./postcommitStatisticsHook.sh

The generated statistics are stored in the Statistics/ subdirectory,
and can be analyzed using the Mathematica notebook that resides at
Mathematica/AnalyzeStatistics.nb.


CGI on Mac OS X
---------------
We use a CGI interface along with any web browser to provide a
poor-person's GUI interface. Installing the CGI version of the
compiler lets us use a web browser and some minimal Javascript to
create a cross-platform GUI and IDE.

(On Mac OS X, $kNoisyBasePath is /Library/WebServer/Documents/tmp.
See config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) for other
platforms.)

>	% mkdir $kNoisyBasePath
>	% cp pdf-icon.png png-icon.png svg-icon.png $kNoisyBasePath/
>	% cp noisycgi-darwin-EN /Library/WebServer/CGI-Executables/
>	% chmod 777 $kNoisyBasePath
>	% sudo chmod 755 $kNoisyBasePath/*.png

On older versions of MacOS (~10.8 and earlier), enable the web
server via the MacOS System Preferences --> Sharing. On Mac OS 10.10
and later, edit /etc/apache2/httpd.conf and (1) uncomment the line
for LoadModule cgi_module (2) restart apache (apachectl restart),
then (3) and visit:

>	% open  http://localhost/cgi-bin/noisycgi-darwin-EN?c=HelloWorld+%3A+progtype%0D%0A%7B%0D%0A++++++++init++++%3A+namegen+%28list+of+string%29%3A%28list+of+string%29%3B%0D%0A%7D%0D%0A%0D%0Ainit+%3D%0D%0A%7B%0D%0A++++++++print+%3A%3D+name2chan+string+%22system.print%22+0.0%3B%0D%0A++++++++print+%3C-%3D+%22Hello+World%21%22%3B%0D%0A%7D%0D%0A&w=980&s=0&o=0&t=0&b=compile

(the above URL encodes the parameters for the backends, passes, as
well as the code, and html render width)

The text editor with syntax coloring we now use is ACE (ace.c9.io),
in conjunction with the jquery-git plugin to make it work for us
(see comments in cgimain.c).

Retrieve the JQuery-git from http://code.jquery.com/jquery-git.js
and copy it to $kNoisyBasePath

>	% wget http://code.jquery.com/jquery-git.js /tmp/
>	% sudo cp /tmp/jquery-git.js $kNoisyBasePath/

Git clone https://github.com/ajaxorg/ace-builds.git and copy src-noconflict subdirectory to $kNoisyBasePath
>	% git clone https://github.com/ajaxorg/ace-builds.git /tmp/ace-builds
>	% sudo cp -r /tmp/ace-builds/src-noconflict $kNoisyBasePath



Details on command line parameters:
-----------------------------------
Compiler pass bitmaps:
- - - - - - - - - - -
TBD.


Compiler backend bitmaps:
- - - - - - - - - - - - -
>	typedef enum
>	{
>		kNoisyIrBackendDot				= (1 << 1),
>	} NoisyIrBackend;


Dot rendering detail bitmaps:
- - - - - - - - - - - - - - -
>	typedef enum
>	{
>		kNoisyDotDetailLevelNoText			= (1 << 0),
>		kNoisyDotDetailLevelNoNilNodes			= (1 << 1),
>	} NoisyDotDetailLevel;
