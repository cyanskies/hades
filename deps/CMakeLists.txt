add_subdirectory(sfml)

set(TGUI_SHARED_LIBS OFF CACHE BOOL "")
#needed for proper static linkage from TGUI to SFML
if(NOT BUILD_SHARED_LIBS)
    add_definitions(-DSFML_STATIC)
endif()

add_subdirectory(tgui)

set(BUILD_GMOCK OFF CACHE BOOL "")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "")
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "")

add_subdirectory(yaml-cpp)
add_subdirectory(zlib)
