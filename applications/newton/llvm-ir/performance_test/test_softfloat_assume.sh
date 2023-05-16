#!/bin/bash
set -e

[ -e assume.log ] && rm assume.log

clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D ASSUME -D lowerBound=0 -D upperBound=100 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 0 100 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D ASSUME -D lowerBound=0 -D upperBound=70 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 0 70 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D ASSUME -D lowerBound=260 -D upperBound=1260 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 260 1260 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D ASSUME -D lowerBound=10 -D upperBound=45 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 10 45 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D ASSUME -D lowerBound=20 -D upperBound=80 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 20 80 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D ASSUME -D lowerBound=0 -D upperBound=50 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 0 50 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D ASSUME -D lowerBound=30 -D upperBound=130 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 30 130 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D ASSUME -D lowerBound=1 -D upperBound=200 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 1 200 >> assume.log

clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D ASSUME -D lowerBound=0 -D upperBound=100 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 0 100 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D ASSUME -D lowerBound=0 -D upperBound=70 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 0 70 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D ASSUME -D lowerBound=260 -D upperBound=1260 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 260 1260 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D ASSUME -D lowerBound=10 -D upperBound=45 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 10 45 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D ASSUME -D lowerBound=20 -D upperBound=80 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 20 80 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D ASSUME -D lowerBound=0 -D upperBound=50 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 0 50 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D ASSUME -D lowerBound=30 -D upperBound=130 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 30 130 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D ASSUME -D lowerBound=1 -D upperBound=200 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 1 200 >> assume.log

clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D ASSUME -D lowerBound=0 -D upperBound=100 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 0 100 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D ASSUME -D lowerBound=0 -D upperBound=70 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 0 70 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D ASSUME -D lowerBound=260 -D upperBound=1260 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 260 1260 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D ASSUME -D lowerBound=10 -D upperBound=45 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 10 45 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D ASSUME -D lowerBound=20 -D upperBound=80 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 20 80 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D ASSUME -D lowerBound=0 -D upperBound=50 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 0 50 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D ASSUME -D lowerBound=30 -D upperBound=130 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 30 130 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D ASSUME -D lowerBound=1 -D upperBound=200 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 1 200 >> assume.log

echo "-----------------without assume-----------------" >> assume.log

clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D lowerBound=0 -D upperBound=100 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 0 100 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D lowerBound=0 -D upperBound=70 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 0 70 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D lowerBound=260 -D upperBound=1260 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 260 1260 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D lowerBound=10 -D upperBound=45 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 10 45 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D lowerBound=20 -D upperBound=80 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 20 80 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D lowerBound=0 -D upperBound=50 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 0 50 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D lowerBound=30 -D upperBound=130 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 30 130 >> assume.log
clang ../CHStone_test/dfadd/float64_add.cpp -D DEBUG -D lowerBound=1 -D upperBound=200 -O3 -o float64_add_assume -lm >& compile.log
./float64_add_assume 1 200 >> assume.log

clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D lowerBound=0 -D upperBound=100 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 0 100 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D lowerBound=0 -D upperBound=70 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 0 70 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D lowerBound=260 -D upperBound=1260 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 260 1260 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D lowerBound=10 -D upperBound=45 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 10 45 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D lowerBound=20 -D upperBound=80 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 20 80 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D lowerBound=0 -D upperBound=50 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 0 50 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D lowerBound=30 -D upperBound=130 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 30 130 >> assume.log
clang ../CHStone_test/dfmul/float64_mul.cpp -D DEBUG -D lowerBound=1 -D upperBound=200 -O3 -o float64_mul_assume -lm >& compile.log
./float64_mul_assume 1 200 >> assume.log

clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D lowerBound=0 -D upperBound=100 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 0 100 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D lowerBound=0 -D upperBound=70 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 0 70 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D lowerBound=260 -D upperBound=1260 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 260 1260 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D lowerBound=10 -D upperBound=45 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 10 45 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D lowerBound=20 -D upperBound=80 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 20 80 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D lowerBound=0 -D upperBound=50 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 0 50 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D lowerBound=30 -D upperBound=130 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 30 130 >> assume.log
clang ../CHStone_test/dfdiv/float64_div.cpp -D DEBUG -D lowerBound=1 -D upperBound=200 -O3 -o float64_div_assume -lm >& compile.log
./float64_div_assume 1 200 >> assume.log
