#include "hades/table.hpp"

#include <cassert>
#include <optional>
#include <vector>

#include "hades/rectangle_math.hpp"

namespace hades
{
	template<typename T>
	inline table<T>::table(const virtual_table<T> &v) : table(v.position(), v.size(), T{})
	{
		const auto size = index_type{ _width, integer_cast<index_type::value_type>(_data.size()) / _width };
		const auto end = _offset + size;

		for (auto y = _offset.y; y < end.y; ++y)
			for (auto x = _offset.x; x < end.x; ++x)
				(*this)[index_type{x, y}] = v[index_type{x, y}];
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
		const auto r_pos = r.position();
		const auto r_siz = r.size();
		const auto l_pos = l.position();
		const auto l_siz = l.size();

		//generate intersecting subrectangle
		using table_t = table<typename TableFirst::value_type>;
		auto area = rect_t<table_t::size_type>{};
		if (!intersect_area({l_pos, l_siz}, {r_pos, r_siz}, area))
			return table_t{ l };

		const auto stride = l_siz.x;
		const auto r_stride = r_siz.x - area.width;
		const auto length = area.width; // width of copyable space
		const auto l_offset = l_pos.y * stride + l_pos.x;
		const auto start = area.y * stride + area.x - l_offset; //index of first space
		const auto jump = l_siz.x - (area.x + area.width) + area.x - l_offset;// distance between length and the next region
		const auto end = area.height * stride + start; //index of last space

		auto tab = table<typename TableFirst::value_type>{ l };
		auto &t = tab.data();
		const auto &tr = r.data();
		auto r_index = std::size_t{};
		auto index = start;
		while (index < end)
		{
			auto count = table_t::size_type{};
			while (count < length)
			{
				const auto i = index;
				t[i] = std::invoke(f, t[i], tr[r_index]);
				++index; ++r_index; ++count;
			}

			index += jump;
			r_index += r_stride;
		}

		return tab;
	}
}