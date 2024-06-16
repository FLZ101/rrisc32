#!/bin/bash

set -e

G=Ninja
G="Unix Makefiles"
cmake -Wno-dev -DCMAKE_BUILD_TYPE=Debug -G"$G" -B build -S .
cmake --build build

./build/tools/assemble/assemble
./build/tools/dump/dump --all ./test/tools/assemble/hello.s.o
