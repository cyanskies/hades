#ifndef HADES_UTIL_TABLE_HPP
#define HADES_UTIL_TABLE_HPP

#include <vector>

//a type representing an arbitarily sized table
//supports adding the contents of tables together

//intended usage is for pathfinding system, ability to add multiple cost maps together to get the sum cost for a specific tile

//TODO: adding tables together should be able to create larger tables

namespace hades {
	//an index type for access table elements
	template<typename Index>
	struct table_index final
	{
		using index_t = Index;
		table_index(index_t x, index_t y) : x(x), y(y) {}
		index_t x, y;
	};

	template<typename Index>
	table_index<Index> operator+(table_index<Index>, const table_index<Index>&);
	template<typename Index>
	table_index<Index>& operator+=(table_index<Index>&, const table_index<Index>&);
	template<typename Index>
	table_index<Index> operator-(table_index<Index>, const table_index<Index>&);
	template<typename Index>
	table_index<Index>& operator-=(table_index<Index>&, const table_index<Index>&);
	//a virtual table base class, for table data that can be generated on demand.
	template<typename Value, typename SizeType = std::vector<Value>::size_type>
	class virtual_table
	{
	public:
		using size_type = SizeType;
		using index_type = table_index<size_type>;
		using value_type = Value;

		virtual ~virtual_table() {}

		virtual value_type operator[](index_type) const = 0;

		index_type position() const { return _offset; }
		index_type size() const { return _size; }

	protected:
		virtual_table(index_type s, index_type o) : _offset(o), _size(s) {}

		index_type _offset, _size;
	};

	//main table class, can be added together to compound tables
	template<typename Value, typename SizeType = std::vector<Value>::size_type>
	class table final
	{
	public:
		using size_type = SizeType;
		using index_type = table_index<size_type>;
		using value_type = Value;

		table(index_type size, index_type position = index_type{ 0, 0 }) : _data(size.x * size.y), _offset(position) {}
		table(const table&) = default;
		table(table&&) = default;

		//returns a table representing the area specified.
		table get_subset(index_type, index_type) const;

		table& operator=(const table&) = default;
		table& operator=(table&&) = default;
		value_type& operator[](index_type);
		const value_type& operator[](index_type) const;

		void clear();

		index_type position() const { return _offset; }
		index_type size() const { return index_type{ _width, _data.size() / _width }; }

		void swap(table &other);
	private:
		index_type _offset;
		size_type _width;
		std::vector<value_type> _data;
	};

	//operators
	template<typename Value, typename SizeType>
	table<Value, SizeType> operator+(table<Value, SizeType>, const table<Value, SizeType>&);

	template<typename Value, typename SizeType>
	table<Value, SizeType>& operator+=(table<Value, SizeType>&, const table<Value, SizeType>&);

	template<typename Value, typename SizeType>
	table<Value, SizeType> operator+(table<Value, SizeType>, const virtual_table<Value, SizeType>&);

	template<typename Value, typename SizeType>
	table<Value, SizeType>& operator+=(table<Value, SizeType>&, const virtual_table<Value, SizeType>&);
}

#include "detail/table.inl"

#endif // !HADES_UTIL_TABLE_HPP
