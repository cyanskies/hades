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
		constexpr inline typename Vector2::value_type offset(Vector2 position, typename Vector2::value_type size) noexcept
		{
			//same as to_1d_index, but we don't prevent negative calculations
			return position.y * size + position.x;
		}
	}

	template<typename T>
	inline std::size_t basic_table<T>::_index(index_type index) const noexcept
	{
		return integer_cast<std::size_t>(to_1d_index(index, _size.x) - detail::offset(_offset, _size.x));
	}

	template<typename T>
	inline std::size_t basic_table<T>::_index(size_type index) const noexcept
	{
		return integer_cast<std::size_t>(index - detail::offset(_offset, _size.x));
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
		_data[base_type::_index(index)] = v;
		return;
	}

	template<typename Value>
	void table<Value>::set(size_type s, value_type v)
	{
		_data[base_type::_index(s)] = v;
		return;
	}

	template<typename Value>
	typename table<Value>::value_type table<Value>::operator[](const index_type index) const
	{
		return _data[base_type::_index(index)];
	}

	template<typename Value>
	typename std::vector<typename table<Value>::value_type>::reference
		table<Value>::operator[](const index_type index)
	{
		return _data[base_type::_index(index)];
	}

	template<typename Value>
	typename table<Value>::value_type table<Value>::operator[](const size_type index) const
	{
		return _data[base_type::_index(index)];
	}

	template<typename Value>
	typename std::vector<typename table<Value>::value_type>::reference
		table<Value>::operator[](const size_type index)
	{
		return _data[base_type::_index(index)];
	}

	template<typename Value>
	typename table_view<Value>::value_type table_view<Value>::operator[](const index_type index) const
	{
		return _data[base_type::_index(index)];
	}

	template<typename Value>
	typename table_view<Value>::value_type table_view<Value>::operator[](const size_type index) const
	{
		return _data[base_type::_index(index)];
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
		return operator[](to_2d_index<index_type>(index, base_type::size().x));
	}
}
