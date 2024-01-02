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

ifdef SOFT_FLOAT
CC_FP_FLAG = -msoft-float
OPT_FP_FLAG = --float-abi=soft
endif

all: default

default: application.ll simple_control_flow.ll inferBound.ll inferBoundControlFlow.ll e_exp.ll sincosf.ll e_log.ll e_acosh.ll e_j0.ll e_y0.ll e_rem_pio2.ll benchmark_suite.ll phi_two_global_arrays.ll func_call.ll test_shift.ll vec_add.ll vec_add_8.ll MadgwickAHRSfix.ll MadgwickAHRS_softfloat.ll MadgwickAHRS.ll arm_sqrt_q15.ll

MadgwickAHRS_softfloat.ll : MadgwickAHRS_softfloat.c
	@echo Compiling $*.c
	$(CC) $(TARGET_FLAG) -g -O0 -Xclang -disable-O0-optnone -S -emit-llvm $(COMMON_FLAGS) -o tmp.ll $<
	llvm-link soft_float_api.ll tmp.ll -o $@
	opt $@ --mem2reg --instsimplify -S -o $@

%.ll : %.c
	@echo Compiling $*.c
	$(CC) $(TARGET_FLAG) $(CC_FP_FLAG) -g -O0 -Xclang -disable-O0-optnone -S -emit-llvm $(COMMON_FLAGS) -o $@ $<
	opt $@ $(OPT_FP_FLAG) --mem2reg --instsimplify -S -o $@

clean::
	$(QUIET)rm -f *.ll *.bc
