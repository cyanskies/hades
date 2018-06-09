# Directory hades/source

# C++ source files
set(HADES_CORE_SRC
	archive.cpp
	files.cpp
	standard_paths.cpp
	../include/hades/archive.hpp
	../include/hades/files.hpp
	../include/hades/standard_paths.hpp
)

source_group("Headers" FILES
	../include/hades/archive.hpp
	../include/hades/files.hpp
)

# Library to build
set(HADES_CORE_LIB ${PROJECT_NAME})

# Determine library suffixes depending on static/shared configuration
add_library(${HADES_CORE_LIB} STATIC ${HADES_CORE_SRC})
set_target_properties(${HADES_CORE_LIB} PROPERTIES DEBUG_POSTFIX -s-d)
set_target_properties(${HADES_CORE_LIB} PROPERTIES RELEASE_POSTFIX -s)

target_include_directories(${HADES_CORE_LIB} PUBLIC ../include)

target_link_libraries(${HADES_CORE_LIB} PUBLIC 
	hades-util
	hades-basic 
	sfml-audio
	sfml-graphics
	sfml-main
	sfml-network
	sfml-system
	sfml-window
	sfgui
	yaml-cpp
	minizip
)

# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${HADES_CORE_LIB}
		RUNTIME DESTINATION lib
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)
