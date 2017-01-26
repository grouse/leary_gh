#!/bin/bash

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mkdir -p build/data/shaders


FLAGS="-std=c++11 -g"
WARNINGS="-Wall -Wextra -Wpedantic"
INCLUDE_DIR="-I$ROOT/src"

OPTIMIZED=-O3
UNOPTIMIZED=-O0

LEARY_LIBS="-lvulkan -lxcb"
LEARY_FLAGS="$FLAGS $WARNINGS $UNOPTIMIZED $INCLUDE_DIR $LEARY_LIBS"


pushd $ROOT/build
$CXX $LEARY_FLAGS -o leary $ROOT/src/platform/linux_main.cpp
popd
