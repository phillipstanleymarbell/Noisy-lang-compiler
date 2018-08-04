#!/bin/sh

#
#	Usage: ./noisyIr2dot.sh <noisy file> <pdf|png|other dot format>
#

if [ $# -ne 2 ]
then
	echo '\n\nUsage: ./noisyIr2dot.sh <noisy file> <pdf|png|other dot format>\n\n'
	exit 1
fi

./noisy-`uname | tr '[:upper:]' '[:lower:]'`-EN --optimize 0 --dot 4 $1 | dot -T$2 -O ; open noname.gv.$2
