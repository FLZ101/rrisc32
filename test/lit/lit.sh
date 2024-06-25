#!/bin/bash

set -e

cmake_current_binary_dir=$1
cmake_binary_dir=$2

if [[ -z $2 ]] ; then
  echo 'lit.sh ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_BINARY_DIR}'
  exit 1
fi

PATH=$cmake_binary_dir/tools/assemble:$PATH
PATH=$cmake_binary_dir/tools/dump:$PATH
PATH=$cmake_binary_dir/tools/link:$PATH
PATH=$cmake_binary_dir/tools/emulate:$PATH
PATH=$cmake_current_binary_dir/python/bin:$PATH
export PATH

if [[ ! -e "$cmake_current_binary_dir/python" ]] ; then
  pushd "$cmake_current_binary_dir"
  python3 -m venv python
  popd
  pip install -U pip
  pip install -r requirements.txt
fi

test_exec_root="$cmake_current_binary_dir" lit . --verbose
