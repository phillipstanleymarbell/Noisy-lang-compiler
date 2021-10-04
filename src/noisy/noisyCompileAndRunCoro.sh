#!/bin/sh

#
#	Usage: ./noisyIr2dot.sh <noisy file> <pdf|png|other dot format> <Noisy Dot Level>
#

if [ $# -ne 1 ]
then
	echo '\n\nUsage: ./noisyCompileAndRun.sh <noisy file>\n\n'
	exit 1
fi
name=$(basename $1 | cut -f 1 -d '.');

./noisy-`uname | tr '[:upper:]' '[:lower:]'`-EN $1;
cd ../../applications/noisy;
opt < $name.bc -O1 -enable-coroutines -S > $name-opt.ll;
llc -filetype=obj $name-opt.ll -o $name.o;
clang $name.o noisyLib.o -o $name;
./$name;
