@if not defined vsdir (
	for /f "usebackq tokens=*" %%i in (`.\build-scripts\vswhere.exe -latest -version "[15.0,16.0)" -requires Microsoft.Component.MSBuild -property installationPath`) do (
  	set vsdir=%%i
	)
)

@call "%vsdir%\Common7\Tools\VsDevCmd.bat"

@IF NOT DEFINED generator @set generator="Visual Studio 15 2017"
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

::@cmake --build . --config %mode%
::@msbuild ALL_BUILD.vcxproj /p:Configuration=%mode%
@cmake --build . --target install --config %mode%
:: target install will automatically build if needed, then run install
::@msbuild INSTALL.vcxproj /p:Configuration=%mode%

@cd ../test

@cmake  -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX=../%install-pfx% -G %generator%
@cmake --build . --target install --config %mode%
