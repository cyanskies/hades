add_subdirectory(sfml)

set(BUILD_GMOCK OFF CACHE BOOL "")
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "")
set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "")

add_subdirectory(yaml-cpp)
#add preprocessor define for yaml version
get_target_property(HADES_YAML_VERSION yaml-cpp VERSION)
target_compile_definitions(yaml-cpp INTERFACE HADES_YAML_VERSION="${HADES_YAML_VERSION}")

add_subdirectory(zlib EXCLUDE_FROM_ALL)
#fix for out of tree builds, zconf.h would only appear in #include <zconf.h>
#but zlib tries to find include "zconf.h"
#target_include_directories(zlib PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/zlib)
target_include_directories(zlibstatic PUBLIC ${CMAKE_CURRENT_BINARY_DIR}/zlib)
add_subdirectory(zlib/contrib/minizip)

add_subdirectory(plf_colony)

#########################
#cmake for imgui
#########################

#version 3.12 needed for CXX_STANDARD settings
cmake_minimum_required(VERSION 3.12)

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
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON) #we use variant
set(CMAKE_CXX_EXTENSIONS OFF)

# C++ source files
set(IMGUI_SRC
	./imconfig.hpp
	./imgui/imconfig.h
	./imgui/imgui.cpp
	./imgui/imgui.h
	./imgui/imgui.h
	./imgui/imgui_demo.cpp
	./imgui/imgui_draw.cpp
	./imgui/imgui_internal.h
	./imgui/imgui_tables.cpp
	./imgui/imgui_widgets.cpp
	./imgui/imstb_rectpack.h
	./imgui/imstb_textedit.h
	./imgui/imstb_truetype.h
	./imgui/misc/cpp/imgui_stdlib.cpp
	./imgui/misc/cpp/imgui_stdlib.h
)

# Library to build
set(IMGUI_LIB ${PROJECT_NAME})

# Determine library suffixes depending on static/shared configuration
add_library(${IMGUI_LIB} STATIC ${IMGUI_SRC})
set_target_properties(${IMGUI_LIB} PROPERTIES DEBUG_POSTFIX -s-d)
set_target_properties(${IMGUI_LIB} PROPERTIES RELEASE_POSTFIX -s)

target_include_directories(${IMGUI_LIB} PUBLIC ./imgui)
target_compile_definitions(${IMGUI_LIB} PUBLIC IMGUI_USER_CONFIG="../imconfig.hpp")


# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${IMGUI_LIB}
		RUNTIME DESTINATION lib
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)

#########################
# cmake for file dialogue
#########################

project(imguifiledialog)

# https://cmake.org/cmake/help/latest/prop_tgt/CXX_STANDARD.html
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON) #we use variant
set(CMAKE_CXX_EXTENSIONS OFF)

# C++ source files
set(FILEDIALOG_SRC
	./imguifiledialog/ImGuiFileDialog.h
	./imguifiledialog/ImGuiFileDialog.cpp
	./imguifiledialog/stb/stb_image.h
	./imguifiledialog/stb/stb_image_resize.h
	./imguifiledialog/dirent/dirent.h
	./ImGuiFileDialogConfig.h
)

# Library to build
set(FILEDIALOG_LIB ${PROJECT_NAME})

# Determine library suffixes depending on static/shared configuration
add_library(${FILEDIALOG_LIB} STATIC ${FILEDIALOG_SRC})
target_compile_definitions(${FILEDIALOG_LIB} PUBLIC CUSTOM_IMGUIFILEDIALOG_CONFIG="../ImGuiFileDialogConfig.h")

set_target_properties(${FILEDIALOG_LIB} PROPERTIES DEBUG_POSTFIX -s-d)
set_target_properties(${FILEDIALOG_LIB} PROPERTIES RELEASE_POSTFIX -s)

target_include_directories(${FILEDIALOG_LIB} PUBLIC ./imguifiledialog)
target_link_libraries(${FILEDIALOG_LIB} PUBLIC imgui)

# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${FILEDIALOG_LIB}
		RUNTIME DESTINATION lib
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)

#########################
# hide unused targets
#########################

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_target_properties(
	example #from zlib
	minigzip #zib example
	zlib
	# yaml-cpp CI
	Continuous
	Experimental
	Nightly
	NightlyMemoryCheck
	# end yaml-cpp CI
	PROPERTIES FOLDER Unused)
