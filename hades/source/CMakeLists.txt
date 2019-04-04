# Directory hades/source

# C++ source files
set(HADES_SRC
	App.cpp
	Debug.cpp
	Console.cpp
	ConsoleView.cpp
	data_system.cpp
	main.cpp
	mission.cpp
	RenderInstance.cpp
	Server.cpp
	simple_resources.cpp
	StateManager.cpp
	yaml_parser.cpp
	yaml_writer.cpp
)

# Library to build
set(HADES_LIB ${PROJECT_NAME})

# Determine library suffixes depending on static/shared configuration
add_library(${HADES_LIB} STATIC ${HADES_SRC})
set_target_properties(${HADES_LIB} PROPERTIES DEBUG_POSTFIX -s-d)
set_target_properties(${HADES_LIB} PROPERTIES RELEASE_POSTFIX -s)

target_include_directories(${HADES_LIB} PUBLIC ../include)

#lin libraries
target_link_libraries(${HADES_LIB} PUBLIC hades-core yaml-cpp)

# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${HADES_LIB}
		RUNTIME DESTINATION lib
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)
