add_subdirectory(sfml)

set(BUILD_GMOCK OFF CACHE BOOL "")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "")
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "")

add_subdirectory(yaml-cpp)
add_subdirectory(zlib)

#########################
#cmake for imgui
#########################

#version 3.10 needed for CXX_STANDARD settings
cmake_minimum_required(VERSION 3.10)

# Specify default build type if none provided (before project() command)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

# For generators with multiple configurations (like VS), only allow Debug and Release
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Limited configurations" FORCE)
	mark_as_advanced(CMAKE_CONFIGURATION_TYPES)
endif()

project(imgui)

# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) #we use variant
set(CMAKE_CXX_EXTENSIONS OFF)

# C++ source files
set(IMGUI_SRC
	./imgui/imconfig.h
	./imgui/imgui.cpp
	./imgui/imgui.h
	./imgui/imgui.h
	./imgui/imgui_demo.cpp
	./imgui/imgui_draw.cpp
	./imgui/imgui_internal.h
	./imgui/imgui_widgets.cpp
	./imgui/imstb_rectpack.h
	./imgui/imstb_textedit.h
	./imgui/imstb_truetype.h
)

# Library to build
set(IMGUI_LIB ${PROJECT_NAME})

# Determine library suffixes depending on static/shared configuration
add_library(${IMGUI_LIB} STATIC ${IMGUI_SRC})
set_target_properties(${IMGUI_LIB} PROPERTIES DEBUG_POSTFIX -s-d)
set_target_properties(${IMGUI_LIB} PROPERTIES RELEASE_POSTFIX -s)

target_include_directories(${IMGUI_LIB} PUBLIC ./imgui)

# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${IMGUI_LIB}
		RUNTIME DESTINATION lib
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)