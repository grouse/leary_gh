#!/bin/bash

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

FLAGS="-std=c++11 -g"
INCLUDE_DIR="-I$ROOT/src"
LIBS="-lvulkan -lxcb"

OPTIMIZED=-O3
UNOPTIMIZED=-O0

mkdir -p build/data/shaders

pushd $ROOT/build
$CXX $FLAGS $UNOPTIMIZED $INCLUDE_DIR $LIBS -o leary $ROOT/src/platform/linux_main.cpp
popd
