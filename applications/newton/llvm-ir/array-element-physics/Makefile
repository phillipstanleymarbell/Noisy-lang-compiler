ifdef QUIET
	_QUIET:=@
endif

LLVM_CONFIG:=llvm-config
CC:=clang # Currently tested with clang-12/clang-13

COMMON_FLAGS=-Wall -Wextra

default: main.ll


%.ll : %.c
	$(_QUIET)$(CC) -g -S -emit-llvm $(COMMON_FLAGS) -o $@ $<

clean::
	$(_QUIET)rm -f *.ll
