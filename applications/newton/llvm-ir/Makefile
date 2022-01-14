### Makefile to generate example `.ll` files for the newton LLVM IR passes. ###

ifndef VERBOSE
	QUIET:=@
endif

LLVM_CONFIG:=llvm-config
CC:=clang-12

vpath %.c c-files

BIN?=application
COMMON_FLAGS=-Wall -Wextra

default: $(BIN).ll

%.ll : %.c
	@echo Compiling $*.c
	$(QUIET)$(CC) -g -S -emit-llvm $(COMMON_FLAGS) -o $@ $<

clean::
	$(QUIET)rm -f *.ll
