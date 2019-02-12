#!/bin/bash

# first off get mozquic sources
if cd mozquic
then
  git pull
else
  git clone https://github.com/jakobod/mozquic.git
  cd mozquic
fi
./build.sh
cd ..

# now build mozquic_example
if cd build
then
  rm -rf *
else
  mkdir build
  cd build
fi
cmake ..
make -j4

