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

set(HADES_EXAMPLE_LIBS
	hades
	hades-editor
)

hades_make_exe(${PROJECT_NAME} "../include" "${HADES_EXAMPLE_SRC}" "${HADES_EXAMPLE_LIBS}")

