#!/bin/bash

newtonDir=$1

srcDir=$newtonDir/src/newton/
trackingDirectory=$newtonDir''trackingDirectory/

echo 'trackingDirectory is '$trackingDirectory

echo $pigroupsApps

cd $srcDir
appsDir=../../applications/newton/invariants/
pigroupsApps=$(ls $appsDir/*pigroups*)

#exit

for app in $pigroupsApps
do
	for (( i=1; i<=5; i++ ))
	do
		#echo $app
		#./newton-darwin-EN -p $app | grep "1 unique"

		# Remove directory and suffix
		strippedApp=$(basename -s .nt $app) #$appsDir/Pendulum-pigroups.nt)
		echo 'strippedApp is '$strippedApp

		echo 'dappprof -b 2048m -ceoT ./newton-darwin-EN -e -P '$app' 2> '$trackingDirectory$strippedApp'-dappprof-eP-'$i
		
		dappprof -b 2048m -ceoT ./newton-darwin-EN -e -P $app 2> $trackingDirectory$strippedApp'-dappprof-eP-'$i
		sleep 1
	done
done
