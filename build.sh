#!/bin/bash

total_start=`date +%s%N`

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

mkdir -p build/data/shaders


FLAGS="-std=c++11 -g"

NOWARNINGS="-Wno-int-to-void-pointer-cast -Wno-nested-anon-types"
WARNINGS="-Wall -Wextra -Wpedantic $NOWARNINGS"

INCLUDE_DIR="-I$ROOT/src -I$ROOT"

OPTIMIZED=-O3
UNOPTIMIZED=-O0

LEARY_LIBS="-lvulkan -lX11 -lXi"
LEARY_FLAGS="$FLAGS $WARNINGS $UNOPTIMIZED $INCLUDE_DIR $LEARY_LIBS"
TOOLS_FLAGS="$FLAGS $WARNINGS $UNOPTIMIZED $INCLUDE_DIR"


pushd $ROOT/build
assets_start=`date +%s%N`
glslangValidator -V $ROOT/src/render/shaders/generic.vert -o $ROOT/build/data/shaders/generic.vert.spv
glslangValidator -V $ROOT/src/render/shaders/generic.frag -o $ROOT/build/data/shaders/generic.frag.spv

glslangValidator -V $ROOT/src/render/shaders/font.vert -o $ROOT/build/data/shaders/font.vert.spv
glslangValidator -V $ROOT/src/render/shaders/font.frag -o $ROOT/build/data/shaders/font.frag.spv

cp -R $ROOT/assets/fonts $ROOT/build/data/fonts
assets_end=`date +%s%N`

leary_start=`date +%s%N`
$CXX $LEARY_FLAGS -o leary $ROOT/src/platform/linux_main.cpp
leary_end=`date +%s%N`

tools_start=`date +%s%N`
$CXX $TOOLS_FLAGS -o preprocessor $ROOT/tools/preprocessor.cpp
tools_end=`date +%s%N`
popd

total_end=`date +%s%N`

assets_duration=$(((assets_end-assets_start)/1000000))
leary_duration=$(((leary_end-leary_start)/1000000))
tools_duration=$(((tools_end-tools_start)/1000000))
total_duration=$(((total_end-total_start)/1000000))

echo "assets built in: $assets_duration ms"
echo "leary built in: $leary_duration ms"
echo "tools built in: $tools_duration ms"
echo "total: $total_duration ms"


