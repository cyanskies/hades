#include "hades/poly_math.hpp"

namespace hades
{
	template<typename T>
	inline constexpr polygon_t<T>::polygon_t(std::vector<vector_t<T>> v) noexcept 
		: _vertex{std::move(v)}
	{}

	template<typename T>
	inline constexpr polygon_t<T>::polygon_t(vector_t<T> p, std::vector<vector_t<T>> v) noexcept
		: _position{p}, polygon_t{std::move(v)}
	{}
}

namespace hades::polygon
{
	template<typename T>
	constexpr polygon_t<T> to_poly(const rect_t<T>& r) noexcept
	{
		return polygon_t<T>{ {r.x, r.y}, {
			{0, 0},
			{r.width, 0},
			{0, r.height},
			{r.width, r.height}
		}};
	}

	template<typename T>
	constexpr void set_origin(polygon_t<T>& p, vector_t<T> origin) noexcept
	{
		const auto dif = p.position - origin;
		for (auto& v : p.vertex)
			v += dif;
		p.position = origin;
		return;
	}
}
