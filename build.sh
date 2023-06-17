#!/bin/bash

cd user
mkdir build
cd build
cmake .. && make
cd ../..

cd kern
mkdir build
cd build
cmake .. && make
cd ../..

mv user/build/NoPspEmuDrm_user.suprx ./NoPspEmuDrm_user.suprx
mv kern/build/NoPspEmuDrm_kern.skprx ./NoPspEmuDrm_kern.skprx