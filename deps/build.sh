generator=$1
mode=$2
library=$3
prepath=../$4

if [-z $generator] then
generator="Unix Makefiles"
fi

if [-z $mode] then
mode="Debug"
fi

if [-z $library] then
library="./dep-library"
fi

install-pfx=$prepath$library

#SFML
cd ./sfml
cmake -DCMAKE_BUILD_TYPE=$mode -DCMAKE_INSTALL_PREFIX=../$install-pfx -G $generator
cmake --build . --target install --config $mode

#TGUI
#depends on SFML

cd ../tgui
cmake -DCMAKE_BUILD_TYPE=$mode -DCMAKE_INSTALL_PREFIX=../$install-pfx -G $generator
cmake --build . --target install --config $mode

#yaml-cpp

cd ../yaml-cpp
cmake -DCMAKE_BUILD_TYPE=$mode -DCMAKE_INSTALL_PREFIX=../$install-pfx -G $generator
cmake --build . --target install --config $mode

#minizlib
cd ../zlib/
./configure
make
cd ../zlib/contrib
make

#Copy lib and dll files

@IF %mode%==Debug @copy /Y ".\zlib\contrib\vstudio\vc15\x86\ZlibDllDebug\zlibwapi.dll" "%install-pfx%\bin\zlibwapi.dll" 
@IF %mode%==Debug @copy /Y ".\zlib\contrib\vstudio\vc15\x86\ZlibDllDebug\zlibwapi.lib" "%install-pfx%\lib\zlibwapi.lib"

@IF %mode%==Release @copy /Y ".\zlib\contrib\vstudio\vc15\x86\ZlibDllRelease\zlibwapi.dll" "%install-pfx%\bin\zlibwapi.dll"
@IF %mode%==Release @copy /Y ".\zlib\contrib\vstudio\vc15\x86\ZlibDllRelease\zlibwapi.lib" "%install-pfx%\lib\zlibwapi.lib"

#copy headers to install dir
@xcopy /Y ".\zlib\*.h" "%install-pfx%\include\zlib\"
@xcopy /Y ".\zlib\contrib\minizip\*.h" "%install-pfx%\include\zlib\"