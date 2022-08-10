### Makefile to generate example `.ll` files for the newton LLVM IR passes. ###

ifndef VERBOSE
	QUIET:=@
endif

LLVM_CONFIG:=llvm-config
CC:=clang++ # Currently tested with clang-12/clang-13

vpath %.cpp adpcm dfadd dfdiv dfmul dfsin gsm motion

BIN?=application
COMMON_FLAGS=-Wall -Wextra

all: default

default: adpcm_main.ll float64_add.ll float64_div.ll float64_mul.ll float64_sin.ll Gsm_LPC_Analysis.ll motion_vectors.ll

%.ll : %.cpp
	@echo Compiling $*.c
	$(QUIET)$(CC) -g -S -emit-llvm $(COMMON_FLAGS) -o $@ $<

clean::
	$(QUIET)rm -f *.ll *.bc
