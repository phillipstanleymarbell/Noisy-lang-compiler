#!/bin/sh

trackingDirectory=statistics
statsFile=`git rev-parse HEAD`.txt

git add $trackingDirectory/$statsFile
