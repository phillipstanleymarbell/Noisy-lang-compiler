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
echo "changeset: `git rev-list --count HEAD`:`git rev-parse HEAD`" >> $trackingDirectory/$statsFile 

cd $libflexDirectory && make clean all &

#wait $!
make clean
make #-j
make README.sloccount

cat src/noisy/version.c >> $trackingDirectory/$statsFile
cat src/newton/version.c >> $trackingDirectory/$statsFile
cp applications/newton/include/NewtonBaseSignals.nt .
echo '\n./src/noisy/noisy-linux-EN -O0 applications/noisy/helloWorld.n -s' >> $trackingDirectory/$statsFile
./src/noisy/noisy-linux-EN -O0 applications/noisy/helloWorld.n -s >> $trackingDirectory/$statsFile
echo '\n./src/newton/newton-linux-EN -v 0 -eP applications/newton/invariants/ViolinWithTemperatureDependence-pigroups.nt' >> $trackingDirectory/$statsFile
./src/newton/newton-linux-EN -v 0 -eP applications/newton/invariants/ViolinWithTemperatureDependence-pigroups.nt >> $trackingDirectory/$statsFile
rm NewtonBaseSignals.nt

cp $trackingDirectory/$statsFile $trackingDirectory/latest.txt
