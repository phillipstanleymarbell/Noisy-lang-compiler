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

NEWTON_BIN_DIR = ../../../../src/newton
newton_opt_fn = ./newton-linux-EN --llvm-ir=../../applications/newton/llvm-ir/fusion/fusion.ll --llvm-ir-liveness-check --llvm-ir-enable-overload --llvm-ir-enable-builtin-assume ../../applications/newton/sensors/BMX055.nt

all: default

default: main.out newton_compile main_opt.out

main.out: fusion.ll
	$(CC) -O2 -g -o $@ $^ -lm

main_opt.out: fusion_opt.ll
	$(CC) -O2 -g -o $@ $^ -lm

perf_main: main.out
	perf stat -B ./main.out if=/dev/zero of=/dev/null count=1000000

perf_main_opt: main_opt.out
	perf stat -B ./main_opt.out if=/dev/zero of=/dev/null count=1000000

fusion.ll: main.bc FusionAhrs.bc FusionCompass.bc FusionOffset.bc soft_float_api.bc
	llvm-link -o fusion.bc $^
	llvm-dis -o $@ fusion.bc

%.ll : %.c
	@echo Compiling $*.c
	$(QUIET)$(CC) $(TARGET_FLAG) -g -O0 -Xclang -disable-O0-optnone -S -emit-llvm $(COMMON_FLAGS) -o $@ $<
	opt $@ --mem2reg --instsimplify -S -o $@

%.bc : %.ll
	llvm-as -o $@ $<

newton_compile:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn)

fusion_opt.ll: fusion_output.bc
	llvm-dis -o $@ $<

clean::
	$(QUIET)rm -f *.ll *.bc *.out *.a
