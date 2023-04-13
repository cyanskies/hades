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
		constexpr inline typename Vector2::value_type offset(Vector2 position, Vector2 size) noexcept
		{
			return position.y * (size.x + position.x) + position.x;
		}
	}

	template<typename T>
	inline table<T>::table(const basic_table<T> &v, const value_type value) : table(v.position(), v.size(), value)
	{
		const auto size = v.size();
		const auto end = integer_cast<size_type>(size.x * size.y);
		for (auto i = size_type{}; i < end; ++i)
			set(i, v[i]);

		return;
	}

	template<typename Value>
	void table<Value>::set(index_type index, value_type v)
	{
		_data[_index(index)] = v;
		return;
	}

	template<typename Value>
	void table<Value>::set(size_type s, value_type v)
	{
		_data[_index(s)] = v;
		return;
	}

	template<typename Value>
	typename table<Value>::value_type table<Value>::operator[](const index_type index) const
	{
		return _data[_index(index)];
	}

	template<typename Value>
	typename std::vector<typename table<Value>::value_type>::reference
		table<Value>::operator[](const index_type index)
	{
		return _data[_index(index)];
	}


	template<typename Value>
	typename table<Value>::value_type table<Value>::operator[](const size_type index) const
	{
		return _data[_index(index)];
	}

	template<typename Value>
	typename std::vector<typename table<Value>::value_type>::reference
		table<Value>::operator[](const size_type index)
	{
		return _data[_index(index)];
	}

	template<typename T>
	inline std::size_t table<T>::_index(index_type index) const noexcept
	{
		const auto size = base_type::size();
        return integer_cast<std::size_t>(to_1d_index(index, size.x) - detail::offset(base_type::position(), size));
	}

	template<typename T>
	inline std::size_t table<T>::_index(size_type index) const noexcept
	{
		const auto size = base_type::size();
        return integer_cast<std::size_t>(index - detail::offset(base_type::position(), size));
	}

	template<typename TableFirst, typename TableSecond, typename CombineFunctor>
	auto combine_table(const TableFirst &l, const TableSecond &r, CombineFunctor&& f)
	{
        static_assert(std::is_invocable_v<CombineFunctor, const typename TableFirst::value_type&, const typename TableSecond::value_type&>);
		static_assert(std::is_convertible_v<std::invoke_result_t<CombineFunctor, const typename TableFirst::value_type&, const typename TableSecond::value_type&>, typename TableFirst::value_type>);

		const auto r_pos = r.position();
		const auto r_siz = r.size();
		const auto l_pos = l.position();
		const auto l_siz = l.size();

		//generate intersecting subrectangle
		using table_t = table<typename TableFirst::value_type>;
		auto area = rect_t<typename table_t::size_type>{};
		if (!intersect_area({l_pos, l_siz}, {r_pos, r_siz}, area))
			return table_t{ l , typename TableFirst::value_type{} };

		const auto stride = l_siz.x;
        const auto r_stride = integer_cast<std::size_t>(r_siz.x - area.width);
		const auto length = area.width; // width of copyable space
		const auto l_offset = l_pos.y * stride + l_pos.x;
		const auto start = area.y * stride + area.x - l_offset; //index of first space
        const auto jump = integer_cast<std::size_t>(l_siz.x - (area.x + area.width) + area.x - l_offset);// distance between length and the next region
        const auto end = integer_cast<std::size_t>(area.height * stride + start); //index of last space

		auto tab = table<typename TableFirst::value_type>{ l, typename TableFirst::value_type{} };
		auto &t = tab.data();
		const auto &tr = r.data();
		auto r_index = std::size_t{};
        auto index = integer_cast<std::size_t>(start);
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

	template<typename T, typename BinaryOp>
	inline hades::table_reduce_view<T, BinaryOp>::table_reduce_view(index_type pos,
		index_type size, value_type def, BinaryOp op)
		: base_type{ pos, size }, _default_val{ def },
		_op{ std::move(op) }
	{}

	template<typename T, typename BinaryOp>
	inline void table_reduce_view<T, BinaryOp>::add_table(const basic_table<T>& t)
	{
		assert(is_within(rect_t{ t.position(), t.size() }, rect_t{ base_type::position(), base_type::size() }));
		_tables.emplace_back(t);
		return;
	}

	template<typename T, typename BinaryOp>
	inline typename hades::table_reduce_view<T, BinaryOp>::value_type
		hades::table_reduce_view<T, BinaryOp>::operator[](index_type index) const
	{
		auto out = _default_val;
		for (const hades::basic_table<T>& t : _tables)
		{
			const auto pos = t.position();
			const auto size = t.size();
			const auto rect = hades::rect_t{ pos, size };
			if (is_within(index, rect))
				out = std::invoke(_op, out, t[index]);
		}
		return out;
	}

	template<typename T, typename BinaryOp>
	inline typename hades::table_reduce_view<T, BinaryOp>::value_type
		hades::table_reduce_view<T, BinaryOp>::operator[](size_type index) const
	{
		return operator[](to_2d_index<index_type>(index, this->size().x));
	}
}
