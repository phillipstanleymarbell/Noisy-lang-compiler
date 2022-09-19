### Makefile to generate example `.ll` files for the newton LLVM IR passes. ###

ifndef VERBOSE
	QUIET:=@
endif

LLVM_CONFIG:=llvm-config
CC:=clang # Currently tested with clang-12/clang-13

vpath %.c c-files

BIN?=application
COMMON_FLAGS=-Wall -Wextra

ifdef CROSS_COMPILE
TARGET_FLAG=-target aarch64
endif

all: default

default: application.ll simple_control_flow.ll inferBound.ll inferBoundControlFlow.ll e_exp.ll sincosf.ll e_log.ll e_acosh.ll e_j0.ll e_y0.ll e_rem_pio2.ll benchmark_suite.ll

%.ll : %.c
	@echo Compiling $*.c
	$(QUIET)$(CC) $(TARGET_FLAG) -g -O0 -Xclang -disable-O0-optnone -S -emit-llvm $(COMMON_FLAGS) -o $@ $<
	opt $@ --mem2reg --instsimplify -S -o $@

clean::
	$(QUIET)rm -f *.ll *.bc
