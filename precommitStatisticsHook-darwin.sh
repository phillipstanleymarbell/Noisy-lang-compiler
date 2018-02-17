#!/bin/sh

dtraceDirectory=
libflexDirectory=
trackingDirectory=Statistics
statsFile=`git rev-parse HEAD`.txt

#
#	Since the pre-commit hook leads to a change of the hash, we save the hash at the point of commit in the file '.noisy-last-head'
#
echo `git rev-parse HEAD` > .noisy-last-head

echo '' > $trackingDirectory/$statsFile
system_profiler -detailLevel mini | grep -A 10 'Hardware Overview' >> $trackingDirectory/$statsFile
echo '' >> $trackingDirectory/$statsFile
echo "changeset: `git rev-list --count HEAD`:`git rev-parse HEAD`" >> $trackingDirectory/$statsFile 

cd $libflexDirectory && make clean all &

#wait $!
make clean
make -j
make README.sloccount

cat version.c >> $trackingDirectory/$statsFile
echo '\n./noisy/noisy-darwin-EN -O0 Examples/helloWorld.n -s' >> $trackingDirectory/$statsFile
./noisy/noisy-darwin-EN -O0 Examples/helloWorld.n -s >> $trackingDirectory/$statsFile
echo '\n./newton/newton-linux-EN -S tmp.smt2 Examples/pendulum_acceleration.nt' >> $trackingDirectory/$statsFile
./newton/newton-linux-EN -S tmp.smt2 Examples/pendulum_acceleration.nt >> $trackingDirectory/$statsFile
rm tmp.smt2

echo '\nsudo dtrace -qs $dtraceDirectory/fcalls.d -c "./noisy/noisy-darwin-EN -O0 Examples/helloWorld.n -s"' >> $trackingDirectory/$statsFile
sudo dtrace -qs $dtraceDirectory/fcalls.d -c "./noisy/noisy-darwin-EN -O0 Examples/helloWorld.n -s" noisy/noisy-darwin-EN >> $trackingDirectory/$statsFile

echo '\nsudo dtrace -qs $dtraceDirectory/malloc.d -c "./noisy/noisy-darwin-EN -O0 Examples/helloWorld.n -s"' >> $trackingDirectory/$statsFile
sudo dtrace -qs $dtraceDirectory/malloc.d -c "./noisy/noisy-darwin-EN -O0 Examples/helloWorld.n -s" >> $trackingDirectory/$statsFile

cp $trackingDirectory/$statsFile $trackingDirectory/latest.txt
