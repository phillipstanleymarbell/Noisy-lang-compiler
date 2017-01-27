#!/bin/sh

trackingDirectory=Statistics

#
#	Since the pre-commit hook leads to a change of the hash, we saved the hash at the point of commit in the file '.noisy-last-head'
#
statsFile=`cat .noisy-last-head`.txt

git add $trackingDirectory/$statsFile
