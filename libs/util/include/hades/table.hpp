#ifndef HADES_UTIL_TABLE_HPP
#define HADES_UTIL_TABLE_HPP

#include <functional>
#include <type_traits>
#include <vector>

#include "hades/utility.hpp"
#include "hades/vector_math.hpp"

//a type representing an arbitarily sized table
//supports adding the contents of tables together

//intended usage is for pathfinding system, ability to add multiple cost maps together to get the sum cost for a specific tile

//TODO: adding tables together should be able to create larger tables

namespace hades {
	using table_index_t = vector_int;

	//a virtual table base class, for table data that can be generated on demand.
	template<typename Value>
	class virtual_table
	{
	public:
		using value_type = Value;
		using index_type = table_index_t;
		using size_type = table_index_t::value_type;

		constexpr virtual_table(index_type o, index_type s) noexcept(std::is_nothrow_constructible_v<index_type, index_type>)
			: _offset{ o }, _size{ s } {}
		virtual ~virtual_table() noexcept = default;

		virtual value_type operator[](index_type);
		virtual value_type operator[](size_type) const = 0;

		index_type position() const { return _offset; }
		index_type size() const { return _size; }

	private:
		index_type _offset, _size;
	};

	template<typename T>
	class always_table final : public virtual_table<T>
	{
	public:
		using value_type = T;
		using base_type = virtual_table<typename value_type>;
		using index_type = typename virtual_table<typename value_type>::index_type;
		using size_type = typename virtual_table<typename value_type>::size_type;

		template<typename U, std::enable_if_t<std::is_same_v<std::decay_t<U>, T>, int> = 0>
		constexpr always_table(index_type position, index_type size, U&& value)
			noexcept(std::is_nothrow_constructible_v<base_type, index_type, index_type>
				&& std::is_nothrow_constructible_v<T, decltype(std::forward<U>(value))>)
			: base_type{ position, size }, _always_value{ std::forward<U>(value) }
		{}

		value_type operator[](const size_type) const noexcept override
		{
			return _always_value;
		}

	private:
		T _always_value;
	};

	//main table class, can be added together to compound tables
	template<typename T>
	class table
	{
	public:
		using value_type = T;
		using index_type = table_index_t;
		using size_type = table_index_t::value_type;

		table() = default;
		table(index_type position, index_type size, T value) : _data(size.x * size.y, value), _offset{ position }, _width{ size.x } {}
		table(const table&) = default;
		table(const virtual_table<T>&);
		table(table&&) = default;

		table& operator=(const table&) = default;
		table& operator=(table&&) = default;
		value_type& operator[](index_type);
		const value_type& operator[](index_type) const;
		value_type& operator[](size_type);
		const value_type& operator[](size_type) const;

		index_type position() const { return _offset; }
		void set_position(index_type p)
		{
			_offset = p;
		}

		index_type size() const
		{
			using U = index_type::value_type;
			const auto end = integer_cast<U>(std::size(_data));
			return index_type{ _width, end / _width };
		}

		std::vector<value_type> &data()
		{
			return _data;
		}

		const std::vector<value_type> &data() const
		{
			return _data;
		}

	private:
		index_type _offset = {0, 0};
		size_type _width = 0;
		std::vector<value_type> _data = {};
	};

	template<typename T>
	struct is_table : std::false_type
	{};

	template<typename T>
	struct is_table<table<T>> : std::true_type
	{};

	template<typename T>
	struct is_table<virtual_table<T>> : std::true_type
	{};

	template<typename T, template <typename> typename U>
	struct is_table<U<T>> : std::bool_constant<std::is_base_of_v<virtual_table<T>, U<T>>>
	{};

	template<typename T>
	constexpr auto is_table_v = is_table<T>::value;

	//combines two tables using CombineFunctor
	//presses any areas of the second table onto the areas
	//of first table that it overlaps
	//return value is the size of the first table
	template<typename TableFirst, typename TableSecond, typename CombineFunctor>
	auto combine_table(const TableFirst&, const TableSecond&, CombineFunctor&&);


	//operators
	/*template<typename T>
	table<Value> operator+(const table<T>&, const table<T>&);

	template<typename Value>
	table<Value>& operator+=(table<Value>&, const table<Value>&);

	template<typename Value>
	table<Value> operator+(const table<Value>&, const virtual_table<Value>&);

	template<typename Value>
	table<Value>& operator+=(table<Value>&, const virtual_table<Value>&);*/
}

#include "detail/table.inl"

#endif // !HADES_UTIL_TABLE_HPP
