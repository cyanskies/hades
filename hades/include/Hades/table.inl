#include "table.hpp"

#include <cassert>
#include <vector>

namespace hades
{
	namespace {

		//returns a table large enough to hold both of the arguments
		template<typename Value, typename SizeType = std::vector<Value>::size_type>
		table<Value, SizeType> superset(const table<Value, SizeType> &lhs, const table<Value, SizeType> &rhs)
		{
			auto lpos = lhs.position(), rpos = rhs.position();
			auto minx = std::min(lpos.x, rpos.y),
				miny = std::min(lpos.y, rpos.y);

			auto lmax = lpos + lhs.size(), rmax = rpos + rhs.size();

			auto maxx = std::max(lmax.x, rmax.x),
				maxy = std::max(lmax.y, rmax.y);

			table<Value, SizeType>::index_type size{ maxx - minx, maxy - miny }, position{ minx, miny };

			return table<Value, SizeType>(size, position);
		}

		template<typename Value, typename SizeType = std::vector<Value>::size_type>
		table<Value, SizeType> as_table(const virtual_table<Value, SizeType> &v)
		{
			table<Value, SizeType> t(v.size(), v.position());

			auto start = v.position();
			auto end = start + v.size();

			for (auto y = start.y; y < end.y; ++y)
				for (auto x = start.x; x < end.x; ++x)
					t[{x, y}] += v[{x, y}];

			return t;
		}
	}

	template<typename Index>
	table_index<Index> operator+(table_index<Index> lhs, const table_index<Index> &rhs)
	{
		return lhs += rhs;
	}

	template<typename Index>
	table_index<Index>& operator+=(table_index<Index> &lhs, const table_index<Index> &rhs)
	{
		lhs.x += rhs.x;
		lhs.y += rhs.y;

		return lhs;
	}

	template<typename Index>
	table_index<Index> operator-(table_index<Index> lhs, const table_index<Index> &rhs)
	{
		return lhs -= rhs;
	}

	template<typename Index>
	table_index<Index>& operator-=(table_index<Index> &lhs, const table_index<Index> &rhs)
	{
		lhs.x -= rhs.x;
		lhs.y -= rhs.y;

		return lhs;
	}

	template<typename Value, typename SizeType>
	table<Value, SizeType> table<Value, SizeType>::get_subset(index_type offset, index_type size) const
	{
		table<Value, SizeType> t(offset, size);

		auto start = offset;
		auto end = start + size;

		for (auto ty = 0, sy = start.y; sy < end.y; ++ty, ++sy)
			for (auto tx = 0, sx = start.x; sx < end.x; ++tx, ++sx)
				t[{tx, ty}] += this->[{sx, sy}];

		return t;
	}
	template<typename Value, typename SizeType>
	typename table<Value, SizeType>::value_type& table<Value, SizeType>::operator[](index_type index)
	{
		return _data[index.y * index.x + index.x];
	}

	template<typename Value, typename SizeType>
	typename const table<Value, SizeType>::value_type& table<Value, SizeType>::operator[](index_type index) const
	{
		return _data[index.y * index.x + index.x];
	}

	template<typename Value, typename SizeType>
	void table<Value, SizeType>::clear()
	{
		_data.clear();
	}

	template<typename Value, typename SizeType>
	table<Value, SizeType> operator+(table<Value, SizeType> lhs, const table<Value, SizeType> &rhs)
	{
		return lhs += rhs;
	}

	template<typename Value, typename SizeType>
	table<Value, SizeType>& operator+=(table<Value, SizeType> &lhs, const table<Value, SizeType> &rhs)
	{
		//get a table that covers the area for both tables
		auto t = superset(lhs, rhs);

		//add all the data from lhs to t
		auto start = lhs.position();
		auto end = start + lhs.size();

		for (auto y = start.y; y < end.y; ++y)
			for (auto x = start.x; x < end.x; ++x)
				t[{x, y}] += rhs[{x, y}];

		//add all the data from rhs to t
		start = rhs.position();
		end = start + rhs.size();

		for (auto y = start.y; y < end.y; ++y)
			for (auto x = start.x; x < end.x; ++x)
				t[{x, y}] += rhs[{x, y}];

		std::swap(t, lhs);

		return lhs;
	}

	template<typename Value, typename SizeType>
	table<Value, SizeType> operator+(table<Value, SizeType> lhs, const virtual_table<Value, SizeType> &rhs)
	{
		return lhs += rhs;
	}

	template<typename Value, typename SizeType>
	table<Value, SizeType>& operator+=(table<Value, SizeType> &lhs, const virtual_table<Value, SizeType> &rhs)
	{
		auto t = as_table(rhs);

		return lhs += t;
	}
}