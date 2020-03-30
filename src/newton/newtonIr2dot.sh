#!/bin/sh

#
#	Usage: ./newtonIr2dot.sh  <Newton binary> <Newton description file> <pdf|png|other dot format> <Newton Dot Level>
#

if [ $# -ne 4 ]
then
	echo '\n\nUsage: ./newtonIr2dot.sh  <Newton binary> <Newton description file> <pdf|png|other dot format> <Newton Dot Level>\n\n'
	exit 1
fi

$1 --optimize 0 --dot $4 $2 | dot -T$3 -O ; open noname.gv.$3
