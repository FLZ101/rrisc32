#!/bin/bash

set -e

cmake -DCMAKE_BUILD_TYPE=Debug -B build -S .
cmake --build build

# ./build/tools/dump/dump
./build/tools/assemble/assemble
