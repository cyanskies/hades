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

		/// @brief generate a vector containing all the 2d index locations within the provided area
		inline std::vector<table_index_t> all_indexes(table_index_t pos, table_index_t size)
		{
			auto positions = std::vector<table_index_t>{};
			positions.reserve(size.x * size.y);
			using index_t = typename table_index_t::value_type;
			for (index_t y = 0; y < size.y; ++y)
				for (index_t x = 0; x < size.x; ++x)
					positions.emplace_back(table_index_t{ pos.x + x, pos.y + y });

			return positions;
		}
	}

	template<typename T>
	inline table<T>::table(const basic_table<T> &v) : table(v.position(), v.size(), T{})
	{
		const auto pos = v.position();
		const auto size = v.size();
		
		const auto indexes = detail::all_indexes(pos, size);
		for (auto i : indexes)
			operator[](i) = v[i];

		return;
	}

	template<typename Value>
	typename table<Value>::value_type table<Value>::operator[](const index_type index) const
	{
		const auto size = base_type::size();
		return _data[to_1d_index(index, size.x) - detail::offset(base_type::position(), size)];
	}

	template<typename Value>
	typename table<Value>::value_type& table<Value>::operator[](const index_type index)
	{
		const auto size = base_type::size();
		return _data[to_1d_index(index, size.x) - detail::offset(base_type::position(), size)];
	}

	template<typename Value>
	typename table<Value>::value_type table<Value>::operator[](const size_type index) const
	{
		const auto size = base_type::size();
		return _data[index - detail::offset(base_type::position(), size)];
	}

	template<typename Value>
	typename table<Value>::value_type& table<Value>::operator[](const size_type index)
	{
		const auto size = base_type::size();
		return _data[index - detail::offset(base_type::position(), size)];
	}

	template<typename TableFirst, typename TableSecond, typename CombineFunctor>
	auto combine_table(const TableFirst &l, const TableSecond &r, CombineFunctor&& f)
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

	template<typename T, typename BinaryOp>
	inline hades::table_reduce_view<T, BinaryOp>::table_reduce_view(index_type pos,
		index_type size, value_type def, std::initializer_list<const basic_table<T>&> list,
		BinaryOp op) : base_type{ pos, size }, _tables{ list }, _default_val{ def },
		_op{ std::move(op) }
	{}

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
			if (hades::is_within(index, rect))
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
