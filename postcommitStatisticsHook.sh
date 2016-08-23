#!/bin/sh

trackingDirectory=statistics
statsFile=`git rev-parse HEAD`.txt

hg add $trackingDirectory/$statsFile
