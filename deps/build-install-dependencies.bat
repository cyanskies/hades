@IF NOT DEFINED generator @set generator="Visual Studio 15 2017"
@IF NOT DEFINED mode @set mode=Debug
@IF NOT DEFINED mode2 @set mode2=debug
@IF NOT DEFINED library @set library=./dep-library

@setlocal
@IF [%1]==[] (@set prepath=./ )	ELSE (@set prepath=../%1)
@set install-pfx=%prepath%%library%

::SFML
@cd ./sfml
@cmake -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX=../%install-pfx% -G %generator%
@cmake --build . --target install --config %mode%

::TGUI
::depends on SFML

@cd ../tgui
@cmake -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX=../%install-pfx%  -G %generator%
@cmake --build . --target install --config %mode%

::yaml-cpp

@cd ../yaml-cpp
@cmake -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX=../%install-pfx% -DBUILD_GMOCK=false -DYAML_CPP_BUILD_TESTS=false -G %generator%
@cmake --build . --target install --config %mode%

::minizlib
@cd ..
@msbuild ./zlib/contrib/vstudio/vc15/zlibvc.vcxproj /p:Configuration=%mode%;WindowsTargetPlatformVersion=%WindowsSdkVersion%

::Copy lib and dll files

@IF %mode%==Debug @copy /Y ".\zlib\contrib\vstudio\vc15\x86\ZlibDllDebug\zlibwapi.dll" "%install-pfx%\bin\zlibwapi.dll" 
@IF %mode%==Debug @copy /Y ".\zlib\contrib\vstudio\vc15\x86\ZlibDllDebug\zlibwapi.lib" "%install-pfx%\lib\zlibwapi.lib"

@IF %mode%==Release @copy /Y ".\zlib\contrib\vstudio\vc15\x86\ZlibDllRelease\zlibwapi.dll" "%install-pfx%\bin\zlibwapi.dll"
@IF %mode%==Release @copy /Y ".\zlib\contrib\vstudio\vc15\x86\ZlibDllRelease\zlibwapi.lib" "%install-pfx%\lib\zlibwapi.lib"

::copy headers to install dir
@xcopy /Y ".\zlib\*.h" "%install-pfx%\include\zlib\"
@xcopy /Y ".\zlib\contrib\minizip\*.h" "%install-pfx%\include\zlib\"