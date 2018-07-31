#!/bin/sh

TEXT=$(cat "$1" | sed '/^#.*/d')

if [ -n "$TEXT" ]
then
    MSG=$(git branch | grep "^*" | sed "s/.*issue-\([0-9]*\).*/Addresses #\1/g")
    echo -e "\n${MSG}\n" >> $1
else
    echo "Empty commit message, aborting..."
    exit 1
fi
