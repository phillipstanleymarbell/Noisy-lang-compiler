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

main.out: main.ll FusionAhrs.ll FusionCompass.ll FusionOffset.ll
	$(CC) -g -lm -o $@ $^

fusion.ll: main.bc FusionAhrs.bc FusionCompass.bc FusionOffset.bc
	llvm-link -o fusion.bc $^
	llvm-dis -o $@ fusion.bc

%.ll : %.c
	@echo Compiling $*.c
	$(QUIET)$(CC) $(TARGET_FLAG) -g -O0 -Xclang -disable-O0-optnone -S -emit-llvm $(COMMON_FLAGS) -o $@ $<
	opt $@ --mem2reg --instsimplify -S -o $@

%.bc : %.ll
	llvm-as -o $@ $<

fusion_opt.ll: fusion_output.bc
	llvm-dis -o $@ $<

clean::
	$(QUIET)rm -f *.ll *.bc
