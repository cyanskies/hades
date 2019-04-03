@IF NOT DEFINED generator @set generator="Visual Studio 16 2019"
@IF NOT DEFINED mode @set mode=Debug
@IF NOT DEFINED library (@set library=./dep-library)

@cmake -DCMAKE_BUILD_TYPE=%mode% -DBUILD_SHARED_LIBS=OFF -DCMAKE_INSTALL_PREFIX=%library% -G %generator% -A x32