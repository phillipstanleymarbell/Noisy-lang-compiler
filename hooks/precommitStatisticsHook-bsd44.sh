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
make #-j
make README.sloccount

cat src/noisy/version.c >> $trackingDirectory/$statsFile
cat src/newton/version.c >> $trackingDirectory/$statsFile
cp applications/newton/include/NewtonBaseSignals.nt .
#echo '\n./src/noisy/noisy-bsd44-EN -O0 applications/noisy/helloWorld.n -s' >> $trackingDirectory/$statsFile
#./src/noisy/noisy-bsd44-EN -O0 applications/noisy/helloWorld.n -s >> $trackingDirectory/$statsFile
echo '\n./src/newton/newton-bsd44-EN -v 0 -eP applications/newton/invariants/ViolinWithTemperatureDependence-pigroups.nt' >> $trackingDirectory/$statsFile
./src/newton/newton-bsd44-EN -v 0 -eP applications/newton/invariants/ViolinWithTemperatureDependence-pigroups.nt >> $trackingDirectory/$statsFile
rm NewtonBaseSignals.nt

# echo '\nsudo dtrace -qs $dtraceDirectory/fcalls.d -c "./noisy/noisy-bsd44-EN -O0 applications/noisy/helloWorld.n -s"' >> $trackingDirectory/$statsFile
# sudo dtrace -qs $dtraceDirectory/fcalls.d -c "./noisy/noisy-bsd44-EN -O0 applications/noisy/helloWorld.n -s" noisy/noisy-bsd44-EN >> $trackingDirectory/$statsFile

# echo '\nsudo dtrace -qs $dtraceDirectory/malloc.d -c "./noisy/noisy-bsd44-EN -O0 applications/noisy/helloWorld.n -s"' >> $trackingDirectory/$statsFile
# sudo dtrace -qs $dtraceDirectory/malloc.d -c "./noisy/noisy-bsd44-EN -O0 applications/noisy/helloWorld.n -s" >> $trackingDirectory/$statsFile

cp $trackingDirectory/$statsFile $trackingDirectory/latest.txt
