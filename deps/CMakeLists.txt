add_subdirectory(sfml)

set(SFML_ROOT sfml)

#add_subdirectory(tgui)

set(BUILD_GMOCK OFF)
set(YAML_CPP_BUILD_TESTS OFF)

add_subdirectory(yaml-cpp)
add_subdirectory(zlib)
