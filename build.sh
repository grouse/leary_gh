#!/bin/bash

ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CONFIG=$1

if [ "$1" = "w64" ]
then
    cmd.exe /c build.bat
else
    pushd $ROOT
    make -Oline -j12
    popd
fi


