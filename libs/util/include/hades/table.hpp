#ifndef HADES_UTIL_TABLE_HPP
#define HADES_UTIL_TABLE_HPP

#include <functional>
#include <vector>

#include "hades/vector_math.hpp"

//a type representing an arbitarily sized table
//supports adding the contents of tables together

//intended usage is for pathfinding system, ability to add multiple cost maps together to get the sum cost for a specific tile

//TODO: adding tables together should be able to create larger tables

namespace hades {

	template<typename T>
	using table_index_t = vector_int;

	//a virtual table base class, for table data that can be generated on demand.
	template<typename Value>
	class virtual_table
	{
	public:
		using value_type = Value;
		using index_type = table_index_t;
		using size_type = table_index_t::value_type;

		virtual_table(index_type o, index_type s) : _offset(o), _size(s) {}
		virtual ~virtual_table() {}

		virtual value_type operator[](index_type) const = 0;

		index_type position() const { return _offset; }
		index_type size() const { return _size; }

	private:
		index_type _offset, _size;
	};

	template<typename T>
	class always_table : public virtual_table<T>
	{
	public:
		always_table(index_type position, index_type size, T value)
			: virtual_table{ position, size }, _always_value{ std::forward<T>(value) }
		{}

		T operator[](index_type) const override
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

		table(index_type position, index_type size, T value) : _data(size.x * size.y, value), _offset(position) {}
		table(const table&) = default;
		table(const virtual_table<T>&);
		table(table&&) = default;

		table& operator=(const table&) = default;
		table& operator=(table&&) = default;
		value_type& operator[](index_type);
		const value_type& operator[](index_type) const;

		index_type position() const { return _offset; }
		void set_position(index_type p)
		{
			_offset = p;
		}

		index_type size() const { return index_type{ _width, _data.size() / _width }; }

		std::vector<value_type> &data()
		{
			return _data;
		}

		const std::vector<value_type> &data() const
		{
			return _data;
		}

	private:
		index_type _offset;
		size_type _width;
		std::vector<value_type> _data;
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
	struct is_table<U<T>> : std::bool_constant<std::is_base_of_v<virtual_table<T>, U<T>>
	{};

	template<T>
	constexpr auto is_table_v = is_table<T>::value;

	//combines two tables using CombineFunctor
	//presses any areas of the second table onto the areas
	//of first table that it overlaps
	template<typename TableFirst, typename TableSecond, typename CombineFunctor>
	auto combine_table(const TableFirst&, const TableSecond&, CombineFunctor);

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
