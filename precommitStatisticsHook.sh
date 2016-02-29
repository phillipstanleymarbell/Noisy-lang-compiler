#!/bin/sh

dtraceDirectory=/Volumes/doos/DTrace-hg
libflexDirectory=/Volumes/doos/libflex-hg-clone
trackingDirectory=Statistics
statsFile=`hg tip | grep 'changeset' | awk -F ':' '{print $3}'`.txt

echo '' > $trackingDirectory/$statsFile
system_profiler -detailLevel mini | grep -A 10 'Hardware Overview' >> $trackingDirectory/$statsFile
echo '' >> $trackingDirectory/$statsFile
hg tip >> $trackingDirectory/$statsFile 

cd $libflexDirectory && make clean all &
make clean
make -j
make README.sloccount

cat version.c >> $trackingDirectory/$statsFile
echo '\n./noisy-darwin-EN -O0 Examples/helloWorld.n -s' >> $trackingDirectory/$statsFile
./noisy-darwin-EN -O0 Examples/helloWorld.n -s >> $trackingDirectory/$statsFile

echo '\nsudo dtrace -qs $dtraceDirectory/fcalls.d -c "./noisy-darwin-EN -O0 Examples/helloWorld.n -s"' >> $trackingDirectory/$statsFile
sudo dtrace -qs $dtraceDirectory/fcalls.d -c "./noisy-darwin-EN -O0 Examples/helloWorld.n -s" noisy-darwin-EN >> $trackingDirectory/$statsFile

echo '\nsudo dtrace -qs $dtraceDirectory/malloc.d -c "./noisy-darwin-EN -O0 Examples/helloWorld.n -s"' >> $trackingDirectory/$statsFile
sudo dtrace -qs $dtraceDirectory/malloc.d -c "./noisy-darwin-EN -O0 Examples/helloWorld.n -s" >> $trackingDirectory/$statsFile

cp $trackingDirectory/$statsFile $trackingDirectory/latest.txt
