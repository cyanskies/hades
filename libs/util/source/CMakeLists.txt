# Directory hades/source

# C++ source files
set(HADES_UTIL_SRC
	parallel_jobs.cpp
	time.cpp
	transaction.cpp
	utility.cpp
	../include/hades/any_map.hpp
	../include/hades/curve.hpp
	../include/hades/for_each_tuple.hpp
	../include/hades/line_math.hpp
	../include/hades/math.hpp
	../include/hades/parallel_jobs.hpp
	../include/hades/poly_math.hpp
	../include/hades/rectangle_math.hpp
	../include/hades/shared_any_map.hpp
	../include/hades/shared_guard.hpp
	../include/hades/shared_map.hpp
	../include/hades/strong_typedef.hpp
	../include/hades/table.hpp
	../include/hades/time.hpp
	../include/hades/transaction.hpp
	../include/hades/transactional.hpp
	../include/hades/types.hpp
	../include/hades/uniqueid.hpp
	../include/hades/utility.hpp
	../include/hades/value_guard.hpp
	../include/hades/vector_math.hpp
	../include/hades/detail/any_map.inl
	../include/hades/detail/for_each_tuple.inl
	../include/hades/detail/line_math.inl
	../include/hades/detail/math.inl
	../include/hades/detail/parallel_jobs.inl
	../include/hades/detail/poly_math.inl
	../include/hades/detail/rectangle_math.inl
	../include/hades/detail/shared_any_map.inl
	../include/hades/detail/shared_map.inl
	../include/hades/detail/table.inl
	../include/hades/detail/transaction.inl
	../include/hades/detail/utility.inl
	../include/hades/detail/vector_math.inl
)

# Library to build
set(HADES_UTIL_LIB ${PROJECT_NAME})

# Determine library suffixes depending on static/shared configuration
add_library(${HADES_UTIL_LIB} STATIC ${HADES_UTIL_SRC})
set_target_properties(${HADES_UTIL_LIB} PROPERTIES DEBUG_POSTFIX -s-d)
set_target_properties(${HADES_UTIL_LIB} PROPERTIES RELEASE_POSTFIX -s)

target_include_directories(${HADES_UTIL_LIB} PUBLIC ../include)

# Add compiler flags for extra warnings
target_compile_options(${HADES_UTIL_LIB} PRIVATE
                           # clang/GCC warnings
                           $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:GNU>>:
                                "-Wall -pedantic-errors -Wextra -Wconversion -Wsign-conversion" >
                           # MSVC warnings
                           $<$<CXX_COMPILER_ID:MSVC>:
                                "/W4" >)

# Install (use "bin" folder for DLL, "lib" for SO files)
install(TARGETS ${HADES_UTIL_LIB}
		RUNTIME DESTINATION lib
		LIBRARY DESTINATION lib
		ARCHIVE DESTINATION lib)
