#include "hades/table.hpp"

#include <cassert>
#include <optional>
#include <vector>

#include "hades/math.hpp"

namespace hades
{
	template<typename T>
	inline table<T>::table(const virtual_table<T> &v) : table(v.position(), v.size(), T{})
	{
		const auto size = { _width, _data.size() / _width };
		const auto end = _position + size;

		for (auto y = _offset.y; sy < end.y; ++y)
			for (auto x = _offset.x; sx < end.x; ++x)
				_data[{x, y}] = v[{x, y}];
	}

	template<typename Value>
	typename table<Value>::value_type& table<Value>::operator[](index_type index)
	{
		return _data[index.y * index.x + index.x];
	}

	template<typename Value>
	typename const table<Value>::value_type& table<Value>::operator[](index_type index) const
	{
		return _data[index.y * index.x + index.x];
	}

	template<typename TableFirst, typename TableSecond, typename CombineFunctor>
	auto combine_table(const TableFirst &l, const TableSecond &r, CombineFunctor f)
	{
		auto t = table<typename TableFirst::value_type>{ l };

		const auto first_pos = l.position();
		const auto first_siz = l.size();

		const auto bounds = 
			rect_t<TableFirst::size_type>{
			first_pos.x, first_pos.y, first_siz.x, first_siz.y
		};

		const auto pos = r.position();
		const auto siz = r.size();
		const auto end = pos + siz;

		for (auto y = pos.y; y < end.y; ++y)
			for (auto x = pos.x; x < end.x; ++x)
				if (intersects(bounds, { x, y, 0, 0 }))
					t[{x, y}] = std::invoke(f, t[{x, y}], r[{x, y}]);

		return t;
	}
}