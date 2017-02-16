#!/bin/bash

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mkdir -p build/data/shaders


FLAGS="-std=c++11 -g"

NOWARNINGS="-Wno-int-to-void-pointer-cast -Wno-nested-anon-types"
WARNINGS="-Wall -Wextra -Wpedantic $NOWARNINGS"

INCLUDE_DIR="-I$ROOT/src -I$ROOT"

OPTIMIZED=-O3
UNOPTIMIZED=-O0

LEARY_LIBS="-lvulkan -lX11"
LEARY_FLAGS="$FLAGS $WARNINGS $UNOPTIMIZED $INCLUDE_DIR $LEARY_LIBS"
TOOLS_FLAGS="$FLAGS $WARNINGS $UNOPTIMIZED $INCLUDE_DIR"


pushd $ROOT/build
glslangValidator -V $ROOT/src/render/shaders/generic.vert -o $ROOT/build/data/shaders/generic.vert.spv
glslangValidator -V $ROOT/src/render/shaders/generic.frag -o $ROOT/build/data/shaders/generic.frag.spv

glslangValidator -V $ROOT/src/render/shaders/font.vert -o $ROOT/build/data/shaders/font.vert.spv
glslangValidator -V $ROOT/src/render/shaders/font.frag -o $ROOT/build/data/shaders/font.frag.spv

cp -R $ROOT/assets/fonts $ROOT/build/data/fonts

$CXX $LEARY_FLAGS -o leary $ROOT/src/platform/linux_main.cpp
$CXX $TOOLS_FLAGS -o preprocessor $ROOT/tools/preprocessor.cpp
popd
