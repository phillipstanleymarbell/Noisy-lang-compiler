### Makefile to generate example `.ll` files for the newton LLVM IR passes. ###

ifndef VERBOSE
	QUIET:=@
endif

LLVM_CONFIG:=llvm-config
CC:=clang # Currently tested with clang-12/clang-13

vpath %.c c-files

BIN?=application
COMMON_FLAGS=-Wall -Wextra

all: default

default: application.ll simple_control_flow.ll inferBound.ll inferBoundControlFlow.ll e_exp.ll sincosf.ll e_log.ll e_acosh.ll e_j0.ll e_y0.ll

%.ll : %.c
	@echo Compiling $*.c
	$(QUIET)$(CC) -g -S -emit-llvm $(COMMON_FLAGS) -o $@ $<

clean::
	$(QUIET)rm -f *.ll *.bc
