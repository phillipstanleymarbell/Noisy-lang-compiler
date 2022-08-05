### Makefile to run the optimization and get the performance data automatically ###

ifndef VERBOSE
	QUIET:=@
endif

MAKE_CLEAN = make clean

SUBDIR = ../
NEWTON_BIN_DIR = ../../../../src/newton

CC = clang
OUT_FILE = out.ll
OUT_BC = out.bc
OUT_S = out.s
OUT_OBJ = out.o
OUT_LIB = libout.a

sed_fn = sed 's/optnone //' ../$(1).ll > ../$(1)_non_opt.ll

opt_fn = opt ../$(1).ll --simplifycfg --instsimplify -S -o $(OUT_FILE)

newton_opt_fn = ./newton-linux-EN --llvm-ir=../../applications/newton/llvm-ir/$(1).ll --llvm-ir-liveness-check ../../applications/newton/sensors/test.nt

compile_main_fn = $(CC) main.c -no-pie -L. -lout -D $(1) -O2 -o main_out -lm

make_ll:
	cd $(SUBDIR) && $(MAKE_CLEAN) && $(MAKE)

compile_lib:
	llvm-as $(OUT_FILE) -o $(OUT_BC)
	llc $(OUT_BC) -o $(OUT_S)
	$(CC) -c $(OUT_S) -o $(OUT_OBJ)
	ar -rc $(OUT_LIB) $(OUT_OBJ)

perf_data:
	perf stat -B ./main_out if=/dev/zero of=/dev/null count=1000000

exp_non_opt:
	$(call sed_fn,e_exp)
	$(call opt_fn,e_exp_non_opt)

exp_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_exp)
	llvm-dis ../e_exp_output.bc
	$(call opt_fn,e_exp_output)

compile_exp:
	$(call compile_main_fn,LIBC_EXP)

log_non_opt:
	$(call sed_fn,e_log)
	$(call opt_fn,e_log_non_opt)

log_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_log)
	llvm-dis ../e_log_output.bc
	$(call opt_fn,e_log_output)

compile_log:
	$(call compile_main_fn,LIBC_LOG)

acosh_non_opt:
	$(call sed_fn,e_acosh)
	$(call opt_fn,e_acosh_non_opt)

acosh_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_acosh)
	llvm-dis ../e_acosh_output.bc
	$(call opt_fn,e_acosh_output)

compile_acosh:
	$(call compile_main_fn,LIBC_ACOSH)

j0_non_opt:
	$(call sed_fn,e_j0)
	$(call opt_fn,e_j0_non_opt)

j0_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_j0)
	llvm-dis ../e_j0_output.bc
	$(call opt_fn,e_j0_output)

compile_j0:
	$(call compile_main_fn,LIBC_J0)

y0_non_opt:
	$(call sed_fn,e_y0)
	$(call opt_fn,e_y0_non_opt)

y0_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_y0)
	llvm-dis ../e_y0_output.bc
	$(call opt_fn,e_y0_output)

compile_y0:
	$(call compile_main_fn,LIBC_Y0)

perf_exp: clean make_ll exp_non_opt compile_lib compile_exp perf_data

perf_exp_opt: clean make_ll exp_opt compile_lib compile_exp perf_data 

perf_log: clean make_ll log_non_opt compile_lib compile_log perf_data 

perf_log_opt: clean make_ll log_opt compile_lib compile_log perf_data 

perf_acosh: clean make_ll acosh_non_opt compile_lib compile_acosh perf_data 

perf_acosh_opt: clean make_ll acosh_opt compile_lib compile_acosh perf_data 

perf_j0: clean make_ll j0_non_opt compile_lib compile_j0 perf_data 

perf_j0_opt: clean make_ll j0_opt compile_lib compile_j0 perf_data 

perf_y0: clean make_ll y0_non_opt compile_lib compile_y0 perf_data 

perf_y0_opt: clean make_ll y0_opt compile_lib compile_y0 perf_data 

all: perf_exp perf_exp_opt perf_log perf_log_opt perf_acosh perf_acosh_opt perf_j0 perf_j0_opt perf_y0 perf_y0_opt

clean:
	$(QUIET)rm -f *.ll *.o *.s *.txt out.* libout.a main_out
