LIBFLEXPATH     = /Volumes/doos/libflex-hg-clone
CONFIGPATH	= /Volumes/doos/libflex-hg-clone
COMPILERVARIANT = .clang
include		$(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT)
include		config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT)

MAKEFLAGS	+= -j

CCFLAGS		= $(PLATFORM_DBGFLAGS) $(PLATFORM_CFLAGS) $(PLATFORM_DFLAGS) $(PLATFORM_OPTFLAGS)
LDFLAGS 	= $(PLATFORM_DBGFLAGS) -lm $(PLATFORM_LFLAGS) `pkg-config --libs 'libprotobuf-c >= 1.0.0'`

LIBNOISY	= Noisy
NOISY_L10N	= EN

#	-std=gnu99 because we use anonymous unions and induction variable defintions in loop head.
CCFLAGS		+= -std=gnu99 -DkNoisyL10N="\"$(NOISY_L10N)\"" -DNOISY_L10N_EN

TARGET		= noisy-$(OSTYPE)-$(NOISY_L10N)
CGI_TARGET	= noisycgi-$(OSTYPE)-$(NOISY_L10N)

WFLAGS		= -Wall -Werror
INCDIRS		= -I. -I$(LIBFLEXPATH)
PROTOC		= protoc-c

NOISYEXTENSION	= n

SOURCES		=\
		version.c\
		noisy.pb-c.c\
		main.c\
		cgimain.c\
		noisy.c\
		noisy-errors$(NOISY_L10N).c\
		noisy-typeSignatures.c\
		noisy-productions.c\
		noisy-tokens.c\
		noisy-timeStamps.c\
		noisy-types.c\
		noisy-lexer.c\
		noisy-parser.c\
		noisy-firstAndFollow.c\
		noisy-irHelpers.c\
		noisy-ffi2code-autoGeneratedSets.c\
		noisy-symbolTable.c\
		noisy-irPass-dotBackend.c\
		noisy-irPass-protobufBackend.c\


#
#	Clang seems to be unable to do LTO unless we have all the objects
#	not tucked into a library, so we don't just simply link maain against
#	-l$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N)
#
OBJS		=\
		version.$(OBJECTEXTENSION)\
		noisy.pb-c.$(OBJECTEXTENSION)\
		noisy-errors$(NOISY_L10N).$(OBJECTEXTENSION)\
		noisy-typeSignatures.$(OBJECTEXTENSION)\
		noisy-productions.$(OBJECTEXTENSION)\
		noisy-tokens.$(OBJECTEXTENSION)\
		noisy-timeStamps.$(OBJECTEXTENSION)\
		noisy.$(OBJECTEXTENSION)\
		noisy-types.$(OBJECTEXTENSION)\
		noisy-lexer.$(OBJECTEXTENSION)\
		noisy-parser.$(OBJECTEXTENSION)\
		noisy-firstAndFollow.$(OBJECTEXTENSION)\
		noisy-ffi2code-autoGeneratedSets.$(OBJECTEXTENSION)\
		noisy-irHelpers.$(OBJECTEXTENSION)\
		noisy-irPass-helpers.$(OBJECTEXTENSION)\
		noisy-symbolTable.$(OBJECTEXTENSION)\
		noisy-irPass-dotBackend.$(OBJECTEXTENSION)\
		noisy-irPass-protobufBackend.$(OBJECTEXTENSION)\
		main.$(OBJECTEXTENSION)\


#
#	Clang seems to be unable to do LTO unless we have all the objects
#	not tucked into a library, so we don't just simply link maain against
#	-l$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N)
#
CGIOBJS		=\
		version.$(OBJECTEXTENSION)\
		noisy.pb-c.$(OBJECTEXTENSION)\
		noisy-errors$(NOISY_L10N).$(OBJECTEXTENSION)\
		noisy-typeSignatures.$(OBJECTEXTENSION)\
		noisy-productions.$(OBJECTEXTENSION)\
		noisy-tokens.$(OBJECTEXTENSION)\
		noisy-timeStamps.$(OBJECTEXTENSION)\
		noisy.$(OBJECTEXTENSION)\
		noisy-types.$(OBJECTEXTENSION)\
		noisy-lexer.$(OBJECTEXTENSION)\
		noisy-parser.$(OBJECTEXTENSION)\
		noisy-firstAndFollow.$(OBJECTEXTENSION)\
		noisy-ffi2code-autoGeneratedSets.$(OBJECTEXTENSION)\
		noisy-irHelpers.$(OBJECTEXTENSION)\
		noisy-irPass-helpers.$(OBJECTEXTENSION)\
		noisy-symbolTable.$(OBJECTEXTENSION)\
		noisy-irPass-dotBackend.$(OBJECTEXTENSION)\
		noisy-irPass-protobufBackend.$(OBJECTEXTENSION)\
		cgimain.$(OBJECTEXTENSION)\


LIBNOISYOBJS =\
		version.$(OBJECTEXTENSION)\
		noisy.pb-c.$(OBJECTEXTENSION)\
		noisy-errors$(NOISY_L10N).$(OBJECTEXTENSION)\
		noisy-typeSignatures.$(OBJECTEXTENSION)\
		noisy-productions.$(OBJECTEXTENSION)\
		noisy-tokens.$(OBJECTEXTENSION)\
		noisy-timeStamps.$(OBJECTEXTENSION)\
		noisy.$(OBJECTEXTENSION)\
		noisy-types.$(OBJECTEXTENSION)\
		noisy-lexer.$(OBJECTEXTENSION)\
		noisy-parser.$(OBJECTEXTENSION)\
		noisy-firstAndFollow.$(OBJECTEXTENSION)\
		noisy-ffi2code-autoGeneratedSets.$(OBJECTEXTENSION)\
		noisy-irHelpers.$(OBJECTEXTENSION)\
		noisy-irPass-helpers.$(OBJECTEXTENSION)\
		noisy-symbolTable.$(OBJECTEXTENSION)\
		noisy-irPass-dotBackend.$(OBJECTEXTENSION)\
		noisy-irPass-protobufBackend.$(OBJECTEXTENSION)\



HEADERS		=\
		$(LIBFLEXPATH)/flex.h\
		version.h\
		noisy.pb-c.h\
		noisy.h\
		noisy-errors.h\
		noisy-timeStamps.h\
		noisy-types.h\
		noisy-lexer.h\
		noisy-parser.h\
		noisy-irHelpers.h\
		noisy-irPass-helpers.h\
		noisy-firstAndFollow.h\
		noisy-symbolTable.h\
		noisy-irPass-dotBackend.h\
		noisy-irPass-protobufBackend.h\



all: lib$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N).a target $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) Makefile cgi

#
#			Libraries
#
lib$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N).a: $(LIBNOISYOBJS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) Makefile 
	$(AR) $(ARFLAGS) $@ $(LIBNOISYOBJS)


#
#			Executables
#
target: $(OBJS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) Makefile 
	$(LD) -L. -L$(LIBFLEXPATH) $(LDFLAGS) $(OBJS) -lflex-$(OSTYPE) -o $(TARGET)


cgi:lib$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N).a $(CGIOBJS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) Makefile 
	$(LD) -L. -L$(LIBFLEXPATH) $(LDFLAGS) $(CGIOBJS) -lflex-$(OSTYPE) -o $(CGI_TARGET)


full: scan README.sloccount


#
#			Objects
#
%.$(OBJECTEXTENSION): %.c $(HEADERS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE)$(COMPILERVARIANT) Makefile 
#	$(SPLINT) $(FLEXFLAGS) $(INCDIRS) $<
#	$(LCLINT) $(FLEXFLAGS) $(INCDIRS) $<
	$(CC) $(FLEXFLAGS) $(INCDIRS) $(CCFLAGS) $(WFLAGS) $(OPTFLAGS) -c $(LINTFLAGS) $<
	$(CC) $(FLEXFLAGS) $(INCDIRS) $(CCFLAGS) $(WFLAGS) $(OPTFLAGS) -c $<

noisy.pb-c.c: noisy.proto Makefile
	$(PROTOC) --c_out=. noisy.proto

noisy.pb-c.h: noisy.pb-c.c

noisy-irPass-protobufBackend.$(OBJECTEXTENSION): noisy-irPass-protobufBackend.c
	$(CC) $(FLEXFLAGS) $(INCDIRS) $(CCFLAGS) `pkg-config --cflags 'libprotobuf-c >= 1.0.0'` $(WFLAGS) $(OPTFLAGS) -c $<

noisy.pb-c.$(OBJECTEXTENSION): noisy.pb-c.c noisy.proto Makefile
	$(CC) $(FLEXFLAGS) $(INCDIRS) $(CCFLAGS) `pkg-config --cflags 'libprotobuf-c >= 1.0.0'` $(WFLAGS) $(OPTFLAGS) -c $<

version.c: $(HEADERS) Makefile
	echo 'char kNoisyVersion[] = "0.1-alpha-'`hg id --id`'('`hg id --num`') (build '`date '+%m-%d-%Y-%H:%M'`-`whoami`@`hostname -s`-`uname -s`-`uname -r`-`uname -m`\)\"\; > version.c


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


installcgi: cgi
	sudo cp $(CGI_TARGET) $(CGI_BIN)
	
test:
	./noisy-darwin-EN --dot examples/helloWorld.n | dot -Tpdf -O ; open noname.gv.pdf 


clean:
	rm -rf version.c $(OBJS) $(CGIOBJS) $(LIBNOISYOBJS) $(TARGET) $(TARGET).dSYM $(CGI_TARGET) $(CGI_TARGET).dsym lib$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N).a *.o *.plist noisy.pb-c.c noisy.pb-c.h noname.gv.pdf noname.gv.png 
