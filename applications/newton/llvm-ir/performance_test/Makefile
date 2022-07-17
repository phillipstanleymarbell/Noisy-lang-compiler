### Makefile to run the optimization and get the performance data automatically ###

ifndef VERBOSE
	QUIET:=@
endif

OPT = opt # Currently tested with opt-12
CC = clang
LLVM-DIS = llvm-dis
LLVM-AS = llvm-as
LLC = llc
AR = ar
MAKE = make
MAKE_CLEAN = make clean

SUBDIR = ../
NEWTON_BIN_DIR = ../../../../src/newton

OUTPUT_BC = $(AIM_FILE)_output.bc
OUTPUT_FILE = $(AIM_FILE)_output.ll
NON_OPT_FILE = $(AIM_FILE)_non_opt.ll
OUT_FILE = out.ll
OUT_BC = out.bc
OUT_S = out.s
OUT_OBJ = out.o
OUT_LIB = libout.a

default:
	cd $(SUBDIR) && $(MAKE_CLEAN) && $(MAKE)

newton_opt = ./newton-linux-EN --llvm-ir=../../applications/newton/llvm-ir/$(AIM_FILE).ll --llvm-ir-liveness-check ../../applications/newton/sensors/test.nt

optimized::
	cd $(NEWTON_BIN_DIR) && $(newton_opt)
	$(LLVM-DIS) ../$(OUTPUT_BC)
	$(OPT) ../$(OUTPUT_FILE) --simplifycfg --instsimplify -S -o $(OUT_FILE)

non_optimized::
	sed 's/optnone //' ../$(AIM_FILE).ll > ../$(NON_OPT_FILE)
	$(OPT) ../$(NON_OPT_FILE) --simplifycfg --instsimplify -S -o $(OUT_FILE)

all::
	$(LLVM-AS) $(OUT_FILE) -o $(OUT_BC)
	$(LLC) $(OUT_BC) -o $(OUT_S)
	$(CC) -c $(OUT_S) -o $(OUT_OBJ)
	$(AR) -rc $(OUT_LIB) $(OUT_OBJ)
	$(CC) main.c -no-pie -L. -lout -O2 -o main_out -lm
	perf stat -B ./main_out if=/dev/zero of=/dev/null count=1000000

clean::
	$(QUIET)rm -f *.ll *.o *.s *.txt out.* libout.a main_out


