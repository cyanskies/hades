@IF NOT DEFINED generator @set generator="Visual Studio 15 2017"
@IF NOT DEFINED mode @set mode=Debug
@IF NOT DEFINED library (@set library=./dep-library)

@cmake -DCMAKE_BUILD_TYPE=%mode% -DCMAKE_INSTALL_PREFIX=%library% -G %generator%
@cmake --build . --target install --config %mode%
