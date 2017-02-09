LIBFLEXPATH=/Users/jonathanlim/Documents/Compiler/libflex
CONFIGPATH=/Users/jonathanlim/Documents/Compiler/libflex
TREEROOT=..
MACHTYPE=x86_64
COMPILERVARIANT=clang
OSTYPE=darwin
include		$(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT)
include		config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT)

MAKEFLAGS	+= -j

CCFLAGS		= $(PLATFORM_DBGFLAGS) $(PLATFORM_CFLAGS) $(PLATFORM_DFLAGS) $(PLATFORM_OPTFLAGS)
LDFLAGS 	= $(PLATFORM_DBGFLAGS) -lm $(PLATFORM_LFLAGS) `pkg-config --libs 'libprotobuf-c >= 1.0.0'`

LIBCOMMON = Common
COMMON_L10N	= EN

LINKDIRS    = -L. -L$(LIBFLEXPATH) 
TARGET		= common-$(OSTYPE)-$(COMMON_L10N)

#	-std=gnu99 because we use anonymous unions and induction variable defintions in loop head.
CCFLAGS		+= -c -std=gnu99 -DkCommonL10N="\"$(COMMON_L10N)\"" -DCOMMON_L10N_EN

WFLAGS		= -Wall -Werror
INCDIRS		= -I. -I$(LIBFLEXPATH) 
PROTOC		= protoc-c

SOURCES		=\
		common-utils.c\
		primenumbers.c\
		common-lexers-helpers.c\
		common-firstAndFollow.c\
		common-errors.c\
		common-timeStamps.c\
		common-irHelpers.c\


#
#	Clang seems to be unable to do LTO unless we have all the objects
#	not tucked into a library, so we don't just simply link maain against
#	-l$(LIBCOMMON)-$(OSTYPE)-$(COMMON_L10N)
#
OBJS		=\
		primenumbers.$(OBJECTEXTENSION)\
		common-utils.$(OBJECTEXTENSION)\
		common-firstAndFollow.$(OBJECTEXTENSION)\
		common-lexers-helpers.$(OBJECTEXTENSION)\
		common-errors$(COMMON_L10N).$(OBJECTEXTENSION)\
		common-timeStamps.$(OBJECTEXTENSION)\
		common-irHelpers.$(OBJECTEXTENSION)\


LIBCOMMONOBJS =\
		common-utils.$(OBJECTEXTENSION)\
		primenumbers.$(OBJECTEXTENSION)\
		common-firstAndFollow.$(OBJECTEXTENSION)\
		common-lexers-helpers.$(OBJECTEXTENSION)\
		common-errors$(COMMON_L10N).$(OBJECTEXTENSION)\
		common-timeStamps.$(OBJECTEXTENSION)\
		common-irHelpers.$(OBJECTEXTENSION)\


HEADERS		=\
		$(LIBFLEXPATH)/flex.h\
		common-lexers-helpers.h\
		common-firstAndFollow.h\
		common-errors.h\
		common-timeStamps.h\
		data-structures.h\
		common-irHelpers.h\



all: installhooks lib$(LIBCOMMON)-$(OSTYPE)-$(COMMON_L10N).a $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) Makefile

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
lib$(LIBCOMMON)-$(OSTYPE)-$(COMMON_L10N).a: $(LIBCOMMONOBJS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) Makefile 
	$(AR) $(ARFLAGS) $@ $(LIBCOMMONOBJS)


#
#			Objects
#
%.$(OBJECTEXTENSION): %.c $(HEADERS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) Makefile 
	$(CC) $(FLEXFLAGS) $(INCDIRS) $(CCFLAGS) $(WFLAGS) $(OPTFLAGS) -c $(LINTFLAGS) $<
	$(CC) $(FLEXFLAGS) $(INCDIRS) $(CCFLAGS) $(WFLAGS) $(OPTFLAGS) -c $<


clean:
	rm -rf $(OBJS) $(LIBCOMMONOBJS) lib$(LIBCOMMON)-$(OSTYPE)-$(COMMON_L10N).a *.o *.plist 
