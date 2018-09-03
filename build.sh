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

if [ ! -d "$library" ]; then
mkdir "$library"
fi

cmake -DCMAKE_BUILD_TYPE="$mode" -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX="../$library" -G "$generator"

