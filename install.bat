@IF NOT DEFINED mode @set mode=Debug

@call build.bat
@cmake --build . --target install --config %mode%
