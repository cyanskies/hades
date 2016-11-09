@call "%vS140COMNTOOLS%vsvars32"

@IF NOT DEFINED generator @set generator="Visual Studio 14 2015"
@IF NOT DEFINED mode @set mode=Debug
@IF NOT DEFINED mode2 @set mode2=debug
@IF NOT DEFINED library @set library=./dep-library

@setlocal
@IF [%1]==[] (@set prepath=./ )	ELSE (@set prepath=../%1)
@set install-pfx=%prepath%%library%

::SFML
@cd ./sfml
@cmake -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX=../%install-pfx% -G %generator%

@msbuild ALL_BUILD.vcxproj /p:Configuration=%mode%
@msbuild INSTALL.vcxproj /p:Configuration=%mode%

::Thor
::depends on SFML

@cd ../thor
@cmake -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX=../%install-pfx%  -G %generator%
@msbuild ALL_BUILD.vcxproj /p:Configuration=%mode%
@msbuild INSTALL.vcxproj /p:Configuration=%mode%

::yaml-cpp

@cd ../yaml-cpp
@cmake -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX=../%install-pfx%  -G %generator%
@msbuild ALL_BUILD.vcxproj /p:Configuration=%mode%
@msbuild INSTALL.vcxproj /p:Configuration=%mode%

::minizlib
@cd ..
@msbuild ./zlib/contrib/vstudio/vc11/zlibvc.vcxproj /p:Configuration=%mode%
@IF %mode%==Debug @copy /Y ".\zlib\contrib\vstudio\vc11\x86\ZlibDllDebug\zlibwapi.dll" "%install-pfx%\bin\zlibwapid.dll" 
@IF %mode%==Debug @copy /Y ".\zlib\contrib\vstudio\vc11\x86\ZlibDllDebug\zlibwapi.lib" "%install-pfx%\lib\zlibwapid.lib"

@IF NOT %mode%==Debug @copy /Y ".\zlib\contrib\vstudio\vc11\x86\ZlibDllRelease\zlibwapi.dll" %install-pfx%\bin\zlibwapi.dll
@IF NOT %mode%==Debug @copy /Y ".\zlib\contrib\vstudio\vc11\x86\ZlibDllRelease\zlibwapi.lib" %install-pfx%\lib\zlibwapi.lib