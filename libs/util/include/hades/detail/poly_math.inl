#include "hades/poly_math.hpp"

namespace hades::polygon
{
	template<typename T>
	constexpr polygon_quad_t<T> to_poly(const rect_t<T>& r) noexcept
	{
		return polygon_quad_t<T>{ {r.x, r.y}, {
			{0, 0},
			{r.width, 0},
			{0, r.height},
			{r.width, r.height}
		}};
	}

	template<typename T, std::size_t N>
	constexpr void set_origin(polygon_t<T, N>& p, vector_t<T> origin) noexcept
	{
		const auto dif = p.position - origin;
		for (auto& v : p.vertex)
			v += dif;
		p.position = origin;
		return;
	}
}
