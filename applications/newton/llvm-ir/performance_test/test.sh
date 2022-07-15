#!/bin/bash

cd ..
make clean
make

if [ $2 == "optimized" ]
then
  cd ../../../src/newton
  ./newton-linux-EN --llvm-ir=../../applications/newton/llvm-ir/$1.ll --llvm-ir-liveness-check ../../applications/newton/sensors/test.nt
  cd ../../applications/newton/llvm-ir/performance_test/
else
  cd performance_test/
fi

rm -rf out.*
rm -rf libout.so
rm -rf main_out
if [ $2 == "optimized" ]
then
  llvm-dis ../$1_output.bc
  opt ../$1_output.ll --simplifycfg -S -o out.ll
else
  sed 's/optnone //' ../$1.ll > ../$1_opt.ll
  opt ../$1_opt.ll --simplifycfg -S -o out.ll
fi
llvm-as out.ll -o out.bc
llc out.bc -o out.s
#gcc -fPIC -shared out.s -O2 -o libout.so
gcc -c out.s -o out.o
ar -rc libout.a out.o
gcc main.c -no-pie -L. -lout -O2 -o main_out -lm
sudo perf stat -B ./main_out if=/dev/zero of=/dev/null count=1000000

