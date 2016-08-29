::run the build and install scripts for each dep
::@IF NOT DEFINED generator @set generator="Visual Studio 11"
::work around apparent bug in vs2012 when building from msbuild
::@IF NOT DEFINED VisualStudioVersion @set VisualStudioVersion=11.0
::@IF NOT DEFINED mode @set mode=Debug
::@IF NOT DEFINED mode2 @set mode2=debug
@set install-pfx=../../dep-library/

@cd ..

::SFML
@cd .\deps\sfml
@cmake -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX=%install-pfx% -G %generator%

@msbuild ALL_BUILD.vcxproj /p:Configuration=%mode%
@msbuild INSTALL.vcxproj /p:Configuration=%mode%

::Thor

@cd ..\thor
@cmake -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX=%install-pfx%  -G %generator%
@msbuild ALL_BUILD.vcxproj /p:Configuration=%mode%
@msbuild INSTALL.vcxproj /p:Configuration=%mode%

::Hades

@cd ..\hades
@cmake -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX=%install-pfx%  -G %generator%
@msbuild ALL_BUILD.vcxproj /p:Configuration=%mode%
@msbuild INSTALL.vcxproj /p:Configuration=%mode%


@cd ../../build-scripts