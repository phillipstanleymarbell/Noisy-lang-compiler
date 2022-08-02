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

change_main_api = sed -i 's/api/$(1)/g' main.c
change_back = sed -i 's/$(1)/api/g' main.c

sed_fn = sed 's/optnone //' ../$(1).ll > ../$(1)_non_opt.ll

opt_fn = opt ../$(1).ll --simplifycfg --instsimplify -S -o $(OUT_FILE)

newton_opt_fn = ./newton-linux-EN --llvm-ir=../../applications/newton/llvm-ir/$(1).ll --llvm-ir-liveness-check ../../applications/newton/sensors/test.nt

make_ll:
	cd $(SUBDIR) && $(MAKE_CLEAN) && $(MAKE)
	cd $(CHStone_DIR) && $(MAKE_CLEAN) && $(MAKE)

perf_data:
	llvm-as $(OUT_FILE) -o $(OUT_BC)
	llc $(OUT_BC) -o $(OUT_S)
	$(CC) -c $(OUT_S) -o $(OUT_OBJ)
	ar -rc $(OUT_LIB) $(OUT_OBJ)
	$(CC) main.c -no-pie -L. -lout -O2 -o main_out -lm
	cset shield --exec -- perf stat -B ./main_out if=/dev/zero of=/dev/null count=1000000

exp_non_opt:
	$(call sed_fn,e_exp)
	$(call opt_fn,e_exp_non_opt)

exp_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_exp)
	llvm-dis ../e_exp_output.bc
	$(call opt_fn,e_exp_output)

change_exp:
	$(call change_main_api,exp)

exp_change_back:
	$(call change_back,exp)

log_non_opt:
	$(call sed_fn,e_log)
	$(call opt_fn,e_log_non_opt)

log_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_log)
	llvm-dis ../e_log_output.bc
	$(call opt_fn,e_log_output)

change_log:
	$(call change_main_api,log)

log_change_back:
	$(call change_back,log)

acosh_non_opt:
	$(call sed_fn,e_acosh)
	$(call opt_fn,e_acosh_non_opt)

acosh_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_acosh)
	llvm-dis ../e_acosh_output.bc
	$(call opt_fn,e_acosh_output)

change_acosh:
	$(call change_main_api,acosh)

acosh_change_back:
	$(call change_back,acosh)

j0_non_opt:
	$(call sed_fn,e_j0)
	$(call opt_fn,e_j0_non_opt)

j0_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_j0)
	llvm-dis ../e_j0_output.bc
	$(call opt_fn,e_j0_output)

change_j0:
	$(call change_main_api,j0)

j0_change_back:
	$(call change_back,j0)

y0_non_opt:
	$(call sed_fn,e_y0)
	$(call opt_fn,e_y0_non_opt)

y0_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,e_y0)
	llvm-dis ../e_y0_output.bc
	$(call opt_fn,e_y0_output)

change_y0:
	$(call change_main_api,y0)

y0_change_back:
	$(call change_back,y0)

sincosf_non_opt:
	$(call sed_fn,sincosf)
	$(call opt_fn,sincosf_non_opt)

sincosf_opt:
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,sincosf)
	llvm-dis ../sincosf_output.bc
	$(call opt_fn,sincosf_output)

change_sincosf:
	$(call change_main_api,sincosf)

sincosf_change_back:
	$(call change_back,sincosf)

float64_add_non_opt:
	mv ../CHStone_test/float64_add.ll ../.
	$(call sed_fn,float64_add)
	$(call opt_fn,float64_add_non_opt)

float64_add_opt:
	mv ../CHStone_test/float64_add.ll ../.
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,float64_add)
	llvm-dis ../float64_add_output.bc
	$(call opt_fn,float64_add_output)

change_float64_add:
	$(call change_main_api,float64_add)

float64_add_change_back:
	$(call change_back,float64_add)

float64_div_non_opt:
	mv ../CHStone_test/float64_div.ll ../.
	$(call sed_fn,float64_div)
	$(call opt_fn,float64_div_non_opt)

float64_div_opt:
	mv ../CHStone_test/float64_div.ll ../.
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,float64_div)
	llvm-dis ../float64_div_output.bc
	$(call opt_fn,float64_div_output)

change_float64_div:
	$(call change_main_api,float64_div)

float64_div_change_back:
	$(call change_back,float64_div)

float64_mul_non_opt:
	mv ../CHStone_test/float64_mul.ll ../.
	$(call sed_fn,float64_mul)
	$(call opt_fn,float64_mul_non_opt)

float64_mul_opt:
	mv ../CHStone_test/float64_mul.ll ../.
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,float64_mul)
	llvm-dis ../float64_mul_output.bc
	$(call opt_fn,float64_mul_output)

change_float64_mul:
	$(call change_main_api,float64_mul)

float64_mul_change_back:
	$(call change_back,float64_mul)

sin_non_opt:
	mv ../CHStone_test/sin.ll ../.
	$(call sed_fn,sin)
	$(call opt_fn,sin_non_opt)

sin_opt:
	mv ../CHStone_test/sin.ll ../.
	cd $(NEWTON_BIN_DIR) && $(call newton_opt_fn,sin)
	llvm-dis ../sin_output.bc
	$(call opt_fn,sin_output)

change_sin:
	$(call change_main_api,sin)

sin_change_back:
	$(call change_back,sin)

perf_exp: clean make_ll exp_non_opt change_exp perf_data exp_change_back

perf_exp_opt: clean make_ll exp_opt change_exp perf_data exp_change_back

perf_log: clean make_ll log_non_opt change_log perf_data log_change_back

perf_log_opt: clean make_ll log_opt change_log perf_data log_change_back

perf_acosh: clean make_ll acosh_non_opt change_acosh perf_data acosh_change_back

perf_acosh_opt: clean make_ll acosh_opt change_acosh perf_data acosh_change_back

perf_j0: clean make_ll j0_non_opt change_j0 perf_data j0_change_back

perf_j0_opt: clean make_ll j0_opt change_j0 perf_data j0_change_back

perf_y0: clean make_ll y0_non_opt change_y0 perf_data y0_change_back

perf_y0_opt: clean make_ll y0_opt change_y0 perf_data y0_change_back

perf_sincosf: clean make_ll sincosf_non_opt change_sincosf perf_data sincosf_change_back

perf_sincosf_opt: clean make_ll sincosf_opt change_sincosf perf_data sincosf_change_back

perf_float64_add: clean make_ll float64_add_non_opt change_float64_add perf_data float64_add_change_back

perf_float64_add_opt: clean make_ll float64_add_opt change_float64_add perf_data float64_add_change_back

perf_float64_div: clean make_ll float64_div_non_opt change_float64_div perf_data float64_div_change_back

perf_float64_div_opt: clean make_ll float64_div_opt change_float64_div perf_data float64_div_change_back

perf_float64_mul: clean make_ll float64_mul_non_opt change_float64_mul perf_data float64_mul_change_back

perf_float64_mul_opt: clean make_ll float64_mul_opt change_float64_mul perf_data float64_mul_change_back

perf_sin: clean make_ll sin_non_opt change_sin perf_data sin_change_back

perf_sin_opt: clean make_ll sin_opt change_sin perf_data sin_change_back

all: perf_exp perf_exp_opt perf_log perf_log_opt perf_acosh perf_acosh_opt perf_j0 perf_j0_opt perf_y0 perf_y0_opt perf_sincosf perf_sincosf_opt perf_float64_add perf_float64_add_opt perf_float64_div perf_float64_div_opt perf_float64_mul perf_float64_mul_opt perf_sin perf_sin_opt

auto_test_compile:
	clang++ auto_test.cpp -g -o auto_test

auto_test:
	./auto_test

clean:
	$(QUIET)rm -f *.ll *.o *.s *.txt out.* libout.a main_out
