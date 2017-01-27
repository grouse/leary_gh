#!/bin/bash

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mkdir -p build/data/shaders


FLAGS="-std=c++11 -g"

NOWARNINGS="-Wno-int-to-void-pointer-cast"
WARNINGS="-Wall -Wextra -Wpedantic $NOWARNINGS"

INCLUDE_DIR="-I$ROOT/src"

OPTIMIZED=-O3
UNOPTIMIZED=-O0

LEARY_LIBS="-lvulkan -lxcb"
LEARY_FLAGS="$FLAGS $WARNINGS $UNOPTIMIZED $INCLUDE_DIR $LEARY_LIBS"


pushd $ROOT/build
glslangValidator -V $ROOT/data/shaders/triangle.vert -o $ROOT/build/data/shaders/triangle_vert.spv
glslangValidator -V $ROOT/data/shaders/triangle.frag -o $ROOT/build/data/shaders/triangle_frag.spv
$CXX $LEARY_FLAGS -o leary $ROOT/src/platform/linux_main.cpp
popd
