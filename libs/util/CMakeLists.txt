# Directory hades

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

project(hades-util)

# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON) #we use variant
set(CMAKE_CXX_EXTENSIONS OFF)

#make visual studio debug the desired project by default
set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT hades)

# Global preprocessor options
#this is to ignore the thousands of warnings that
#yaml generates through iterator usage
add_definitions(-D_SILENCE_CXX17_ITERATOR_BASE_CLASS_DEPRECATION_WARNING)

# C++ source code
add_subdirectory(source)

# Install include directory and license files
install(DIRECTORY include
		DESTINATION .)
