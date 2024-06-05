#!/bin/bash

set -e

G=Ninja
G="Unix Makefiles"
cmake -DCMAKE_BUILD_TYPE=Debug -G"$G" -B build -S .
cmake --build build

# ./build/tools/dump/dump
./build/tools/assemble/assemble
