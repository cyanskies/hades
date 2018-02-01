add_subdirectory(sfml)

set(SFML_ROOT sfml)

add_subdirectory(tgui)

set(BUILD_GMOCK OFF CACHE BOOL "")
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "")

add_subdirectory(yaml-cpp)
add_subdirectory(zlib)
