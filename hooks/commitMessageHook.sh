#!/bin/sh

TEXT=$(cat "$1" | sed '/^#.*/d')

if [ -n "$TEXT" ]
then
    MSG=$(git branch | grep "^*" | sed "s/.*issue-\([0-9]*\).*/Addresses #\1/g")
    echo >> $1
    echo "${MSG}." >> $1
    echo >> $1
else
    echo "Empty commit message, aborting..."
    exit 1
fi
