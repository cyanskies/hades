#ifndef HADES_UTIL_POLY_MATH_HPP
#define HADES_UTIL_POLY_MATH_HPP

#include <array>
#include <vector>

#include "hades/rectangle_math.hpp"
#include "hades/vector_math.hpp"

//#ref: https://www.youtube.com/watch?v=7Ik2vowGcU0

namespace hades
{
	// pass the desired vertex count as the template parameter N to avoid
	// dynamic allocation
	template<typename T, std::size_t N = 0>
	struct polygon_t
	{
		static_assert(N == 0 || N > 2, "polygon_t must have either 0 vertex(dynamic), or greater than 2 vertex");

		using value_type = T;
		static const bool static_size = std::bool_constant<N != 0>::value;

		//constructors
		constexpr polygon_t() noexcept = default;

		//construct dynamic polygon from any static polygon
		template<std::size_t N2>
		constexpr polygon_t(const polygon_t<T, N2>& rhs)
			: position{ rhs.position },
			vertex{ std::begin(rhs.vertex), std::end(rhs.vertex) }
		{
			static_assert(N == 0, "can only convert static polygons to a dynamic polygon");
		}

		vector_t<T> position;
		std::conditional_t<N == 0, 
			std::vector<vector_t<T>>,
			std::array<vector_t<T>, N>> vertex;
	};

	using polygon_int = polygon_t<int32>;
	using polygon_float = polygon_t<float>;

	template<typename T>
	using polygon_tri_t = polygon_t<T, 3>;
	using polygon_tri_float = polygon_tri_t<float>;

	template<typename T>
	using polygon_quad_t = polygon_t<T, 4>;
	using polygon_quad_float = polygon_quad_t<float>;

	static_assert(std::is_nothrow_constructible_v<polygon_int>);
	static_assert(std::is_trivial_v<polygon_tri_t<int32>>);

	template<typename T>
	struct is_polygon_t : std::false_type {};

	template<typename T, std::size_t N>
	struct is_polygon_t<polygon_t<T, N>> : std::true_type {};

	template<typename T>
	constexpr auto is_polygon_t_v = is_polygon_t<T>::value;

	namespace polygon
	{
		template<typename T>
		constexpr polygon_quad_t<T> to_poly(const rect_t<T>&) noexcept;

		//moves the position of the poly and translates the vertex
		//the total location of all vertex stays the same,
		//but the position will have moved to the location passed in
		template<typename T, std::size_t N>
		constexpr void set_origin(polygon_t<T, N>& p, vector_t<T> origin) noexcept;
	}
}

#include "hades/detail/poly_math.inl"

#endif // !HADES_UTIL_POLY_MATH_HPP
