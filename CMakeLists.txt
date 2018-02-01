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

#needed for proper static linkage to SFML
if(NOT BUILD_SHARED_LIBS)
    add_definitions(-DSFML_STATIC)
endif()

add_subdirectory(deps)
add_subdirectory(hades)

#if defined build example
add_subdirectory(test)