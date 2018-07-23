#!/bin/sh

dtraceDirectory=submodules/dtrace-scripts
libflexDirectory=submodules/libflex
trackingDirectory=analysis/statistics
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
echo '\n./src/noisy/noisy-darwin-EN -O0 applications/noisy/helloWorld.n -s' >> $trackingDirectory/$statsFile
./src/noisy/noisy-darwin-EN -O0 applications/noisy/helloWorld.n -s >> $trackingDirectory/$statsFile
echo '\n./src/newton/newton-linux-EN -S tmp.smt2 applications/newton/invariants/PendulumAcceleration.nt' >> $trackingDirectory/$statsFile
./src/newton/newton-linux-EN -S tmp.smt2 applications/newton/invariants/PendulumAcceleration.nt >> $trackingDirectory/$statsFile
rm tmp.smt2

echo '\nsudo dtrace -qs $dtraceDirectory/fcalls.d -c "./noisy/noisy-darwin-EN -O0 applications/noisy/helloWorld.n -s"' >> $trackingDirectory/$statsFile
sudo dtrace -qs $dtraceDirectory/fcalls.d -c "./noisy/noisy-darwin-EN -O0 applications/noisy/helloWorld.n -s" noisy/noisy-darwin-EN >> $trackingDirectory/$statsFile

echo '\nsudo dtrace -qs $dtraceDirectory/malloc.d -c "./noisy/noisy-darwin-EN -O0 applications/noisy/helloWorld.n -s"' >> $trackingDirectory/$statsFile
sudo dtrace -qs $dtraceDirectory/malloc.d -c "./noisy/noisy-darwin-EN -O0 applications/noisy/helloWorld.n -s" >> $trackingDirectory/$statsFile

cp $trackingDirectory/$statsFile $trackingDirectory/latest.txt
