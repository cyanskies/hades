# Directory Xing

cmake_minimum_required(VERSION 3.10)

# Specify default build type if none provided (before project() command)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/;${CMAKE_MODULE_PATH}")

# For generators with multiple configurations (like VS), only allow Debug and Release
if(CMAKE_CONFIGURATION_TYPES)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Limited configurations" FORCE)
	mark_as_advanced(CMAKE_CONFIGURATION_TYPES)
endif()

project(example)

# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) #we use variant
set(CMAKE_CXX_EXTENSIONS OFF)

# Global preprocessor and include options
include_directories(${CMAKE_SOURCE_DIR}/include ${CMAKE_INSTALL_PREFIX}/include)
add_definitions(-D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

# C++ source code
add_subdirectory(source)