#!/bin/bash
set -e

[ -e assume.log ] && rm assume.log

ASSUME=true lowerBound=-2 upperBound=2 make perf_madgwick >& compile.log
./main_out -2 2 >> assume.log
ASSUME=true lowerBound=-4 upperBound=4 make perf_madgwick >& compile.log
./main_out -4 4 >> assume.log
ASSUME=true lowerBound=-8 upperBound=8 make perf_madgwick >& compile.log
./main_out -8 8 >> assume.log
ASSUME=true lowerBound=-16 upperBound=16 make perf_madgwick >& compile.log
./main_out -16 16 >> assume.log
ASSUME=true lowerBound=-125 upperBound=125 make perf_madgwick >& compile.log
./main_out -125 125 >> assume.log
ASSUME=true lowerBound=-40 upperBound=110 make perf_madgwick >& compile.log
./main_out -40 110 >> assume.log
ASSUME=true lowerBound=-55 upperBound=150 make perf_madgwick >& compile.log
./main_out -55 150 >> assume.log
ASSUME=true lowerBound=0 upperBound=100 make perf_madgwick >& compile.log
./main_out 0 100 >> assume.log
ASSUME=true lowerBound=0 upperBound=70 make perf_madgwick >& compile.log
./main_out 0 70 >> assume.log
ASSUME=true lowerBound=260 upperBound=1260 make perf_madgwick >& compile.log
./main_out 260 1260 >> assume.log
ASSUME=true lowerBound=10 upperBound=45 make perf_madgwick >& compile.log
./main_out 10 45 >> assume.log
ASSUME=true lowerBound=-55 upperBound=125 make perf_madgwick >& compile.log
./main_out -55 125 >> assume.log
ASSUME=true lowerBound=20 upperBound=80 make perf_madgwick >& compile.log
./main_out 20 80 >> assume.log
ASSUME=true lowerBound=0 upperBound=50 make perf_madgwick >& compile.log
./main_out 0 50 >> assume.log
ASSUME=true lowerBound=-0.2 upperBound=2 make perf_madgwick >& compile.log
./main_out -0.2 2 >> assume.log
ASSUME=true lowerBound=30 upperBound=130 make perf_madgwick >& compile.log
./main_out 30 130 >> assume.log
ASSUME=true lowerBound=1 upperBound=200 make perf_madgwick >& compile.log
./main_out 1 200 >> assume.log
echo "-----------------without assume-----------------" >> assume.log
make perf_madgwick >& compile.log
# shellcheck disable=SC2129
./main_out -2 2 >> assume.log
./main_out -4 4 >> assume.log
./main_out -8 8 >> assume.log
./main_out -16 16 >> assume.log
./main_out -125 125 >> assume.log
./main_out -40 110 >> assume.log
./main_out -55 150 >> assume.log
./main_out 0 100 >> assume.log
./main_out 0 70 >> assume.log
./main_out 260 1260 >> assume.log
./main_out 10 45 >> assume.log
./main_out -55 125 >> assume.log
./main_out 20 80 >> assume.log
./main_out 0 50 >> assume.log
./main_out -0.2 2 >> assume.log
./main_out 30 130 >> assume.log
./main_out 1 200 >> assume.log
