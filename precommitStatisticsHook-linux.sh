#!/bin/sh

dtraceDirectory=
libflexDirectory=
trackingDirectory=Statistics
statsFile=`git rev-parse HEAD`.txt

#
#	Since the pre-commit hook leads to a change of the hash, we save the hash at the point of commit in the file '.noisy-last-head'
#
echo `git rev-parse HEAD` > .noisy-last-head

echo '' >> $trackingDirectory/$statsFile
echo "changeset: `git rev-list --count HEAD`:`git rev-parse HEAD`" >> $trackingDirectory/$statsFile 

cd $libflexDirectory && make clean all &

#wait $!
make clean
make -j
make README.sloccount

cat version.c >> $trackingDirectory/$statsFile
echo '\n./noisy/noisy-linux-EN -O0 Examples/helloWorld.n -s' >> $trackingDirectory/$statsFile
./noisy/noisy-linux-EN -O0 Examples/helloWorld.n -s >> $trackingDirectory/$statsFile

cp $trackingDirectory/$statsFile $trackingDirectory/latest.txt
