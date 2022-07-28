ifdef QUIET
	_QUIET:=@
endif

LLVM_CONFIG:=llvm-config
CC:=clang # Currently tested with clang-12/clang-13
LLC:=llc
LLVM-DIS:= llvm-dis

COMMON_FLAGS=-Wall -Wextra

default: main.ll

main : main.o runtime_dimensionality_check.o
	$(_QUIET)$(CC) $^ -o $@

main.o : main.bc
	$(_QUIET)$(LLC) -filetype=obj $< -o $@

main_opt.ll : main.bc
	$(_QUIET)$(LLVM-DIS) $< -o $@

%.ll : %.c
	$(_QUIET)$(CC) -g -S -emit-llvm $(COMMON_FLAGS) -o $@ $<

runtime_dimensionality_check.o : runtime_dimensionality_check.c
	$(_QUIET)$(CC) -o $@ -c $<

clean::
	$(_QUIET)rm -f *.ll *.o *.bc main
