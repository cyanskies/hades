@call "%vS140COMNTOOLS%vsvars32"


@IF NOT DEFINED generator @set generator="Visual Studio 14 2015"
@IF NOT DEFINED mode @set mode=Debug
@IF NOT DEFINED mode2 @set mode2=debug
@IF NOT DEFINED library (@set library=./dep-library) ELSE (set library=../%library%)

setlocal
IF [%1]==[] (@set prepath=./ )	ELSE (@set prepath=../%1)

@cd ./deps

@call build-install-dependencies.bat %prepath%

@cd ..