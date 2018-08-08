#!/bin/bash

# Exit on error
set -e
# Print executed commands
set -x

echo "Building in " `pwd`

# Build for boost libraries
rm -rf build && mkdir build
pushd build && cmake .. && cmake --build .

LC_ALL=C.UTF-8 make check

popd

# Build for standard libraries
rm -rf build && mkdir build
pushd build && cmake .. -Duse-std-filesystem=ON && cmake --build .

LC_ALL=C.UTF-8 make check

popd
