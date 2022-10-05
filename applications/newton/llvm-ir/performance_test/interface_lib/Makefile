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

default: make_ll get_o libInterface.a

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

get_o: e_acosh_output.ll e_exp_output.ll e_j0_output.ll e_log_output.ll e_rem_pio2_output.ll e_y0_output.ll float64_add_output.ll float64_div_output.ll float64_mul_output.ll float64_sin_output.ll
	$(CC) -c $^ -no-pie -fPIC

libInterface.a: e_acosh_output.o e_exp_output.o e_j0_output.o e_log_output.o e_rem_pio2_output.o e_y0_output.o float64_add_output.o float64_div_output.o float64_mul_output.o float64_sin_output.o
	ar -rc $@ $^

clean::
	$(QUIET)rm -f *.ll *.bc *.a *.out *.o