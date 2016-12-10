@call "%vS140COMNTOOLS%vsvars32"


@IF NOT DEFINED generator @set generator="Visual Studio 14 2015"
@IF NOT DEFINED mode @set mode=Debug
@IF NOT DEFINED mode2 @set mode2=debug
@IF NOT DEFINED library (@set library=./dep-library)

@setlocal
@IF [%1]==[] (@set prepath=./)	ELSE (@set prepath=../%1)
@set install-pfx=%prepath%%library%

@cd ./deps

@call build-install-dependencies.bat %prepath%

@cd ../hades

@cmake  -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX=../%install-pfx% -G %generator%

@msbuild ALL_BUILD.vcxproj /p:Configuration=%mode%
@msbuild INSTALL.vcxproj /p:Configuration=%mode%

@cd ../test

@cmake  -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX=../%install-pfx% -G %generator%

@msbuild ALL_BUILD.vcxproj /p:Configuration=%mode%
@msbuild INSTALL.vcxproj /p:Configuration=%mode%
