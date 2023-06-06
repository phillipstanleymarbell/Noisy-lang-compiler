#!/bin/sh

#
#	Usage: ./noisyCompileAndRun.sh <noisy file>
#

if [ $# -ne 1 ]
then
	echo '\n\nUsage: ./noisyCompileAndRun.sh <noisy file>\n\n'
	exit 1
fi
name=$(basename $1 | cut -f 1 -d '.');

./noisy-`uname | tr '[:upper:]' '[:lower:]'`-EN $1;
cd ../../applications/noisy;
llc -filetype=obj $name.bc -o $name.o;
clang $name.o noisyLib.o -lm -o $name;
./$name;
