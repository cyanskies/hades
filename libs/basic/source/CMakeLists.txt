# Directory hades/source

# C++ source files
set(HADES_BASIC_SRC
	data.cpp
	input.cpp
	../include/hades/collision.hpp
	../include/hades/data.hpp
	../include/hades/exceptions.hpp
	../include/hades/input.hpp
	../include/hades/resource_base.hpp
	../include/hades/detail/collision.inl
	../include/hades/detail/data.inl
	../include/hades/detail/input.inl
)

source_group("Headers" FILES
	../include/hades/collision.hpp
	../include/hades/data.hpp
	../include/hades/exceptions.hpp
	../include/hades/input.hpp
	../include/hades/resource_base.hpp
)

source_group("Detail" FILES
	../include/hades/detail/collision.inl
	../include/hades/detail/data.inl
	../include/hades/detail/input.inl
)

# Library to build
set(HADES_BASIC_LIB ${PROJECT_NAME})

# Determine library suffixes depending on static/shared configuration
add_library(${HADES_BASIC_LIB} STATIC ${HADES_BASIC_SRC})
set_target_properties(${HADES_BASIC_LIB} PROPERTIES DEBUG_POSTFIX -s-d)
set_target_properties(${HADES_BASIC_LIB} PROPERTIES RELEASE_POSTFIX -s)

target_include_directories(${HADES_BASIC_LIB} PUBLIC ../include)

target_link_libraries(${HADES_BASIC_LIB} PUBLIC hades-util)

# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${HADES_BASIC_LIB}
		RUNTIME DESTINATION lib
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)
