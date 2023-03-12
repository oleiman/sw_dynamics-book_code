#!/bin/bash

if test "$#" -lt 2; then
    echo "usage: build.sh FILENAME O_LVL"
    exit
fi


fullfilename="${1%.*}"
filename=$(basename -- "$fullfilename")

opt="$2"

mkdir -p build

pushd build

g++ -fno-tree-reassoc -I../../src "../${1}" -O"${opt}" -o "${filename}_${opt}"
g++ -fno-tree-reassoc -fverbose-asm -I../../src "../${1}" -O"${opt}" -o "${filename}_${opt}.asm" -S


#-fno-tree-reassoc
popd


