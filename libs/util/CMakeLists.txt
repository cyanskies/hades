include(../../cmake.txt)

hades_make_library(hades-util include " ")

set(HADES_UTIL_HEADERS
	./include/hades/any_map.hpp
	./include/hades/async.hpp
	./include/hades/collision_grid.hpp
	./include/hades/curve.hpp
	./include/hades/line_math.hpp
	./include/hades/math.hpp
	./include/hades/poly_math.hpp
	./include/hades/random.hpp
	./include/hades/rectangle_math.hpp
	./include/hades/string.hpp
	./include/hades/strong_typedef.hpp
	./include/hades/table.hpp
	./include/hades/time.hpp
	./include/hades/types.hpp
	./include/hades/tuple.hpp
	./include/hades/uniqueid.hpp
	./include/hades/utility.hpp
	./include/hades/value_guard.hpp
	./include/hades/vector_math.hpp
	./include/hades/zip.hpp
	./include/hades/detail/any_map.inl
	./include/hades/detail/collision_grid.inl
	./include/hades/detail/line_math.inl
	./include/hades/detail/math.inl
	./include/hades/detail/poly_math.inl
	./include/hades/detail/string.inl
	./include/hades/detail/rectangle_math.inl
	./include/hades/detail/table.inl
	./include/hades/detail/tuple.inl
	./include/hades/detail/utility.inl
	./include/hades/detail/vector_math.inl
	./include/hades/detail/zip.inl
)

target_sources(hades-util 
	PRIVATE
	./source/async.cpp
	./source/string.cpp
	./source/time.cpp
	PUBLIC FILE_SET headers TYPE HEADERS
	FILES "${HADES_UTIL_HEADERS}"
)
