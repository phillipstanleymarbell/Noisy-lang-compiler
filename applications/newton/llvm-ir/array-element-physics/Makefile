ifdef QUIET
	_QUIET:=@
endif

LLVM_CONFIG:=llvm-config
CC:=clang # Currently tested with clang-12/clang-13
LLC:=llc

COMMON_FLAGS=-Wall -Wextra

default: main.ll

main : main.o runtime_dimensionality_check.o
	$(_QUIET)$(CC) $^ -o $@

main.o : out.bc
	$(_QUIET)$(LLC) -filetype=obj out.bc -o $@

%.ll : %.c
	$(_QUIET)$(CC) -g -S -emit-llvm $(COMMON_FLAGS) -o $@ $<

runtime_dimensionality_check.o : runtime_dimensionality_check.c
	$(_QUIET)$(CC) -o $@ -c $<

clean::
	$(_QUIET)rm -f *.ll *.o *.bc main
