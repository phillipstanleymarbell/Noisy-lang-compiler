### Makefile to generate example `.ll` files for the newton LLVM IR passes. ###

ifndef VERBOSE
	QUIET:=@
endif

LLVM_CONFIG:=llvm-config
CC:=clang # Currently tested with clang-12/clang-13

vpath %.c %.h

BIN?=application
COMMON_FLAGS=-Wall -Wextra

ifdef CROSS_COMPILE
TARGET_FLAG=-target aarch64
endif

all: default

default: main.out

run: main.out
	./main.out inputs/Acceleration_with_g_2020-09-25_19-09-09_ROI.csv

main.out: main.ll
	$(CC) -lm -O2 -g -o $@ $^

%.ll : %.c
	@echo Compiling $*.c
	$(QUIET)$(CC) $(TARGET_FLAG) -g -O0 -Xclang -disable-O0-optnone -S -emit-llvm $(COMMON_FLAGS) -o $@ $<
	opt $@ --mem2reg --instsimplify -S -o $@

%.bc : %.ll
	llvm-as -o $@ $<

clean::
	$(QUIET)rm -f *.ll *.bc *.out
