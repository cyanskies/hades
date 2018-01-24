#!/bin/bash

generator=$1
mode=$2
library=$3
prepath=$4

if [ -z "${generator}"  ] ;
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

#SFML
cd ./sfml
cmake -DCMAKE_BUILD_TYPE="$mode" -DCMAKE_INSTALL_PREFIX="../$installpfx" -G "$generator"
cmake --build . --target install --config "$mode"

#TGUI
#depends on SFML

cd ../tgui
cmake -DCMAKE_BUILD_TYPE="$mode" -DCMAKE_INSTALL_PREFIX="../$installpfx" -G "$generator"
cmake --build . --target install --config "$mode"

#yaml-cpp

cd ../yaml-cpp
cmake -DCMAKE_BUILD_TYPE="$mode" -DCMAKE_INSTALL_PREFIX="../$installpfx" -G "$generator"
cmake --build . --target install --config "$mode"

cmake -DCMAKE_BUILD_TYPE="$mode" -DCMAKE_INSTALL_PREFIX="../$installpfx" -DBUILD_GMOCK=false -DYAML_CPP_BUILD_TESTS=false -G "$generator"
cmake --build . --target install --config "$mode"

#minizlib

#build zlib
cd ../zlib
./configure
make

#build minizip
cd ./contrib/minizip
make -f hades_makefile

#go back to the zlib root
cd ../../

#copy headers to install dir

zlibheaderdir="../$installpfx/include/zlib"

if [ ! -d "$zlibheaderdir" ]; then
mkdir "$zlibheaderdir"
fi

cp *.h "$zlibheaderdir"
cp ./contrib/minizip/*.h "$zlibheaderdir"

#copy lib files
#not sure if we need the zlib.a files
cp *.a "../$installpfx/lib"
cp ./contrib/minizip/*.so "../$installpfx/lib"

#back to the build.sh root folder
cd ../