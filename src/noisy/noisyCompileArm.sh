#!/bin/sh

#
#	Usage: ./noisyCompileAr.sh quicksort.n
#
#       Works only for quicksort.n because other examples need noisyLib to work.

if [ $# -ne 1 ]
then
	echo '\n\nUsage: ./noisyCompileArm.sh <noisy file>\n\n'
	exit 1
fi
name=$(basename $1 | cut -f 1 -d '.');

llc_flags="-filetype=obj -mtriple=thumbv6m-none-eabi --float-abi=soft -mcpu=cortex-m0plus"
ld_flags="-shared -fno-exceptions --verbose"

./noisy-`uname | tr '[:upper:]' '[:lower:]'`-EN $1;
cd ../../applications/noisy;
llc  $llc_flags $name.bc -o $name-arm.o;
arm-none-eabi-ld $ld_flags $name-arm.o -lc -o $name-arm;
./$name;