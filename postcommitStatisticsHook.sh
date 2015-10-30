#!/bin/sh

trackingDirectory=statistics
statsFile=`hg tip | grep 'changeset' | awk -F ':' '{print $3}'`.txt

hg add $trackingDirectory/$statsFile
