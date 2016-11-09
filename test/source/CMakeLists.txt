# Directory xing/source

# C++ source files
set(XING_SRC
	ConsoleTestState.cpp
	main.cpp
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
else()
	set(SFML_MAIN_RELEASE)
	set(SFML_MAIN_DEBUG)
	set(GLEW GLEW)
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
	thor
	libyaml-cpp
	zlib
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
	thor-d
	libyaml-cppmdd
	zlibd
	)
endif()

# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${XING_EXE}
		RUNTIME DESTINATION bin)
