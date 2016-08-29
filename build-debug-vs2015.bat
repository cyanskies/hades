@cd build-scripts

@call "%vS140COMNTOOLS%vsvars32"
@set generator="Visual Studio 14 2015"
@set boosttoolset=msvc-14.0
@set mode=Debug
@set mode2=debug

@call build-install-dependencies.bat
@call build-install-hades-xing.bat

::copy deps back into source/debug so that the debug can be run without issue.
@XCOPY .\dep-library\bin\*.* .\xing\source\Debug\ /y

::Copy game data to xing/source so that the debug build can find it.

::@XCOPY .\game .\Xing\source\ /s /y