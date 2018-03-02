add_subdirectory(sfml)

set(SFGUI_BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(SFML_STATIC_LIBRARIES ON CACHE BOOL "")

add_subdirectory(sfgui)

set(BUILD_GMOCK OFF CACHE BOOL "")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "")
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "")

add_subdirectory(yaml-cpp)
add_subdirectory(zlib)
