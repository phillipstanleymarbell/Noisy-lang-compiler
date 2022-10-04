### Makefile to run the optimization and get the performance data automatically ###

ifndef VERBOSE
	QUIET:=@
endif

MAKE_CLEAN = make clean

SUBDIR = ../
CHStone_DIR = ../CHStone_test/
NEWTON_BIN_DIR = ../../../../src/newton

CC = clang
OUT_FILE = out.ll
OUT_BC = out.bc
OUT_S = out.s
OUT_OBJ = out.o
OUT_LIB = libout.a

ifdef CROSS_COMPILE
TARGET_FLAG = -target aarch64
TARGET_LLC_FLAG = --march=aarch64
endif

max_opt_fn = opt ../$(1).ll --O3 -S -o $(OUT_FILE)
non_opt_fn = cp ../$(1).ll $(OUT_FILE)
necessary_opt_fn = opt ../$(1).ll --simplifycfg --instsimplify -S -o $(OUT_FILE)

newton_opt_fn = ./newton-linux-EN --llvm-ir=../../applications/newton/llvm-ir/$(1).ll --llvm-ir-liveness-check ../../applications/newton/sensors/test.nt

compile_main_fn = $(CC) $(TARGET_FLAG) main.c -no-pie -L. -lout -D $(1) -O2 -g -o main_out -lm

make_ll:
	cd $(SUBDIR) && $(MAKE_CLEAN) && $(MAKE)
	cd $(CHStone_DIR) && $(MAKE_CLEAN) && $(MAKE)
	cp ../CHStone_test/*.ll ../.

compile_lib:
	llvm-as $(OUT_FILE) -o $(OUT_BC)
	llc $(TARGET_LLC_FLAG) $(OUT_BC) -o $(OUT_S)
	$(CC) $(TARGET_FLAG) -c $(OUT_S) -o $(OUT_OBJ)
	ar -rc $(OUT_LIB) $(OUT_OBJ)

exp_non_opt:
	$(call max_opt_fn,e_exp)

exp_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_exp)
	llvm-dis ../e_exp_output.bc
	$(call max_opt_fn,e_exp_output)

compile_exp:
	$(call compile_main_fn,LIBC_EXP)

log_non_opt:
	$(call max_opt_fn,e_log)

log_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_log)
	llvm-dis ../e_log_output.bc
	$(call max_opt_fn,e_log_output)

compile_log:
	$(call compile_main_fn,LIBC_LOG)

acosh_non_opt:
	$(call max_opt_fn,e_acosh)

acosh_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_acosh)
	llvm-dis ../e_acosh_output.bc
	$(call max_opt_fn,e_acosh_output)

compile_acosh:
	$(call compile_main_fn,LIBC_ACOSH)

j0_non_opt:
	$(call max_opt_fn,e_j0)

j0_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_j0)
	llvm-dis ../e_j0_output.bc
	$(call max_opt_fn,e_j0_output)

compile_j0:
	$(call compile_main_fn,LIBC_J0)

y0_non_opt:
	$(call max_opt_fn,e_y0)

y0_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_y0)
	llvm-dis ../e_y0_output.bc
	$(call max_opt_fn,e_y0_output)

compile_y0:
	$(call compile_main_fn,LIBC_Y0)

rem_pio2_non_opt:
	$(call max_opt_fn,e_rem_pio2)

rem_pio2_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_rem_pio2)
	llvm-dis ../e_rem_pio2_output.bc
	$(call max_opt_fn,e_rem_pio2_output)

compile_rem_pio2:
	$(call compile_main_fn,LIBC_REM_PIO2)

sincosf_non_opt:
	$(call max_opt_fn,sincosf)

sincosf_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,sincosf)
	llvm-dis ../sincosf_output.bc
	$(call max_opt_fn,sincosf_output)

compile_sincosf:
	$(call compile_main_fn,LIBC_SINCOSF)

float64_add_non_opt:
	$(call max_opt_fn,float64_add)

float64_add_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,float64_add)
	llvm-dis ../float64_add_output.bc
	$(call max_opt_fn,float64_add_output)

compile_float64_add:
	$(call compile_main_fn,FLOAT64_ADD)

float64_div_non_opt:
	$(call max_opt_fn,float64_div)

float64_div_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,float64_div)
	llvm-dis ../float64_div_output.bc
	$(call max_opt_fn,float64_div_output)

compile_float64_div:
	$(call compile_main_fn,FLOAT64_DIV)

float64_mul_non_opt:
	$(call max_opt_fn,float64_mul)

float64_mul_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,float64_mul)
	llvm-dis ../float64_mul_output.bc
	$(call max_opt_fn,float64_mul_output)

compile_float64_mul:
	$(call compile_main_fn,FLOAT64_MUL)

float64_sin_non_opt:
	$(call max_opt_fn,float64_sin)

float64_sin_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,float64_sin)
	llvm-dis ../float64_sin_output.bc
	$(call max_opt_fn,float64_sin_output)

compile_float64_sin:
	$(call compile_main_fn,FLOAT64_SIN)

benchmark_suite_non_opt:
	$(call max_opt_fn,benchmark_suite)

benchmark_suite_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,benchmark_suite)
	llvm-dis ../benchmark_suite_output.bc
	$(call max_opt_fn,benchmark_suite_output)

compile_benchmark_suite_int:
	$(call compile_main_fn,BENCHMARK_SUITE_INT)

compile_benchmark_suite_double:
	$(call compile_main_fn,BENCHMARK_SUITE_DOUBLE)

compile_benchmark_suite_float:
	$(call compile_main_fn,BENCHMARK_SUITE_FLOAT)

perf_exp: clean make_ll exp_non_opt compile_lib compile_exp

perf_exp_opt: clean make_ll exp_opt compile_lib compile_exp

perf_log: clean make_ll log_non_opt compile_lib compile_log

perf_log_opt: clean make_ll log_opt compile_lib compile_log 

perf_acosh: clean make_ll acosh_non_opt compile_lib compile_acosh 

perf_acosh_opt: clean make_ll acosh_opt compile_lib compile_acosh 

perf_j0: clean make_ll j0_non_opt compile_lib compile_j0 

perf_j0_opt: clean make_ll j0_opt compile_lib compile_j0 

perf_y0: clean make_ll y0_non_opt compile_lib compile_y0 

perf_y0_opt: clean make_ll y0_opt compile_lib compile_y0

perf_rem_pio2: clean make_ll rem_pio2_non_opt compile_lib compile_rem_pio2

perf_rem_pio2_opt: clean make_ll rem_pio2_opt compile_lib compile_rem_pio2

perf_sincosf: clean make_ll sincosf_non_opt compile_lib compile_sincosf

perf_sincosf_opt: clean make_ll sincosf_opt compile_lib compile_sincosf

perf_float64_add: clean make_ll float64_add_non_opt compile_lib compile_float64_add

perf_float64_add_opt: clean make_ll float64_add_opt compile_lib compile_float64_add

perf_float64_div: clean make_ll float64_div_non_opt compile_lib compile_float64_div

perf_float64_div_opt: clean make_ll float64_div_opt compile_lib compile_float64_div

perf_float64_mul: clean make_ll float64_mul_non_opt compile_lib compile_float64_mul

perf_float64_mul_opt: clean make_ll float64_mul_opt compile_lib compile_float64_mul

perf_float64_sin: clean make_ll float64_sin_non_opt compile_lib compile_float64_sin

perf_float64_sin_opt: clean make_ll float64_sin_opt compile_lib compile_float64_sin

perf_benchmark_suite_int: clean make_ll benchmark_suite_non_opt compile_lib compile_benchmark_suite_int

perf_benchmark_suite_int_opt: clean make_ll benchmark_suite_opt compile_lib compile_benchmark_suite_int

perf_benchmark_suite_double: clean make_ll benchmark_suite_non_opt compile_lib compile_benchmark_suite_double

perf_benchmark_suite_double_opt: clean make_ll benchmark_suite_opt compile_lib compile_benchmark_suite_double

perf_benchmark_suite_float: clean make_ll benchmark_suite_non_opt compile_lib compile_benchmark_suite_float

perf_benchmark_suite_float_opt: clean make_ll benchmark_suite_opt compile_lib compile_benchmark_suite_float

auto_test_compile:
	clang++ auto_test.cpp -g -o auto_test

all: default

default: perf_exp perf_exp_opt perf_log perf_log_opt perf_acosh perf_acosh_opt perf_j0 perf_j0_opt perf_y0 perf_y0_opt perf_rem_pio2 perf_rem_pio2_opt perf_sincosf perf_sincosf_opt perf_float64_add perf_float64_add_opt perf_float64_div perf_float64_div_opt perf_float64_mul perf_float64_mul_opt perf_float64_sin perf_float64_sin_opt auto_test_compile

clean:
	$(QUIET)rm -f *.ll *.o *.s *.txt out.* libout.a main_out auto_test
