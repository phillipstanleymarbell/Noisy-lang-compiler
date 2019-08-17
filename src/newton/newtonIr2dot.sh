#!/bin/sh

#
#	Usage: ./newtonIr2dot.sh <Newton description file> <pdf|png|other dot format> <Newton Dot Level>
#

if [ $# -ne 3 ]
then
	echo '\n\nUsage: ./newtonIr2dot.sh <Newton description file> <pdf|png|other dot format> <Newton Dot Level>\n\n'
	exit 1
fi

./newton-`uname | tr '[:upper:]' '[:lower:]'`-EN --optimize 0 --dot $3 $1 | dot -T$2 -O ; open noname.gv.$2
