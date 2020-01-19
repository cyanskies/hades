#ifndef HADES_UTIL_POLY_MATH_HPP
#define HADES_UTIL_POLY_MATH_HPP

#include <vector>

#include "hades/rectangle_math.hpp"
#include "hades/vector_math.hpp"

namespace hades
{
	template<typename T>
	struct polygon_t
	{
		using value_type = T;

		constexpr polygon_t() noexcept = default;
		constexpr polygon_t(std::vector<vector_t<T>> vertex) noexcept;
		constexpr polygon_t(vector_t<T> position, std::vector<vector_t<T>> vertex) noexcept;

		vector_t<T> position;
		std::vector<vector_t<T>> vertex;
	};

	static_assert(std::is_trivial_v<polygon_t<int32>>);
	
	using polygon_int = polygon_t<int32>;
	using polygon_float = polygon_t<float>;

	namespace polygon
	{
		template<typename T>
		constexpr polygon_t<T> to_poly(const rect_t<T>&) noexcept;

		template<typename T>
		constexpr void set_origin(polygon_t<T>& p, vector_t<T> origin) noexcept;

		//#ref: https://www.youtube.com/watch?v=7Ik2vowGcU0

		//moves the position of the poly and translates the vertex
		//the total location of all vertex stays the same,
		//but the position will have moved by the amount passed in
		//move_origin()
	}
}

#include "hades/detail/poly_math.inl"

#endif // !HADES_UTIL_POLY_MATH_HPP
