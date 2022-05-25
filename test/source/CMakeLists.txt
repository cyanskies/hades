# Directory xing/source

# C++ source files
set(HADES_EXAMPLE_SRC
	ConsoleTestState.cpp
	ExampleJob.cpp
	gui_state.cpp
	input_names.cpp
	main.cpp
	simple_instance_state.cpp
)

# Library to build
set(EXAMPLE_EXE ${PROJECT_NAME})

# Determine library suffixes depending on static/shared configuration
add_executable(${EXAMPLE_EXE} ${HADES_EXAMPLE_SRC})
set_target_properties(${EXAMPLE_EXE} PROPERTIES DEBUG_POSTFIX -d)

target_include_directories(${EXAMPLE_EXE} PRIVATE ../include)

set(HADES_EXAMPLE_LIBS
	hades
	hades-editor
)

target_link_libraries(${EXAMPLE_EXE} "${HADES_EXAMPLE_LIBS}")


# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${EXAMPLE_EXE}
		RUNTIME DESTINATION bin)
