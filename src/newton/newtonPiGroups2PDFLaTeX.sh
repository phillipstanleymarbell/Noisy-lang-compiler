#!/bin/sh

#
#	Usage: ./newtonPiGroups2PDFLaTeX.sh <Newton description file>
#

if [ $# -ne 1 ]
then
	echo '\n\nUsage: ./newtonPiGroups2PDFLaTeX.sh <Newton description file>\n\n'
	exit 1
fi

./newton-`uname | tr '[:upper:]' '[:lower:]'`-EN $1 -ex | grep -v 'LaTeX Backend Output:' | grep -v '\-\-\-\-\-' | pdflatex -jobname=$1; open $1.pdf
