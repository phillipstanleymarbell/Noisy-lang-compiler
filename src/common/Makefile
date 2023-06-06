TREEROOT	= ../..

include		$(TREEROOT)/config.local

PRECOMMITHOOK	= precommitStatisticsHook-$(OSTYPE).sh 
COMMONPATH	= ../common
COMPILERVARIANT = gcc	#clang

LLVM_CONFIG?=llvm-config

include		$(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT)
include		config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT)

PLATFORM_CFLAGS	+= -Wno-gnu-designator
MAKEFLAGS	+= #-j

CCFLAGS		= $(PLATFORM_DBGFLAGS) $(PLATFORM_CFLAGS) $(PLATFORM_DFLAGS) -DVariant$(SYSNAME) $(PLATFORM_OPTFLAGS)
LDFLAGS 	= $(PLATFORM_DBGFLAGS) -lm $(PLATFORM_LFLAGS) `pkg-config --libs 'libprotobuf-c >= 1.0.0'`

LDFLAGS+=$(shell $(LLVM_CONFIG) --ldflags)
CCFLAGS+=$(shell $(LLVM_CONFIG) --cflags)

LIBCOMMON	= Common
COMMON_L10N	= EN

LINKDIRS	= -L. -L$(LIBFLEXPATH) 
TARGET		= common-$(OSTYPE)-$(COMMON_L10N)

#	-std=gnu99 because we use anonymous unions and induction variable defintions in loop head.
CCFLAGS		+= -c -std=gnu99 -DkCommonL10N="\"$(COMMON_L10N)\"" -DCOMMON_L10N_EN

WFLAGS		= -Wall -Werror
INCDIRS		= -I../noisy -I../newton -I. -I$(LIBFLEXPATH) 
PROTOC		= protoc-c

SOURCES		=\
		common-utils.c\
		primenumbers.c\
		common-symbolTable.c\
		common-lexers-helpers.c\
		common-firstAndFollow.c\
		common-irPass-helpers.c\
		common-errors.c\
		common-timeStamps.c\
		common-irHelpers.c\
		common-astTransform.c\


#
#	Clang seems to be unable to do LTO unless we have all the objects
#	_not_ tucked into a library, so we don't just simply link main against
#	-l$(LIBCOMMON)-$(OSTYPE)-$(COMMON_L10N)
#
OBJS		=\
		primenumbers.$(OBJECTEXTENSION)\
		common-utils.$(OBJECTEXTENSION)\
		common-firstAndFollow.$(OBJECTEXTENSION)\
		common-symbolTable.$(OBJECTEXTENSION)\
		common-lexers-helpers.$(OBJECTEXTENSION)\
		common-errors$(COMMON_L10N).$(OBJECTEXTENSION)\
		common-timeStamps.$(OBJECTEXTENSION)\
		common-irHelpers.$(OBJECTEXTENSION)\
		common-irPass-helpers.$(OBJECTEXTENSION)\
		common-astTransform.$(OBJECTEXTENSION)\


LIBCOMMONOBJS =\
		primenumbers.$(OBJECTEXTENSION)\
		common-utils.$(OBJECTEXTENSION)\
		common-firstAndFollow.$(OBJECTEXTENSION)\
		common-symbolTable.$(OBJECTEXTENSION)\
		common-lexers-helpers.$(OBJECTEXTENSION)\
		common-errors$(COMMON_L10N).$(OBJECTEXTENSION)\
		common-timeStamps.$(OBJECTEXTENSION)\
		common-irHelpers.$(OBJECTEXTENSION)\
		common-irPass-helpers.$(OBJECTEXTENSION)\
		common-astTransform.$(OBJECTEXTENSION)\


HEADERS		=\
		$(LIBFLEXPATH)/flex.h\
		common-symbolTable.h\
		common-lexers-helpers.h\
		common-firstAndFollow.h\
		common-errors.h\
		common-timeStamps.h\
		common-data-structures.h\
		common-irHelpers.h\
		common-irPass-helpers.h\
		common-astTransform.h\



all: lib$(LIBCOMMON)-$(OSTYPE)-$(COMMON_L10N).a $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) Makefile

#
#			Libraries
#
lib$(LIBCOMMON)-$(OSTYPE)-$(COMMON_L10N).a: $(LIBCOMMONOBJS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) Makefile
	$(AR) $(ARFLAGS) $@ $(LIBCOMMONOBJS)


#
#			Objects
#
%.$(OBJECTEXTENSION): %.c #$(HEADERS) $(CONFIGPATH)/config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) config.$(OSTYPE)-$(MACHTYPE).$(COMPILERVARIANT) Makefile
	$(CC) $(FLEXFLAGS) $(INCDIRS) $(CCFLAGS) $(WFLAGS) $(OPTFLAGS) -c $(LINTFLAGS) $<
	$(CC) $(FLEXFLAGS) $(INCDIRS) $(CCFLAGS) $(WFLAGS) $(OPTFLAGS) -c $<


clean:
	rm -rf $(OBJS) $(LIBCOMMONOBJS) lib$(LIBCOMMON)-$(OSTYPE)-$(COMMON_L10N).a *.o *.plist
