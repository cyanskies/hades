#include "hades/table.hpp"

#include <cassert>
#include <optional>
#include <vector>

#include "hades/rectangle_math.hpp"
#include "hades/utility.hpp"

namespace hades
{
	namespace detail
	{
		//returns the 1d position of this rect in the global space
		template<typename Vector2>
		static inline typename Vector2::value_type offset(Vector2 position, Vector2 size)
		{
			return position.y * (size.x + position.x) + position.x;
		}
	}

	template<typename Value>
	inline typename virtual_table<Value>::value_type virtual_table<Value>::operator[](index_type i)
	{
		return operator[](to_1d_index(i, _size.x) - detail::offset(_offset, _size));
	}

	template<typename T>
	inline table<T>::table(const virtual_table<T> &v) : table(v.position(), v.size(), T{})
	{
		const auto end = std::size(_data);
		for (auto i = std::size_t{}; i != end; ++i)
			_data[i] = v[integer_cast<virtual_table<T>::size_type>(i)];
	}

	template<typename Value>
	typename table<Value>::value_type& table<Value>::operator[](index_type index)
	{
		const auto size = to_2d_index<index_type>(std::size(_data), _width);
		return _data[to_1d_index(index, size.x) - detail::offset(_offset, size)];
	}

	template<typename Value>
	typename const table<Value>::value_type& table<Value>::operator[](index_type index) const
	{
		const auto size = to_2d_index<index_type>(std::size(_data), _width);
		return _data[to_1d_index(index, size.x) - detail::offset(_offset, size)];
	}

	template<typename Value>
	typename table<Value>::value_type& table<Value>::operator[](size_type index)
	{
		assert(index >= 0 && integer_cast<std::size_t>(index) < std::size(_data));
		return _data[index];
	}

	template<typename Value>
	typename const table<Value>::value_type& table<Value>::operator[](size_type index) const
	{
		assert(index >= 0 && integer_cast<std::size_t>(index) < std::size(_data));
		return _data[index];
	}

	template<typename TableFirst, typename TableSecond, typename CombineFunctor>
	auto combine_table(const TableFirst &l, const TableSecond &r, CombineFunctor f)
	{
		static_assert(std::is_invocable_v<CombineFunctor, const TableFirst::value_type&, const TableSecond::value_type&>);
		static_assert(std::is_convertible_v<std::invoke_result_t<CombineFunctor, const TableFirst::value_type&, const TableSecond::value_type&>, TableFirst::value_type>);

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
			auto count = typename table_t::size_type{};
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