#!/bin/bash

generator=$1
mode=$2
library=$3
prepath=$4

if [ -z ${generator}  ] ;
then generator="Unix Makefiles"
fi

if [ -z ${mode} ] ;
then mode="Debug"
fi

if [ -z ${library} ] ;
then library="./dep-library"
fi

if [ -z ${prepath} ] ;
then prepath="./"
fi

installpfx="$prepath$library"

if [ ! -d "$installpfx" ]; then
mkdir "$installpfx"
fi

cd ./deps
./build.sh "$generator" "$mode" "$library" "../$prepath"

#hades
cd ../hades
cmake -DCMAKE_BUILD_TYPE="$mode" -DCMAKE_INSTALL_PREFIX="../$installpfx" -G "$generator"
cmake --build . --target install --config "$mode"

#test
cd ../test
cmake -DCMAKE_BUILD_TYPE="$mode" -DCMAKE_INSTALL_PREFIX="../$installpfx" -G "$generator"
cmake --build . --target install --config "$mode"