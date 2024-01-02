#!/bin/bash
set -e

# test with v0.1: only type shrinkage
# shellcheck disable=SC2086
cp $PWD/../../../../src/newton/newton-linux-EN_v0_1 $PWD/../../../../src/newton/newton-linux-EN
make auto_test_compile
./auto_test
if [ $? -eq 0 ]; then
    echo "v0.1 finished successfully." >> err.log
    cp $PWD/perf.log ~/Documents/FORI/perf_shrinkage_type.log
    cp $PWD/average_speedup.log ~/Documents/FORI/average_speedup_shrinkage_type.log
else
    echo "v0.1 failed with error code $?." >> err.log
fi

# test with v0.2: type shrinkage and branch elimination
# shellcheck disable=SC2086
cp $PWD/../../../../src/newton/newton-linux-EN_v0_2 $PWD/../../../../src/newton/newton-linux-EN
make auto_test_compile
./auto_test
if [ $? -eq 0 ]; then
    echo "v0.2 finished successfully." >> err.log
    cp $PWD/perf.log ~/Documents/FORI/perf_type_branch.log
    cp $PWD/average_speedup.log ~/Documents/FORI/average_speedup_type_branch.log
else
    echo "v0.2 failed with error code $?." >> err.log
fi

# test with v0.3: type shrinkage, branch elimination and constant substitution
# shellcheck disable=SC2086
cp $PWD/../../../../src/newton/newton-linux-EN_v0_3 $PWD/../../../../src/newton/newton-linux-EN
make auto_test_compile
./auto_test
if [ $? -eq 0 ]; then
    echo "v0.3 finished successfully." >> err.log
    cp $PWD/perf.log ~/Documents/FORI/perf_type_branch_const.log
    cp $PWD/average_speedup.log ~/Documents/FORI/average_speedup_type_branch_const.log
else
    echo "v0.3 failed with error code $?." >> err.log
fi

# test with v1: type shrinkage, branch elimination, constant substitution and overload function
# shellcheck disable=SC2086
cp $PWD/../../../../src/newton/newton-linux-EN_v1 $PWD/../../../../src/newton/newton-linux-EN
make auto_test_compile
./auto_test
if [ $? -eq 0 ]; then
    echo "v1 finished successfully." >> err.log
    cp $PWD/perf.log ~/Documents/FORI/perf_overload.log
    cp $PWD/average_speedup.log ~/Documents/FORI/average_speedup_overload.log
else
    echo "v1 failed with error code $?." >> err.log
fi
