### Makefile to generate example `.ll` files for the newton LLVM IR passes. ###

ifndef QUIET
	_QUIET:=@
endif

LLVM_CONFIG:=llvm-config
CC:=clang # Currently tested with clang-12/clang-13

BIN?=RandomAccelerationWalk-estSynth
COMMON_FLAGS=-Wall -Wextra

default: app.ll

app.ll: $(BIN).ll matrix.ll matrixadv.ll
	llvm-link -S -o $@ $<

%.ll : %.c
	@echo Compiling $*.c
	$(_QUIET)$(CC) -g -S -emit-llvm $(COMMON_FLAGS) -o $@ $<

clean::
	$(_QUIET)rm -f *.ll
