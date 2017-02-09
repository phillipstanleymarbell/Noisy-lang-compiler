GTEST_DIR = /Users/jonathanlim/Documents/Compiler/googletest/googletest

NEWTONPATH=../newton
INCLUDEPATH=../include

USER_DIR = ./
INCDIRS		= -I. -I$(NEWTONPATH)
LINKDIRS    = -L. -L$(NEWTONPATH) -lNewton-darwin-EN

# Flags passed to the preprocessor.
# Set Google Test's header directory as a system directory, such that
# the compiler doesn't generate warnings in Google Test headers.
CPPFLAGS += -isystem $(GTEST_DIR)/include

# Flags passed to the C++ compiler.
CXXFLAGS += -g -Wall -Wextra
OBJECTEXTENSION=o

# All tests produced by this Makefile.  Remember to add new tests you
# created to the list.
TESTS = all_tests

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

# House-keeping build targets.

all : $(TESTS)

# Builds gtest.a and gtest_main.a.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

SOURCES		=\
		test-newton-api.cc\

OBJS		=\
		test-newton-api.$(OBJECTEXTENSION)\

HEADERS		=\
		test-newton-api.h           \
		$(INCLUDEPATH)/newton-api.h \

#
#			Objects
#
%.$(OBJECTEXTENSION): %.cc $(HEADERS) $(GTEST_HEADERS)
	$(CXX) $(INCDIRS) $(CPPFLAGS) $(CXXFLAGS) -c $<

# Builds a sample test.  A test should link with either gtest.a or
# gtest_main.a, depending on whether it defines its own main()
# function.

all_tests : $(OBJS) gtest_main.a
	$(CXX) $(LINKDIRS) $(CPPFLAGS) $(CXXFLAGS) -lpthread $^ -o $@

clean:
	rm -rf $(OBJS) $(TESTS) gtest.a gtest_main.a *.o *.plist 
