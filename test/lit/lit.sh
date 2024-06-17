#!/bin/bash

set -e

cmake_current_binary_dir=$1
assemble_dir=$2
dump_dir=$3
if [[ -z $3 ]] ; then
  echo 'lit.sh ${CMAKE_CURRENT_BINARY_DIR} $<TARGET_FILE_DIR:assemble> $<TARGET_FILE_DIR:dump>'
  exit 1
fi

export PATH=$assemble_dir:$dump_dir:$cmake_current_binary_dir/python/bin:$PATH

if [[ ! -e "$cmake_current_binary_dir/python" ]] ; then
  pushd "$cmake_current_binary_dir"
  python3 -m venv python
  popd
  pip install -U pip
  pip install -r requirements.txt
fi

test_exec_root="$cmake_current_binary_dir" lit . --verbose
