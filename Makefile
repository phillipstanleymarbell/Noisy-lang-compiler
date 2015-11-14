LIBFLEXPATH     = /Volumes/doos/libflex-hg-clone
CONFIGPATH	= /Volumes/doos/libflex-hg-clone
COMPILERVARIANT = .clang
include		$(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT)
include		config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT)

#MAKEFLAGS	+= -j8

CCFLAGS		= $(PLATFORM_DBGFLAGS) $(PLATFORM_CFLAGS) $(PLATFORM_DFLAGS) $(PLATFORM_OPTFLAGS)
LDFLAGS 	= $(PLATFORM_DBGFLAGS) -lm $(PLATFORM_LFLAGS) `pkg-config --libs cairo`

LIBNOISY	= libNoisy
NOISY_L10N	= EN

#	-std=gnu99 because we use anonymous unions and induction variable defintions in loop head.
CCFLAGS		+= -std=gnu99 -DkNoisyL10N="\"$(NOISY_L10N)\"" -DNOISY_L10N_EN `pkg-config --cflags cairo`

TARGET		= noisy-$(OSTYPE)-$(NOISY_L10N)
CGI_TARGET	= noisycgi-$(OSTYPE)-$(NOISY_L10N)

WFLAGS		= -Wall -Werror
INCDIRS		= -I. -I$(LIBFLEXPATH)

NOISYEXTENSION	= n

SOURCES		=\
		version.c\
		main.c\
		cgimain.c\
		noisy.c\
		noisy-errors$(NOISY_L10N).c\
		noisy-typeSignatures.c\
		noisy-productions.c\
		noisy-tokens.c\
		noisy-timeStamps.c\


OBJS		=\
		version.$(OBJECTEXTENSION)\
		noisy-errors$(NOISY_L10N).$(OBJECTEXTENSION)\
		noisy-typeSignatures.$(OBJECTEXTENSION)\
		noisy-productions.$(OBJECTEXTENSION)\
		noisy-tokens.$(OBJECTEXTENSION)\
		noisy-timeStamps.$(OBJECTEXTENSION)\
		noisy.$(OBJECTEXTENSION)\
		main.$(OBJECTEXTENSION)\


CGIOBJS		=\
		version.$(OBJECTEXTENSION)\
		noisy-errors$(NOISY_L10N).$(OBJECTEXTENSION)\
		noisy-typeSignatures.$(OBJECTEXTENSION)\
		noisy-productions.$(OBJECTEXTENSION)\
		noisy-tokens.$(OBJECTEXTENSION)\
		noisy-timeStamps.$(OBJECTEXTENSION)\
		noisy.$(OBJECTEXTENSION)\
		cgimain.$(OBJECTEXTENSION)\


LIBNOISYOBJS =\
		version.$(OBJECTEXTENSION)\
		noisy-errors$(NOISY_L10N).$(OBJECTEXTENSION)\
		noisy-typeSignatures.$(OBJECTEXTENSION)\
		noisy-productions.$(OBJECTEXTENSION)\
		noisy-tokens.$(OBJECTEXTENSION)\
		noisy-timeStamps.$(OBJECTEXTENSION)\
		noisy.$(OBJECTEXTENSION)\



HEADERS		=\
		$(LIBFLEXPATH)/flex.h\
		version.h\
		noisy.h\
		noisy-errors.h\



all: $(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N).a target $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) Makefile cgi

#
#			Libraries
#
$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N).a: $(LIBNOISYOBJS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) Makefile 
	$(AR) $(ARFLAGS) $@ $(LIBNOISYOBJS)


#
#			Executables
#
target: $(OBJS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) Makefile 
	$(LD) -L. -L$(LIBFLEXPATH) $(LDFLAGS) $(OBJS) -lflex-$(OSTYPE) -o $(TARGET)


cgi:$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N).a $(CGIOBJS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) Makefile 
	$(LD) -L. -L$(LIBFLEXPATH) $(LDFLAGS) $(CGIOBJS) -lflex-$(OSTYPE) -o $(CGI_TARGET)


full: scan README.sloccount


#
#			Objects
#
%.$(OBJECTEXTENSION): %.c $(HEADERS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) Makefile 
#	$(SPLINT) $(FLEXFLAGS) $(INCDIRS) $<
#	$(LCLINT) $(FLEXFLAGS) $(INCDIRS) $<
	$(CC) $(FLEXFLAGS) $(INCDIRS) $(CCFLAGS) $(WFLAGS) $(OPTFLAGS) -c --analyze $<
	$(CC) $(FLEXFLAGS) $(INCDIRS) $(CCFLAGS) $(WFLAGS) $(OPTFLAGS) -c $<

version.c: $(HEADERS) Makefile
	echo 'char kNoisyVersion[] = "0.1-alpha-'`hg id --id`' (build '`date '+%m-%d-%Y-%H:%M'`-`whoami`@`hostname -s`-`uname -s`-`uname -r`-`uname -m`\), Phillip Stanley-Marbell\"\; > version.c


#
#			Debug and Documentation
#
#	Need --dsymutil=yes flag to valgrind to get line numbers in error messages
debug:	all
	valgrind --dsymutil=yes --leak-check=yes ./$(TARGET) EXAMPLES/test.crayon >& debug.valgrind.log; mate debug.valgrind.log &


scan:	clean
	scan-build --use-cc=$(CC) -k -V make -j4


README.sloccount: $(HEADERS) $(SOURCES)
	sloccount *.c *.h *.grammar *.ffi *.$(NOISYEXTENSION) */*.$(NOISYEXTENSION) > README.sloccount


installcgi:
	sudo cp $(CGI_TARGET) $(CGI_BIN)
	
test:
	./noisy-darwin-EN --dot examples/helloWorld.n | dot -Tpdf -O ; open noname.gv.pdf 


clean:
	rm -rf version.c $(OBJS) $(CGIOBJS) $(LIBNOISYOBJS) $(TARGET) $(TARGET).dSYM $(CGI_TARGET) $(CGI_TARGET).dsym $(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N).a *.o *.plist
