#!/bin/bash

if test "$#" -ne 2; then
    echo "usage: build.sh FILENAME O_LVL"
    exit
fi


fullfilename="${1%.*}"
filename=$(basename -- "$fullfilename")

opt="$2"

mkdir -p build

pushd build

g++ "../${1}" -O"${opt}" -o "${filename}_${opt}"
g++ "../${1}" -O"${opt}" -o "${filename}_${opt}.asm" -S

popd


