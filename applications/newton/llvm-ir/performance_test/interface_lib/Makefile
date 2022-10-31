### Makefile to compile all *_output.ll into one libInterface.a ###

ifndef VERBOSE
	QUIET:=@
endif

LLVM_CONFIG:=llvm-config
CC:=clang # Currently tested with clang-12/clang-13
SUB_DIR = ../
CURRENT_DIR = $(shell pwd)

vpath %.c %.h

BIN?=application
COMMON_FLAGS=-Wall -Wextra

PLUGSRC = $(CURRENT_DIR)/*.ll
SRCS = $(wildcard $(PLUGSRC))
OBJ = $(SRCS:.c=.o)
SOBJ = $(SRCS:.c=.a)

all: default

default: make_ll libInterface.a

make_ll:
	cd $(SUB_DIR) && make interface_lib
	cp $(SUB_DIR)../*_output.ll $(CURRENT_DIR)

#%.bc : e_acosh_output.ll e_exp_output.ll e_j0_output.ll e_log_output.ll e_rem_pio2_output.ll e_y0_output.ll float64_add_output.ll float64_div_output.ll float64_mul_output.ll float64_sin_output.ll
#	@echo llvm-as -o $@ $<
#
#%.s : %.bc
#	@echo llc -o $@ $<
#
#%.o : %.s
#	@echo $(CC) -o $@ -c $<

#%.a : %.o
#	@echo ar -rc $@ $<

soft_float_api.o: soft_float_api_output.ll
	$(CC) -o $@ -c $^ -no-pie -fPIC

libInterface.a: soft_float_api.o
	#$(CC) -shared -o $@ -ldl $^
	ar -rc $@ $^
	cp $@ ../../fusion/. 
	cp $@ ../../pedometer/.

clean::
	$(QUIET)rm -f *.ll *.bc *.a *.out *.o
