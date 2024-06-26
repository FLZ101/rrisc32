#!/bin/bash

set -e

G=Ninja
G="Unix Makefiles"
cmake -Wno-dev -DCMAKE_BUILD_TYPE=Debug -G"$G" -B build -S .
cmake --build build
# cmake --build build --target check-all

./build/tools/emulate/emulate --debug emulation ./build/test/lit/linkage/basic/Output/bar.s.tmp.exe
