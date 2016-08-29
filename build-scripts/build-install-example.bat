::run the build and install scripts for each dep
@call "%vS110COMNTOOLS%vsvars32"

@IF NOT DEFINED generator @set generator="Visual Studio 11"
@IF NOT DEFINED VisualStudioVersion @set VisualStudioVersion=11.0
@IF NOT DEFINED mode @set mode=Debug
@set install-pfx=../dep-library/

::Hades
@cd ..\Hades
@cmake -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX="%install-pfx%" -G %generator%

@msbuild ALL_BUILD.vcxproj /p:Configuration=%mode%
@msbuild INSTALL.vcxproj /p:Configuration=%mode%

::Xing
@cd ..\Xing
@cmake -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX="%install-pfx%" -G %generator%

@msbuild ALL_BUILD.vcxproj /p:Configuration=%mode%
@msbuild INSTALL.vcxproj /p:Configuration=%mode%
::Game Assets
@cd ..

::XCOPY .\game\*.* .\Xing /s /y
