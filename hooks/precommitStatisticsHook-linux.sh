#!/bin/sh

dtraceDirectory=
libflexDirectory=
trackingDirectory=analysis/Statistics
statsFile=`git rev-parse HEAD`.txt

#
#	Since the pre-commit hook leads to a change of the hash, we save the hash at the point of commit in the file '.noisy-last-head'
#
echo `git rev-parse HEAD` > .noisy-last-head

echo '' > $trackingDirectory/$statsFile
echo "changeset: `git rev-list --count HEAD`:`git rev-parse HEAD`" >> $trackingDirectory/$statsFile 

cd $libflexDirectory && make clean all &

#wait $!
make clean
make -j
make README.sloccount

cat version.c >> $trackingDirectory/$statsFile
echo '\n./noisy/noisy-linux-EN -O0 Examples/noisy/helloWorld.n -s' >> $trackingDirectory/$statsFile
./src/noisy/noisy-linux-EN -O0 Examples/noisy/helloWorld.n -s >> $trackingDirectory/$statsFile
echo '\n./newton/newton-linux-EN -S tmp.smt2 Examples/newton/pendulum_acceleration.nt' >> $trackingDirectory/$statsFile
./src/newton/newton-linux-EN -S tmp.smt2 Examples/newton/pendulum_acceleration.nt >> $trackingDirectory/$statsFile
rm tmp.smt2

cp $trackingDirectory/$statsFile $trackingDirectory/latest.txt
