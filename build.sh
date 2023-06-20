#!/bin/bash

cd user
mkdir build
cd build
cmake .. && make

retVal=$?
if [ $retVal -ne 0 ]; then
    echo "Build Failed.";
	exit;
fi;

cd ../..
cd kern
mkdir build
cd build
cmake .. && make

retVal=$?
if [ $retVal -ne 0 ]; then
    echo "Build Failed.";
	exit;
fi;

cd ../..

mv user/build/NoPspEmuDrm_user.suprx ./NoPspEmuDrm_user.suprx
mv kern/build/NoPspEmuDrm_kern.skprx ./NoPspEmuDrm_kern.skprx