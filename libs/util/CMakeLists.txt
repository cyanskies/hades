include(../../cmake.txt)

set(HADES_UTIL_SRC
	source/time.cpp
	source/utility.cpp
	include/hades/any_map.hpp
	include/hades/curve.hpp
	include/hades/for_each_tuple.hpp
	include/hades/line_math.hpp
	include/hades/math.hpp
	include/hades/poly_math.hpp
	include/hades/rectangle_math.hpp
	include/hades/strong_typedef.hpp
	include/hades/table.hpp
	include/hades/time.hpp
	include/hades/types.hpp
	include/hades/uniqueid.hpp
	include/hades/utility.hpp
	include/hades/value_guard.hpp
	include/hades/vector_math.hpp
	include/hades/detail/any_map.inl
	include/hades/detail/for_each_tuple.inl
	include/hades/detail/line_math.inl
	include/hades/detail/math.inl
	include/hades/detail/poly_math.inl
	include/hades/detail/rectangle_math.inl
	include/hades/detail/table.inl
	include/hades/detail/utility.inl
	include/hades/detail/vector_math.inl
)

hades_make_library(hades-util include "${HADES_UTIL_SRC}")
