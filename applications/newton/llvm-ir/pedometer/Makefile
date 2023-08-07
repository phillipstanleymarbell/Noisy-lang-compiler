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
newton_opt_fn = ./newton-linux-EN --llvm-ir=../../applications/newton/llvm-ir/pedometer/perf_main.ll --llvm-ir-liveness-check --llvm-ir-enable-overload --llvm-ir-enable-builtin-assume ../../applications/newton/sensors/BMX055.nt

all: default

default: perf_main.out newton_compile perf_main_opt.out

run: perf_main.out
	./perf_main.out inputs/Acceleration_with_g_2020-09-25_19-09-09_ROI.csv

perf_main.out: perf_main.ll
	$(CC) -lm -O2 -g -o $@ $^

perf_main_opt.out: perf_main_opt.ll
	$(CC) -lm -O2 -g -o $@ $^

perf_main.ll : main.c
	@echo Compiling $*.c
	$(QUIET)$(CC) $(TARGET_FLAG) -g -O0 -Xclang -disable-O0-optnone -S -emit-llvm $(COMMON_FLAGS) -o $@ $<
	opt $@ --mem2reg --instsimplify -S -o $@

%.bc : %.ll
	llvm-as -o $@ $<

newton_compile:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn)

perf_main_opt.ll: perf_main_output.bc
	llvm-dis -o $@ $<

perf_main:
	./perf_main.out inputs/Acceleration_with_g_2020-09-25_19-09-09_ROI.csv if=/dev/zero of=/dev/null count=1000000

perf_main_opt:
	./perf_main_opt.out inputs/Acceleration_with_g_2020-09-25_19-09-09_ROI.csv if=/dev/zero of=/dev/null count=1000000

auto_test_compile:
	clang++ auto_test.cpp -g -o auto_test

clean::
	$(QUIET)rm -f *.ll *.bc *.out *.a
