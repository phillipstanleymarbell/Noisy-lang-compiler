#!/bin/sh

#
#	Usage: ./newtonPiGroups2PDFLaTeX.sh <Newton binary> <Newton description file> <destination path>
#

if [ $# -ne 3 ]
then
	echo '\n\nUsage: ./newtonPiGroups2PDFLaTeX.sh <Newton binary> <Newton description file> <destination path>\n\n'
	exit 1
fi

$1 -ex $2 | grep -v 'LaTeX Backend Output:' | grep -v '\-\-\-\-\-' > /tmp/tmp-newton.tex;  pdflatex -jobname=`basename $2` -output-directory=$3 /tmp/tmp-newton.tex; open $3/`basename $2`.pdf
