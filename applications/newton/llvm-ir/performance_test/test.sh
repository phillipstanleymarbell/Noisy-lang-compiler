#!/bin/bash

if [ $2 == "optimized" ]
then
  cd ..
  make clean
  make
  cd ../../../src/newton
  ./newton-linux-EN --llvm-ir=../../applications/newton/llvm-ir/$1.ll --llvm-ir-liveness-check ../../applications/newton/sensors/test.nt
  cd ../../applications/newton/llvm-ir/performance_test/
fi

rm -rf out.*
rm -rf libout.so
rm -rf main_out
if [ $2 == "optimized" ]
then
  opt-12 ../$1_output.ll -O2 -S -o out.ll
else
  opt-12 ../$1.ll -O2 -S -o out.ll
fi
llvm-as-12 out.ll -o out.bc
llc out.bc -o out.s
gcc -fPIC -shared out.s -O2 -o libout.so
gcc main.c -L. -lout -O2 -o main_out
perf stat -B ./main_out if=/dev/zero of=/dev/null count=1000000
