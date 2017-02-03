LIBFLEXPATH=/Users/jonathanlim/Documents/Compiler/libflex
CONFIGPATH=/Users/jonathanlim/Documents/Compiler/libflex
TREEROOT=..
NOISYPATH=../noisy
NEWTONPATH=../newton
COMMONPATH=./common
MACHTYPE=x86_64
COMPILERVARIANT=clang
OSTYPE=darwin
include		$(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT)
include		config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT)

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
INCDIRS		= -I. -I$(LIBFLEXPATH) -I$(NOISYPATH) -I$(NEWTONPATH)
PROTOC		= protoc-c

NOISYEXTENSION	= n
NEWTONEXTENSION	= nt

SOURCES		=\
		version.c\
		cgimain.c\


#
#	Clang seems to be unable to do LTO unless we have all the objects
#	not tucked into a library, so we don't just simply link maain against
#	-l$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N)
#
OBJS		=\
		version.$(OBJECTEXTENSION)\


#
#	Clang seems to be unable to do LTO unless we have all the objects
#	not tucked into a library, so we don't just simply link maain against
#	-l$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N)
#
CGIOBJS		=\
		version.$(OBJECTEXTENSION)\
		cgimain.$(OBJECTEXTENSION)\


LIBNOISYOBJS =\
		version.$(OBJECTEXTENSION)\


HEADERS		=\
		$(LIBFLEXPATH)/flex.h\
		version.h\



all: installhooks lib$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N).a $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) Makefile

#
#			Install the pre-commit hooks. There doesn't seem to be any other way to force people to install the hook when comitting on Noisy.
#
installhooks:
	cp $(TREEROOT)/precommitStatisticsHook.sh $(TREEROOT)/.git/hooks/pre-commit
	cp $(TREEROOT)/postcommitStatisticsHook.sh $(TREEROOT)/.git/hooks/post-commit
	chmod +x $(TREEROOT)/.git/hooks/pre-commit
	chmod +x $(TREEROOT)/.git/hooks/post-commit


#
#			Libraries
#
lib$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N).a: $(LIBNOISYOBJS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) Makefile 
	$(AR) $(ARFLAGS) $@ $(LIBNOISYOBJS)


#
#			Executables
#
target: $(OBJS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) Makefile 
	$(LD) -L. -L$(LIBFLEXPATH) $(LDFLAGS) $(OBJS) -lflex-$(OSTYPE) -lm -o $(TARGET)


cgi:lib$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N).a $(CGIOBJS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) Makefile 
	$(LD) -L. -L$(LIBFLEXPATH) $(LDFLAGS) $(CGIOBJS) -lflex-$(OSTYPE) -o $(CGI_TARGET)


full: scan README.sloccount


#
#			Objects
#
%.$(OBJECTEXTENSION): %.c $(HEADERS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) Makefile 
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
	echo 'char kNoisyVersion[] = "0.1-alpha-'`git rev-list --count HEAD`' ('`git rev-parse HEAD`') (build '`date '+%m-%d-%Y-%H:%M'`-`whoami`@`hostname -s`-`uname -s`-`uname -r`-`uname -m`\)\"\; > version.c



scan:	clean
	scan-build --use-cc=$(CC) -k -V make -j4


README.sloccount: $(HEADERS) $(SOURCES)
	sloccount *.c *.h *.grammar *.ffi *.$(NOISYEXTENSION) */*.$(NOISYEXTENSION) > README.sloccount


installcgi: cgi
	sudo cp $(CGI_TARGET) $(CGI_BIN)
	
test:
	./noisy-darwin-EN --dot examples/helloWorld.n | dot -Tpdf -O ; open noname.gv.pdf 


clean:
	rm -rf version.c $(OBJS) $(CGIOBJS) $(LIBNOISYOBJS) $(TARGET) $(TARGET).dSYM $(CGI_TARGET) $(CGI_TARGET).dsym lib$(LIBNOISY)-$(OSTYPE)-$(NOISY_L10N).a *.o *.plist 
