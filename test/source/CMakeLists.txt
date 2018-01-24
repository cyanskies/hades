# Directory xing/source

# C++ source files
set(XING_SRC
	ConsoleTestState.cpp
	ExampleJob.cpp
	input_names.cpp
	main.cpp
	Snake.cpp
	snake_loader.cpp
)

# Library to build
set(XING_EXE ${PROJECT_NAME})

# Determine library suffixes depending on static/shared configuration
add_executable(${XING_EXE} ${XING_SRC})
set_target_properties(${XING_EXE} PROPERTIES DEBUG_POSTFIX -d)

if(WIN32)
	set(SFML_MAIN_RELEASE sfml-main)
	set(SFML_MAIN_DEBUG sfml-main-d)
	set(GLEW)
	set(YAML libyaml-cppmd)
	set(PTHREAD)
	set(GCCFS)
else()
	set(SFML_MAIN_RELEASE)
	set(SFML_MAIN_DEBUG)
	set(GLEW GLEW)
	set(YAML yaml-cpp)
	set(PTHREAD pthread)
# this won't be needed once gcc properly publish the non-experimental filesystem support
	set(GCCFS stdc++fs)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Release)
	target_link_libraries(${XING_EXE}
	hades-s
	${GLEW}
	sfml-system
	sfml-window
	sfml-graphics
	sfml-audio
	sfml-network
	${SFML_MAIN_RELEASE}
	tgui
	${YAML}
	zlibwapi
	${PTHREAD}
	${GCCFS}
	)
else()
	target_link_libraries(${XING_EXE}
	hades-s-d
	${GLEW}
	sfml-system-d
	sfml-window-d
	sfml-graphics-d
	sfml-audio-d
	sfml-network-d
	${SFML_MAIN_DEBUG}
	tgui-d
	${YAML}
	zlibwapi
	${PTHREAD}
	${GCCFS}
	)
endif()

# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${XING_EXE}
		RUNTIME DESTINATION bin)
