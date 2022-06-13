#!/bin/bash

if [ $2 == "optimized" ]
then
  ./src/newton/newton-linux-EN --llvm-ir=$1 --llvm-ir-liveness-check applications/newton/sensors/test.nt
fi

opt-12 $1 -O2 -S -o out.ll
llvm-as-12 out.ll -o out.bc
llc out.bc -o out.s
gcc -fPIC -shared out.s -O2 -o libout.so
mv libout.so applications/newton/llvm-ir/c-files/
cd applications/newton/llvm-ir/c-files/
gcc main.c -L. -lout -O2 -o main_out
perf stat -B ./main_out if=/dev/zero of=/dev/null count=1000000
