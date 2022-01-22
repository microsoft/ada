#!/bin/bash

SOURCE="${BASH_SOURCE[0]}"
echo "Building $(dirname $SOURCE)"
cd "$( dirname "$SOURCE" )"

if [[ ! -d "build" ]]
then
  echo "Creating build directory"
  mkdir build
  cd build
  cmake ..
  cd ..
fi


cd build
make
cd ..

