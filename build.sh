#!/bin/bash
# wakeonlan API - project build script
if [ ! -d "./build" ]
then
	mkdir build
fi
cd build || exit
cmake .. && make -j 4
