# Directory hades/source

# C++ source files
set(HADES_SRC
	App.cpp
	archive.cpp
	Bind.cpp
	Console.cpp
	ConsoleView.cpp
	DataManager.cpp
	data_manager.cpp
	files.cpp
	Logging.cpp
	main.cpp
	parallel_jobs.cpp
	Properties.cpp
	ResourceManager.cpp
	simple_resources.cpp
	StandardPaths.cpp
	State.cpp
	StateManager.cpp
	System.cpp
	TimerManager.cpp
	Types.cpp
)

# Library to build
set(HADES_LIB ${PROJECT_NAME})


# Determine library suffixes depending on static/shared configuration
add_library(${HADES_LIB} STATIC ${HADES_SRC})
set_target_properties(${HADES_LIB} PROPERTIES DEBUG_POSTFIX -s-d)
set_target_properties(${HADES_LIB} PROPERTIES RELEASE_POSTFIX -s)

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
	target_link_libraries(${HADES_LIB}
	${GLEW}
	sfml-system
	sfml-window
	sfml-graphics
	sfml-audio
	sfml-network
	${SFML_MAIN_RELEASE}
	thor
	tgui
	libyaml-cpp
	zlibwapi
	)
else()
	target_link_libraries(${HADES_LIB}
	${GLEW}
	sfml-system-d
	sfml-window-d
	sfml-graphics-d
	sfml-audio-d
	sfml-network-d
	${SFML_MAIN_DEBUG}
	thor-d
	tgui-d
	libyaml-cppmdd
	zlibwapi
	)
endif()

# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${HADES_LIB}
		RUNTIME DESTINATION lib
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)
